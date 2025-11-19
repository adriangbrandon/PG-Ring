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


#include <triple_pattern.hpp>
#include <ltj_iterator.hpp>
#include <ltj_iterator_edge_expr.hpp>
#include <ltj_iterator_edge_label.hpp>
#include <ltj_iterator_node_expr.hpp>
#include <veo_adaptive_pg.hpp>
#include <results_collector.hpp>
#include <query/query_parser.hpp>

#include "ltj_iterator_node_comp.hpp"


namespace ring {
    template<class results_t = ::util::results_collector<std::vector<uint64_t> >, class veo_t = veo::veo_adaptive_pg<> >
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
        typedef std::unordered_map<var_type, std::unordered_map<value_type, std::vector<ltj_iter_type *> > >
        var_to_prop_iterators_type;
        //typedef std::vector<std::pair<var_type, value_type>> tuple_type;
        typedef std::vector<value_type> tuple_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;
        typedef results_t results_type;

    private:
        const patterns_type *m_ptr_patterns;
        const where_type *m_ptr_where;
        veo_type m_veo;
        ring_type *m_ptr_ring;
        std::vector<ltj_iter_type *> m_iterators;
        var_to_iterators_type m_var_to_iterators;
        var_to_prop_iterators_type m_var_to_prop_iterators;
        bool m_is_empty = false;


        void copy(const ltj_algorithm_pg &o) {
            m_ptr_patterns = o.m_ptr_patterns;
            m_ptr_where = o.m_ptr_where;
            m_veo = o.m_veo;
            m_ptr_ring = o.m_ptr_ring;
            m_iterators = o.m_iterators;
            m_var_to_iterators = o.m_var_to_iterators;
            m_var_to_prop_iterators = o.m_var_to_prop_iterators;
            m_is_empty = o.m_is_empty;
        }


        inline void add_var_to_iterator(const var_type var, ltj_iter_type *ptr_iterator) {
            auto it = m_var_to_iterators.find(var);
            if (it != m_var_to_iterators.end()) {
                it->second.push_back(ptr_iterator);
            } else {
                std::vector<ltj_iter_type *> vec = {ptr_iterator};
                m_var_to_iterators.insert({var, vec});
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

        void setting_properties(const var_type x_j, const value_type c) {
            //2.a Setting the value of each property
            for (const auto &prop_map: m_var_to_prop_iterators[x_j]) {
                //Find value of the property
                value_type prop_value = 0;
                for (ltj_iter_type *ptr_iterator: prop_map.second) {
                    prop_value = ptr_iterator->get_prop_value(x_j);
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
        }

        void process_where_expression(const query::where_expr_parser::expr &expr, const std::set<value_type> &vars_in_nodes) {
            if (expr.is_var[0] && vars_in_nodes.find(expr.values[0]) != vars_in_nodes.end()
                || expr.is_var[1] && vars_in_nodes.find(expr.values[1]) != vars_in_nodes.end()) {
                m_iterators.push_back(
                    new ltj_iterator_node_comp<ring_type, var_type, const_type>(&expr, m_ptr_ring));

                if (expr.is_var[0] && vars_in_nodes.find(expr.values[0]) != vars_in_nodes.end()) {
                    add_var_to_iterator(expr.values[0], m_iterators.back());
                    if (expr.property_values[0]) {
                        add_var_to_prop_iterator(expr.values[0], expr.property_values[0],
                                                 m_iterators.back());
                    }
                }

                if (expr.is_var[1] && vars_in_nodes.find(expr.values[1]) != vars_in_nodes.end()) {
                    add_var_to_iterator(expr.values[1], m_iterators.back());
                    if (expr.property_values[1]) {
                        add_var_to_prop_iterator(expr.values[1], expr.property_values[1],
                                                 m_iterators.back());
                    }
                }
                } else {
                    /*m_iterators.push_back(new ltj_iterator_edge_comp<ring_type, var_type, const_type>(&expr, m_ptr_ring));

                    if (expr.is_var[0] && vars_in_nodes.find(expr.values[0]) != vars_in_nodes.end()) {
                        add_var_to_iterator(expr.values[0], m_iterators.back());
                    }
                    if (expr.is_var[1] && vars_in_nodes.find(expr.values[1]) != vars_in_nodes.end()) {
                        add_var_to_iterator(expr.values[1], m_iterators.back());
                    }*/
                    std::cout << "Por agora non entre por aqui!" << std::endl;
                }
        }



    public:
        ltj_algorithm_pg() = default;

        ltj_algorithm_pg(const patterns_type *triple_patterns, const where_type *where, ring_type *ring) {
            m_ptr_patterns = triple_patterns;
            m_ptr_where = where;
            m_ptr_ring = ring;

            m_iterators.reserve(m_ptr_patterns->size()); //minimum number of iterators

            std::set<value_type> vars_in_nodes;
            //iterators of the edge labels
            for (const auto &pattern: *m_ptr_patterns) {
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
                    vars_in_nodes.insert(pattern.subj.var_value);
                    add_var_to_iterator(pattern.subj.var_value, m_iterators.back());
                }
                if (pattern.edge.is_var()) {
                    add_var_to_iterator(pattern.edge.var_value, m_iterators.back());
                }
                if (pattern.obj.is_var()) {
                    vars_in_nodes.insert(pattern.obj.var_value);
                    add_var_to_iterator(pattern.obj.var_value, m_iterators.back());
                }
            }

            //iterators of the node labels
            for (const auto &pattern: *m_ptr_patterns) {
                if (pattern.subj.is_var() && !pattern.subj.is_empty()) {
                    m_iterators.push_back(
                        new ltj_iterator_node_expr<ring_type, var_type, const_type>(
                            &(pattern.subj.expr), m_ptr_ring, true));
                    add_var_to_iterator(pattern.subj.var_value, m_iterators.back());
                }

                if (pattern.obj.is_var() && !pattern.obj.is_empty()) {
                    m_iterators.push_back(
                        new ltj_iterator_node_expr<ring_type, var_type, const_type>(
                            &(pattern.obj.expr), m_ptr_ring, false));
                    add_var_to_iterator(pattern.obj.var_value, m_iterators.back());
                }
            }

            //iterators of the where expressions
            if (m_ptr_where->type != query::WAND) {
                process_where_expression(*m_ptr_where, vars_in_nodes);
            }else {
                for (const auto &expr: m_ptr_where->args) {
                    process_where_expression(expr, vars_in_nodes);
                }
            }


            m_veo = veo_type(m_ptr_patterns, &m_iterators, &m_var_to_iterators, m_ptr_ring);
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
                m_ptr_patterns = o.m_ptr_patterns;
                m_ptr_where = o.m_ptr_where;
                m_veo = std::move(o.m_veo);
                m_ptr_ring = o.m_ptr_ring;
                m_iterators = std::move(o.m_iterators);
                m_var_to_iterators = std::move(o.m_var_to_iterators);
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_algorithm_pg &o) {
            std::swap(m_ptr_patterns, o.m_ptr_patterns);
            std::swap(m_ptr_where, o.m_ptr_where);
            std::swap(m_veo, o.m_veo);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_iterators, o.m_iterators);
            std::swap(m_var_to_iterators, o.m_var_to_iterators);
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
                //Report results
                res.add(tuple);
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
                //Report results
                res.add(tuple);
                std::cout << "Add result" << std::endl;
                uint i = 1;
                for (const auto &dat: tuple) {
                    std::cout << "x_" << i << "=" << dat << " ";
                    ++i;
                }
                std::cout << std::endl;
            } else {
                var_type x_j = m_veo.next();
                std::cout << "Variable: " << (uint64_t) x_j << std::endl;
                std::vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
                bool ok;
                if (itrs.size() == 1 && itrs[0]->in_last_level()) {
                    //Lonely variables
                    std::cout << "Seeking (last level)" << std::endl;
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
                    std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
                    while (c != 0) {
                        //If empty c=0
                        //1. Adding result to tuple
                        tuple[x_j - 1] = c;
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type *iter: sorted_itrs) {
                            iter->down(x_j, c);
                        }
                        //2.a Setting the value of each property
                        setting_properties(x_j, c);
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
                        std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" << std::endl;
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
            value_type c_i, c_prev = 0, i = 0, n_ok = 0;
            while (true) {
                //Compute leap for each triple that contains x_j
                if (c == -1) {
                    c_i = itrs[i]->leap(x_j);
                } else {
                    c_i = itrs[i]->leap(x_j, c);
                }
                if (c_i == 0) return 0; //Empty intersection
                n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
                if (n_ok == itrs.size()) return c_i;
                c = c_prev = c_i;
                i = (i + 1 == itrs.size()) ? 0 : i + 1;
            }
        }


        value_type seek(std::vector<ltj_iter_type *> &itrs, const var_type x_j, value_type c = -1) {
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
                    n_ok = 0;
                    c = c_i;
                }
            }
        }

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
