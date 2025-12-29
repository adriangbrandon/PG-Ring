/*
 * gao.hpp
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


#ifndef RING_VEO_ADAPTIVE_PG_V2_HPP
#define RING_VEO_ADAPTIVE_PG_V2_HPP

#include <ltj_iterator.hpp>
#include <triple_pattern.hpp>
#include <unordered_map>
#include <vector>
#include <utils.hpp>
#include <unordered_set>

#include "ltj_iterator_comp.hpp"
#include "ltj_iterator_comp_id.hpp"
#include "ltj_iterator_node_expr.hpp"

namespace ring {

        namespace veo {


            template<class veo_trait_t = util::trait_size, class operator_t = util::op_minimum>
            class veo_adaptive_pg_v2 {

            public:
                typedef ltj_iterator_base<uint8_t, uint32_t> ltj_iter_type;
                typedef ltj_iter_type::var_type var_type;
                typedef ltj_iter_type::value_type value_type;
                typedef uint64_t size_type;
                typedef ring_pg<> ring_type;
                typedef typename operator_t::weight_type weight_type;


                /*enum spo_type {subject, predicate, object};
                typedef struct {
                    std::vector<spo_type> vec_spo;
                    size_type iter_pos;
                } spo_iter_type;*/

                typedef struct {
                    double_t weight; //triples * selectivity
                    size_type triples = UINT64_MAX;
                    double_t selectivity = 1.0;
                    std::unordered_set<var_type> related;
                    bool is_bound;
                    bool is_lonely;
                } info_var_type;


                typedef struct {
                    size_type pos;
                    double_t w;
                    size_type triples;
                    double_t selectivity;
                } update_type;


                typedef std::vector<update_type> version_type;
                typedef query::pg_query query_type;
                typedef std::pair<size_type, var_type> pair_type;
                typedef veo_trait_t veo_trait_type;
                typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
                typedef std::stack<version_type> versions_type;
                typedef std::stack<size_type> bound_type;

            private:
                const query_type *m_ptr_query;
                const std::vector<ltj_iter_type*> *m_ptr_iterators;
                const var_to_iterators_type *m_ptr_var_iterators;
                ring_type *m_ptr_ring;

                std::vector<info_var_type> m_var_info;
                std::vector<var_type> m_lonely; //stores the lonely variables
                size_type m_index;

                bound_type m_bound;
                versions_type m_versions;



                void copy(const veo_adaptive_pg_v2 &o) {
                    m_ptr_query = o.m_ptr_query;
                    m_ptr_iterators = o.m_ptr_iterators;
                    m_ptr_var_iterators = o.m_ptr_var_iterators;
                    m_ptr_ring = o.m_ptr_ring;
                    m_lonely = o.m_lonely;
                    m_index = o.m_index;
                    m_bound = o.m_bound;
                    m_versions = o.m_versions;
                    m_var_info = o.m_var_info;
                }


            public:

                veo_adaptive_pg_v2() = default;

                veo_adaptive_pg_v2(const query_type *query,
                             const std::vector<ltj_iter_type*> *iterators,
                             const var_to_iterators_type *var_iterators,
                             ring_type *r) {
                    m_ptr_query = query;
                    m_ptr_iterators = iterators;
                    m_ptr_var_iterators = var_iterators;
                    m_ptr_ring = r;

                    std::vector<var_type> lonely_edges;
                    auto nvars = m_ptr_query->ht.size();
                    m_var_info.resize(nvars+1);

                    for (var_type v = 1; v < m_var_info.size(); ++v) {
                        const auto &iters = m_ptr_var_iterators->at(v);
                        if(iters.size() == 1){
                            if (m_ptr_query->vnodes[v]) {
                                m_lonely.emplace_back(v);
                            }else {
                                lonely_edges.emplace_back(v);
                            }
                            m_var_info[v].is_bound = false;
                            m_var_info[v].is_lonely = true;
                        }else {
                            for (const auto &iter : iters) {
                                m_var_info[v].selectivity *= iter->selectivity();
                                m_var_info[v].triples = std::min(m_var_info[v].triples, iter->interval_length());
                            }
                            m_var_info[v].weight = m_var_info[v].triples * m_var_info[v].selectivity;
                            m_var_info[v].is_bound = false;
                            m_var_info[v].is_lonely = false;
                        }
                    }

                    //relacionar variables que no son lonely
                    for (const auto &pattern : m_ptr_query->patterns) {
                        if (pattern.subj.is_var() && !m_var_info[pattern.subj.var_value].is_lonely
                            && pattern.obj.is_var() && !m_var_info[pattern.obj.var_value].is_lonely) {
                            m_var_info[pattern.subj.var_value].related.insert(pattern.obj.var_value);
                            m_var_info[pattern.obj.var_value].related.insert(pattern.subj.var_value);
                        }
                    }

                    if (m_ptr_query->where.type == query::WAND) {
                        for (const auto &comp : m_ptr_query->where.args) {
                            if (comp.is_var[0] && comp.is_var[1]) {
                                m_var_info[comp.values[0]].related.insert(comp.values[1]);
                                m_var_info[comp.values[1]].related.insert(comp.values[0]);
                            }
                        }
                    }else {
                        const auto &comp = m_ptr_query->where;
                        if (comp.is_var[0] && comp.is_var[1]) {
                            m_var_info[comp.values[0]].related.insert(comp.values[1]);
                            m_var_info[comp.values[1]].related.insert(comp.values[0]);
                        }
                    }

                    //insert lonely edges at the end
                    m_lonely.insert(m_lonely.end(), lonely_edges.begin(), lonely_edges.end());
                    m_index = 0;
                    /*for(const auto & v : m_var_info){
                        std::cout << "var=" << (uint64_t) v.name << " weight=" << v.weight << std::endl;
                    }*/
                    //std::cout << "Done. " << std::endl;

                }

                //! Copy constructor
                veo_adaptive_pg_v2(const veo_adaptive_pg_v2 &o) {
                    copy(o);
                }

                //! Move constructor
                veo_adaptive_pg_v2(veo_adaptive_pg_v2 &&o) {
                    *this = std::move(o);
                }

                //! Copy Operator=
                veo_adaptive_pg_v2 &operator=(const veo_adaptive_pg_v2 &o) {
                    if (this != &o) {
                        copy(o);
                    }
                    return *this;
                }

                //! Move Operator=
                veo_adaptive_pg_v2 &operator=(veo_adaptive_pg_v2 &&o) {
                    if (this != &o) {
                        m_ptr_query = std::move(o.m_ptr_query);
                        m_ptr_iterators = std::move(o.m_ptr_iterators);
                        m_ptr_var_iterators = std::move(o.m_ptr_var_iterators);
                        m_ptr_ring = std::move(o.m_ptr_ring);
                        m_lonely = std::move(o.m_lonely);
                        m_index = o.m_index;
                        m_bound = std::move(o.m_bound);
                        m_versions = std::move(o.m_versions);
                        m_var_info = std::move(o.m_var_info);
                    }
                    return *this;
                }

                void swap(veo_adaptive_pg_v2 &o) {
                    std::swap(m_ptr_query, o.m_ptr_query);
                    std::swap(m_ptr_iterators, o.m_ptr_iterators);
                    std::swap(m_ptr_var_iterators, o.m_ptr_var_iterators);
                    std::swap(m_ptr_ring, o.m_ptr_ring);
                    std::swap(m_lonely, o.m_lonely);
                    std::swap(m_index, o.m_index);
                    std::swap(m_bound, o.m_bound);
                    std::swap(m_versions, o.m_versions);
                    std::swap(m_var_info, o.m_var_info);
                }

                inline var_type next() {

                    if(m_index < nolonely_size()){ //No lonely
                        auto min = std::numeric_limits<weight_type>::max();
                        size_type min_pos = 0;
                        for (size_type i = 1; i < m_var_info.size(); ++i) {
                            const auto &v = m_var_info[i];
                            if (v.is_lonely || v.is_bound) continue;
                            if (min > v.weight) {
                                min = v.weight;
                                min_pos = i;
                            }
                        }
                        m_var_info[min_pos].is_bound = true;
                        m_bound.emplace(min_pos);
                        ++m_index;
                        return min_pos;
                    }else{
                        //Return the next lonely variable
                        ++m_index;
                        return m_lonely[m_index-1-nolonely_size()];
                    }
                }



                inline void down() {

                    if(m_index-1 < nolonely_size()){ //No lonely

                        auto pos_last = m_bound.top();
                        const auto &related = m_var_info[pos_last].related;
                        version_type version;
                        for(const auto &rel : related){ //Iterates on the related variables
                            if(!m_var_info[rel].is_bound){
                                weight_type init_w = m_var_info[rel].weight;
                                auto &iters = m_ptr_var_iterators->at(rel);
                                double_t selectivity = 1.0;
                                size_type triples = UINT64_MAX;
                                for(ltj_iter_type* iter : iters){ //Check each iterator
                                    selectivity *= iter->selectivity();
                                    triples = std::min(triples, iter->interval_length());
                                }

                                if(selectivity * triples < init_w){
                                    update_type update{rel,  init_w};
                                    version.emplace_back(update);  //Store an update
                                    m_var_info[rel].triples = triples;
                                    m_var_info[rel].selectivity = selectivity;
                                    m_var_info[rel].weight = selectivity * triples;
                                }
                            }
                        }
                        m_versions.emplace(version); //Add the updates to versions
                    }

                };

                inline void up() {
                    if(m_index-1 < nolonely_size()){ //No lonely
                        for(const update_type& update : m_versions.top()){ //Restart the weights
                            m_var_info[update.pos].weight = update.w;
                            m_var_info[update.pos].selectivity = update.selectivity;
                            m_var_info[update.pos].triples = update.triples;
                        }
                        m_versions.pop();
                    }

                };

                inline void done() {
                    --m_index;
                    if(m_index < nolonely_size()) { //No lonely
                        auto pos = m_bound.top();
                        m_var_info[pos].is_bound = false;
                        m_bound.pop();
                    }
                }


                inline size_type size() {
                    return m_var_info.size()-1;
                }

                inline size_type nolonely_size() {
                    return m_var_info.size() - m_lonely.size()-1;
                }
            };
        };
}

#endif //RING_VEO_ADAPTIVE_HPP
