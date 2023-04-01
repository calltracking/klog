#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <ctime>
#include <iomanip>
#include <chrono>

struct Stats {
    size_t invites = 0;
    size_t acks = 0;
    size_t byes = 0;
    size_t errors = 0;
    size_t cancels = 0;
    size_t lines = 0;
};

void parse_date(const std::string& date_str, std::tm &tm, int year) {
    std::istringstream iss(date_str);
    std::string month_str;

    iss >> month_str;
    iss >> tm.tm_mday;
    iss.get(); // Ignore the space
    iss >> tm.tm_hour;
    iss.get(); // Ignore the ':'
    iss >> tm.tm_min;
    iss.get(); // Ignore the ':'
    iss >> tm.tm_sec;

    tm.tm_year = 2023 - 1900; // Set the year to 2023

    static const std::unordered_map<std::string, int> month_map = {
        {"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4}, {"Jun", 5},
        {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11},
    };

    tm.tm_mon = month_map.at(month_str);
    tm.tm_year = year;
}

bool process_log_file(const std::string& log_path, time_t &offset_time, Stats& stats, const bool progress=false) {

    int fd = open(log_path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }

    char* addr = static_cast<char*>(mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (addr == MAP_FAILED) {
        perror("Error mmap-ing file");
        close(fd);
        return 1;
    }

    size_t pos = 0;
    size_t file_size = static_cast<size_t>(sb.st_size);

    std::regex matcher(R"(\/usr\/local\/sbin\/kamailio\[\d+\]: (INFO|ERROR|NOTICE): (\{(.*)\}|<core>))");
    std::regex error_match(R"(\{(\d+)\s(\d+)\s(ERROR)\s(.*)\})");
    std::regex log_match(R"(\{(\d+)\s(\d+)\s(\w+)\s(.*)\})");

    std::smatch match_results;
    std::unordered_set<std::string> invites, acks, byes, errors, cancels;

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    size_t error_count = 0;
    size_t line_count = 0;

    while (pos < file_size) {
        size_t end_pos = pos;
        while (end_pos < static_cast<size_t>(sb.st_size) && addr[end_pos] != '\n') {
            ++end_pos;
        }
        std::string line(addr + pos, addr + end_pos);
	pos = end_pos + 1;
	++line_count;

        if (std::regex_search(line, match_results, matcher)) {
            std::string level = match_results[1];
            std::string log = match_results[2];

	    std::string date_str = line.substr(0, 15);
            std::tm tm = {};
	    parse_date(date_str, tm, now_tm.tm_year);

            time_t ts = mktime(&tm);

            if (ts < offset_time) {
                fprintf(stderr, "seeking[%ld]: '%ld' < '%ld'\r", line_count, ts, offset_time);
                continue;
            }
	    if (progress) {
                fprintf(stderr, "INVITES: %ld, ACKS: %ld, BYES: %ld, CANCELS: %ld, ERRORS: %ld\r", invites.size(), acks.size(), byes.size(), cancels.size(), errors.size() + error_count);
	    }

	    if (level == "ERROR") {
		//fprintf(stderr, "%s\r", line.c_str());
		error_count++;
            } else if (std::regex_search(log, match_results, log_match)) {
                std::string status = match_results[3];
                std::string call_id = match_results[4];

                if (status == "INVITE") {
                    invites.insert(call_id);
                } else if (status == "ACK") {
                    acks.insert(call_id);
                } else if (status == "BYE") {
                    byes.insert(call_id);
                } else if (status == "ERROR") {
                    errors.insert(call_id);
                } else if (status == "CANCEL") {
                    cancels.insert(call_id);
                }
            }
            offset_time = ts;
        }
    }

    munmap(addr, sb.st_size);
    close(fd);

    stats.invites = invites.size();
    stats.acks = acks.size();
    stats.byes = byes.size();
    stats.errors = errors.size() + error_count;
    stats.cancels = cancels.size();
    stats.lines = line_count;
    return true;
}

int main(int argc, char **argv) {
    std::string log_filename = "/var/log/messages";
    std::string offset_filename = "/var/log/klog.time";

    time_t offset_time = 0;
    {
        std::ifstream offset_file(offset_filename);
        if (offset_file) {
            offset_file >> offset_time;
        }
    }
    printf("starting offset: %ld\n", offset_time);

    Stats stats;
    bool progress = !!argv[1];
    if (process_log_file(log_filename, offset_time, stats, progress)) {
	if (progress) {
            fprintf(stderr, "\n\n");
	}
	printf("\nfinal offset: %ld\n", offset_time);


        std::cout << "INVITES: " << stats.invites << ", ACKS: " << stats.acks
                  << ", BYES: " << stats.byes << ", ERRORS: " << stats.errors << ", CANCELS: " << stats.cancels << ", LINES: " << stats.lines << std::endl;
        {
            std::ofstream offset_file(offset_filename);
            if (offset_file) {
                offset_file << std::time(nullptr);
            }
        }

        return 0;
    } else {
	printf("\nError processing\n");
        return 1;
    }

}
