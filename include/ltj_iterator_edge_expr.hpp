/*
 * ltj_iterator_edge_expr.hpp
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

#ifndef RING_LTJ_ITERATOR_EDGE_EXPR_HPP
#define RING_LTJ_ITERATOR_EDGE_EXPR_HPP

#define VERBOSE 0


#include <ltj_iterator_base.hpp>

namespace ring {

    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_edge_expr : public ltj_iterator_base< var_t, cons_t>{

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef  wt_range_iterator<typename ring_type::bwt_type::wm_type> wt_so_iterator_type;
        typedef  wt_range_iterator<typename ring_type::bwt_p_type::wm_type> wt_p_iterator_type;
        //std::vector<value_type> leap_result_type;

    private:
        //TODO: penso que maximo vai ter 2 niveles porque o p xa está fixado de algunha forma (pero o p é unha label, non é o id da arista)
        //Teño que gardar os rangos nunha variable
        //Teño que ter un iterador do wt que filtre polos rangos no alfabeto
        const triple_pattern *m_pattern;
        ring_type *m_ptr_ring; //TODO: should be const
        std::array<value_type, 3> m_consts;
        std::array<state_type, 3> m_state;
        size_type m_level = 0;
        bool m_is_empty = false;
        //std::stack<state_type> m_states;

        std::vector<range_type> m_ranges_expr; //TODO calcular a partir de expresión. rangos en alfabeto de P
        std::array<std::vector<range_type>, 3> m_ranges_level;
        std::array<size_type, 3> m_length_level;

        //Mechanism to simulate leap on the last level
        size_type m_range_i = 0; //current range in last level
        size_type m_triple_j = 0; //current triple in current range

        void copy(const ltj_iterator_edge_expr &o) {
            m_is_empty = o.m_is_empty;
            m_pattern = o.m_ptr_triple_pattern;
            m_ptr_ring = o.m_ptr_ring;
            m_state = o.m_state;
            m_level = o.m_level;
            m_consts = o.m_consts;
        }

        void compute_ranges(const std::vector<range_type> &expr_ranges, std::vector<range_type> &bwt_ranges) {

            uint64_t lb, rb;
            for (const auto &r: expr_ranges) {
                lb = r[0]; rb = r[1];
                //Convert to BWT ranges
                if (lb < rb) {
                    lb = m_ptr_ring->init_P(lb).first;
                    rb = m_ptr_ring->init_P(rb).second;
                }else {
                    std::tie(lb, rb) = m_ptr_ring->init_P(lb);
                }
                bwt_ranges.push_back(range_type{lb, rb});
            }

            std::sort(bwt_ranges.begin(), bwt_ranges.end());
            //Merge overlapping ranges
            std::unique(bwt_ranges.begin(), bwt_ranges.end(),
                [](const range_type &a, const range_type &b) {
                            return !(a[1] < b[0] || b[1] < a[0]);
                        });

        }

        bool down_P_to_S(value_type s) {
            //2. facer bwd_step para obter o rango en SPO
            bool exists = false;
            for (const auto &r : m_ranges_level[0]) {
                auto r_aux = m_ptr_ring->edge_expr_down_P_S(r, s);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[1].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        bool down_P_to_O(value_type o) {
            auto r = m_ptr_ring->init_O(o);
            auto ps =  m_ptr_ring->edge_expr_all_P_in_range(r, m_ranges_expr);
            bool exists = false;
            //2. por cada p posible facer bwd_step con o para obter o rango en POS (vai dar varios rangos)
            for (const auto &p : ps) {
                auto r_aux = m_ptr_ring->edge_expr_down_O_P(r, p);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[1].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        bool down_PS_to_O(value_type o) {
            bool exists = false;
            for (const auto &r : m_ranges_level[1]) {
                auto r_aux = m_ptr_ring->edge_expr_down_S_O(r, m_pattern->term_o.value);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[2].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        bool down_PO_to_S(value_type s) {
            //2. facer bwd_step para obter o rango en SPO
            bool exists = false;
            for (const auto &r : m_ranges_level[1]) {
                auto r_aux = m_ptr_ring->edge_expr_down_P_S(r, s);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[2].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        void compute_length(size_type level) {
            m_length_level[level] = 0;
            for (const auto &r : m_ranges_level[level]) {
                 m_length_level[level] += sdsl::size(r);
            }
        }

    public:
        //const bool &is_empty = m_is_empty;
        const size_type& level = m_level;
        const std::array<state_type, 3>& state = m_state;
        const std::array<value_type, 3>& consts = m_consts;

        ltj_iterator_edge_expr() = default;

        //TODO: Property Graphs modificar o triple pattern
        ltj_iterator_edge_expr(const triple_pattern *triple, ring_type *ring) {


            //TODO: vou asumir que os rangos de ranges_expr están ben por agora
            m_pattern = triple;
            m_ptr_ring = ring;


            //There is a label expression in P, but can have a lonely variable
            if (m_pattern->s_is_variable() && m_pattern->o_is_variable()) {
                //TODO: calcular os rangos a partir de m_ranges_expr
                compute_ranges(m_ranges_expr, m_ranges_level[0]);
            }else if (!m_pattern->s_is_variable() && m_pattern->o_is_variable()) {
                //TODO: 1. calcular os rangos a partir de m_ranges_expr
                compute_ranges(m_ranges_expr, m_ranges_level[0]);
                //TODO: 2. facer bwd_step para obter o rango en SPO
                bool exists = down_P_to_S(m_pattern->term_s.value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = m_pattern->term_s.value;
                m_level = 1;
            }else if (m_pattern->s_is_variable() ) { //&& !m_pattern->o_is_variable()
                bool exists = down_P_to_O(m_pattern->term_o.value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = o;
                m_consts[0] = m_pattern->term_o.value;
                m_level = 1;

            }else { //!m_pattern->s_is_variable() && !m_pattern->o_is_variable()
                //TODO: 1. calcular os rangos a partir de m_ranges_expr
                compute_ranges(m_ranges_expr, m_ranges_level[0]);

                //TODO: 2. facer bwd_step con S para obter o rango en SPO
                bool exists = down_P_to_S(m_pattern->term_s.value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = m_pattern->term_s.value;

                //TODO: 3. facer bwd_step con O para obter o rango en OSP
                exists = down_PS_to_O(m_pattern->term_o.value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;
                m_consts[1] = m_pattern->term_o.value;

                m_level = 2;
            }
            compute_length(m_level);


        }

        //! Copy constructor
        ltj_iterator_edge_expr(const ltj_iterator_edge_expr &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_edge_expr(ltj_iterator_edge_expr &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_edge_expr &operator=(const ltj_iterator_edge_expr &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_edge_expr &operator=(ltj_iterator_edge_expr &&o) {
            if (this != &o) {
                m_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_consts = std::move(o.m_consts);
                m_state = std::move(o.m_state);
                m_level = o.m_level;
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_iterator_edge_expr &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_consts, o.m_consts);
            std::swap(m_state, o.m_state);
            std::swap(m_level, o.m_level);
            std::swap(m_is_empty, o.m_is_empty);
        }


        inline bool is_variable_subject(var_type var) {
            return m_pattern->term_s.is_variable && var == m_pattern->term_s.value;
        }

        inline bool is_variable_predicate(var_type var) {
            return m_pattern->term_p.is_variable && var == m_pattern->term_p.value;
        }

        inline bool is_variable_object(var_type var) {
            return m_pattern->term_o.is_variable && var == m_pattern->term_o.value;
        }


        inline bool is_empty(){
            return m_is_empty;
        }

        void down(var_type var, size_type c) { //Go down in the trie
            if (m_level > 2) return;
            if (m_level == 0) {
                if (is_variable_subject(var)) {
                    down_P_to_S(c);
                    m_state[m_level] = s;
                } else {
                    down_P_to_O(c);
                    m_state[m_level] = o;
                }
            } else if(m_level == 1) {//m_level = 1
                if (m_state[0] == o) {
                    down_PO_to_S(c);
                    m_state[m_level] = s;
                } else {
                    down_PS_to_O(c);
                    m_state[m_level] = o;
                }
            }
            m_consts[m_level] = c;
            ++m_level;
            compute_length(m_level);
        };

        void down(var_type var, size_type c, size_type k){
            down(var, c);
        };


        void up(var_type var) { //Go up in the trie
            if(m_level == 0) return;
            --m_level;
        };

        value_type
        leap(var_type var) { //Return the minimum in the range
            //0. Which term of our triple pattern is var
            if(m_level == 0){
                if(is_variable_subject(var)){
                    return m_ptr_ring->edge_expr_min_S_in_P(m_ranges_level[0]);
                }else {
                    return m_ptr_ring->edge_expr_min_O_in_P(m_ranges_expr);
                }
            }else if (m_level == 1){
                if (m_state[0] == o) {
                    return m_ptr_ring->edge_expr_min_S_in_PO(m_ranges_level[1]);
                } else {
                    return m_ptr_ring->edge_expr_min_O_in_SP(m_ranges_level[1]);
                }
            }else{
                //TODO: esto penso que non e necesario, deberian ser lonely (enton deberian ir por seek_all)
                if (m_state[0] == o) {
                    //POS_to_ID
                }else {
                    //PSO_to_ID
                }
            }
            return 0;
        };

        value_type leap(var_type var, size_type c) { //Return the next value greater or equal than c in the range
            if(m_level == 0){
                if(is_variable_subject(var)){
                    return m_ptr_ring->edge_expr_next_S_in_P(m_ranges_level[0], c);
                }else {
                    return m_ptr_ring->edge_expr_next_O_in_P(m_ranges_expr, c);
                }
            }else if (m_level == 1){
                if (m_state[0] == o) {
                    return m_ptr_ring->edge_expr_next_S_in_PO(m_ranges_level[1], c);
                } else {
                    return m_ptr_ring->edge_expr_next_O_in_SP(m_ranges_level[1], c);
                }
            }else{
                //TODO: esto penso que non e necesario, deberian ser lonely (enton deberian ir por seek_all)
                if (m_state[0] == o) {
                    //POS_to_ID
                }else {
                    //PSO_to_ID
                }
            }
            return 0;
        }

        inline bool in_last_level(){
            return m_level == 2;
        }

        inline size_type interval_length() const{
            return m_length_level[m_level];
        }

        //TODO: non vai funcionar (solo se usa para ver o número de valores distintos)
        /*inline const bwt_interval& interval() const{
            return m_intervals[m_level];
        }*/

        //Solo funciona en último nivel, en otro caso habría que reajustar
        /*std::vector<uint64_t> seek_all(var_type var){
            if (is_variable_subject(var)){
                return m_ptr_ring->all_S_in_range(m_intervals[2]);
            }else if (is_variable_predicate(var)){
                return m_ptr_ring->all_P_in_range(m_intervals[2]);
            }else if (is_variable_object(var)){
                return m_ptr_ring->all_O_in_range(m_intervals[2]);
            }
            return {};
        }*/

        value_type seek_last(var_type var){
            m_range_i = 0;
            const auto &r = m_ranges_level[2][m_range_i];
            m_triple_j = r[0];
            if (m_state[1] == o) {
                return m_ptr_ring->edge_expr_map_PSO_to_ID(m_triple_j);
            }else {
                //TODO: este metodo pode mellorarse, porque sei o valor de O que é o que cae na última columna
                return m_ptr_ring->edge_expr_map_POS_to_ID(m_triple_j);
            }
        }

        value_type seek_last_next(var_type var){
            auto &r = m_ranges_level[2][m_range_i];
            ++m_triple_j;
            if (m_triple_j > r[1]) {
                if (m_range_i + 1 == m_ranges_level[2].size()) {
                    return 0; //No more triples
                }
                r = m_ranges_level[2][++m_range_i];
                m_triple_j = r[0];
            }
            if (m_state[1] == o) {
                return m_ptr_ring->edge_expr_map_PSO_to_ID(m_triple_j);
            }else {
                //TODO: este metodo pode mellorarse, porque sei o valor de O que é o que cae na última columna
                return m_ptr_ring->edge_expr_map_POS_to_ID(m_triple_j);
            }
        }


    };

}

#endif //RING_LTJ_ITERATOR_EDGE_EXPR_HPP