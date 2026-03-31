/*
 * ltj_iterator_comp_id_range.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 * Range iterator for ID comparisons (without properties)
 * Handles: ?v > c1 AND ?v < c2, etc.
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
            m_selectivity = o.m_selectivity;
        }

        double_t compute_selectivity() {
            value_type max_id = m_comp_edges ? m_ptr_ring->n_triples : m_ptr_ring->max_s;

            value_type effective_lower = m_has_lower ? m_lower_bound : 1;
            value_type effective_upper = m_has_upper ? m_upper_bound : max_id;

            // If range is invalid, empty
            if (effective_lower > effective_upper) {
                m_is_empty = true;
                return 0.0;
            }

            value_type total_range = max_id;
            value_type query_range = effective_upper - effective_lower + 1;

            if (total_range <= 0) return 0.0;

            return static_cast<double_t>(query_range) / static_cast<double_t>(total_range);
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

            value_type max_id = comp_edges ? ring->n_triples : ring->max_s;

            // Set bounds
            m_lower_bound = has_lower ? lower : 1;
            m_upper_bound = has_upper ? upper : max_id;

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
                // Check if value is in range
                if (c < m_lower_bound || c > m_upper_bound) {
                    m_is_empty = true;
                }
            }
        }

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        }

        void up(var_type var) {
            if (var == m_var && m_is_fixed) {
                m_is_fixed = false;
                m_current_value = 0;
                m_is_empty = (m_lower_bound > m_upper_bound); // Reset to initial empty state
            }
        }

        value_type leap(var_type var) {
            return leap(var, 1);
        }

        value_type leap(var_type var, size_type c) {
            if (var != m_var) return 0;

            // Find next ID in range [lower_bound, upper_bound]
            // c is the starting point

            if (c < m_lower_bound) {
                // c is below the range, start from lower_bound
                c = m_lower_bound;
            }

            // Check if c is within range
            if (c <= m_upper_bound) {
                // Verify that this ID exists (is valid)
                value_type max_id = m_comp_edges ? m_ptr_ring->n_triples : m_ptr_ring->max_s;
                if (c <= max_id) {
                    return c;
                }
            }

            // c is beyond the range
            return 0;
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

