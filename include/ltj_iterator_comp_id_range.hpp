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

#ifndef RING_LTJ_ITERATOR_COMP_ID_RANGE_HPP
#define RING_LTJ_ITERATOR_COMP_ID_RANGE_HPP

#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_comp_id_range : public ltj_iterator_base<var_t, cons_t> {
    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;

    private:
        var_type m_var;
        bool m_comp_edges;
        ring_type *m_ptr_ring;

        value_type m_lower_bound;  // Inclusive lower bound
        value_type m_upper_bound;  // Inclusive upper bound
        bool m_has_lower;
        bool m_has_upper;

        bool m_is_empty = false;
        bool m_is_fixed = false;
        value_type m_current_value = 0;
        size_type m_elements;

        double_t m_selectivity;

        void copy(const ltj_iterator_comp_id_range &o) {
            m_var = o.m_var;
            m_comp_edges = o.m_comp_edges;
            m_ptr_ring = o.m_ptr_ring;
            m_lower_bound = o.m_lower_bound;
            m_upper_bound = o.m_upper_bound;
            m_has_lower = o.m_has_lower;
            m_has_upper = o.m_has_upper;
            m_is_empty = o.m_is_empty;
            m_is_fixed = o.m_is_fixed;
            m_current_value = o.m_current_value;
            m_elements = o.m_elements;
            m_selectivity = o.m_selectivity;

        }

        double_t compute_selectivity() {
            value_type query_range = m_upper_bound - m_lower_bound + 1;
            return static_cast<double_t>(query_range) / static_cast<double_t>(m_elements);
        }

    public:
        ltj_iterator_comp_id_range() = default;

        // Constructor for range queries on variable IDs
        ltj_iterator_comp_id_range(var_type var, bool comp_edges,
                                   value_type lower, value_type upper,
                                   bool has_lower, bool has_upper, ring_type *ring) {
            m_var = var;
            m_comp_edges = comp_edges;
            m_ptr_ring = ring;
            m_has_lower = has_lower;
            m_has_upper = has_upper;

            m_elements = comp_edges ? ring->n_triples : ring->max_s;

            // Set bounds
            m_lower_bound = has_lower ? lower : 1;
            m_upper_bound = has_upper ? upper : m_elements;

            // Check if range is valid
            if (m_lower_bound > m_upper_bound) {
                m_is_empty = true;
                m_selectivity = 0.0;
            } else {
                m_selectivity = compute_selectivity();
            }
        }

        //! Copy constructor
        ltj_iterator_comp_id_range(const ltj_iterator_comp_id_range &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_comp_id_range(ltj_iterator_comp_id_range &&o) noexcept {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_comp_id_range &operator=(const ltj_iterator_comp_id_range &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_comp_id_range &operator=(ltj_iterator_comp_id_range &&o) noexcept {
            if (this != &o) {
                m_var = std::move(o.m_var);
                m_comp_edges = std::move(o.m_comp_edges);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_lower_bound = std::move(o.m_lower_bound);
                m_upper_bound = std::move(o.m_upper_bound);
                m_has_lower = std::move(o.m_has_lower);
                m_has_upper = std::move(o.m_has_upper);
                m_is_empty = std::move(o.m_is_empty);
                m_is_fixed = std::move(o.m_is_fixed);
                m_current_value = std::move(o.m_current_value);
                m_elements = std::move(o.m_elements);
                m_selectivity = std::move(o.m_selectivity);
            }
            return *this;
        }

        void swap(ltj_iterator_comp_id_range &o) noexcept {
            std::swap(m_var, o.m_var);
            std::swap(m_comp_edges, o.m_comp_edges);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_lower_bound, o.m_lower_bound);
            std::swap(m_upper_bound, o.m_upper_bound);
            std::swap(m_has_lower, o.m_has_lower);
            std::swap(m_has_upper, o.m_has_upper);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_is_fixed, o.m_is_fixed);
            std::swap(m_current_value, o.m_current_value);
            std::swap(m_elements, o.m_elements);
            std::swap(m_selectivity, o.m_selectivity);
        }

        inline bool is_variable_subject(var_type var) {
            return false;
        }

        inline bool is_variable_predicate(var_type var) {
            return false;
        }

        inline bool is_variable_object(var_type var) {
            return false;
        }

        inline bool is_empty() {
            return m_is_empty;
        }

        void down(var_type var, size_type c) {
            if (var == m_var && !m_is_fixed) {
                m_is_fixed = true;
                m_current_value = c;
            }
        }

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        }

        void up(var_type var) {
            if (var == m_var && m_is_fixed) {
                m_is_fixed = false;
                m_current_value = 0;
            }
        }

        value_type leap(var_type var) {
            return leap(var, 1);
        }

        value_type leap(var_type var, size_type c) {
            if (c < m_lower_bound) {
                return m_lower_bound;
            }
            if (c > m_upper_bound) {
                return 0;
            }
            return c;
        }

        inline bool in_last_level() {
            return false;
        }

        inline size_type interval_length() const {
            if (m_is_empty) return 0;
            return m_upper_bound - m_lower_bound + 1;
        }

        inline double_t selectivity() const {
            return m_selectivity;
        }

        inline double_t opt_selectivity() const {
            return m_selectivity;
        }

        value_type seek_last(var_type var) {
            return 0;
        }

        value_type seek_last_next(var_type var) {
            return 0;
        }
    };
}

#endif //RING_LTJ_ITERATOR_COMP_ID_RANGE_HPP

