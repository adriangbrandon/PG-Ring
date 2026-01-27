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

            static bool is_integer(const std::string& s, int64_t& result) {
                if (s.empty()) return false;
                char* end;
                int64_t val = std::strtol(s.c_str(), &end, 10);
                if (end != s.c_str() && *end == '\0') {
                    result = val;
                    return true;
                }
                return false;
            }

            static int64_t double_to_int64(double x)
            {
                uint64_t bits;
                std::memcpy(&bits, &x, sizeof(double));

                if (bits >> 63) {
                    // negativo: invertir todos los bits
                    bits = ~bits;
                } else {
                    // positivo: flip del bit de signo
                    bits ^= (1ULL << 63);
                }
                return static_cast<int64_t>(bits);
            }

            static double int64_to_double(int64_t x)
            {
                auto bits = static_cast<uint64_t>(x);

                if (bits >> 63) {
                    // era positivo: desflip del signo
                    bits ^= (1ULL << 63);
                } else {
                    // era negativo: reinvertir todo
                    bits = ~bits;
                }
                double d;
                std::memcpy(&d, &bits, sizeof(double));
                return d;
            }

            static bool is_double(const std::string& s, int64_t& result) {
                if (s.empty()) return false;
                char* end;
                double val = std::strtod(s.c_str(), &end);
                if (end != s.c_str() && *end == '\0') {
                    result = double_to_int64(val);
                    return true;
                }
                return false;
            }

            // Nuevo formato: ±YYYY-MM-DDThh:mm:ssZ (por ejemplo: +1983-00-00T00:00:00Z o -0450-01-01T00:00:00Z)
            static bool is_date(const std::string& s, int64_t& result) {
                // Longitud fija esperada: 21 caracteres: ±YYYY-MM-DDThh:mm:ssZ
                if (s.size() != 21) return false;
                if (s[0] != '+' && s[0] != '-') return false;
                if (s[5] != '-' || s[8] != '-' || s[11] != 'T' || s[14] != ':' || s[17] != ':' || s[20] != 'Z') return false;

                auto parse_int = [](const std::string& str, size_t start, size_t len, int& out) -> bool {
                    if (start + len > str.size()) return false;
                    int val = 0;
                    for (size_t i = start; i < start + len; ++i) {
                        char c = str[i];
                        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                        val = val * 10 + (c - '0');
                    }
                    out = val;
                    return true;
                };

                int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
                if (!parse_int(s, 1, 4, year)) return false;
                if (s[0] == '-') year = -year; // permitir años negativos
                if (!parse_int(s, 6, 2, month)) return false;
                if (!parse_int(s, 9, 2, day)) return false;
                if (!parse_int(s, 12, 2, hour)) return false;
                if (!parse_int(s, 15, 2, min)) return false;
                if (!parse_int(s, 18, 2, sec)) return false;

                result = ((((year * 13 + month) * 32 + day)* 24 + hour) * 60 + min)* 60 + sec;
                return true;
            }

            static bool is_date_old(const std::string& s, uint64_t& result) {
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



        }

    }
}

#endif //OPERAND_UTILS_HPP
