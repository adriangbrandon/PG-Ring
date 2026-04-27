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
// Created by Adrián on 19/10/2019.
//

#ifndef SDSL_SUCC_SUPPORT_SD_HPP
#define SDSL_SUCC_SUPPORT_SD_HPP

#include <sdsl/int_vector.hpp>
#include <sdsl/sd_vector.hpp>
#include <sdsl/succ_support_v.hpp>

namespace sdsl {

    template<uint8_t t_b          = 1,
            class t_hi_bit_vector= bit_vector,
            class t_select_1     = typename t_hi_bit_vector::select_1_type,
            class t_select_0     = typename t_hi_bit_vector::select_0_type>
    class succ_support_sd;
    //! Select data structure for sd_vector
/*! \tparam t_b             Bit pattern.
 *  \tparam t_hi_bit_vector Type of the bitvector used for the unary decoded differences of
 *                          the high part of the positions of the 1s.
 *  \tparam t_select_1      Type of the select structure which is used to select ones in HI.
 *  \tparam t_select_0      Type of the select structure which is used to select zeros in HI.
 */
    template<uint8_t t_b, class t_hi_bit_vector, class t_select_1, class t_select_0>
    class succ_support_sd
    {
    public:
        typedef bit_vector::size_type size_type;
        typedef sdsl::sd_vector<t_hi_bit_vector, t_select_1, t_select_0> bit_vector_type;
        enum { bit_pat = t_b };
        enum { bit_pat_len = (uint8_t)1 };
    private:
        const bit_vector_type* m_v = nullptr;
        sdsl::succ_support_v<1> m_succ_high;

        void copy(const succ_support_sd& ss){
            m_v = ss.m_v;
            m_succ_high = ss.m_succ_high;
            if(m_v != nullptr){
                m_succ_high.set_vector(&(m_v->high));
            }
        }
    public:

        succ_support_sd() = default;

        explicit succ_support_sd(const bit_vector_type* v)
        {
            set_vector(v);
            if(v != nullptr){
                sdsl::util::init_support(m_succ_high, &(m_v->high));
            }
        }

        //! Returns the position of the i-th occurrence in the bit vector.
        size_type succ(size_type i)const
        {
            size_type high_val = (i >> (m_v->wl));
            size_type val_low = i & bits::lo_set[ m_v->wl ];
            size_type sel_high = m_v->high_0_select(high_val+1);
            size_type rank_low = sel_high - high_val;


            int64_t r = rank_low;
            int64_t s = sel_high;
            do {
                --s; --r;
            }while(s >= 0 and m_v->high[s] and m_v->low[r] >= val_low);

            if(m_v->high[s+1]){
                return m_v->low[r+1] + ((high_val) << m_v->wl);
            }else{
                size_type succ_high = m_succ_high(sel_high);
                if(succ_high < m_v->high.size()){
                    return m_v->low[rank_low] +
                           ((high_val+(succ_high-sel_high)) << m_v->wl);
                }
                return m_v->size();
            }

        }

        size_type operator()(size_type i)const
        {
            return succ(i);
        }

        size_type size()const
        {
            return m_v->size();
        }

        void set_vector(const bit_vector_type* v=nullptr)
        {
            m_v = v;
            if(m_v != nullptr){
                m_succ_high.set_vector(&(m_v->high));
            }
        }

        succ_support_sd(const succ_support_sd& p){
            copy(p);
        };

        succ_support_sd(succ_support_sd&& p){
            *this = std::move(p);
        };

        succ_support_sd& operator=(const succ_support_sd& ss)
        {
            if (this != &ss) {
                copy(ss);
            }
            return *this;
        }

        succ_support_sd& operator=(succ_support_sd&& ss)
        {
            if (this != &ss) {
                m_v = std::move(ss.m_v);
                m_succ_high = std::move(ss.m_succ_high);
                if(m_v != nullptr){
                    m_succ_high.set_vector(&(m_v->high));
                }
            }
            return *this;
        }

        void swap(succ_support_sd& ss) {
            std::swap(m_v, ss.m_v);
            m_succ_high.swap(ss.m_succ_high);
            if(m_v != nullptr){
                m_succ_high.set_vector(&(m_v->high));
            }else{
                m_succ_high.set_vector(nullptr);
            }
            if(ss.m_v != nullptr){
                ss.m_succ_high.set_vector(&(ss.m_v->high));
            }else{
                ss.m_succ_high.set_vector(nullptr);
            }
        }

        void load(std::istream& in, const bit_vector_type* v=nullptr)
        {
            m_v = v;
            if(m_v != nullptr){
                m_succ_high.load(in, &(m_v->high));
            }else{
                m_succ_high.load(in);
            }

        }

        size_type serialize(std::ostream& out, structure_tree_node* v=nullptr, std::string name="")const
        {
            sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_succ_high.serialize(out, child, "succ_high");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
    };
}

#endif //RUNS_VECTORS_SUCC_SUPPORT_SD_HPP
