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

#ifndef RING_LTJ_ITERATOR_NODE_COMP_HPP
#define RING_LTJ_ITERATOR_NODE_COMP_HPP

#define VERBOSE 0


#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_node_comp : public ltj_iterator_base<var_t, cons_t> {
    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef query::triple_parser::triple_type pattern_type;
        typedef query::where_expr_parser::expr_property_type expr_type;
        typedef wt_range_iterator<typename ring_type::bwt_type::wm_type> wt_so_iterator_type;
        typedef wt_range_iterator<typename ring_type::bwt_p_type::wm_type> wt_p_iterator_type;
        //std::vector<value_type> leap_result_type;

    private:
        const expr_type *m_expr;
        ring_type *m_ptr_ring; //TODO: should be const

        std::array<value_type, 2> m_prop_values;
        std::array<bool, 2> m_state = {false, false};
        size_type m_nfixed = 0;
        bool m_is_empty = false;


        void copy(const ltj_iterator_node_comp &o) {
            m_is_empty = o.m_is_empty;
            m_prop_values = o.m_prop_values;
            m_state = o.m_state;
            m_nfixed = o.m_nfixed;
            m_expr = o.m_expr;
            m_ptr_ring = o.m_ptr_ring;
        }

    public:
        //const bool &is_empty = m_is_empty;

        ltj_iterator_node_comp() = default;

        ltj_iterator_node_comp(const expr_type *expr, ring_type *ring) {
            m_ptr_ring = ring;
            m_expr = expr;
            m_is_empty = false;
            if (!m_expr->is_var[0] && m_expr->is_var[1]) {
                m_nfixed = 1;
                m_prop_values[0] = m_expr->values[0];
                m_state[0] = true; //fixed the first element
            }else if (m_expr->is_var[0] && !m_expr->is_var[1]) {
                m_nfixed = 1;
                m_prop_values[1] = m_expr->values[1];
                m_state[0] = true; //fixed the second element
            }
        }

        //! Copy constructor
        ltj_iterator_node_comp(const ltj_iterator_node_comp &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_node_comp(ltj_iterator_node_comp &&o) noexcept {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_node_comp &operator=(const ltj_iterator_node_comp &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_node_comp &operator=(ltj_iterator_node_comp &&o) noexcept {
            if (this != &o) {
                m_expr = std::move(o.m_expr);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_prop_values = std::move(o.m_prop_values);
                m_state = std::move(o.m_state);
                m_nfixed = std::move(o.m_nfixed);
                m_is_empty = std::move(o.m_is_empty);
            }
            return *this;
        }

        void swap(ltj_iterator_node_comp &o) noexcept {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_expr, o.m_expr);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_prop_values, o.m_prop_values);
            std::swap(m_state, o.m_state);
            std::swap(m_nfixed, o.m_nfixed);
            std::swap(m_is_empty, o.m_is_empty);
        }


        inline bool is_variable_subject(var_type var) { //useless
            return false;
        }

        inline bool is_variable_predicate(var_type var) { //useless
            return false;
        }

        inline bool is_variable_object(var_type var) { //useless
            return false;
        }


        inline bool is_empty() {
            return m_is_empty;
        }

        void down(var_type var, size_type c) {
            //Go down in the trie
            if (m_expr->is_var[0] && var == m_expr->values[0]) {
                m_state[0] = true;
            }else if (m_expr->is_var[1] && var == m_expr->values[1]) {
                m_state[1] = true;
            }
            ++m_nfixed;
        };

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        };

        void set_prop_value(var_type var, value_type value) {
            if (!m_nfixed) return;
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                m_prop_values[0] = value;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                m_prop_values[1] = value;
            }
        }

        value_type get_prop_value(var_type var) {
            if (!m_nfixed) return 0;
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                return m_prop_values[0];
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                return m_prop_values[1];
            }
            return 0;
        }

        value_type compute_prop_value(var_type var, value_type c) {
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                return m_ptr_ring->get_node_property_value(m_expr->property_values[0], c);
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                return m_ptr_ring->get_node_property_value(m_expr->property_values[1], c);
            }
            return 0;
        }


        void up(var_type var) {
            //Go up in the trie
            if (m_expr->is_var[0] && var == m_expr->values[0]) {
                m_prop_values[0] = false;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1]) {
                m_prop_values[1] = false;
            }
            --m_nfixed;
        };

        value_type leap(var_type var) {
            return leap(var, 1);
        };

        value_type leap(var_type var, size_type c) {
            if (!m_nfixed) return c; // no filter is applied
            if (m_expr->is_var[0] && var == m_expr->values[0]) {
                auto res = m_ptr_ring->next_node_property(m_expr->property_values[0], c, m_prop_values[1], m_expr->type);
                m_prop_values[0] = res.second; // set the value of the property
                return res.first;
            }else if (m_expr->is_var[1] && var == m_expr->values[1]) {
                auto res = m_ptr_ring->next_node_property(m_expr->property_values[1], c, m_prop_values[0], query::opposite_expr_property[m_expr->type]);
                m_prop_values[1] = res.second; // set the value of the property
                return res.first;
            }
            return 0;
        };

        inline bool in_last_level() {
            return false;
        }

        inline size_type interval_length() const {
            return 1; //TODO: fix this depending on the priority
        }

        value_type seek_last(var_type var) {
            return 0;
        }

        value_type seek_last_next(var_type var) {
            return 0;
        }
    };
}

#endif //RING_LTJ_ITERATOR_NODE_COMP_HPP
