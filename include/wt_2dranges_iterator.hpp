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

#ifndef RING_RPQ_WT_2DITERATOR_HPP
#define RING_RPQ_WT_2DITERATOR_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {


    template<class wt_t>
    class wt_2dranges_iterator {
    public:
        typedef wt_t wt_type;
        typedef typename wt_type::size_type size_type;
        typedef typename wt_type::value_type value_type;
        typedef typename wt_type::node_type node_type;
        typedef struct {
            node_type node;
            range_type range;
            size_type ilb;
            size_type irb;
        } pnvr_type;
        typedef std::stack<pnvr_type> stack_type;

    private:
        const wt_type* m_wt_ptr;
        stack_type m_stack;
        std::vector<range_type> m_sigma_ranges;
        size_type m_i_sr = 0; // current sigma range

        void copy(const wt_2dranges_iterator &o) {
            m_wt_ptr = o.m_wt_ptr;
            m_stack = o.m_stack;
        }

        bool in_sigma_range(value_type b, value_type e) {
            while (m_sigma_ranges[m_i_sr][1] < b && m_i_sr < m_sigma_ranges.size()) {
                ++m_i_sr;
            }
            return (m_i_sr < m_sigma_ranges.size() &&
                    m_sigma_ranges[m_i_sr][0] <= e && m_sigma_ranges[m_i_sr][1] >= b);
        }

    public:

        wt_2dranges_iterator() = default;

        wt_2dranges_iterator(const wt_type* wt_ptr, const range_type &range, const std::vector<range_type> &sigma_ranges){
            m_wt_ptr = wt_ptr;
            m_sigma_ranges = sigma_ranges;
            pnvr_type element{m_wt_ptr->root(), range, 0, (1ULL << m_wt_ptr->max_level)-1};
            m_stack.emplace(element);
        }

        /***
         * Next value of an intersection between WTs on the same alphabet
         */
        value_type next(){

            while (!m_stack.empty() && m_i_sr < m_sigma_ranges.size()) {
                const pnvr_type x = m_stack.top(); m_stack.pop();

                if (!in_sigma_range(x.ilb, x.irb)) {
                    continue;
                }

                if (m_wt_ptr->is_leaf(x.node)) {
                   auto r = value_type(x.node.sym);
                   return r;
                }else{

                    size_type rnk;
                    array<range_type, 2> child_ranges;
                    auto child =  m_wt_ptr->my_expand(x.node, x.range,
                                                          child_ranges[0], child_ranges[1], rnk);
                    size_type mid = (x.irb + x.ilb+1)>>1;
                    if(!empty(child_ranges[1])){
                        m_stack.push(pnvr_type{std::move(child[1]), child_ranges[1], mid, x.irb});
                    }
                    if(!empty(child_ranges[0])){
                        m_stack.push(pnvr_type{std::move(child[0]), child_ranges[0], x.ilb, mid-1});
                    }
                }
            }
            return 0; //No more values
        }


        //! Copy constructor
        wt_2dranges_iterator(const wt_2dranges_iterator &o) {
            copy(o);
        }

        //! Move constructor
        wt_2dranges_iterator(wt_2dranges_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        wt_2dranges_iterator &operator=(const wt_2dranges_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        wt_2dranges_iterator &operator=(wt_2dranges_iterator &&o) {
            if (this != &o) {
                m_wt_ptr = std::move(o.m_wt_ptr);
                m_stack = std::move(o.m_stack);
            }
            return *this;
        }

        void swap(wt_2dranges_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wt_ptr, o.m_wt_ptr);
            std::swap(m_stack, o.m_stack);
        }
    };

}

#endif //RING_RPQ_WT_2DITERATOR_HPP
