//
// Created by adrian on 19/12/25.
//

#ifndef OPERAND_UTILS_HPP
#define OPERAND_UTILS_HPP
#include <cstdint>
#include <cstring>
#include <string>
#include <ctime>

namespace ring {

    namespace query {

        namespace constant {

            static std::string ZONED_TIME_PREFIX = "ZONED_DATETIME('";
            static std::string ZONED_TIME_SUFFIX = "')";
            static int64_t YEAR_TO_SEC = 13LL * 32 * 24 * 60 * 60;
            static int64_t MONTH_TO_SEC = 32LL * 24 * 60 * 60;
            static int64_t DAY_TO_SEC = 24LL * 60 * 60;
            static int64_t HOUR_TO_SEC = 60LL * 60;
            static int64_t MIN_TO_SEC = 60LL;

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

            static bool parse_integer(const std::string& str, uint64_t start, uint64_t len, int64_t& result) {
                if (start + len > str.size()) return false;
                int64_t val = 0;
                for (size_t i = start; i < start + len; ++i) {
                    char c = str[i];
                    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                    val = val * 10 + (c - '0');
                }
                result = val;
                return true;
            }

            static int64_t double_to_int64(double x) {

                uint64_t bits;
                std::memcpy(&bits, &x, sizeof(double));
                if (bits >> 63) {
                    //flip bits except the sign bit
                    uint64_t mask = (1ULL << 63) - 1;
                    bits ^= mask;
                }
                return static_cast<int64_t>(bits);
            }

            static double int64_to_double(int64_t x) {
                auto bits = static_cast<uint64_t>(x);

                if (bits >> 63) {
                    uint64_t mask = (1ULL << 63) - 1;
                    bits ^= mask;
                }
                double d;
                std::memcpy(&d, &bits, sizeof(double));
                return d;
            }


            static std::string int64_to_date(int64_t x) {
                int64_t sec, min, hour, day, month, year;
                if (x < 0) {
                    year = x / YEAR_TO_SEC - (x % YEAR_TO_SEC != 0); // round down for negative years
                }else {
                    year = x / YEAR_TO_SEC;
                }
                x -= year * YEAR_TO_SEC;
                month = x / MONTH_TO_SEC;
                x -= month * MONTH_TO_SEC;
                day = x / DAY_TO_SEC;
                x -= day * DAY_TO_SEC;
                hour = x / HOUR_TO_SEC;
                x -= hour * HOUR_TO_SEC;
                min = x / MIN_TO_SEC;
                x -= min * MIN_TO_SEC;
                sec = x;

                char buffer[30];
                std::snprintf(buffer, sizeof(buffer), "%+05lld-%02lld-%02lldT%02lld:%02lld:%02lldZ",
                  year, month, day, hour, min, sec);
                return std::string(buffer);
            }

            static int64_t date_to_int64(const std::string& s) {
                int64_t year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
                if (!parse_integer(s, 1, 4, year)) return false;
                if (s[0] == '-') year = -year; // negative years
                if (!parse_integer(s, 6, 2, month)) return false;
                if (!parse_integer(s, 9, 2, day)) return false;
                if (!parse_integer(s, 12, 2, hour)) return false;
                if (!parse_integer(s, 15, 2, min)) return false;
                if (!parse_integer(s, 18, 2, sec)) return false;

                return (year * YEAR_TO_SEC) + (month * MONTH_TO_SEC) + (day * DAY_TO_SEC) + (hour * HOUR_TO_SEC) + (min * MIN_TO_SEC) + sec;
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

            // Admits YYYY-MM-DDThh:mm:ssZ or ZONED_DATETIME('YYYY-MM-DDThh:mm:ssZ')  both with optional sign.
            static bool is_date(const std::string& sd, int64_t& result) {

                // check if it is in the form ZONED_DATETIME('YYYY-MM-DDThh:mm:ssZ')
                std::string s = sd;
                if (s.size() > 20 &&
                    s.substr(0, ZONED_TIME_PREFIX.size()) == ZONED_TIME_PREFIX &&
                    s.substr(s.size() - ZONED_TIME_SUFFIX.size()) == ZONED_TIME_SUFFIX) {
                    s = s.substr(ZONED_TIME_PREFIX.size(), s.size() - ZONED_TIME_PREFIX.size() - ZONED_TIME_SUFFIX.size());
                }

                if (s.size() > 21 || s.size() < 20) return false;
                if (s.size() == 20) s = "+" + s; // add default sign
                if (s[0] != '+' && s[0] != '-') return false;
                if (s[5] != '-' || s[8] != '-' || s[11] != 'T' || s[14] != ':' || s[17] != ':' || s[20] != 'Z') return false;
                result = date_to_int64(s);
                return true;
            }

            static bool is_string(const std::string& s) {
                return s.size() >= 2 && (s.front() == '"' && s.back() == '"' );
            }



        }

    }
}

#endif //OPERAND_UTILS_HPP
