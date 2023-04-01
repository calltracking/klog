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
};

std::tm parse_date(const std::string& date_str) {
    std::tm tm = {};
    std::istringstream iss(date_str);
    iss >> std::get_time(&tm, "%b %d %H:%M:%S");

    if (date_str[4] == ' ') {
        tm.tm_mday = date_str[5] - '0';
    } else {
        tm.tm_mday = std::stoi(date_str.substr(4, 2));
    }

    return tm;
}

bool process_log_file(const std::string& filename, time_t &offset_time, Stats& stats, const bool progress=false) {
    std::ifstream log_file(filename);
    if (!log_file.is_open()) {
        std::cerr << "Error: Unable to access file '" << filename << "'. Check your permissions." << std::endl;
        return false;
    }
    std::string line;

    std::regex matcher(R"(\/usr\/local\/sbin\/kamailio\[\d+\]: (INFO|ERROR|NOTICE): (\{(.*)\}|<core>))");
    std::regex error_match(R"(\{(\d+)\s(\d+)\s(ERROR)\s(.*)\})");
    std::regex log_match(R"(\{(\d+)\s(\d+)\s(\w+)\s(.*)\})");

    std::smatch match_results;
    std::unordered_set<std::string> invites, acks, byes, errors;

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    size_t error_count = 0;

    while (std::getline(log_file, line)) {
        if (std::regex_search(line, match_results, matcher)) {
            std::string level = match_results[1];
            std::string log = match_results[2];

	    std::string date_str = line.substr(0, 15);
            std::tm tm = parse_date(date_str);

            tm.tm_year = now_tm.tm_year;

            time_t ts = mktime(&tm);

            if (ts < offset_time) {
                fprintf(stderr, "seeking: %ld < %ld\r", ts, offset_time);
                return false;
            }
	    if (progress) {
                fprintf(stderr, "INVITES: %ld, ACKS: %ld, BYES: %ld, ERRORS: %ld\r", invites.size(), acks.size(), byes.size(), errors.size() + error_count);
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
                }
            }
            offset_time = ts;
        }
    }

    stats.invites = invites.size();
    stats.acks = acks.size();
    stats.byes = byes.size();
    stats.errors = errors.size() + error_count;
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
    if (process_log_file(log_filename, offset_time, stats, !!argv[1])) {
	printf("\nfinal offset: %d\n", offset_time);

        std::cout << "INVITES: " << stats.invites << ", ACKS: " << stats.acks
                  << ", BYES: " << stats.byes << ", ERRORS: " << stats.errors << std::endl;
        {
            std::ofstream offset_file(offset_filename);
            if (offset_file) {
                offset_file << std::time(nullptr);
            }
        }

        return 0;
    } else {
        return 1;
    }

}
