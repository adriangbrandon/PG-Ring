//
// Created by adrian on 19/12/25.
//

#ifndef OPERAND_UTILS_HPP
#define OPERAND_UTILS_HPP
#include <cstdint>
#include <string>
#include <ctime>

namespace ring {

    namespace query {

        namespace constant {

            enum enum_constant_type {INTEGER, DOUBLE, STRING, DATE};
            typedef struct {
                enum_constant_type type;
                std::uint32_t value;
                std::string string;
            } constant_type;


            static bool is_integer(const std::string& s, std::uint32_t& result) {
                if (s.empty()) return false;
                char* end;
                unsigned long val = std::strtoul(s.c_str(), &end, 10);
                if (end != s.c_str() && *end == '\0') {
                    result = static_cast<std::uint32_t>(val);
                    return true;
                }
                return false;
            }

            static bool is_double(const std::string& s, double& result) {
                if (s.empty()) return false;
                char* end;
                double val = std::strtod(s.c_str(), &end);
                if (end != s.c_str() && *end == '\0') {
                    result = val;
                    return true;
                }
                return false;
            }

            static bool is_date(const std::string& s, std::uint64_t& result) {
                // Accepts: D/M/YYYY or DD/MM/YYYY, with optional time: [ H[:M[:S]] ]
                size_t pos = 0;
                // Parse day
                size_t slash1 = s.find('/', pos);
                if (slash1 == std::string::npos) return false;
                std::string day_str = s.substr(pos, slash1 - pos);
                if (day_str.empty() || day_str.size() > 2) return false;
                for (char c : day_str) if (!isdigit(c)) return false;
                int day = std::stoi(day_str);
                pos = slash1 + 1;
                // Parse month
                size_t slash2 = s.find('/', pos);
                if (slash2 == std::string::npos) return false;
                std::string month_str = s.substr(pos, slash2 - pos);
                if (month_str.empty() || month_str.size() > 2) return false;
                for (char c : month_str) if (!isdigit(c)) return false;
                int month = std::stoi(month_str);
                pos = slash2 + 1;
                // Parse year
                size_t space = s.find(' ', pos);
                std::string year_str = (space == std::string::npos) ? s.substr(pos) : s.substr(pos, space - pos);
                if (year_str.empty() || year_str.size() > 4) return false;
                for (char c : year_str) if (!isdigit(c)) return false;
                int year = std::stoi(year_str);
                int hour = 0, min = 0, sec = 0;
                if (space != std::string::npos) {
                    pos = space + 1;
                    // Parse hour
                    size_t colon1 = s.find(':', pos);
                    if (colon1 == std::string::npos) {
                        std::string hour_str = s.substr(pos);
                        if (hour_str.empty() || hour_str.size() > 2) return false;
                        for (char c : hour_str) if (!isdigit(c)) return false;
                        hour = std::stoi(hour_str);
                    } else {
                        std::string hour_str = s.substr(pos, colon1 - pos);
                        if (hour_str.empty() || hour_str.size() > 2) return false;
                        for (char c : hour_str) if (!isdigit(c)) return false;
                        hour = std::stoi(hour_str);
                        pos = colon1 + 1;
                        // Parse minute
                        size_t colon2 = s.find(':', pos);
                        if (colon2 == std::string::npos) {
                            std::string min_str = s.substr(pos);
                            if (min_str.empty() || min_str.size() > 2) return false;
                            for (char c : min_str) if (!isdigit(c)) return false;
                            min = std::stoi(min_str);
                        } else {
                            std::string min_str = s.substr(pos, colon2 - pos);
                            if (min_str.empty() || min_str.size() > 2) return false;
                            for (char c : min_str) if (!isdigit(c)) return false;
                            min = std::stoi(min_str);
                            pos = colon2 + 1;
                            // Parse second
                            std::string sec_str = s.substr(pos);
                            if (sec_str.empty() || sec_str.size() > 2) return false;
                            for (char c : sec_str) if (!isdigit(c)) return false;
                            sec = std::stoi(sec_str);
                        }
                    }
                }
                if (day < 1 || day > 31) return false;
                if (month < 1 || month > 12) return false;
                if (year < 1) return false;
                if (hour < 0 || hour > 23) return false;
                if (min < 0 || min > 59) return false;
                if (sec < 0 || sec > 59) return false;
                std::tm tm_date = {};
                tm_date.tm_mday = day;
                tm_date.tm_mon = month - 1;
                tm_date.tm_year = year - 1900;
                tm_date.tm_hour = hour;
                tm_date.tm_min = min;
                tm_date.tm_sec = sec;
                tm_date.tm_isdst = -1;
                std::time_t ts = std::mktime(&tm_date);
                if (ts == -1) return false;
                result = static_cast<std::uint64_t>(ts);
                return true;
            }

            static bool is_string(const std::string& s) {
                // Debe empezar y acabar por comillas dobles, pero permite cualquier cosa dentro
                return s.size() >= 2 && s.front() == '"' && s.back() == '"';
            }

            static constant_type get_constant(size_t &pos, const std::string &s) {
                constant_type c;
                std::string aux_str = s.substr(pos);
                uint int_res;
                double double_res;
                uint64_t date_res;
                if (is_integer(aux_str, int_res)) {
                    c.type = INTEGER;
                    c.value = int_res;
                    pos += aux_str.size();
                } else if (is_double(aux_str, double_res)) {
                    c.type = DOUBLE;
                    c.value = static_cast<uint32_t>(double_res);
                    pos += aux_str.size();
                } else if (is_date(aux_str, date_res)) {
                    c.type = DATE;
                    c.value = static_cast<uint32_t>(date_res);
                    pos += aux_str.size();
                }else {
                    c.type = STRING;
                    c.string = aux_str.substr(1, aux_str.size() - 2); //removing the quotes
                }
                return c;
            }



        }

    }
}

#endif //OPERAND_UTILS_HPP
