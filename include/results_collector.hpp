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

#ifndef UTIL_RESULTS_COLLECTOR_HPP
#define UTIL_RESULTS_COLLECTOR_HPP

#include <array>

namespace util {

    template<class Type>
    class results_collector {

    public:
        typedef Type value_type;
        typedef uint64_t size_type;
        constexpr static size_type buckets = (1ULL << 20);

    private:

        value_type* m_results;
        size_type m_cnt = 0;

        void copy(const results_collector &o) {
            for(size_type i = 0; i < buckets; ++i){
                m_results[i] = o.m_results[i];
            }
            m_cnt = o.m_cnt;
        }

    public:

        results_collector(){
            m_results = new value_type[buckets];
        }

        ~results_collector(){
            delete [] m_results;
        }

        value_type operator[](size_type i) const {
            return m_results[i];
        }

        void clear() {
            m_cnt = 0;
        }

        inline void add(const value_type &val){
            m_results[(m_cnt & (buckets-1))] = val;
            ++m_cnt;
        }

        inline size_type size(){
            return m_cnt;
        }

        //! Copy constructor
        results_collector(const results_collector &o) {
            copy(o);
        }

        //! Move constructor
        results_collector(results_collector &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        results_collector &operator=(const results_collector &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        results_collector &operator=(results_collector &&o) {
            if (this != &o) {
                m_results = std::move(o.m_results);
                m_cnt = std::move(o.m_cnt);
            }
            return *this;
        }

        void swap(results_collector &o) {
            std::swap(m_results, o.m_results);
            std::swap(m_cnt, o.m_cnt);
        }

    };

}

#endif //UTIL_RESULTS_COLLECTOR_HPP