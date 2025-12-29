/*
 * ltj_iterator.hpp
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

#ifndef RING_LTJ_ITERATOR_HPP
#define RING_LTJ_ITERATOR_HPP

#define VERBOSE 0


#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator : public ltj_iterator_base<var_t, cons_t> {
    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef query::triple_parser::triple_type pattern_type;
        typedef wt_range_iterator<typename ring_type::bwt_type::wm_type> wt_so_iterator_type;
        typedef wt_range_iterator<typename ring_type::bwt_p_type::wm_type> wt_p_iterator_type;
        //std::vector<value_type> leap_result_type;

    private:
        const pattern_type *m_pattern;
        ring_type *m_ptr_ring; //TODO: should be const
        std::array<bwt_interval, 3> m_intervals;
        std::array<value_type, 3> m_consts;
        std::array<state_type, 3> m_state;
        //wt_so_iterator_type m_so_last_iterator;
        //wt_p_iterator_type m_p_last_iterator;
        size_type m_level = 0;
        bool m_is_empty = false;
        size_type m_triple_j = 0;
        //std::stack<state_type> m_states;


        void copy(const ltj_iterator &o) {
            m_is_empty = o.m_is_empty;
            m_pattern = o.m_pattern;
            m_ptr_ring = o.m_ptr_ring;
            m_intervals = o.m_intervals;
            m_state = o.m_state;
            m_level = o.m_level;
            m_consts = o.m_consts;
            m_triple_j = o.m_triple_j;
        }

        bool down_to_E(value_type e, size_type l) {
            m_intervals[l]= bwt_interval{e, e};
            return true;
        }

    public:
        //const bool &is_empty = m_is_empty;
        const size_type &level = m_level;
        const std::array<state_type, 3> &state = m_state;
        const std::array<value_type, 3> &consts = m_consts;

        ltj_iterator() = default;

        ltj_iterator(const pattern_type *triple, ring_type *ring) {
            m_pattern = triple;
            m_ptr_ring = ring;

            m_intervals[0] = m_ptr_ring->open_POS();

            if (!m_pattern->subj.is_var() && !m_pattern->obj.is_var()) {

                //Interval in S
                auto s_aux = m_ptr_ring->next_S(m_intervals[0], m_pattern->subj.const_value);
                //Is the constant of S in m_i_s?
                if (s_aux != m_pattern->subj.const_value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = s_aux;

                //Interval in O
                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                auto o_aux = m_ptr_ring->next_O_in_S(m_intervals[1], m_pattern->obj.const_value);
                //Is the constant of O in m_i_o?
                if (o_aux != m_pattern->obj.const_value) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;
                m_consts[1] = o_aux;

                m_intervals[2] = m_ptr_ring->down_S_O(m_intervals[1], o_aux);
                m_level = 2;
            } else if (m_pattern->subj.is_var() && !m_pattern->obj.is_var()) {

                //Interval in O
                auto o_aux = m_ptr_ring->next_O(m_intervals[0], m_pattern->obj.const_value);
                //Is the constant of O in m_i_s?
                if (o_aux != m_pattern->obj.const_value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = o;
                m_consts[0] = o_aux;
                m_intervals[1] = m_ptr_ring->down_O(o_aux);
                m_level = 1;
            } else if (!m_pattern->subj.is_var()) {//&& m_pattern->obj.is_var()

                //Interval in S
                auto s_aux = m_ptr_ring->next_S(m_intervals[0], m_pattern->subj.const_value);
                //Is the constant of S in m_i_s?
                if (s_aux != m_pattern->subj.const_value) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = s_aux;
                m_intervals[1] = m_ptr_ring->down_S(s_aux);
                m_level = 1;
            }
        }

        //! Copy constructor
        ltj_iterator(const ltj_iterator &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator(ltj_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator &operator=(const ltj_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator &operator=(ltj_iterator &&o) {
            if (this != &o) {
                m_pattern = std::move(o.m_pattern);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_intervals = std::move(o.m_intervals);
                m_consts = std::move(o.m_consts);
                m_state = std::move(o.m_state);
                m_level = o.m_level;
                m_is_empty = o.m_is_empty;
                m_triple_j = o.m_triple_j;
            }
            return *this;
        }

        void swap(ltj_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_pattern, o.m_pattern);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_intervals, o.m_intervals);
            std::swap(m_consts, o.m_consts);
            std::swap(m_state, o.m_state);
            std::swap(m_level, o.m_level);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_triple_j, o.m_triple_j);
        }


        inline bool is_variable_subject(var_type var) {
            return m_pattern->subj.var_value == var;
        }

        inline bool is_variable_predicate(var_type var) {
            return m_pattern->edge.var_value == var;
        }

        inline bool is_variable_object(var_type var) {
            return m_pattern->obj.var_value == var;
        }


        inline bool is_empty() {
            return m_is_empty;
        }

        void down(var_type var, size_type c) {
            //Go down in the trie
            if (m_level > 2) return;
            if (m_level == 0) {
                if (is_variable_subject(var)) {
                    m_intervals[1] = m_ptr_ring->down_S(c);
                    m_state[m_level] = s;
                } else if (is_variable_predicate(var)) {
                    down_to_E(c, m_level+1);
                    m_state[m_level] = p;
                } else {
                    m_intervals[1] = m_ptr_ring->down_O(c);
                    m_state[m_level] = o;
                }
            } else if (m_level == 1) {
                if (is_variable_subject(var)) {
                    if (m_state[0] != p) {
                        m_intervals[2] = m_ptr_ring->down_O_S(m_intervals[1], m_consts[0], c);
                    }
                    m_state[m_level] = s;
                } else if (is_variable_predicate(var)) {
                    down_to_E(c, m_level+1);
                    m_state[m_level] = p;
                } else {
                    if (m_state[0] != p) {
                        m_intervals[2] = m_ptr_ring->down_S_O(m_intervals[1], c);
                    }
                    m_state[m_level] = o;
                }
            }
            m_consts[m_level] = c;
            ++m_level;
        };

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        };


        void up(var_type var) {
            //Go up in the trie
            if (m_level == 0) return;
            --m_level;
        };

        value_type leap(var_type var) {
            std::cout << "leap: var=" << (uint) var << ", level=" << m_level << std::endl;
            //Return the minimum in the range
            //0. Which term of our triple pattern is var
            if (m_level == 0) {
                if (is_variable_subject(var)) {
                    return m_ptr_ring->min_S(m_intervals[0]);
                }else if (is_variable_predicate(var)) {
                    return 1;
                } else {
                    return m_ptr_ring->min_O(m_intervals[0]);
                }
            } else if (m_level == 1) {
                if (m_state[0] == s) {
                    if (is_variable_predicate(var)) {
                        return m_ptr_ring->min_E_in_SP(m_intervals[0], m_consts[0]);
                    }
                    if (is_variable_object(var)) {
                        return m_ptr_ring->min_O_in_S(m_intervals[1]);
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->edge_expr_get_S(m_consts[0]);
                    }
                    if (is_variable_object(var)) {
                        return m_ptr_ring->edge_expr_get_O(m_consts[0]);
                    }
                } else if (m_state[0] == o) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->min_S_in_O(m_intervals[1], m_consts[0]);
                    }
                    if (is_variable_predicate(var)) {
                        return m_ptr_ring->min_E_in_OS(m_intervals[1]);
                    }
                }
            } else if (m_level == 2) {
                if (is_variable_subject(var)) {
                    auto i = (m_state[0] == p) ? 0 : 1;
                    return m_ptr_ring->edge_expr_get_S(m_consts[i]);
                }
                if (is_variable_object(var)) {
                    auto i = (m_state[0] == p) ? 0 : 1;
                    return m_ptr_ring->edge_expr_get_O(m_consts[i]);
                }
                if (is_variable_predicate(var)) {
                    auto v =  m_ptr_ring->map_OSP_to_POS( m_intervals[2].left());
                    return v;
                }
            }
            throw std::out_of_range("ltj_iterator::leap");
        };

        value_type leap(var_type var, size_type c) {
            std::cout << "leap: var=" << (uint) var << " c=" << c << ", level=" << m_level << std::endl;
            //Return the minimum in the range
            //0. Which term of our triple pattern is var
            if (m_level == 0) {
                if (is_variable_subject(var)) {
                    return m_ptr_ring->next_S(m_intervals[0], c);
                }else if (is_variable_predicate(var)) {
                    if (c > m_ptr_ring->n_triples) return 0;
                    return c;
                } else {
                    return m_ptr_ring->next_O(m_intervals[0], c);
                }
            } else if (m_level == 1) {
                if (m_state[0] == s) {
                    if (is_variable_predicate(var)) {
                        return m_ptr_ring->next_E_in_SP(m_intervals[0], m_consts[0], c);
                    }
                    if (is_variable_object(var)) {
                        return m_ptr_ring->next_O_in_S(m_intervals[1], c);
                    }
                } else if (m_state[0] == p) {
                    if (is_variable_subject(var)) {
                        auto v = m_ptr_ring->edge_expr_get_S(m_consts[0]);
                        if (v >= c) return v;
                        return 0;
                    }
                    if (is_variable_object(var)) {
                        auto v = m_ptr_ring->edge_expr_get_O(m_consts[0]);
                        if (v >= c) return v;
                        return 0;
                    }
                } else if (m_state[0] == o) {
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->next_S_in_O(m_intervals[1], m_consts[0], c);
                    }
                    if (is_variable_predicate(var)) {
                        return m_ptr_ring->next_E_in_O(m_intervals[1], c);
                    }
                }
            } else if (m_level == 2) {
                if (is_variable_subject(var)) {
                    auto i = (m_state[1] == p);
                    auto v = m_ptr_ring->edge_expr_get_S(m_consts[i]);
                    if (v >= c) return v;
                    return 0;
                }
                if (is_variable_object(var)) {
                    auto i = (m_state[1] == p);
                    auto v = m_ptr_ring->edge_expr_get_O(m_consts[i]);
                    if (v >= c) return v;
                    return 0;
                }
                if (is_variable_predicate(var)) {
                    auto v = m_ptr_ring->next_E_in_OS(m_intervals[2], c);
                    std::cout << "value: " << v << std::endl;
                    return v;
                    //return m_ptr_ring->next_E_in_OS(m_intervals[2], c);
                }
            }

            throw std::out_of_range("ltj_iterator::leap");
        };

        inline bool in_last_level() {
            return m_level == 2;
        }

        inline size_type interval_length() const {
            return m_intervals[m_level].size();
        }

        inline double_t selectivity() const {
            return 1.0;
        }

        inline double_t opt_selectivity() const {
            return 1.0;
        }

        inline const bwt_interval &interval() const {
            return m_intervals[m_level];
        }

        value_type seek_last(var_type var) {
            if (is_variable_subject(var)) {
                auto i = (m_state[1] == p);
                return m_ptr_ring->edge_expr_get_S(m_consts[i]);
            }
            if (is_variable_object(var)) {
                auto i = (m_state[1] == p);
                return m_ptr_ring->edge_expr_get_O(m_consts[i]);
            }
            if (is_variable_predicate(var)) {
                m_triple_j = m_intervals[2].left();
                //both cases are in OSP
                return m_ptr_ring->map_OSP_to_POS(m_triple_j);
            }
            return 0;
        }

        value_type seek_last_next(var_type var) {
            if (!is_variable_predicate(var)) return 0; //no more triples
            ++m_triple_j;
            if (m_triple_j > m_intervals[2].right()) {
                return 0;
            }
            return m_ptr_ring->map_OSP_to_POS(m_triple_j);
        }

        /*void set_prop_value(var_type var, value_type value) {
            throw std::out_of_range("ltj_iterator::set_value_property");
        }

        value_type get_prop_value(var_type var, value_type c) {
            throw std::out_of_range("ltj_iterator::get_prop_value");
        }

        value_type compute_prop_value(var_type var, value_type c) {
            throw std::out_of_range("ltj_iterator::compute_prop_value");
        }*/
    };
}

#endif //RING_LTJ_ITERATOR_HPP
