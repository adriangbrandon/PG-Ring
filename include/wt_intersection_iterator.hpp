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


#ifndef RING_RPQ_WT_INTERSECTION_ITERATOR_HPP
#define RING_RPQ_WT_INTERSECTION_ITERATOR_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {


    template<class wt_t>
    class wt_intersection_iterator {
    public:
        typedef wt_t wt_type;
        typedef typename wt_type::size_type size_type;
        typedef typename wt_type::value_type value_type;
        typedef typename wt_type::node_type node_type;
        typedef std::vector<range_type> range_vec_type;
        typedef std::vector<node_type> node_vec_type;
        typedef std::pair<node_vec_type, range_vec_type> pnvr_type;
        typedef std::stack<pnvr_type> stack_type;

    private:
        std::vector<const wt_type*> m_wt_ptrs;
        stack_type m_stack;
        size_type m_size = 0;

        void copy(const wt_intersection_iterator &o) {
            m_wt_ptrs = o.m_wt_ptrs;
            m_stack = o.m_stack;
            m_size = o.m_size;
        }

    public:

        wt_intersection_iterator() = default;

        template<class Iterator>
        wt_intersection_iterator(const std::vector<Iterator*>& iterators, const uint64_t x_j){
            m_size = iterators.size();
            pnvr_type element;
            for(size_type i = 0; i < m_size; ++i){
                auto wm_data = iterators[i]->get_wm_data(x_j);
                m_wt_ptrs.emplace_back(wm_data.wm_ptr);
                element.first.emplace_back(std::move(m_wt_ptrs[i]->root()));
                element.second.emplace_back(std::move(wm_data.range));
            }
            m_stack.emplace(element);
        }

        /***
         * Next value of an intersection between WTs on the same alphabet
         */
        value_type next(){

            while (!m_stack.empty()) {
                const pnvr_type &x = m_stack.top();
                if (m_wt_ptrs[0]->is_leaf(x.first[0])) {
                   auto r = value_type(x.first[0].sym);
                   m_stack.pop();
                   return r;
                }else{
                    node_vec_type left_nodes, right_nodes;
                    range_vec_type left_ranges, right_ranges;
                    std::array<range_type, 2> child_ranges;
                    size_type rnk;
                    for(size_type i = 0; i < m_size; ++i){
                        auto child =  m_wt_ptrs[i]->my_expand(x.first[i], x.second[i],
                                                       child_ranges[0], child_ranges[1], rnk);

                        if(left_nodes.size() == i && !empty(child_ranges[0])){
                            left_nodes.emplace_back(std::move(child[0]));
                            left_ranges.emplace_back(child_ranges[0]);
                        }

                        if(right_nodes.size() == i && !empty(child_ranges[1])){
                            right_nodes.emplace_back(std::move(child[1]));
                            right_ranges.emplace_back(child_ranges[1]);
                        }

                        if(right_nodes.size() < i+1 && left_nodes.size() < i+1){
                            break;
                        }
                    }
                    m_stack.pop();
                    if(right_nodes.size() == m_size){
                        m_stack.emplace(std::move(right_nodes), std::move(right_ranges));
                    }
                    if(left_nodes.size() == m_size){
                        m_stack.emplace(std::move(left_nodes), std::move(left_ranges));
                    }
                }
            }
            return 0; //No intersection
        }


        bool is_empty() const {
            return m_size == 0;
        }

        //! Copy constructor
        wt_intersection_iterator(const wt_intersection_iterator &o) {
            copy(o);
        }

        //! Move constructor
        wt_intersection_iterator(wt_intersection_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        wt_intersection_iterator &operator=(const wt_intersection_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        wt_intersection_iterator &operator=(wt_intersection_iterator &&o) {
            if (this != &o) {
                m_wt_ptrs = std::move(o.m_wt_ptrs);
                m_stack = std::move(o.m_stack);
                m_size = o.m_size;
            }
            return *this;
        }

        void swap(wt_intersection_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wt_ptrs, o.m_wt_ptrs);
            std::swap(m_stack, o.m_stack);
            std::swap(m_size, o.m_size);
        }
    };

}

#endif //RING_RPQ_WT_INTERSECTION_ITERATOR_HPP
