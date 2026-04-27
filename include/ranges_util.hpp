/*
 * Copyright (C) 2026 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef RANGES_UTIL_HPP
#define RANGES_UTIL_HPP

#include <vector>
#include <algorithm>

#include <wt_range_iterator.hpp>


namespace ring {

    namespace ranges {

        // Une rangos solapados y adyacentes
        inline std::vector<sdsl::range_type> merge(const std::vector<sdsl::range_type>& ranges) {
            if (ranges.empty()) return {};
            std::vector<sdsl::range_type> sorted = ranges;
            std::sort(sorted.begin(), sorted.end());
            std::vector<sdsl::range_type> merged;
            merged.push_back(sorted[0]);
            for (size_t i = 1; i < sorted.size(); ++i) {
                if (merged.back()[1] + 1 >= sorted[i][0]) {
                    merged.back()[1] = std::max(merged.back()[1], sorted[i][1]); //amplia
                } else {
                    merged.push_back(sorted[i]);
                }
            }
            return merged;
        }

        // Intersección de varios vectores de rangos
        inline std::vector<sdsl::range_type> intersect(const std::vector<std::vector<sdsl::range_type>>& all_ranges) {
            if (all_ranges.empty()) return {};
            std::vector<sdsl::range_type> result = all_ranges[0];
            for (size_t i = 1; i < all_ranges.size(); ++i) {
                std::vector<sdsl::range_type> temp;
                size_t a = 0, b = 0;
                while (a < result.size() && b < all_ranges[i].size()) {
                    const auto& r1 = result[a];
                    const auto& r2 = all_ranges[i][b];
                    uint64_t start = std::max(r1[0], r2[0]);
                    uint64_t end = std::min(r1[1], r2[1]);
                    if (start <= end) temp.push_back({start, end});
                    if (r1[1] < r2[1]) ++a; else ++b;
                }
                result = std::move(temp);
            }
            return result;
        }

    }



}

#endif // RANGES_UTIL_HPP

