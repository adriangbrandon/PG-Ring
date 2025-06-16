/*
 * utils.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
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


#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include "ring.hpp"


namespace ring {

    namespace util {


        /*template<class Iterator, class Ring>
        uint64_t get_size_interval(Ring* ptr_ring, const Iterator &iter) {
            if(iter.cur_s == -1 && iter.cur_p == -1 && iter.cur_o == -1){
                return iter.i_s.size(); //open
            } else if (iter.cur_s == -1 && iter.cur_p != -1 && iter.cur_o == -1) {
                return iter.i_s.size(); //i_s = i_o
            } else if (iter.cur_s == -1 && iter.cur_p == -1 && iter.cur_o != -1) {
                return iter.i_s.size(); //i_s = i_p
            } else if (iter.cur_s != -1 && iter.cur_p == -1 && iter.cur_o == -1) {
                return iter.i_o.size(); //i_o = i_p
            } else if (iter.cur_s != -1 && iter.cur_p != -1 && iter.cur_o == -1) {
                return iter.i_o.size();
            } else if (iter.cur_s != -1 && iter.cur_p == -1 && iter.cur_o != -1) {
                return iter.i_p.size();
            } else if (iter.cur_s == -1 && iter.cur_p != -1 && iter.cur_o != -1) {
                return iter.i_s.size();
            }
            return 0;
        }*/


        struct trait_size_old {

            template<class Iterator, class Ring>
            static uint64_t subject(Ring* ptr_ring, Iterator &iter){
                return iter.i_s.size();
            }

            template<class Iterator, class Ring>
            static uint64_t predicate(Ring* ptr_ring, Iterator &iter){
                return iter.i_p.size();
            }

            template<class Iterator, class Ring>
            static uint64_t object(Ring* ptr_ring, Iterator &iter){
                return iter.i_o.size();
            }

        };

        struct trait_size {

            template<class Iterator, class Ring>
            static uint64_t get(Ring* ptr_ring, Iterator &iter, state_type state){
                return iter.interval_length();
            }

            template<class Iterator, class Ring>
            static uint64_t subject(Ring* ptr_ring, Iterator &iter){
                return iter.interval_length();
            }

            template<class Iterator, class Ring>
            static uint64_t predicate(Ring* ptr_ring, Iterator &iter){
                return iter.interval_length();
            }

            template<class Iterator, class Ring>
            static uint64_t object(Ring* ptr_ring, Iterator &iter){
                return iter.interval_length();
            }

        };

        struct trait_probability {

            template<class Iterator, class Ring>
            static double get(Ring* ptr_ring, Iterator &iter, state_type state){
                return iter.interval_length() / (double) ptr_ring->n_triples;
            }

            template<class Iterator, class Ring>
            static double subject(Ring* ptr_ring, Iterator &iter){
                return iter.interval_length() / (double) ptr_ring->n_triples;
            }

            template<class Iterator, class Ring>
            static double predicate(Ring* ptr_ring, Iterator &iter){
                return iter.interval_length() / (double) ptr_ring->n_triples;
            }

            template<class Iterator, class Ring>
            static double object(Ring* ptr_ring, Iterator &iter){
                return iter.interval_length() / (double) ptr_ring->n_triples;
            }

        };

        struct trait_distinct {

            template<class Iterator, class Ring>
            static uint64_t get(Ring* ptr_ring, Iterator &iter, state_type state){
                if(state == s){
                    if(iter.level == 0) return ptr_ring->max_s;
                    if(iter.level == 2){
                        return ptr_ring->distinct_PO_S(iter.interval());
                    }else {//iter.level == 1
                        if(iter.state[0] == p){
                            return ptr_ring->distinct_PO_S(iter.interval());
                        }else{
                            return ptr_ring->distinct_OP_S(iter.interval());
                        }
                    }
                }else if (state == p){
                    if(iter.level == 0) return ptr_ring->max_p;
                    if(iter.level == 2){
                        return ptr_ring->distinct_OS_P(iter.interval());
                    }else {//iter.level == 1
                        if(iter.state[0] == s){
                            return ptr_ring->distinct_SO_P(iter.interval());
                        }else{
                            return ptr_ring->distinct_OS_P(iter.interval());
                        }
                    }
                }else{
                    if(iter.level == 0) return ptr_ring->max_o;
                    if(iter.level == 2){
                        return ptr_ring->distinct_SP_O(iter.interval());
                    }else {//iter.level == 1
                        if(iter.state[0] == s){
                            return ptr_ring->distinct_SP_O(iter.interval());
                        }else{
                            return ptr_ring->distinct_PS_O(iter.interval());
                        }
                    }
                };
            }

            template<class Iterator, class Ring>
            static uint64_t subject(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 0) return ptr_ring->max_s;
                if(iter.level == 2){
                    return ptr_ring->distinct_PO_S(iter.interval());
                }else {//iter.level == 1
                    if(iter.state[0] == p){
                        return ptr_ring->distinct_PO_S(iter.interval());
                    }else{
                        return ptr_ring->distinct_OP_S(iter.interval());
                    }
                }
            }

            template<class Iterator, class Ring>
            static uint64_t predicate(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 0) return ptr_ring->max_p;
                if(iter.level == 2){
                    return ptr_ring->distinct_OS_P(iter.interval());
                }else {//iter.level == 1
                    if(iter.state[0] == s){
                        return ptr_ring->distinct_SO_P(iter.interval());
                    }else{
                        return ptr_ring->distinct_OS_P(iter.interval());
                    }
                }
            }

            template<class Iterator, class Ring>
            static uint64_t object(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 0) return ptr_ring->max_o;
                if(iter.level == 2){
                    return ptr_ring->distinct_SP_O(iter.interval());
                }else {//iter.level == 1
                    if(iter.state[0] == s){
                        return ptr_ring->distinct_SP_O(iter.interval());
                    }else{
                        return ptr_ring->distinct_PS_O(iter.interval());
                    }
                }
            }

        };

        template <uint64_t k=1>
        struct trait_intersect {

            template<class Iterator, class Ring>
            static std::vector<uint64_t> get(Ring* ptr_ring, Iterator &iter, state_type state){
                if(state == s){
                    if(iter.level == 1 && iter.state[0] == o){
                        return ptr_ring->ai_fwd_S(k, iter.consts[0]);
                    }else{
                        return ptr_ring->ai_bwd_S(k, iter.interval());
                    }
                }else if (state == p){
                    if(iter.level == 1 && iter.state[0] == s){
                        return ptr_ring->ai_fwd_P(k, iter.consts[0]);
                    }else{
                        return ptr_ring->ai_bwd_P(k, iter.interval());
                    }
                }else{
                    if(iter.level == 1 && iter.state[0] == p){
                        return ptr_ring->ai_fwd_O(k, iter.consts[0]);
                    }else{
                        return ptr_ring->ai_bwd_O(k, iter.interval());
                    }
                };
            }

            template<class Iterator, class Ring>
            static std::vector<uint64_t> subject(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 1 && iter.state[0] == o){
                    return ptr_ring->ai_fwd_S(k, iter.consts[0]);
                }else{
                    return ptr_ring->ai_bwd_S(k, iter.interval());
                }
            }

            template<class Iterator, class Ring>
            static std::vector<uint64_t> predicate(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 1 && iter.state[0] == s){
                    return ptr_ring->ai_fwd_P(k, iter.consts[0]);
                }else{
                    return ptr_ring->ai_bwd_P(k, iter.interval());
                }
            }

            template<class Iterator, class Ring>
            static std::vector<uint64_t> object(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 1 && iter.state[0] == p){
                    return ptr_ring->ai_fwd_O(k, iter.consts[0]);
                }else{
                    return ptr_ring->ai_bwd_O(k, iter.interval());
                }
            }
        };

        struct trait_distinct_uni {

            template<class Iterator, class Ring>
            static uint64_t get(Ring* ptr_ring, Iterator &iter, state_type state) {
                if (state == s) {
                    if(iter.level == 0) return ptr_ring->max_s;
                    if(iter.level == 2){
                        if(iter.state[0] == p && iter.state[1] == o){
                            return ptr_ring->distinct_PO_S(iter.interval());
                        }else{
                            return ptr_ring->distinct_OP_S(iter.interval());
                        }
                    }else {//iter.level == 1
                        if(iter.state[0] == p){
                            return ptr_ring->distinct_PO_S(iter.interval());
                        }else{
                            return ptr_ring->distinct_OP_S(iter.interval());
                        }
                    }
                }else if (state == p){
                    if(iter.level == 0) return ptr_ring->max_p;
                    if(iter.level == 2){
                        if(iter.state[0] == s && iter.state[1] == o){
                            return ptr_ring->distinct_SO_P(iter.interval());
                        }else{
                            return ptr_ring->distinct_OS_P(iter.interval());
                        }
                    }else {//iter.level == 1
                        if(iter.state[0] == s){
                            return ptr_ring->distinct_SO_P(iter.interval());
                        }else{
                            return ptr_ring->distinct_OS_P(iter.interval());
                        }
                    }
                }else{
                    if(iter.level == 0) return ptr_ring->max_o;
                    if(iter.level == 2){
                        if(iter.state[0] == s && iter.state[1] == p){
                            return ptr_ring->distinct_SP_O(iter.interval());
                        }else{
                            return ptr_ring->distinct_PS_O(iter.interval());
                        }
                    }else {//iter.level == 1
                        if(iter.state[0] == s){
                            return ptr_ring->distinct_SP_O(iter.interval());
                        }else{
                            return ptr_ring->distinct_PS_O(iter.interval());
                        }
                    }
                }
            }

            template<class Iterator, class Ring>
            static uint64_t subject(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 0) return ptr_ring->max_s;
                if(iter.level == 2){
                    if(iter.state[0] == p && iter.state[1] == o){
                        return ptr_ring->distinct_PO_S(iter.interval());
                    }else{
                        return ptr_ring->distinct_OP_S(iter.interval());
                    }
                }else {//iter.level == 1
                    if(iter.state[0] == p){
                        return ptr_ring->distinct_PO_S(iter.interval());
                    }else{
                        return ptr_ring->distinct_OP_S(iter.interval());
                    }
                }
            }

            template<class Iterator, class Ring>
            static uint64_t predicate(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 0) return ptr_ring->max_p;
                if(iter.level == 2){
                    if(iter.state[0] == s && iter.state[1] == o){
                        return ptr_ring->distinct_SO_P(iter.interval());
                    }else{
                        return ptr_ring->distinct_OS_P(iter.interval());
                    }
                }else {//iter.level == 1
                    if(iter.state[0] == s){
                        return ptr_ring->distinct_SO_P(iter.interval());
                    }else{
                        return ptr_ring->distinct_OS_P(iter.interval());
                    }
                }
            }

            template<class Iterator, class Ring>
            static uint64_t object(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 0) return ptr_ring->max_o;
                if(iter.level == 2){
                    if(iter.state[0] == s && iter.state[1] == p){
                        return ptr_ring->distinct_SP_O(iter.interval());
                    }else{
                        return ptr_ring->distinct_PS_O(iter.interval());
                    }
                }else {//iter.level == 1
                    if(iter.state[0] == s){
                        return ptr_ring->distinct_SP_O(iter.interval());
                    }else{
                        return ptr_ring->distinct_PS_O(iter.interval());
                    }
                }
            }

        };

        template <uint64_t k=1>
        struct trait_intersect_uni {

            template<class Iterator, class Ring>
            static std::vector<uint64_t> get(Ring* ptr_ring, Iterator &iter, state_type state){
                if(state == s){
                    if(iter.level == 1 && iter.state[0] == o){
                        return ptr_ring->ai_fwd_S(k, iter.interval());
                    }else{
                        return ptr_ring->ai_bwd_S(k, iter.interval());
                    }
                }else if (state == p){
                    if(iter.level == 1 && iter.state[0] == s){
                        return ptr_ring->ai_fwd_P(k, iter.interval());
                    }else{
                        return ptr_ring->ai_bwd_P(k, iter.interval());
                    }
                }else{
                    if(iter.level == 1 && iter.state[0] == p){
                        return ptr_ring->ai_fwd_O(k, iter.interval());
                    }else{
                        return ptr_ring->ai_bwd_O(k, iter.interval());
                    }
                };
            }

            template<class Iterator, class Ring>
            static std::vector<uint64_t> subject(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 1 && iter.state[0] == o){
                    return ptr_ring->ai_fwd_S(k, iter.interval());
                }else{
                    return ptr_ring->ai_bwd_S(k, iter.interval());
                }
            }

            template<class Iterator, class Ring>
            static std::vector<uint64_t> predicate(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 1 && iter.state[0] == s){
                    return ptr_ring->ai_fwd_P(k, iter.interval());
                }else{
                    return ptr_ring->ai_bwd_P(k, iter.interval());
                }
            }

            template<class Iterator, class Ring>
            static std::vector<uint64_t> object(Ring* ptr_ring, Iterator &iter){
                if(iter.level == 1 && iter.state[0] == p){
                    return ptr_ring->ai_fwd_O(k, iter.interval());
                }else{
                    return ptr_ring->ai_bwd_O(k, iter.interval());
                }
            }
        };

        struct op_minimum {

            typedef uint64_t weight_type;

            static inline void init(weight_type &weight, weight_type value){
                weight = value;
            }

            static inline void add(weight_type &weight, weight_type value){
                if(value < weight) weight = value;
            }
        };

        struct op_product {
            typedef double weight_type;

            static inline void init(weight_type &weight, weight_type value){
                weight = value;
            }

            static inline void add(weight_type &weight, weight_type value){
                weight *= value;
            }
        };
    }

}



#endif