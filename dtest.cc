#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>



std::time_t parse_date(const std::string& date_str) {
    std::tm tm = {};
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

    printf("month: %d\n", tm.tm_mon);
    printf("day: %d\n", tm.tm_mday);
    printf("hour: %d\n", tm.tm_hour);
    printf("minute: %d\n", tm.tm_min);
    printf("second: %d\n", tm.tm_sec);

    return mktime(&tm);
}

int main() {
    std::vector<std::string> dates = {"Apr  1 20:03:24", "Apr  1 20:03:23", "Apr  1 20:02:48"};
    for (const auto& date : dates) {
        std::time_t timestamp = parse_date(date);
        std::cout << "Date: " << date << " -> Timestamp: " << timestamp << std::endl;
    }
    return 0;
}
