/*
 * ltj_algorithm.hpp
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


#ifndef RING_LTJ_ALGORITHM_PG_HPP
#define RING_LTJ_ALGORITHM_PG_HPP

#include <type_traits>
#include <triple_pattern.hpp>
#include <ltj_iterator.hpp>
#include <ltj_iterator_edge_expr.hpp>
#include <ltj_iterator_edge_label.hpp>
#include <ltj_iterator_node_expr.hpp>
#include <veo_adaptive_pg2.hpp>
#include <veo_simple_pg.hpp>
#include <results_collector.hpp>
#include <query/query_parser.hpp>

#include "ltj_iterator_comp.hpp"
#include "ltj_iterator_comp_id.hpp"
#include <ltj_iterator_comp_id_range.hpp>
#include <ltj_iterator_comp_range.hpp>


namespace ring {
    template<class results_t = ::util::results_collector<std::vector<std::string> >, class veo_t = veo::veo_simple_pg<> >
    class ltj_algorithm_pg {
    public:
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef typename query::pg_query::patterns_type patterns_type;
        typedef typename query::pg_query::where_type where_type;
        typedef ltj_iterator_base<uint8_t, uint32_t> ltj_iter_type;
        typedef ring_pg<> ring_type;
        typedef typename ltj_iter_type::var_type var_type;
        typedef typename ltj_iter_type::value_type const_type;
        typedef veo_t veo_type;
        typedef std::unordered_map<var_type, std::vector<ltj_iter_type *> > var_to_iterators_type;
        typedef std::unordered_map<var_type, std::unordered_map<value_type, std::vector<ltj_iter_type *> > > var_to_prop_iterators_type;
        typedef std::unordered_map<var_type, uint> cnt_iterators_pattern_type;

        //typedef std::vector<std::pair<var_type, value_type>> tuple_type;
        typedef std::vector<value_type> tuple_type;
        typedef std::vector<string> tuple_str_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;
        typedef results_t results_type;

    private:
        const query::pg_query *m_ptr_query;

        veo_type m_veo;
        ring_type *m_ptr_ring;
        std::vector<ltj_iter_type *> m_iterators;
        var_to_iterators_type m_var_to_iterators;
        var_to_prop_iterators_type m_var_to_prop_iterators;
        cnt_iterators_pattern_type m_cnt_iterators_pattern;
        bool m_is_empty = false;


        void copy(const ltj_algorithm_pg &o) {
            m_ptr_query = o.m_ptr_query;
            m_veo = o.m_veo;
            m_ptr_ring = o.m_ptr_ring;
            m_iterators = o.m_iterators;
            m_var_to_iterators = o.m_var_to_iterators;
            m_cnt_iterators_pattern = o.m_cnt_iterators_pattern;
            m_var_to_prop_iterators = o.m_var_to_prop_iterators;
            m_is_empty = o.m_is_empty;
        }


        inline void add_var_to_iterator(const var_type var, ltj_iter_type *ptr_iterator, bool is_pattern) {
            auto it = m_var_to_iterators.find(var);
            if (it != m_var_to_iterators.end()) {
                it->second.push_back(ptr_iterator);
                if (is_pattern) m_cnt_iterators_pattern[var]++;
            } else {
                std::vector<ltj_iter_type *> vec = {ptr_iterator};
                m_var_to_iterators.insert({var, vec});
                if (is_pattern) m_cnt_iterators_pattern.insert({var, 1});
            }
        }

        inline void add_var_to_prop_iterator(const var_type var, const value_type prop_id,
                                             ltj_iter_type *ptr_iterator) {
            auto it = m_var_to_prop_iterators.find(var);
            if (it != m_var_to_prop_iterators.end()) {
                auto it2 = it->second.find(prop_id);
                if (it2 != it->second.end()) {
                    it2->second.push_back(ptr_iterator);
                } else {
                    std::vector<ltj_iter_type *> vec = {ptr_iterator};
                    it->second.insert({prop_id, vec});
                }
            } else {
                std::unordered_map<value_type, std::vector<ltj_iter_type *> > umap = {};
                std::vector<ltj_iter_type *> vec = {ptr_iterator};
                umap.insert({prop_id, vec});
                m_var_to_prop_iterators.insert({var, umap});
            }
        }

        /*void setting_properties(const var_type x_j, const value_type c) {
            //2.a Setting the value of each property
            for (const auto &prop_map: m_var_to_prop_iterators[x_j]) {
                //Find value of the property
                value_type prop_value = 0;
                for (ltj_iter_type *ptr_iterator: prop_map.second) {
                    prop_value = ptr_iterator->get_prop_value(x_j, c);
                    if (prop_value) break;
                }

                //If it was not computed previously, compute it now
                if (!prop_value) {
                    prop_value = prop_map.second.front()->compute_prop_value(x_j, c);
                }

                //Update the iterators with the property value
                for (ltj_iter_type *ptr_iterator: prop_map.second) {
                    ptr_iterator->set_prop_value(x_j, prop_value);
                }
            }
        }*/

       /* void process_where_expression(const query::where_expr_parser::expr &expr,
                                      const std::vector<bool> &vars_in_nodes) {
            if (expr.is_var[0] && vars_in_nodes[expr.values[0]] || expr.is_var[1] && vars_in_nodes[expr.values[1]]) {
                m_iterators.push_back(
                    new ltj_iterator_node_comp<ring_type, var_type, const_type>(&expr, m_ptr_ring));

                if (expr.is_var[0] && vars_in_nodes[expr.values[0]]) {
                    add_var_to_iterator(expr.values[0], m_iterators.back());
                    if (expr.property_values[0]) {
                        add_var_to_prop_iterator(expr.values[0], expr.property_values[0],
                                                 m_iterators.back());
                    }
                }

                if (expr.is_var[1] && vars_in_nodes[expr.values[1]]) {
                    add_var_to_iterator(expr.values[1], m_iterators.back());
                    if (expr.property_values[1]) {
                        add_var_to_prop_iterator(expr.values[1], expr.property_values[1],
                                                 m_iterators.back());
                    }
                }
            } else {
                m_iterators.push_back(new ltj_iterator_edge_comp<ring_type, var_type, const_type>(&expr, m_ptr_ring));

                if (expr.is_var[0] && !vars_in_nodes[expr.values[0]]) {
                    add_var_to_iterator(expr.values[0], m_iterators.back());
                    if (expr.property_values[0]) {
                        add_var_to_prop_iterator(expr.values[0], expr.property_values[0],
                                                 m_iterators.back());
                    }
                }

                if (expr.is_var[1] && !vars_in_nodes[expr.values[1]]) {
                    add_var_to_iterator(expr.values[1], m_iterators.back());
                    if (expr.property_values[1]) {
                        add_var_to_prop_iterator(expr.values[1], expr.property_values[1],
                                                 m_iterators.back());
                    }
                }
            }
        }*/

        struct var_range {
            var_type var;
            uint32_t property_id;  // 0 if comparing IDs directly (no property)
            bool is_edge;
            value_type lower_bound = 1;
            value_type upper_bound = UINT64_MAX;
            bool has_lower = false;
            bool has_upper = false;

            inline bool is_valid() const {
                return !has_lower || !has_upper || lower_bound <= upper_bound;
            }
        };

        static inline bool is_range_operator(query::enum_comp_where_type type) {
            return type == query::GT || type == query::GE ||
                   type == query::ST || type == query::SE;
        }

        void add_to_range(var_range &range, const query::where_expr_parser::expr &expr) {
            const int var_idx = expr.is_var[0] ? 0 : 1;
            const auto op = (var_idx == 0) ? expr.type : query::opposite_comp_where[expr.type];
            const value_type value = expr.values[1 - var_idx];

            // Update bounds based on operator
            switch (op) {
                case query::GT:  // var > value → var >= (value + 1)
                    range.lower_bound = range.has_lower ? std::max(range.lower_bound, value + 1) : value + 1;
                    range.has_lower = true;
                    break;

                case query::GE:  // var >= value
                    range.lower_bound = range.has_lower ? std::max(range.lower_bound, value) : value;
                    range.has_lower = true;
                    break;

                case query::ST:  // var < value → var <= (value - 1)
                    range.upper_bound = range.has_upper ? std::min(range.upper_bound, value - 1) : value - 1;
                    range.has_upper = true;
                    break;

                case query::SE:  // var <= value
                    range.upper_bound = range.has_upper ? std::min(range.upper_bound, value) : value;
                    range.has_upper = true;
                    break;

                default:
                    return;
            }

        }

        void process_where_expression(const query::where_expr_parser::expr &expr,
                                      std::map<std::tuple<var_type, uint32_t, bool>, var_range> &range_map) {

            const bool is_range = is_range_operator(expr.type) &&
                        ((expr.is_var[0] && !expr.is_var[1]) ||
                         (!expr.is_var[0] && expr.is_var[1]));

            if (!is_range) {
                // Not a range - create normal iterator immediately
                build_individual_iterator(expr);
                return;
            }

            const int var_idx = expr.is_var[0] ? 0 : 1;
            const var_type var = expr.values[var_idx];
            const uint32_t prop_id = expr.property_values[var_idx];
            const bool is_edge = !m_ptr_query->vnodes[var];

            auto key = std::make_tuple(var, prop_id, is_edge);

            var_range range;
            auto it = range_map.find(key);
            if (it == range_map.end()) {
                range.var = var;
                range.property_id = prop_id;
                range.is_edge = is_edge;
                it = range_map.insert({key, range}).first;
            }
            add_to_range(it->second, expr);
        }


            void build_range_iterators(std::map<std::tuple<var_type, uint32_t, bool>, var_range> &range_map) {

                for (auto &[key, range] : range_map) {
                    if (!range.is_valid()) {
                        m_is_empty = true;
                        continue;
                    }

                    if (range.property_id == 0) {
                        m_iterators.push_back(new ltj_iterator_comp_id_range<ring_type, var_type, const_type>(
                            range.var, range.is_edge,
                            range.lower_bound, range.upper_bound,
                            range.has_lower, range.has_upper,
                            m_ptr_ring
                        ));
                    } else {
                        m_iterators.push_back(new ltj_iterator_comp_range<ring_type, var_type, const_type>(
                            range.var, range.property_id, range.is_edge,
                            range.lower_bound, range.upper_bound,
                            range.has_lower, range.has_upper,
                            m_ptr_ring
                        ));
                    }

                    add_var_to_iterator(range.var, m_iterators.back(), false);
                }
                range_map.clear();

        }

        void build_individual_iterator(const query::where_expr_parser::expr &expr) {

            auto var0_edge = (expr.is_var[0] && !m_ptr_query->vnodes[expr.values[0]]);
            auto var1_edge = (expr.is_var[1] && !m_ptr_query->vnodes[expr.values[1]]);
            if (expr.has_property()) {
                m_iterators.push_back(new ltj_iterator_comp<ring_type, var_type, const_type>(&expr, var0_edge, var1_edge, m_ptr_ring));
            }else {
                m_iterators.push_back(new ltj_iterator_comp_id<ring_type, var_type, const_type>(&expr, var0_edge, m_ptr_ring));
            }

            if (expr.is_var[0]) {
                add_var_to_iterator(expr.values[0], m_iterators.back(), false);
            }
            if (expr.is_var[1]) {
                add_var_to_iterator(expr.values[1], m_iterators.back(), false);
            }
        }




        void translate_tuple_to_strings(const tuple_type &tuple_ids, tuple_str_type &tuple_str) const {
            tuple_str.resize(tuple_ids.size());
            for (size_type i = 0; i < tuple_ids.size(); ++i) {
                var_type var = i + 1; // Variables are 1-indexed
                if (var < m_ptr_query->vnodes.size() && m_ptr_query->vnodes[var]) {
                    // It's a node variable - extract string from dictionary
                    tuple_str[i] = m_ptr_ring->dict_nodes.extract(tuple_ids[i]);
                } else {
                    // It's an edge variable - just convert ID to string
                    tuple_str[i] = std::to_string(tuple_ids[i]);
                }
            }
        }

        // translates to string (decision in compilation)
        template<typename T>
        typename std::enable_if<std::is_same<typename T::value_type, std::vector<std::string>>::value, void>::type
        add_result_to_collector(T& res, const tuple_type& tuple) const {
            tuple_str_type tuple_str;
            translate_tuple_to_strings(tuple, tuple_str);
            res.add(tuple_str);
        }

        // adds directly (decision in compilation)
        template<typename T>
        typename std::enable_if<std::is_same<typename T::value_type, std::vector<uint64_t>>::value, void>::type
        add_result_to_collector(T& res, const tuple_type& tuple) const {
            res.add(tuple);
        }



    public:
        ltj_algorithm_pg() = default;

        ltj_algorithm_pg(const query::pg_query *query, ring_type *ring) {
            m_ptr_query = query;
            m_ptr_ring = ring;

            m_iterators.reserve(m_ptr_query->patterns.size()); //minimum number of iterators

            //iterators of the edge labels
            for (const auto &pattern: m_ptr_query->patterns) {
                //Bulding iterators
                if (pattern.edge.is_empty()) {
                    //the edge has no constraints on the labels -> normal iterator
                    m_iterators.push_back(new ltj_iterator<ring_type, var_type, const_type>(&pattern, m_ptr_ring));
                    // m_iterators.push_back(new ltj_iterator<ring_type, var_type, const_type>(&pattern, m_ptr_ring));
                } else if (pattern.edge.is_label()) {
                    //the edge has a label -> iterator with label
                    m_iterators.push_back(
                        new ltj_iterator_edge_label<ring_type, var_type, const_type>(&pattern, m_ptr_ring));
                } else {
                    //the edge has an expression -> iterator with expression
                    m_iterators.push_back(
                        new ltj_iterator_edge_expr<ring_type, var_type, const_type>(&pattern, m_ptr_ring));
                }

                if (m_iterators.back()->is_empty()) {
                    m_is_empty = true;
                    return;
                }

                if (pattern.subj.is_var()) {
                    add_var_to_iterator(pattern.subj.var_value, m_iterators.back(), true);
                }
                if (pattern.edge.is_var()) {
                    add_var_to_iterator(pattern.edge.var_value, m_iterators.back(), true);
                }
                if (pattern.obj.is_var()) {
                    add_var_to_iterator(pattern.obj.var_value, m_iterators.back(), true);
                }
            }

            //iterators of the node labels
            for (const auto &pattern: m_ptr_query->patterns) {
                if (pattern.subj.is_var() && !pattern.subj.is_empty()) {
                    m_iterators.push_back(
                        new ltj_iterator_node_expr<ring_type, var_type, const_type>(
                            &(pattern.subj.expr), m_ptr_ring, true));
                    add_var_to_iterator(pattern.subj.var_value, m_iterators.back(), true);
                }

                if (pattern.obj.is_var() && !pattern.obj.is_empty()) {
                    m_iterators.push_back(
                        new ltj_iterator_node_expr<ring_type, var_type, const_type>(
                            &(pattern.obj.expr), m_ptr_ring, false));
                    add_var_to_iterator(pattern.obj.var_value, m_iterators.back(), true);
                }
            }

            //iterators of the where expressions
            if (m_ptr_query->where.type != query::WAND) {
                build_individual_iterator(m_ptr_query->where);
            }else {
                std::map<std::tuple<var_type, uint32_t, bool>, var_range> ranges;
                for (const auto &expr: m_ptr_query->where.args) {
                    process_where_expression(expr, ranges);
                }
                build_range_iterators(ranges);
            }


           // m_veo = veo_type(&(m_ptr_query->patterns), &m_iterators, &m_var_to_iterators, m_ptr_ring);
            m_veo = veo_type(m_ptr_query, &m_iterators, &m_var_to_iterators, m_ptr_ring);
            m_veo.print();
        }

        //! Copy constructor
        ltj_algorithm_pg(const ltj_algorithm_pg &o) {
            copy(o);
        }

        //! Move constructor
        ltj_algorithm_pg(ltj_algorithm_pg &&o) {
            *this = std::move(o);
        }

        ~ltj_algorithm_pg() {
            //delete the iterators
            for (auto &it: m_iterators) {
                delete it;
            }
        }

        //! Copy Operator=
        ltj_algorithm_pg &operator=(const ltj_algorithm_pg &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_algorithm_pg &operator=(ltj_algorithm_pg &&o) {
            if (this != &o) {
                m_ptr_query = o.m_ptr_query;
                m_veo = std::move(o.m_veo);
                m_ptr_ring = o.m_ptr_ring;
                m_iterators = std::move(o.m_iterators);
                m_var_to_iterators = std::move(o.m_var_to_iterators);
                m_cnt_iterators_pattern = std::move(o.m_cnt_iterators_pattern);
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_algorithm_pg &o) {
            std::swap(m_ptr_query, o.m_ptr_query);
            std::swap(m_veo, o.m_veo);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_iterators, o.m_iterators);
            std::swap(m_var_to_iterators, o.m_var_to_iterators);
            std::swap(m_cnt_iterators_pattern, o.m_cnt_iterators_pattern);
            std::swap(m_is_empty, o.m_is_empty);
        }


        /**
        *
        * @param res               Results
        * @param limit_results     Limit of results
        * @param timeout_seconds   Timeout in seconds
        */
        void join_v2(results_type &res,
                     const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            if (m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_veo.size());
            search_v2(0, t, res, start, limit_results, timeout_seconds);
        };

        void join_v3(results_type &res,
                     const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            if (m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_veo.size());
            search_v3(0, t, res, start, limit_results, timeout_seconds);
        };

        void join_prefilter(results_type &res,
                     const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            if (m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_veo.size());
            base_prefilter(0, t, res, start, limit_results, timeout_seconds);
        };

        void join_postfilter(results_type &res,
                     const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            if (m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_veo.size());
            base_postfilter(0, t, res, start, limit_results, timeout_seconds);
        };

        static bool compare_iterator(ltj_iter_type *iter1, ltj_iter_type *iter2) {
            return iter1->interval_length() < iter2->interval_length();
        }


        bool search_v2(const size_type j, tuple_type &tuple, results_type &res,
                       const time_point_type start,
                       const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            //(Optional) Check timeout
            if (timeout_seconds > 0) {
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
                if (sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if (limit_results > 0 && res.size() == limit_results) return false;

            if (j == m_veo.size()) {
                //Report results - translate node IDs to strings if needed
                add_result_to_collector(res, tuple);
                /*std::cout << "Add result" << std::endl;
                for(const auto &dat : tuple){
                    std::cout << "{" << (uint64_t) dat.first << "=" << dat.second << "} ";
                }
                std::cout << std::endl;*/
            } else {
                var_type x_j = m_veo.next();
                //std::cout << "Variable: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
                bool ok;
                if (itrs.size() == 1 && itrs[0]->in_last_level()) {
                    //Lonely variables
                    //std::cout << "Seeking (last level)" << std::endl;
                    value_type c = itrs[0]->seek_last(x_j);
                    //auto results = itrs[0]->seek_all(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    //std::cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" << results.size() << ")" <<std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j, c);
                        m_veo.down();
                        //2. Search with the next variable x_{j+1}
                        ok = search_v2(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                        m_veo.up();

                        c = itrs[0]->seek_last_next(x_j);
                    }
                } else {
                    value_type c = seek(x_j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type *iter: itrs) {
                            iter->down(x_j, c);
                        }
                        m_veo.down();
                        //3. Search with the next variable x_{j+1}
                        ok = search_v2(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the tries by removing x_j = c
                        for (ltj_iter_type *iter: itrs) {
                            iter->up(x_j);
                        }
                        m_veo.up();
                        //5. Next constant for x_j
                        c = seek(x_j, c + 1);
                        //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    }
                }
                m_veo.done();
            }
            return true;
        };

        bool search_v3(const size_type j, tuple_type &tuple, results_type &res,
                       const time_point_type start,
                       const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            //(Optional) Check timeout
            if (timeout_seconds > 0) {
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
                if (sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if (limit_results > 0 && res.size() == limit_results) return false;

            if (j == m_veo.size()) {
                //Report results - translate node IDs to strings if needed
                add_result_to_collector(res, tuple);
                /*std::cout << "Add result" << std::endl;
                uint i = 1;
                for (const auto &dat: tuple) {
                    std::cout << "x_" << i << "=" << dat << " ";
                    ++i;
                }
                std::cout << std::endl;*/
            } else {
                var_type x_j = m_veo.next();
                //std::cout << "Variable: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
                bool ok;
                if (itrs.size() == 1 && itrs[0]->in_last_level()) {
                    //Lonely variables
                    //std::cout << "Seeking (last level)" << std::endl;
                    value_type c = itrs[0]->seek_last(x_j);
                    //auto results = itrs[0]->seek_all(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    //std::cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" << results.size() << ")" <<std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j, c);
                        m_veo.down();
                        //2. Search with the next variable x_{j+1}
                        ok = search_v3(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                        m_veo.up();

                        c = itrs[0]->seek_last_next(x_j);
                    }
                } else {
                    std::vector<ltj_iter_type *> sorted_itrs = itrs; //copy iterators to sort them by interval length
                    std::sort(sorted_itrs.begin(), sorted_itrs.end(), compare_iterator);
                    value_type c = seek(sorted_itrs, x_j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type *iter: sorted_itrs) {
                            iter->down(x_j, c);
                        }
                        //2.a Setting the value of each property
                        //setting_properties(x_j, c);
                        m_veo.down();
                        //3. Search with the next variable x_{j+1}
                        ok = search_v3(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the tries by removing x_j = c
                        for (ltj_iter_type *iter: sorted_itrs) {
                            iter->up(x_j);
                        }
                        m_veo.up();
                        //5. Next constant for x_j
                        c = seek(sorted_itrs, x_j, c + 1);
                        //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                    }
                }
                m_veo.done();
            }
            return true;
        };


        bool base_postfilter(const size_type j, tuple_type &tuple, results_type &res,
                       const time_point_type start,
                       const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            //(Optional) Check timeout
            if (timeout_seconds > 0) {
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
                if (sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if (limit_results > 0 && res.size() == limit_results) return false;

            if (j == m_veo.size()) {
                //Report results - translate node IDs to strings if needed
                add_result_to_collector(res, tuple);
                /*std::cout << "Add result" << std::endl;
                uint i = 1;
                for (const auto &dat: tuple) {
                    std::cout << "x_" << i << "=" << dat << " ";
                    ++i;
                }
                std::cout << std::endl;*/
            } else {
                var_type x_j = m_veo.next();
                //std::cout << "Variable: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
                bool ok;
                if (itrs.size() == 1 && itrs[0]->in_last_level()) {
                    //Lonely variables
                    //std::cout << "Seeking (last level)" << std::endl;
                    value_type c = itrs[0]->seek_last(x_j);
                    //auto results = itrs[0]->seek_all(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    //std::cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" << results.size() << ")" <<std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j, c);
                        m_veo.down();
                        //2. Search with the next variable x_{j+1}
                        ok = base_postfilter(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                        m_veo.up();

                        c = itrs[0]->seek_last_next(x_j);
                    }
                } else {
                    std::vector<ltj_iter_type *> sorted_itrs = itrs; //copy iterators to sort them by interval length
                    auto beg_where = m_cnt_iterators_pattern[x_j];
                    std::sort(sorted_itrs.begin(), sorted_itrs.begin() + beg_where, compare_iterator);
                    std::sort(sorted_itrs.begin()+ beg_where, sorted_itrs.end(), compare_iterator);
                    value_type c = seek_postfilter(sorted_itrs, beg_where, x_j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type *iter: sorted_itrs) {
                            iter->down(x_j, c);
                        }
                        //2.a Setting the value of each property
                        //setting_properties(x_j, c);
                        m_veo.down();
                        //3. Search with the next variable x_{j+1}
                        ok = base_postfilter(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the tries by removing x_j = c
                        for (ltj_iter_type *iter: sorted_itrs) {
                            iter->up(x_j);
                        }
                        m_veo.up();
                        //5. Next constant for x_j
                        c = seek_postfilter(sorted_itrs, beg_where, x_j, c + 1);
                        //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                    }
                }
                m_veo.done();
            }
            return true;
        };


        bool base_prefilter(const size_type j, tuple_type &tuple, results_type &res,
                       const time_point_type start,
                       const size_type limit_results = 0, const size_type timeout_seconds = 0) {
            //(Optional) Check timeout
            if (timeout_seconds > 0) {
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
                if (sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if (limit_results > 0 && res.size() == limit_results) return false;

            if (j == m_veo.size()) {
                //Report results - translate node IDs to strings if needed
                add_result_to_collector(res, tuple);
                /*std::cout << "Add result" << std::endl;
                uint i = 1;
                for (const auto &dat: tuple) {
                    std::cout << "x_" << i << "=" << dat << " ";
                    ++i;
                }
                std::cout << std::endl;*/
            } else {
                var_type x_j = m_veo.next();
                //std::cout << "Variable: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
                bool ok;
                if (itrs.size() == 1 && itrs[0]->in_last_level()) {
                    //Lonely variables
                    //std::cout << "Seeking (last level)" << std::endl;
                    value_type c = itrs[0]->seek_last(x_j);
                    //auto results = itrs[0]->seek_all(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    //std::cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" << results.size() << ")" <<std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j, c);
                        m_veo.down();
                        //2. Search with the next variable x_{j+1}
                        ok = base_prefilter(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if (!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                        m_veo.up();

                        c = itrs[0]->seek_last_next(x_j);
                    }
                } else {
                    std::vector<ltj_iter_type *> sorted_itrs = itrs; //copy iterators to sort them by interval length
                    auto beg_where = m_cnt_iterators_pattern[x_j];
                    std::sort(sorted_itrs.begin(), sorted_itrs.begin() + beg_where, compare_iterator);
                    std::sort(sorted_itrs.begin()+ beg_where, sorted_itrs.end(), compare_iterator);
                    if (sorted_itrs.size() == beg_where) {
                        value_type c = seek(sorted_itrs, x_j);
                        //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                        while (c != 0) {
                            //If empty c=0
                            //1. Adding result to tuple
                            tuple[x_j - 1] = c;
                            //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                            for (ltj_iter_type *iter: sorted_itrs) {
                                iter->down(x_j, c);
                            }
                            //2.a Setting the value of each property
                            //setting_properties(x_j, c);
                            m_veo.down();
                            //3. Search with the next variable x_{j+1}
                            ok = base_prefilter(j + 1, tuple, res, start, limit_results, timeout_seconds);
                            if (!ok) return false;
                            //4. Going up in the tries by removing x_j = c
                            for (ltj_iter_type *iter: sorted_itrs) {
                                iter->up(x_j);
                            }
                            m_veo.up();
                            //5. Next constant for x_j
                            c = seek(sorted_itrs, x_j, c+1);
                            //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                        }
                    }else {
                        value_type c = seek_prefilter(sorted_itrs, beg_where, x_j);
                        //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                        while (c != 0) {
                            //If empty c=0
                            //1. Adding result to tuple
                            tuple[x_j - 1] = c;
                            //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                            for (ltj_iter_type *iter: sorted_itrs) {
                                iter->down(x_j, c);
                            }
                            //2.a Setting the value of each property
                            //setting_properties(x_j, c);
                            m_veo.down();
                            //3. Search with the next variable x_{j+1}
                            ok = base_prefilter(j + 1, tuple, res, start, limit_results, timeout_seconds);
                            if (!ok) return false;
                            //4. Going up in the tries by removing x_j = c
                            for (ltj_iter_type *iter: sorted_itrs) {
                                iter->up(x_j);
                            }
                            m_veo.up();
                            //5. Next constant for x_j
                            c = seek_prefilter(sorted_itrs, beg_where, x_j, c+1);
                            //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                        }
                    }
                }
                m_veo.done();
            }
            return true;
        };

        /**
         *
         * @param x_j   Variable
         * @param c     Constant. If it is unknown the value is -1
         * @return      The next constant that matches the intersection between the triples of x_j.
         *              If the intersection is empty, it returns 0.
         */

        value_type seek(const var_type x_j, value_type c = -1) {
            std::vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
            const size_type n = itrs.size();

            value_type c_i = (c == -1) ? itrs[0]->leap(x_j) : itrs[0]->leap(x_j, c);
            if (c_i == 0) return 0; //Empty intersection
            if (n == 1) return c_i; // Single iterator, already done

            value_type c_prev = c_i;
            size_type i = 1;
            size_type n_ok = 1;

            while (true) {
                c_i = itrs[i]->leap(x_j, c_i);
                if (c_i == 0) return 0; //Empty intersection

                if (c_i == c_prev) {
                    ++n_ok;
                    if (n_ok == n) return c_i;
                } else {
                    n_ok = 1;
                    c_prev = c_i;
                }

                ++i;
                if (i == n) i = 0;
            }
        }

        //Seek normal
        value_type seek(std::vector<ltj_iter_type*>& itrs, const var_type x_j, value_type c = -1) {
            value_type c_i = (c == -1) ? itrs[0]->leap(x_j) : itrs[0]->leap(x_j, c);
            if (c_i == 0) return 0; //Empty intersection
            const size_type n = itrs.size();
            if (n == 1) return c_i; // Single iterator, already done

            value_type c_prev = c_i;
            size_type i = 1; // Start from 1 since we already did itrs[0]
            size_type n_ok = 1;

            while (true) {
                c_i = itrs[i]->leap(x_j, c_i);
                if (c_i == 0) return 0; //Empty intersection

                if (c_i == c_prev) {
                    ++n_ok;
                    if (n_ok == n) return c_i;
                } else {
                    n_ok = 1;
                    c_prev = c_i;
                }

                ++i;
                if (i == n) i = 0;
            }
        }

        value_type seek_postfilter(std::vector<ltj_iter_type*>& itrs, size_type beg_where,
                                   const var_type x_j, value_type c = -1) {

            const size_type n = itrs.size();

            // Edge case: if beg_where is 0, means no pattern iterators, only WHERE iterators
            if (beg_where == 0) {
                // Use standard seek on WHERE iterators only
                return seek(itrs, x_j, c);
            }

            // Edge case: if beg_where equals n, means only pattern iterators, no WHERE iterators
            if (beg_where >= n) {
                // Use standard seek on pattern iterators only
                return seek(itrs, x_j, c);
            }


            value_type c_prev = -1ULL;
            value_type c_i = c;
            size_type i = 0; // Start from 1 since we already did itrs[0]
            size_type n_ok = 0;

            while (true) {
                c_i = (c_i == -1) ? itrs[i]->leap(x_j) : itrs[i]->leap(x_j, c_i);  // Loop through iterators in patterns
                if (c_i == 0) return 0; //Empty intersection

                if (c_i == c_prev || beg_where == 1) { // If beg_where is 1, means only one pattern iterator, so we can start checking the filter iterators
                    ++n_ok;
                    if (n_ok == beg_where) { //Check in the filter (all pattern iterators matched)
                        auto j = beg_where;
                        c_prev = c_i;
                        while (c_i == c_prev && j < n) {
                            c_i = itrs[j]->leap(x_j, c_i);
                            ++j;
                        }
                        if (j == n && c_i == c_prev) return c_i; // All iterators in the filter match
                        // Not all matched, find next candidate
                        c_i = c_prev + 1; // next candidate
                        n_ok = 0;
                    }
                } else {
                    n_ok = 1;
                    c_prev = c_i;
                }
                if (++i == beg_where) i = 0;
            }
        }

        value_type seek_prefilter(std::vector<ltj_iter_type*>& itrs, size_type beg_where,
                                   const var_type x_j, value_type c = -1) {

            const size_type n = itrs.size();

            // Edge case: if beg_where is 0, means no pattern iterators, only WHERE iterators
            if (beg_where == 0) {
                // Use standard seek on WHERE iterators only
                return seek(itrs, x_j, c);
            }

            // Edge case: if beg_where equals or exceeds n, means only pattern iterators, no WHERE iterators
            if (beg_where >= n) {
                // Use standard seek on pattern iterators only
                return seek(itrs, x_j, c);
            }

            // Prefilter: First check WHERE iterators (beg_where to n-1), then verify with PATTERN iterators (0 to beg_where-1)
            // Similar structure to seek_postfilter but inverted

            value_type c_prev = -1ULL;
            value_type c_i = c;
            size_type i = beg_where; // Start from first WHERE iterator
            size_type n_ok = 0;
            const size_type n_where = n - beg_where; // Number of WHERE iterators

            while (true) {
                c_i = (c_i == -1) ? itrs[i]->leap(x_j) : itrs[i]->leap(x_j, c_i);  // Loop through WHERE iterators
                if (c_i == 0) return 0; //Empty intersection

                if (c_i == c_prev || n_where == 1) { // If n_where is 1, means only one WHERE iterator, so we can start checking the PATTERN iterators
                    ++n_ok;
                    if (n_ok == n_where) { //Check in the PATTERN iterators (all WHERE iterators matched)
                        auto j = 0;
                        c_prev = c_i;
                        while (c_i == c_prev && j < beg_where) {
                            c_i = itrs[j]->leap(x_j, c_i);
                            ++j;
                        }
                        if (j == beg_where && c_i == c_prev) return c_i; // All iterators match
                        // Not all matched, find next candidate
                        c_i = c_prev + 1; // next candidate
                        n_ok = 0;
                    }
                } else {
                    n_ok = 1;
                    c_prev = c_i;
                }
                if (++i == n) i = beg_where; // Loop back to first WHERE iterator
            }
        }


        /**
         * Seek avoiding recomputing the iterators that have already matched
         */
        value_type seek_v2(std::vector<ltj_iter_type*>& itrs, const var_type x_j, value_type c = -1) {
            value_type c_i = (c == -1) ? itrs[0]->leap(x_j) : itrs[0]->leap(x_j, c);
            if (c_i == 0) return 0;
            c = c_i;
            size_type i = 1, match = 0;
            while (i < itrs.size()){
                if (i == match) {
                    ++i;
                    continue;
                }
                c_i = itrs[i]->leap(x_j, c);
                if (c_i == 0) return 0;
                if (c_i == c ) {
                    ++i;
                }else {
                    match = i;
                    i = 0;
                }
                c = c_i;
            }
            return c_i;
        }


        value_type seek_v3(std::vector<ltj_iter_type*>& itrs, const var_type x_j, value_type c = -1) {
            value_type tgt = (c == -1) ? itrs[0]->leap(x_j) : itrs[0]->leap(x_j, c);
            value_type aux = tgt;
            while (tgt) {
                //std::cout << "Target = " << tgt << std::endl;
                for (size_type i = 1; i < itrs.size(); ++i) {
                    aux = itrs[i]->leap(x_j, aux);
                    //std::cout << "Leap of " << (uint64_t) x_j << " in iterator: " << i << " gets " << (uint64_t) aux << std::endl;
                    if (aux == 0) return 0;
                }
                if (aux == tgt) return tgt;
                tgt = itrs[0]->leap(x_j, aux);
                aux = tgt;
            }
            return 0;
        }

        value_type seek_v1(std::vector<ltj_iter_type*>& itrs, const var_type x_j, value_type c=-1) {
            value_type c_i = 0, i = 0;
            while (i < itrs.size()){
                //Compute leap for each triple that contains x_j
                //std::cout << "Leap of " << (::uint64_t) x_j << " in iterator: " << i << std::endl;
                c_i = (c == -1) ? itrs[i]->leap(x_j) : itrs[i]->leap(x_j, c);
                //std::cout << "Gets " << (::uint64_t) c_i << std::endl;
                if(c_i == 0) return 0;
                i = (i == 0 || c_i == c) ? i + 1 : 0;
                c = c_i;
            }
            return c_i;
        }

       /* value_type seek(std::vector<ltj_iter_type *> &itrs, const var_type x_j, value_type c = -1) {
            value_type c_i = (c == -1) ? itrs[0]->leap(x_j) : itrs[0]->leap(x_j, c);
            if (itrs.size() == 1 || c_i == 0) return c_i;
            c = c_i;
            value_type i = 1, seed = 0, n_ok = 1;
            while (true) {
                //Compute leap for each triple that contains x_j
                c_i = itrs[i]->leap(x_j, c);
                if (c_i == 0) return 0; //Empty intersection
                if (c == c_i) {
                    ++n_ok;
                    if (n_ok == itrs.size()) return c;
                    i = (i + 1) % itrs.size();
                } else {
                    //seed = i;
                    i = 0;
                    n_ok = 1;
                    c = c_i;
                }
            }
        }*/

        void print_veo(std::unordered_map<uint8_t, std::string> &ht) {
            std::cout << "veo: ";
            for (uint64_t j = 0; j < m_veo.size(); ++j) {
                std::cout << "?" << ht[m_veo.next()] << " ";
            }
            std::cout << std::endl;
        }

        /*void print_query(std::unordered_map<uint8_t, std::string> &ht){
            std::cout << "Query: " << std::endl;
            for(size_type i = 0; i <  m_ptr_patterns->size(); ++i){
                m_ptr_patterns->at(i).print(ht);
                if(i < m_ptr_patterns->size()-1){
                    std::cout << " . ";
                }
            }
            std::cout << std::endl;
        }*/

        void print_results(std::vector<tuple_type> &res, std::unordered_map<uint8_t, std::string> &ht) {
            std::cout << "Results: " << std::endl;
            uint64_t i = 1;
            for (tuple_type &tuple: res) {
                std::cout << "[" << i << "]: ";
                uint64_t j = 1;
                for (value_type &v: tuple) {
                    std::cout << "?x_" << j << "=" << v << " ";
                }
                std::cout << std::endl;
                ++i;
            }
        }


    };
}

#endif //RING_LTJ_ALGORITHM_PG_HPP
