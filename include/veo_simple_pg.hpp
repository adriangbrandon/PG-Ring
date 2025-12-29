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


#ifndef RING_VEO_SIMPLE_PG_HPP
#define RING_VEO_SIMPLE_PG_HPP

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
            class veo_simple_pg {

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
                    var_type name;
                    std::unordered_set<var_type> related;
                    double_t weight; //triples * selectivity
                    uint8_t lonely_type; //0=no, 1=node, 2=edge
                } info_var_type;




                typedef query::pg_query query_type;
                typedef std::pair<size_type, var_type> pair_type;
                typedef veo_trait_t veo_trait_type;
                typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
                typedef std::stack<size_type> bound_type;
                typedef std::priority_queue<pair_type, std::vector<pair_type>, greater<pair_type>> min_heap_type;

                void fill_heap(const var_type var,
                           std::unordered_map<var_type, size_type> &hash_table,
                           std::vector<info_var_type> &vec,
                           std::vector<bool> &checked,
                           min_heap_type &heap) {

                    auto pos_var = hash_table[var];
                    for (const auto &e : vec[pos_var].related) {
                        auto pos_rel = hash_table[e];
                        if (!checked[pos_rel] && vec[pos_rel].lonely_type == 0) {
                            heap.push({vec[pos_rel].weight, e});
                            checked[pos_rel] = true;
                        }
                    }
                }

                struct compare_var_info {
                    inline bool operator()(const info_var_type &linfo, const info_var_type &rinfo) {
                        if (linfo.lonely_type == rinfo.lonely_type) {
                            return linfo.weight < rinfo.weight;
                        }
                        return linfo.lonely_type < rinfo.lonely_type;
                    }
                };

            private:
                const query_type *m_ptr_query;
                const std::vector<ltj_iter_type*> *m_ptr_iterators;
                const var_to_iterators_type *m_ptr_var_iterators;
                ring_type *m_ptr_ring;

                std::vector<var_type> m_order;
                size_type m_index;
                size_type m_nolonely_size;




                void copy(const veo_simple_pg &o) {
                    m_ptr_query = o.m_ptr_query;
                    m_ptr_iterators = o.m_ptr_iterators;
                    m_ptr_var_iterators = o.m_ptr_var_iterators;
                    m_ptr_ring = o.m_ptr_ring;
                    m_index = o.m_index;
                    m_order = o.m_order;
                }


            public:

                veo_simple_pg() = default;

                veo_simple_pg(const query_type *query,
                             const std::vector<ltj_iter_type*> *iterators,
                             const var_to_iterators_type *var_iterators,
                             ring_type *r) {
                    m_ptr_query = query;
                    m_ptr_iterators = iterators;
                    m_ptr_var_iterators = var_iterators;
                    m_ptr_ring = r;

                    auto nvars = m_ptr_query->ht.size();
                    std::vector<info_var_type> aux(nvars+1);
                    m_nolonely_size = 0;
                    for (var_type v = 1; v < aux.size(); ++v) {
                        const auto &iters = m_ptr_var_iterators->at(v);
                        if(iters.size() == 1){
                            if (m_ptr_query->vnodes[v]) {
                                aux[v].lonely_type = 1;
                            }else {
                                aux[v].lonely_type = 2;
                            }
                        } else {
                            double_t selectivity = 1.0;
                            size_type triples = UINT64_MAX;
                            for (const auto &iter : iters) {
                                selectivity *= iter->opt_selectivity();
                                triples = std::min(triples, iter->interval_length());
                            }
                            aux[v].weight = triples * selectivity;
                            aux[v].lonely_type = 0;
                            ++m_nolonely_size;
                        }
                        aux[v].name = v;
                    }

                    //related field
                    for (const auto &pattern : m_ptr_query->patterns) {
                        if (pattern.subj.is_var() && !aux[pattern.subj.var_value].lonely_type
                            && pattern.obj.is_var() && !aux[pattern.obj.var_value].lonely_type) {
                            aux[pattern.subj.var_value].related.insert(pattern.obj.var_value);
                            aux[pattern.obj.var_value].related.insert(pattern.subj.var_value);
                            }
                    }

                    if (m_ptr_query->where.type == query::WAND) {
                        for (const auto &comp : m_ptr_query->where.args) {
                            if (comp.is_var[0] && comp.is_var[1]) {
                                aux[comp.values[0]].related.insert(comp.values[1]);
                                aux[comp.values[1]].related.insert(comp.values[0]);
                            }
                        }
                    }else {
                        const auto &comp = m_ptr_query->where;
                        if (comp.is_var[0] && comp.is_var[1]) {
                            aux[comp.values[0]].related.insert(comp.values[1]);
                            aux[comp.values[1]].related.insert(comp.values[0]);
                        }
                    }

                    //insert lonely edges at the end
                    std::sort(aux.begin(), aux.end(), compare_var_info());


                    //mapping to positions
                    std::unordered_map<var_type, size_type> hash_table_position;
                    for (size_type i = 0; i < aux.size(); ++i) {
                        hash_table_position[aux[i].name] = i;
                    }

                    std::vector<bool> checked(m_nolonely_size, false);
                    m_order.reserve(aux.size()-1);
                    size_type i = 1;
                    while (i <= m_nolonely_size) { //Related variables
                        if (!checked[i]) {
                            m_order.push_back(aux[i].name); //Adding var to veo
                            checked[i] = true;
                            min_heap_type heap; //Stores the related variables that are related with the chosen ones
                            auto var_name = aux[i].name;
                            fill_heap(var_name, hash_table_position, aux, checked, heap);
                            while (!heap.empty()) {
                                var_name = heap.top().second;
                                heap.pop();
                                m_order.push_back(var_name);
                                fill_heap(var_name, hash_table_position, aux, checked, heap);
                            }
                        }
                        ++i;
                    }
                    while (i < aux.size()) { //Lonely variables
                        m_order.push_back(aux[i].name); //Adding var to gao
                        ++i;
                    }
                    m_index = 0;

                    /*for(const auto & v : m_var_info){
                        std::cout << "var=" << (uint64_t) v.name << " weight=" << v.weight << std::endl;
                    }*/
                    //std::cout << "Done. " << std::endl;

                }

                //! Copy constructor
                veo_simple_pg(const veo_simple_pg &o) {
                    copy(o);
                }

                //! Move constructor
                veo_simple_pg(veo_simple_pg &&o) {
                    *this = std::move(o);
                }

                //! Copy Operator=
                veo_simple_pg &operator=(const veo_simple_pg &o) {
                    if (this != &o) {
                        copy(o);
                    }
                    return *this;
                }

                //! Move Operator=
                veo_simple_pg &operator=(veo_simple_pg &&o) {
                    if (this != &o) {
                        m_ptr_query = std::move(o.m_ptr_query);
                        m_ptr_iterators = std::move(o.m_ptr_iterators);
                        m_ptr_var_iterators = std::move(o.m_ptr_var_iterators);
                        m_ptr_ring = std::move(o.m_ptr_ring);
                        m_index = o.m_index;
                        m_order = std::move(o.m_order);
                    }
                    return *this;
                }

                void swap(veo_simple_pg &o) {
                    std::swap(m_ptr_query, o.m_ptr_query);
                    std::swap(m_ptr_iterators, o.m_ptr_iterators);
                    std::swap(m_ptr_var_iterators, o.m_ptr_var_iterators);
                    std::swap(m_ptr_ring, o.m_ptr_ring);
                    std::swap(m_index, o.m_index);
                    std::swap(m_order, o.m_order);
                }

                inline var_type next() {
                    ++m_index;
                    return m_order[m_index-1];
                }



                inline void down() {
                };

                inline void up() {

                };

                inline void done() {
                    --m_index;
                }


                inline size_type size() {
                    return m_order.size();
                }

                inline size_type nolonely_size() {
                    return m_nolonely_size;
                }
            };
        };
}

#endif //RING_VEO_ADAPTIVE_HPP
