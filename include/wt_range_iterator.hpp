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


//
// Created by Adrián on 22/9/22.
//

#ifndef RING_RPQ_WT_ITERATOR_HPP
#define RING_RPQ_WT_ITERATOR_HPP

#include <algorithm>
#include <utility>
#include <sdsl/wt_helper.hpp>

namespace sdsl {


    template<class wt_t>
    class wt_range_iterator {
    public:
        typedef wt_t wt_type;
        typedef typename wt_type::size_type size_type;
        typedef typename wt_type::value_type value_type;
        typedef typename wt_type::node_type node_type;
        typedef std::pair<node_type, range_type> pnvr_type;
        typedef std::stack<pnvr_type> stack_type;

    private:
        const wt_type* m_wt_ptr;
        stack_type m_stack;

        void copy(const wt_range_iterator &o) {
            m_wt_ptr = o.m_wt_ptr;
            m_stack = o.m_stack;
        }

    public:

        wt_range_iterator() = default;

        wt_range_iterator(const wt_type* wt_ptr, const range_type &range){
            m_wt_ptr = wt_ptr;
            pnvr_type element{m_wt_ptr->root(), range};
            m_stack.emplace(element);
        }

        /***
         * Next value of an intersection between WTs on the same alphabet
         */
        value_type next(){

            while (!m_stack.empty()) {
                const pnvr_type &x = m_stack.top();
                if (m_wt_ptr->is_leaf(x.first)) {
                   auto r = value_type(x.first.sym);
                   m_stack.pop();
                   return r;
                }else{
                    size_type rnk;
                    array<range_type, 2> child_ranges;
                    auto child =  m_wt_ptr->my_expand(x.first, x.second,
                                                          child_ranges[0], child_ranges[1], rnk);
                    m_stack.pop();
                    if(!empty(child_ranges[1])){
                        m_stack.emplace(std::move(child[1]), child_ranges[1]);
                    }
                    if(!empty(child_ranges[0])){
                        m_stack.emplace(std::move(child[0]), child_ranges[0]);
                    }
                }
            }
            return 0; //No more values
        }


        //! Copy constructor
        wt_range_iterator(const wt_range_iterator &o) {
            copy(o);
        }

        //! Move constructor
        wt_range_iterator(wt_range_iterator &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        wt_range_iterator &operator=(const wt_range_iterator &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        wt_range_iterator &operator=(wt_range_iterator &&o) {
            if (this != &o) {
                m_wt_ptr = std::move(o.m_wt_ptr);
                m_stack = std::move(o.m_stack);
            }
            return *this;
        }

        void swap(wt_range_iterator &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_wt_ptr, o.m_wt_ptr);
            std::swap(m_stack, o.m_stack);
        }
    };

}

#endif //RING_RPQ_WT_ITERATOR_HPP
