//
// Created by Adrián on 24/10/23.
//

#ifndef UTIL_RESULTS_COLLECTOR_TEST_HPP
#define UTIL_RESULTS_COLLECTOR_TEST_HPP

#include <algorithm>
#include <array>
#include <vector>

namespace util {

    template<class Type>
    class results_collector_test {

    public:
        typedef Type value_type;
        typedef uint64_t size_type;

    private:

        std::vector<value_type> m_results;

        void copy(const results_collector_test &o) {
            m_results = o.m_results;
        }

    public:

        const std::vector<value_type> &results = m_results;

        results_collector_test(){};


        inline void add(const value_type &val){
            m_results.push_back(val);
        }

        inline size_type size(){
            return m_results.size();
        }

        //! Copy constructor
        results_collector_test(const results_collector_test &o) {
            copy(o);
        }

        //! Move constructor
        results_collector_test(results_collector_test &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        results_collector_test &operator=(const results_collector_test &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        results_collector_test &operator=(results_collector_test &&o) {
            if (this != &o) {
                m_results = std::move(o.m_results);
            }
            return *this;
        }

        void swap(results_collector_test &o) {
            std::swap(m_results, o.m_results);
        }

        void sort() {
            std::sort(m_results.begin(), m_results.end());
        }


    };

}

#endif //UTIL_RESULTS_COLLECTOR_HPP