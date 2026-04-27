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

//
// Created by Adrián on 27/11/2018.
//

#ifndef UTILITIES_TIME_HPP
#define UTILITIES_TIME_HPP

#include <cstdint>
#include <sys/resource.h>

namespace util {

    namespace time {

        const static uint64_t nanoseconds     = 1;
        const static uint64_t microseconds    = 1000;
        const static uint64_t milliseconds    = 1000000;
        const static uint64_t seconds         = 1000000000;
        const static uint64_t minutes         = 60000000000;

        template<uint64_t ratio = 1>
        static double duration_cast(uint64_t value){
            return value / (double) ratio;
        }

        class user {

        public:

            /***
             * User time in microseconds
             * @return the user time in microseconds
             */
            static uint64_t now(){

                struct rusage r_usage;
                getrusage(RUSAGE_SELF, &r_usage);
                return (r_usage.ru_utime.tv_sec *1000000 + r_usage.ru_utime.tv_usec)*1000;
            }

        };

        class system {

        public:

            /***
             * Sys time in microseconds
             * @return the system time in microseconds
             */
            static uint64_t now(){

                struct rusage r_usage;
                getrusage(RUSAGE_SELF, &r_usage);
                return (r_usage.ru_stime.tv_sec * 1000000 + r_usage.ru_stime.tv_usec)*1000;
            }

        };

        class usage {


        public:

            typedef struct {
                uint64_t user;
                uint64_t system;
                uint64_t elapsed;
            } usage_type;
            /***
             * Sys time in microseconds
             * @return the system time in microseconds
             */
            static usage_type now(){

                struct rusage r_usage;
                getrusage(RUSAGE_SELF, &r_usage);
                usage_type res;
                res.user = (r_usage.ru_utime.tv_sec * 1000000 + r_usage.ru_utime.tv_usec)*1000;
                res.system = (r_usage.ru_stime.tv_sec * 1000000 + r_usage.ru_stime.tv_usec)*1000;
                res.elapsed = res.user + res.system;
                return res;
            }

        };


    };
}

#endif