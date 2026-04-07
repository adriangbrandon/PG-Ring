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

#ifndef RING_LTJ_ITERATOR_COMP_HPP
#define RING_LTJ_ITERATOR_COMP_HPP

#define VERBOSE 0


#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_comp : public ltj_iterator_base<var_t, cons_t> {
    public:
        typedef cons_t id_type;
        typedef int64_t value_type;
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
        std::array<std::pair<id_type, value_type>, 2> m_id_values; //<id, value>
        std::array<bool, 2> m_state = {false, false};
        std::array<bool, 2> m_is_edge = {false, false};
        size_type m_nfixed = 0;
        bool m_is_empty = false;
        bool m_same_var = false; //if compares properties of the same variable
        size_type m_elements;

        double_t m_selectivity_no_fixed;


        void copy(const ltj_iterator_comp &o) {
            m_is_empty = o.m_is_empty;
            m_same_var = o.m_same_var;
            m_fixed_values = o.m_fixed_values;
            m_id_values = o.m_id_values;
            m_state = o.m_state;
            m_is_edge = o.m_is_edge;
            m_nfixed = o.m_nfixed;
            m_expr = o.m_expr;
            m_ptr_ring = o.m_ptr_ring;
            m_elements = o.m_elements;
            m_selectivity_no_fixed = o.m_selectivity_no_fixed;
        }


        //a > b
        double_t selectivity_gt(int min_a, int max_a, int min_b, int max_b) {
            if (max_a <= min_b) return 0.0;
            if (min_a > max_b) return 1.0;
            //triangle and rectangle areas in a 2D plane

            auto overlap = std::max(0, std::min(max_a, max_b) - std::max(min_a, min_b) + 1);
            //0 + 1 ... + overlap-1
            auto triangle = (overlap - 1) * overlap / 2.0;

            //rectangle area
            auto width = std::max(0, max_a - max_b);
            auto rectangle = width * (max_b - min_b + 1);
            auto total = (max_a - min_a + 1) * (max_b - min_b + 1);
            return (triangle + rectangle) / (double_t) total;
        }

        //a >= b
        double_t selectivity_ge(int min_a, int max_a, int min_b, int max_b) {
            if (max_a < min_b) return 0.0;
            if (min_a >= max_b) return 1.0;
            //triangle and rectangle areas in a 2D plane

            auto overlap = std::max(0, std::min(max_a, max_b) - std::max(min_a, min_b) + 1);
            //0 + 1 ... + overlap-1 + overlap
            auto triangle = overlap * (overlap+1) / 2.0;

            //rectangle area
            auto width = std::max(0, max_a - max_b);
            auto rectangle = width * (max_b - min_b + 1);
            auto total = (max_a - min_a + 1) * (max_b - min_b + 1);
            return (triangle + rectangle) / (double_t) total;
        }

        //a == b
        double_t selectivity_eq(int min_a, int max_a, int min_b, int max_b) {
            if (max_a < min_b || max_b < min_b) return 0.0;
            //triangle and rectangle areas in a 2D plane
            auto overlap = std::max(0, std::min(max_a, max_b) - std::max(min_a, min_b) + 1);
            auto total = (max_a - min_a + 1) * (max_b - min_b + 1);
            return overlap / (double_t) total;
        }

        double_t compute_selectivity_no_fixed() {

            //if both properties are of the same type and is the same property id
            if (m_is_edge[0] == m_is_edge[1] && m_expr->property_values[0] == m_expr->property_values[1]) {
                double_t elements = m_elements;
                switch (m_expr->type) {
                    case query::EQ:
                        return 1 / elements;
                    case query::NEQ:
                        return 1 - 1 / elements;
                    default:
                        return 0.5;
                }
            }

            //different properties
            int min_a, max_a, min_b, max_b;
            if (m_is_edge[0]) {
                std::tie(min_a, max_a) = m_ptr_ring->get_edge_property_range(m_expr->property_values[0]);
            } else {
                std::tie(min_a, max_a) = m_ptr_ring->get_node_property_range(m_expr->property_values[0]);
            }
            if (m_is_edge[1]) {
                std::tie(min_b, max_b) = m_ptr_ring->get_edge_property_range(m_expr->property_values[1]);
            } else {
                std::tie(min_b, max_b) = m_ptr_ring->get_node_property_range(m_expr->property_values[1]);
            }

            switch (m_expr->type) {
                case query::EQ:
                    return selectivity_eq(min_a, max_a, min_b, max_b);
                case query::NEQ:
                    return 1.0 - selectivity_eq(min_a, max_a, min_b, max_b);
                case query::ST:
                    return 1.0 - selectivity_ge(min_a, max_a, min_b, max_b);
                case query::GT:
                    return selectivity_gt(min_a, max_a, min_b, max_b);
                case query::SE:
                    return 1.0 - selectivity_gt(min_a, max_a, min_b, max_b);
                case query::GE:
                    return selectivity_ge(min_a, max_a, min_b, max_b);
                default:
                    return 0.0;
            }
        }

        double_t compute_opt_selectivity(value_type c, size_type p) const {
            auto prop_id = m_expr->property_values[p];
            auto type = (p == 0) ? m_expr->type : query::opposite_comp_where[m_expr->type];
            auto fixed_value = (p == 0) ? m_fixed_values[1] : m_fixed_values[0];

            if (m_is_edge[p]) {
                // Edge properties
                double_t total_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id);
                double_t base_selectivity = total_prop_count / (double_t) m_ptr_ring->n_triples;
                double_t range_prop_count;

                switch (type) {
                    case query::EQ:
                        range_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id, fixed_value, fixed_value);
                        return base_selectivity * (range_prop_count / total_prop_count);
                    case query::NEQ:
                        range_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id, fixed_value, fixed_value);
                        return base_selectivity * (1.0 - range_prop_count / total_prop_count);
                    case query::ST:
                        range_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id, 1, fixed_value - 1);
                        return base_selectivity * (range_prop_count / total_prop_count);
                    case query::GT:
                        range_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id, 1, fixed_value);
                        return base_selectivity * (1.0 - range_prop_count / total_prop_count);
                    case query::SE:
                        range_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id, 1, fixed_value);
                        return base_selectivity * (range_prop_count / total_prop_count);
                    case query::GE:
                        range_prop_count = m_ptr_ring->cnt_edge_property_value(prop_id, 1, fixed_value - 1);
                        return base_selectivity * (1.0 - range_prop_count / total_prop_count);
                    default:
                        return 1.0;
                }
            } else {
                // Node properties
                double_t total_prop_count = m_ptr_ring->cnt_node_property_value(prop_id);
                double_t base_selectivity = total_prop_count / (double_t) m_ptr_ring->max_s;
                double_t range_prop_count;

                switch (type) {
                    case query::EQ:
                        range_prop_count = m_ptr_ring->cnt_node_property_value(prop_id, fixed_value, fixed_value);
                        return base_selectivity * (range_prop_count / total_prop_count);
                    case query::NEQ:
                        range_prop_count = m_ptr_ring->cnt_node_property_value(prop_id, fixed_value, fixed_value);
                        return base_selectivity * (1.0 - range_prop_count / total_prop_count);
                    case query::ST:
                        range_prop_count = m_ptr_ring->cnt_node_property_value(prop_id, 1, fixed_value - 1);
                        return base_selectivity * (range_prop_count / total_prop_count);
                    case query::GT:
                        range_prop_count = m_ptr_ring->cnt_node_property_value(prop_id, 1, fixed_value);
                        return base_selectivity * (1.0 - range_prop_count / total_prop_count);
                    case query::SE:
                        range_prop_count = m_ptr_ring->cnt_node_property_value(prop_id, 1, fixed_value);
                        return base_selectivity * (range_prop_count / total_prop_count);
                    case query::GE:
                        range_prop_count = m_ptr_ring->cnt_node_property_value(prop_id, 1, fixed_value - 1);
                        return base_selectivity * (1.0 - range_prop_count / total_prop_count);
                    default:
                        return 1.0;
                }
            }

        }

        double_t compute_selectivity(value_type c, size_type p) const {
            value_type min_a, max_a;
            if (m_is_edge[p]) {
                std::tie(min_a, max_a) = m_ptr_ring->get_edge_property_range(m_expr->property_values[p]);
            } else {
                std::tie(min_a, max_a) = m_ptr_ring->get_node_property_range(m_expr->property_values[p]);
            }

            auto type = (p == 0) ? m_expr->type : query::opposite_comp_where[m_expr->type];
            auto range_size = static_cast<double_t>(max_a - min_a + 1);

            switch (type) {
                case query::EQ:
                    return 1.0 / range_size;
                case query::NEQ:
                    return 1.0 - 1.0 / range_size;
                case query::ST:
                    return 1.0 - std::max(0.0, range_size - static_cast<double_t>(c) + 1.0) / range_size;
                case query::GT:
                    return std::max(0.0, range_size - static_cast<double_t>(c)) / range_size;
                case query::SE:
                    return 1.0 - std::max(0.0, range_size - static_cast<double_t>(c) + 1.0) / range_size;
                case query::GE:
                    return std::max(0.0, range_size - static_cast<double_t>(c) + 1.0) / range_size;
                default:
                    return 1.0;
            }
        }

        template<class T>
        bool compare (const T &a, const T &b) const {
            switch (m_expr->type) {
                case query::EQ: return a == b;
                case query::NEQ: return a != b;
                case query::ST: return a < b;
                case query::GT: return a > b;
                case query::SE: return a <= b;
                case query::GE: return a >= b;
                default: return false;
            }
        };

    public:
        const bool &same_var = m_same_var;

        ltj_iterator_comp() = default;

        ltj_iterator_comp(const expr_type *expr, bool var0_edge, bool var1_edge, ring_type *ring) {
            m_ptr_ring = ring;
            m_expr = expr;
            m_is_empty = false;
            m_fixed_values = {0, 0};
            m_id_values[0] = {0,0};
            m_id_values[1] = {0,0};
            m_is_edge = {var0_edge, var1_edge};
            m_elements = m_is_edge[0] ? m_ptr_ring->n_triples : m_ptr_ring->max_s;
            if (!m_expr->is_var[0] && m_expr->is_var[1]) {
                m_nfixed = 1;
                if (m_expr->strs[0].empty()) {
                    m_fixed_values[0] = m_expr->values[0];
                }else {
                    m_fixed_values[0] = m_ptr_ring->get_string_id(m_expr->strs[0], m_expr->type);
                }
                m_state[0] = true; //fixed the first element
            }else if (m_expr->is_var[0] && !m_expr->is_var[1]) {
                m_nfixed = 1;
                if (m_expr->strs[1].empty()) {
                    m_fixed_values[1] = m_expr->values[1];
                }else {
                    m_fixed_values[1] = m_ptr_ring->get_string_id(m_expr->strs[1], query::opposite_comp_where[m_expr->type]);
                }
                m_state[1] = true; //fixed the second element
            }else {
                m_selectivity_no_fixed = compute_selectivity_no_fixed();
                m_same_var = (m_expr->values[0] == m_expr->values[1]);
            }
        }

        //! Copy constructor
        ltj_iterator_comp(const ltj_iterator_comp &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_comp(ltj_iterator_comp &&o) noexcept {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_comp &operator=(const ltj_iterator_comp &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_comp &operator=(ltj_iterator_comp &&o) noexcept {
            if (this != &o) {
                m_expr = std::move(o.m_expr);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_fixed_values = std::move(o.m_fixed_values);
                m_id_values = std::move(o.m_id_values);
                m_state = std::move(o.m_state);
                m_is_edge = std::move(o.m_is_edge);
                m_nfixed = std::move(o.m_nfixed);
                m_is_empty = std::move(o.m_is_empty);
                m_elements = std::move(o.m_elements);
                m_selectivity_no_fixed = std::move(o.m_selectivity_no_fixed);
            }
            return *this;
        }

        void swap(ltj_iterator_comp &o) noexcept {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_expr, o.m_expr);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_fixed_values, o.m_fixed_values);
            std::swap(m_id_values, o.m_id_values);
            std::swap(m_state, o.m_state);
            std::swap(m_is_edge, o.m_is_edge);
            std::swap(m_nfixed, o.m_nfixed);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_elements, o.m_elements);
            std::swap(m_selectivity_no_fixed, o.m_selectivity_no_fixed);
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

        void down_same_var() {
                m_state[0] = true;
                m_fixed_values[0] = m_id_values[0].second;
                ++m_nfixed;
        };

        void down_old(var_type var, size_type c) {
            //Go down in the trie
            if (m_expr->is_var[0] && var == m_expr->values[0]) {
                m_state[0] = true;
                m_fixed_values[0] = m_id_values[0].second;
            }else if (m_expr->is_var[1] && var == m_expr->values[1]) {
                m_state[1] = true;
                m_fixed_values[1] = m_id_values[1].second;
            }
            ++m_nfixed;
        };

        void down(var_type var, size_type c) {
            //Go down in the trie (if same_var => goes down two levels)
            if (m_expr->is_var[0] && var == m_expr->values[0]) {
                m_state[0] = true;
                m_fixed_values[0] = m_id_values[0].second;
                ++m_nfixed;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1]) {
                m_state[1] = true;
                m_fixed_values[1] = m_id_values[1].second;
                ++m_nfixed;
            }
        };

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        };

        /*void set_prop_value(var_type var, value_type value) {
            std::cout << "{" << m_fixed_values[0] << ", " << m_fixed_values[1] << "} -> ";
            if (!m_nfixed) return;
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                m_fixed_values[0] = value;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                m_fixed_values[1] = value;
            }
            std::cout << "{" << m_fixed_values[0] << ", " << m_fixed_values[1] << "}" << std::endl;
        }

        value_type get_prop_value(var_type var, value_type c) {
            std::cout << "Get prop_value of var " << var << std::endl;
            if (!m_nfixed) return 0;
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_id_values[0].first == c) {
                return m_id_values[0].second;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_id_values[1].first == c) {
                return m_id_values[1].second;
            }
            return 0;
        }

        value_type compute_prop_value(var_type var, value_type c) {
            std::cout << "Compute prop_value of " << c << std::endl;
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                auto v = m_ptr_ring->get_node_property_value(m_expr->property_values[0], c);
                m_id_values[0] = {c, v};
                return v;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                auto v = m_ptr_ring->get_node_property_value(m_expr->property_values[1], c);
                m_id_values[1] = {c, v};
                return v;
            }
            return 0;
        }*/


        void up(var_type var) {
            //Go up in the trie
            if (m_expr->is_var[0] && var == m_expr->values[0] && m_state[0]) {
                m_fixed_values[0] = 0;
                m_id_values[0] = {0, 0};
                m_state[0] = false;
                --m_nfixed;
            }
            if (m_expr->is_var[1] && var == m_expr->values[1] && m_state[1]) {
                m_fixed_values[1] = 0;
                m_id_values[1] = {0, 0};
                m_state[1] = false;
                --m_nfixed;
            }
        };

        id_type leap(var_type var) {
            return leap(var, 1);
        };

        id_type leap(var_type var, size_type c) {

            if (c > m_elements) return 0;
            if (m_same_var) { //comparing two properties of the same variable
                if (m_is_edge[0]) {
                    m_id_values[0].first = c+1;
                    m_id_values[1].first = c;
                    while (true) {
                        m_id_values[0] = m_ptr_ring->next_edge_in_property(m_expr->property_values[0], m_id_values[1].first); //get next id and value of the first property
                        if (m_id_values[0].first == 0) return 0; //no more ids to try
                        m_id_values[1] = m_ptr_ring->next_edge_in_property(m_expr->property_values[1], m_id_values[0].first); //get the value of the second property
                        if (m_id_values[1].first == 0) return 0; //no more ids to try
                        if (m_id_values[0].first == m_id_values[1].first && compare(m_id_values[0].second, m_id_values[1].second)) {
                            return m_id_values[0].first; //the same id satisfies the condition for both properties
                        }else if (m_id_values[0].first == m_id_values[1].first && !compare(m_id_values[0].second, m_id_values[1].second)) {
                            m_id_values[1].first++;
                        }
                    }
                    return m_id_values[0].first; //the same id satisfies the condition for both properties
                }else {
                    m_id_values[0].first = c+1;
                    m_id_values[1].first = c;
                    while (true) {
                        m_id_values[0] = m_ptr_ring->next_node_in_property(m_expr->property_values[0], m_id_values[1].first); //get next id and value of the first property
                        if (m_id_values[0].first == 0) return 0; //no more ids to try
                        m_id_values[1] = m_ptr_ring->next_node_in_property(m_expr->property_values[1], m_id_values[0].first); //get the value of the second property
                        if (m_id_values[1].first == 0) return 0; //no more ids to try
                        if (m_id_values[0].first == m_id_values[1].first && compare(m_id_values[0].second, m_id_values[1].second)) {
                            return m_id_values[0].first; //the same id satisfies the condition for both properties
                        }else if (m_id_values[0].first == m_id_values[1].first && !compare(m_id_values[0].second, m_id_values[1].second)) {
                            m_id_values[1].first++;
                        }
                    }
                }
            }

            if (!m_nfixed) {
                if ((m_expr->is_var[0] && var == m_expr->values[0])) { //if same_var, I choose the first operand
                    if (m_is_edge[0]) {
                        m_id_values[0] = m_ptr_ring->next_edge_in_property(m_expr->property_values[0], c);
                    }else {
                        m_id_values[0] = m_ptr_ring->next_node_in_property(m_expr->property_values[0], c);
                    }
                    return m_id_values[0].first;
                }else if (m_expr->is_var[1] && var == m_expr->values[1]) {
                    if (m_is_edge[1]) {
                        m_id_values[1] = m_ptr_ring->next_edge_in_property(m_expr->property_values[1], c);
                    }else {
                        m_id_values[1] = m_ptr_ring->next_node_in_property(m_expr->property_values[1], c);
                    }
                    return m_id_values[1].first;
                }
            }else {
                if (m_expr->is_var[1] && var == m_expr->values[1]) {
                    if (m_is_edge[1]) {
                        m_id_values[1] = m_ptr_ring->next_edge_property(m_expr->property_values[1], c, m_fixed_values[0], query::opposite_comp_where[m_expr->type]);
                    }else {
                        m_id_values[1] = m_ptr_ring->next_node_property(m_expr->property_values[1], c, m_fixed_values[0], query::opposite_comp_where[m_expr->type]);
                    }
                    return m_id_values[1].first;
                }else if ( m_expr->is_var[0] && var == m_expr->values[0]) {
                    if (m_is_edge[0]) {
                        m_id_values[0] = m_ptr_ring->next_edge_property(m_expr->property_values[0], c, m_fixed_values[1], m_expr->type);
                    }else {
                        m_id_values[0] = m_ptr_ring->next_node_property(m_expr->property_values[0], c, m_fixed_values[1], m_expr->type);
                    }
                    return m_id_values[0].first;
                }
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
            if (!m_nfixed) return m_selectivity_no_fixed;
            if (m_fixed_values[0]) {
                return compute_selectivity(m_fixed_values[0], 1);
            }else {
                return compute_selectivity(m_fixed_values[1], 0);
            }

            //return m_ptr_ring->contar_in_range()/m_ptr_ring->n_nodes_en_propiedad();
        }

        inline double_t opt_selectivity() const {
            if (!m_nfixed) return m_selectivity_no_fixed;
            if (m_fixed_values[0]) {
                return compute_opt_selectivity(m_fixed_values[0], 1);
            }else {
                return compute_opt_selectivity(m_fixed_values[1], 0);
            }
        }

        id_type seek_last(var_type var) {
            return 0;
        }

        id_type seek_last_next(var_type var) {
            return 0;
        }
    };
}

#endif //RING_LTJ_ITERATOR_COMP_HPP
