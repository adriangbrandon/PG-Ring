/*
 * ltj_iterator_range.hpp
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

#ifndef RING_LTJ_ITERATOR_RANGE_HPP
#define RING_LTJ_ITERATOR_RANGE_HPP

#define VERBOSE 0

#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_comp_range : public ltj_iterator_base<var_t, cons_t> {
    public:
        typedef cons_t id_type;
        typedef int64_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef query::where_expr_parser::expr_property_type expr_type;

    private:
        var_type m_var;
        uint32_t m_property_id;
        bool m_is_edge;
        ring_type *m_ptr_ring;


        value_type m_min_prop; // Minimum property value in the dataset
        value_type m_max_prop; // Maximum property value in the dataset
        size_type m_cnt_prop; // Total count of values for this property
        size_type m_elements; // Total elements

        value_type m_lower_bound;  // Inclusive lower bound
        value_type m_upper_bound;  // Inclusive upper bound
        bool m_has_lower;
        bool m_has_upper;

        bool m_is_empty = false;
        bool m_is_fixed = false;
        size_type m_current_value = 0;

        double_t m_selectivity;

        void copy(const ltj_iterator_comp_range &o) {
            m_var = o.m_var;
            m_property_id = o.m_property_id;
            m_is_edge = o.m_is_edge;
            m_ptr_ring = o.m_ptr_ring;
            m_min_prop = o.m_min_prop;
            m_max_prop = o.m_max_prop;
            m_cnt_prop = o.m_cnt_prop;
            m_lower_bound = o.m_lower_bound;
            m_upper_bound = o.m_upper_bound;
            m_has_lower = o.m_has_lower;
            m_has_upper = o.m_has_upper;
            m_is_empty = o.m_is_empty;
            m_is_fixed = o.m_is_fixed;
            m_current_value = o.m_current_value;
            m_selectivity = o.m_selectivity;
        }

    public:
        ltj_iterator_comp_range() = default;

        // Constructor for range queries
        ltj_iterator_comp_range(var_type var, uint32_t property_id, bool is_edge,
                          value_type lower, value_type upper,
                          bool has_lower, bool has_upper, ring_type *ring) {
            m_var = var;
            m_property_id = property_id;
            m_is_edge = is_edge;
            m_ptr_ring = ring;
            m_has_lower = has_lower;
            m_has_upper = has_upper;

            if (m_is_edge) {
                m_elements = m_ptr_ring->n_triples;
                m_cnt_prop = m_ptr_ring->cnt_edge_property_value(m_property_id);
                std::tie(m_min_prop, m_max_prop) = m_ptr_ring->get_edge_property_range(m_property_id);
            } else {
                m_elements = m_ptr_ring->max_s;
                m_cnt_prop = m_ptr_ring->cnt_node_property_value(m_property_id);
                std::tie(m_min_prop, m_max_prop) = m_ptr_ring->get_node_property_range(m_property_id);
            }

            // Set bounds, using max values if not specified
            if (has_lower) {
                m_lower_bound = lower;
            } else {
                m_lower_bound = m_min_prop;
            }

            if (has_upper) {
                m_upper_bound = upper;
            } else {
                // Use max node/edge id as upper bound
                m_upper_bound = m_max_prop;
            }

            // Check if range is valid
            if (m_lower_bound > m_upper_bound || m_upper_bound < m_min_prop || m_lower_bound > m_max_prop) {
                m_is_empty = true;
                return;
            }
            double_t e = static_cast<double_t>(m_cnt_prop) / static_cast<double_t>(m_elements);
            double_t p = static_cast<double_t>(m_max_prop - m_min_prop) / static_cast<double_t>(m_upper_bound - m_lower_bound);
            m_selectivity = p * e;
        }

        //! Copy constructor
        ltj_iterator_comp_range(const ltj_iterator_comp_range &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_comp_range(ltj_iterator_comp_range &&o) noexcept {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_comp_range &operator=(const ltj_iterator_comp_range &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_comp_range &operator=(ltj_iterator_comp_range &&o) noexcept {
            if (this != &o) {
                m_var = std::move(o.m_var);
                m_property_id = std::move(o.m_property_id);
                m_is_edge = std::move(o.m_is_edge);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_min_prop = std::move(o.m_min_prop);
                m_max_prop = std::move(o.m_max_prop);
                m_cnt_prop = std::move(o.m_cnt_prop);
                m_elements = std::move(o.m_elements);
                m_lower_bound = std::move(o.m_lower_bound);
                m_upper_bound = std::move(o.m_upper_bound);
                m_has_lower = std::move(o.m_has_lower);
                m_has_upper = std::move(o.m_has_upper);
                m_is_empty = std::move(o.m_is_empty);
                m_is_fixed = std::move(o.m_is_fixed);
                m_current_value = std::move(o.m_current_value);
                m_selectivity = std::move(o.m_selectivity);
            }
            return *this;
        }

        void swap(ltj_iterator_comp_range &o) noexcept {
            std::swap(m_var, o.m_var);
            std::swap(m_property_id, o.m_property_id);
            std::swap(m_is_edge, o.m_is_edge);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_min_prop, o.m_min_prop);
            std::swap(m_max_prop, o.m_max_prop);
            std::swap(m_cnt_prop, o.m_cnt_prop);
            std::swap(m_elements, o.m_elements);
            std::swap(m_lower_bound, o.m_lower_bound);
            std::swap(m_upper_bound, o.m_upper_bound);
            std::swap(m_has_lower, o.m_has_lower);
            std::swap(m_has_upper, o.m_has_upper);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_is_fixed, o.m_is_fixed);
            std::swap(m_current_value, o.m_current_value);
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
            m_is_fixed = true;
            m_current_value = c;
        }

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        }

        void up(var_type var) {
            m_is_fixed = false;
        }

        id_type leap(var_type var) {
            return leap(var, 1);
        }

        id_type leap(var_type var, size_type c) {
            // Find next node/edge that has a property value in the range [lower_bound, upper_bound]
            // Strategy: use next_ge(lower_bound) and check if result <= upper_bound
            std::pair<id_type, value_type> result;
            if (c > m_upper_bound) return 0;
            if (c < m_lower_bound) c = m_lower_bound;
            if (m_is_edge) {
                // For edges, find next with value >= lower_bound
                result = m_ptr_ring->next_edge_property(m_property_id, c, m_lower_bound, m_upper_bound);
                m_current_value = result.second;
                return result.first;
            } else {
                // For nodes, find next with value >= lower_bound
                result = m_ptr_ring->next_node_property(m_property_id, c, m_lower_bound, m_upper_bound);
                m_current_value = result.second;
                return result.first;
            }
        }

        inline bool in_last_level() {
            return false;
        }

        inline size_type interval_length() const {
            return UINT64_MAX; // infinite
        }

        inline double_t selectivity() const {
            return m_selectivity;
        }

        inline double_t opt_selectivity() const {
            return m_selectivity;
        }

        id_type seek_last(var_type var) {
            return 0;
        }

        id_type seek_last_next(var_type var) {
            return 0;
        }
    };
}

#endif //RING_LTJ_ITERATOR_RANGE_HPP

