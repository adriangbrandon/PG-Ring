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

#ifndef RING_LTJ_ITERATOR_COMP_ID_HPP
#define RING_LTJ_ITERATOR_COMP_ID_HPP

#define VERBOSE 0


#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_comp_id : public ltj_iterator_base<var_t, cons_t> {
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

        std::array<value_type, 2> m_fixed_values;
        std::array<bool, 2> m_state = {false, false};
        size_type m_nfixed = 0;
        bool m_is_empty = false;

        double_t m_selectivity_no_fixed;


        void copy(const ltj_iterator_comp_id &o) {
            m_is_empty = o.m_is_empty;
            m_fixed_values = o.m_fixed_values;
            m_state = o.m_state;
            m_nfixed = o.m_nfixed;
            m_expr = o.m_expr;
            m_ptr_ring = o.m_ptr_ring;
        }

        value_type next(value_type c, value_type fv, query::enum_comp_where_type t) {
            switch (t) {
                case query::EQ:
                    if (c <= fv) return fv;
                    return 0;
                case query::NEQ:
                    if (c == fv) return fv+1;
                    return c;
                case query::ST:
                    if (c < fv) return c;
                    return 0;
                case query::GT:
                    if (c > fv) return c;
                    return fv+1;
                case query::SE:
                    if (c <= fv) return c;
                    return 0;
                case query::GE:
                    if (c >= fv) return c;
                    return fv;
                default:
                    return 0;
            }
        }

    public:
        //const bool &is_empty = m_is_empty;

        ltj_iterator_comp_id() = default;

        ltj_iterator_comp_id(const expr_type *expr, ring_type *ring) {
            m_ptr_ring = ring;
            m_expr = expr;
            m_is_empty = false;
            m_fixed_values = {0, 0};
            if (!m_expr->is_var[0] && m_expr->is_var[1]) {
                m_nfixed = 1;
                m_fixed_values[0] = m_expr->values[0];
                m_state[0] = true; //fixed the first element
            }else if (m_expr->is_var[0] && !m_expr->is_var[1]) {
                m_nfixed = 1;
                m_fixed_values[1] = m_expr->values[1];
                m_state[1] = true; //fixed the second element
            }
        }

        //! Copy constructor
        ltj_iterator_comp_id(const ltj_iterator_comp_id &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_comp_id(ltj_iterator_comp_id &&o) noexcept {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_comp_id &operator=(const ltj_iterator_comp_id &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_comp_id &operator=(ltj_iterator_comp_id &&o) noexcept {
            if (this != &o) {
                m_expr = std::move(o.m_expr);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_fixed_values = std::move(o.m_fixed_values);
                m_state = std::move(o.m_state);
                m_nfixed = std::move(o.m_nfixed);
                m_is_empty = std::move(o.m_is_empty);
            }
            return *this;
        }

        void swap(ltj_iterator_comp_id &o) noexcept {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_expr, o.m_expr);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_fixed_values, o.m_fixed_values);
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
                m_fixed_values[0] = c;
            }else if (m_expr->is_var[1] && var == m_expr->values[1]) {
                m_state[1] = true;
                m_fixed_values[1] = c;
            }
            ++m_nfixed;
        };

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        };


        void up(var_type var) {
            //Go up in the trie
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                m_fixed_values[0] = 0;
                m_state[0] = false;
                --m_nfixed;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                m_fixed_values[1] = 0;
                m_state[1] = false;
                --m_nfixed;
            }
        };

        value_type leap(var_type var) {
            return leap(var, 1);
        };

        value_type leap(var_type var, size_type c) {
            if (!m_nfixed) return c;
            if (m_expr->is_var[0] && var == m_expr->values[0]) {
                return next(c, m_fixed_values[1], m_expr->type);
            }
            if (m_expr->is_var[1] && var == m_expr->values[1]) {
                return next(c, m_fixed_values[0], query::opposite_comp_where[m_expr->type]);
            }
            return 0;
        };

        inline bool in_last_level() {
            return false;
        }

        inline size_type interval_length() const {
            return UINT64_MAX; //infinite
        }

        inline double_t selectivity() const {
            switch (m_expr->type) {
                case query::EQ:
                    return 0.1;
                case query::NEQ:
                    return 0.9;
                default:
                    return 0.5;
            }
            return 1.0;
            //if (!m_nfixed) return CONSTANTE;
            //return m_ptr_ring->contar_in_range()/m_ptr_ring->n_nodes();
        }

        value_type seek_last(var_type var) {
            return 0;
        }

        value_type seek_last_next(var_type var) {
            return 0;
        }
    };
}

#endif //RING_LTJ_ITERATOR_COMP_ID_HPP
