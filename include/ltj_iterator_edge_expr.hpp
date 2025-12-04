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


#include <query/query_parser.hpp>
#include <ltj_iterator_base.hpp>
#include <ranges_util.hpp>

namespace ring {

    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_edge_expr : public ltj_iterator_base< var_t, cons_t>{

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef query::triple_parser::triple_type pattern_type;
        typedef  wt_range_iterator<typename ring_type::bwt_type::wm_type> wt_so_iterator_type;
        typedef  wt_range_iterator<typename ring_type::bwt_p_type::wm_type> wt_p_iterator_type;
        //std::vector<value_type> leap_result_type;

    private:
        //Teño que gardar os rangos nunha variable
        //Teño que ter un iterador do wt que filtre polos rangos no alfabeto
        const pattern_type *m_pattern;
        ring_type *m_ptr_ring; //TODO: should be const
        std::array<value_type, 3> m_consts;
        std::array<state_type, 3> m_state;
        size_type m_level = 0;
        bool m_is_empty = false;
        //std::stack<state_type> m_states;

        std::vector<range_type> m_ranges_expr;
        std::array<std::vector<range_type>, 3> m_ranges_level;
        std::array<size_type, 3> m_length_level;

        size_type m_range_i_last = 0; //current range in last level of a lonely variable
        size_type m_triple_j = 0; //current triple in current range

        void copy(const ltj_iterator_edge_expr &o) {
            m_is_empty = o.m_is_empty;
            m_pattern = o.m_ptr_triple_pattern;
            m_ptr_ring = o.m_ptr_ring;
            m_state = o.m_state;
            m_level = o.m_level;
            m_consts = o.m_consts;
            m_ranges_expr = o.m_ranges_expr;
            m_ranges_level = o.m_ranges_level;
            m_length_level = o.m_length_level;
            m_range_i_last = o.m_range_i_last;
            m_triple_j = o.m_triple_j;
        }


        std::vector<range_type> get_ranges_expr(const query::label_expr_parser::expr_label_type &expr) {

            std::vector<range_type> ans, tmp;
            if (expr.type == query::LAB) {
                ans.emplace_back(range_type{expr.label, expr.label});
            }else if (expr.type == query::NEG) {
                if (expr.label > 1) ans.emplace_back(range_type{1, expr.label-1});
                if (expr.label < m_ptr_ring->max_p) ans.push_back(range_type{expr.label+1, m_ptr_ring->max_p});
            }else if (expr.type == query::OR) {
                for (const auto &arg : expr.args) {
                    auto r_aux = get_ranges_expr(arg);
                    tmp.insert(tmp.end(), r_aux.begin(), r_aux.end());
                }
                ans = ranges::merge(tmp);
            }else if (expr.type == query::AND) {
                std::vector<std::vector<range_type>> all_ranges;
                for (const auto &arg : expr.args) {
                    auto r_aux = get_ranges_expr(arg);
                    all_ranges.emplace_back(r_aux);
                }
                ans = ranges::intersect(all_ranges);
            }
            return ans;
        }


        void compute_ranges_initial_level() {
            uint64_t lb, rb;
            for (const auto &r: m_ranges_expr) {
                lb = r[0]; rb = r[1];
                //Convert to BWT ranges
                if (lb < rb) {
                    lb = m_ptr_ring->init_P(lb).first;
                    rb = m_ptr_ring->init_P(rb).second;
                }else {
                    std::tie(lb, rb) = m_ptr_ring->init_P(lb);
                }
                m_ranges_level[0].push_back(range_type{lb, rb});
            }
        }

        /*void compute_ranges(const std::vector<range_type> &expr_ranges, std::vector<range_type> &bwt_ranges) {

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

        }*/

        bool down_P_to_S(value_type s) {
            //Bwd step to get the range in SPO
            bool exists = false;
            for (auto &r : m_ranges_level[0]) {
                auto r_aux = m_ptr_ring->edge_expr_down_P_S(r, s);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[1].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        bool down_P_to_O(value_type o) {
            //Initial range in OSP
            auto aux = m_ptr_ring->init_O(o);
            auto r = range_type{aux.first, aux.second};
            //Compute the possible Ps in the range
            auto ps =  m_ptr_ring->edge_expr_all_P_in_range(r, m_ranges_expr);
            bool exists = false;
            //For each p in Ps, Bwd step to get ranges in POS
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
            //Bwd step to get the range in OSP
            bool exists = false;
            for (auto &r : m_ranges_level[1]) {
                auto r_aux = m_ptr_ring->edge_expr_down_S_O(r, o);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[2].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        bool down_PO_to_S(value_type s) {
            //Bwd step to get the range in SPO
            bool exists = false;
            for (auto &r : m_ranges_level[1]) {
                auto r_aux = m_ptr_ring->edge_expr_down_P_S(r, s);
                if (!sdsl::empty(r_aux)) {
                    m_ranges_level[2].push_back(r_aux);
                    exists = true;
                }
            }
            return exists;
        }

        bool down_to_E(value_type e, size_type l) {
            m_ranges_level[l].push_back(range_type{e, e});
            return true;
        }

        value_type min_in_ranges(size_type level) {
            return m_ranges_level[level][0][0];
        }

        value_type next_in_ranges(size_type level, value_type current) {
            auto r_i = 0;
            while (m_ranges_level[level][r_i][1] < current) {
                //Move to next range
                ++r_i;
                if (r_i == m_ranges_level[level].size()) {
                    return 0; //No more triples
                }
            }
            if (m_ranges_level[level][r_i][0] >= current) {
                return m_ranges_level[level][r_i][0];
            }
            return current;
        }


        void compute_length(size_type level) {
            m_length_level[level] = 0;
            for (auto &r : m_ranges_level[level]) {
                 m_length_level[level] += sdsl::size(r);
            }
        }

    public:
        //const bool &is_empty = m_is_empty;
        const size_type& level = m_level;
        const std::array<state_type, 3>& state = m_state;
        const std::array<value_type, 3>& consts = m_consts;

        ltj_iterator_edge_expr() = default;

        ltj_iterator_edge_expr(const pattern_type *triple, ring_type *ring) {


            m_pattern = triple;
            m_ptr_ring = ring;


            m_ranges_expr = get_ranges_expr(m_pattern->edge.expr);
            //There is a label expression in P, but can have a lonely variable
            if (m_pattern->subj.is_var() && m_pattern->obj.is_var()) {
                //Compute ranges from m_ranges_expr
                compute_ranges_initial_level();
            }else if (!m_pattern->subj.is_var() && m_pattern->obj.is_var()) {
                //Compute ranges from m_ranges_expr
                compute_ranges_initial_level();
                bool exists = down_P_to_S(m_pattern->subj.const_value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = m_pattern->subj.const_value;
                m_level = 1;
            }else if (m_pattern->subj.is_var() ) { //&& !m_pattern->obj.is_var()
                bool exists = down_P_to_O(m_pattern->obj.const_value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = o;
                m_consts[0] = m_pattern->obj.const_value;
                m_level = 1;

            }else { //!m_pattern->s_is_variable() && !m_pattern->o_is_variable()
                //Compute ranges from m_ranges_expr
                compute_ranges_initial_level();

                bool exists = down_P_to_S(m_pattern->subj.const_value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[0] = s;
                m_consts[0] = m_pattern->subj.const_value;

                exists = down_PS_to_O(m_pattern->obj.const_value);
                if (!exists) {
                    m_is_empty = true;
                    return;
                }
                m_state[1] = o;
                m_consts[1] = m_pattern->obj.const_value;

                m_level = 2;
            }
            compute_length(m_level);


        }

        //! Copy constructor
        ltj_iterator_edge_expr(const ltj_iterator_edge_expr &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_edge_expr(ltj_iterator_edge_expr &&o) noexcept {
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
        ltj_iterator_edge_expr &operator=(ltj_iterator_edge_expr &&o) noexcept {
            if (this != &o) {
                m_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_consts = std::move(o.m_consts);
                m_state = std::move(o.m_state);
                m_level = o.m_level;
                m_is_empty = o.m_is_empty;
                m_ranges_expr = std::move(o.m_ranges_expr);
                m_ranges_level = std::move(o.m_ranges_level);
                m_length_level = std::move(o.m_length_level);
                m_range_i_last = o.m_range_i_last;
                m_triple_j = o.m_triple_j;
            }
            return *this;
        }

        void swap(ltj_iterator_edge_expr &o) noexcept {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_consts, o.m_consts);
            std::swap(m_state, o.m_state);
            std::swap(m_level, o.m_level);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_is_empty, o.m_is_empty);
        }


        inline bool is_variable_subject(var_type var) {
            return m_pattern->subj.var_value == var;
        }

        inline bool is_variable_predicate(var_type var) {
            return m_pattern->edge.var_value == var;
        }

        inline bool is_variable_object(var_type var) {
            return m_pattern->obj.var_value == var;
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
                } else if (is_variable_predicate(var)) {
                    down_to_E(c, m_level+1);
                    m_state[m_level] = p;
                } else {
                    down_P_to_O(c);
                    m_state[m_level] = o;
                }
            } else if(m_level == 1) {//m_level = 1
                if (is_variable_subject(var)) {
                    down_PO_to_S(c);
                    m_state[m_level] = s;
                } else if (is_variable_predicate(var)) {
                    down_to_E(c, m_level+1);
                    m_state[m_level] = p;
                } else {
                    down_PS_to_O(c);
                    m_state[m_level] = o;
                }
            }
            m_consts[m_level] = c;
            ++m_level;
            if (m_level < 3) compute_length(m_level);
        };

        void down(var_type var, size_type c, size_type k){
            down(var, c);
        };


        void up(var_type var) { //Go up in the trie
            if(m_level == 0) return;
            std::cout << "Up edge_expr" << std::endl;
            m_ranges_level[m_level].clear();
            --m_level;
        };

        value_type
        leap(var_type var) { //Return the minimum in the range
            //0. Which term of our triple pattern is var
            if(m_level == 0){
                if(is_variable_subject(var)) {
                    return m_ptr_ring->edge_expr_min_S_in_P(m_ranges_level[0]);
                } else if (is_variable_predicate(var)) {
                    return min_in_ranges(m_level);
                }else {
                    return m_ptr_ring->edge_expr_min_O_in_P(m_ranges_expr);
                }
            }else if (m_level == 1){
                if (m_state[0] == s) { //fixed subject
                    if (is_variable_object(var)) {
                        return m_ptr_ring->edge_expr_min_O_in_SP(m_ranges_level[1]);
                    }
                    if (is_variable_predicate(var)) {
                        //Select next in POS with constant of s
                        return m_ptr_ring->edge_expr_min_E_in_SP(m_ranges_level[0], m_consts[0]);
                    }
                }else if (m_state[0] == p) { //fixed edge
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->edge_expr_get_S(m_consts[0]); //TODO: chequear que estea en consts fixado
                    }
                    if (is_variable_object(var)) {
                        //TODO: LF mapping + get value at position in SPO
                        return m_ptr_ring->edge_expr_get_O(m_consts[0]);
                    }
                }else if (m_state[0] == o) { //fixed object
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->edge_expr_min_S_in_PO(m_ranges_level[1]);
                    }
                    if (is_variable_predicate(var)) {
                        return min_in_ranges(m_level);
                    }
                }
            }else if (m_level == 2) {
                if (is_variable_subject(var)) {
                    auto i = (m_state[0] == p) ? 0 : 1;
                    return m_ptr_ring->edge_expr_get_S(m_consts[i]);
                }
                if (is_variable_object(var)) {
                    auto i = (m_state[0] == p) ? 0 : 1;
                    return m_ptr_ring->edge_expr_get_O(m_consts[i]);
                }
                if (is_variable_predicate(var)) {
                    if (m_state[1] == s) {
                        return m_ptr_ring->edge_expr_min_E_in_SP(m_ranges_level[1], m_consts[1]);
                    }else {
                        return m_ptr_ring->edge_expr_min_E_in_OS(m_ranges_level[2]);
                    }
                }
            }
            throw std::out_of_range("ltj_iterator_edge_expr::leap");
        };

        value_type leap(var_type var, size_type c) { //Return the next value greater or equal than c in the range
            if(m_level == 0){
                if(is_variable_subject(var)){
                    return m_ptr_ring->edge_expr_next_S_in_P(m_ranges_level[0], c);
                } else if (is_variable_predicate(var)) {
                    return next_in_ranges(m_level, c);
                }else {
                    return m_ptr_ring->edge_expr_next_O_in_P(m_ranges_expr, c);
                }
            }else if (m_level == 1){
                if (m_state[0] == s) { //fixed subject
                    if (is_variable_object(var)) {
                        return m_ptr_ring->edge_expr_next_O_in_SP(m_ranges_level[1], c);
                    }
                    if (is_variable_predicate(var)) {
                        //Select next in POS with constant of s
                        return m_ptr_ring->edge_expr_next_E_in_SP(m_ranges_level[0], m_consts[0], c);
                    }
                }else if (m_state[0] == p) { //fixed edge
                    if (is_variable_subject(var)) {
                        auto v = m_ptr_ring->edge_expr_get_S(m_consts[0]);
                        if (v >= c) return v;
                        return 0;
                    }
                    if (is_variable_object(var)) {
                        auto v = m_ptr_ring->edge_expr_get_O(m_consts[0]);
                        if (v >= c) return v;
                        return 0;
                    }
                }else if (m_state[0] == o) { //fixed object
                    if (is_variable_subject(var)) {
                        return m_ptr_ring->edge_expr_next_S_in_PO(m_ranges_level[1], c);
                    }
                    if (is_variable_predicate(var)) {
                        return next_in_ranges(m_level, c);
                    }
                }
            }else if (m_level == 2) {
                if (is_variable_subject(var)) {
                    auto i = (m_state[1] == p);
                    auto v = m_ptr_ring->edge_expr_get_S(m_consts[i]);
                    if (v >= c) return v;
                    return 0;
                }
                if (is_variable_object(var)) {
                    auto i = (m_state[1] == p);
                    auto v = m_ptr_ring->edge_expr_get_O(m_consts[i]);
                    if (v >= c) return v;
                    return 0;
                }
                if (is_variable_predicate(var)) {
                    if (m_state[1] == s) {
                        return m_ptr_ring->edge_expr_next_E_in_SP(m_ranges_level[1], m_consts[1], c);
                    }else {
                        return m_ptr_ring->edge_expr_next_E_in_OS(m_ranges_level[2], c);
                    }
                }
            }
            throw std::out_of_range("ltj_iterator_edge_expr::leap");
        }

        inline bool in_last_level(){
            return m_level == 2;
        }

        inline size_type interval_length() const{
            return m_length_level[m_level];
        }

        value_type seek_last(var_type var){ //var should be in an edge
            if (is_variable_subject(var)) {
                auto i = (m_state[1] == p);
                return m_ptr_ring->edge_expr_get_S(m_consts[i]);
            }
            if (is_variable_object(var)) {
                auto i = (m_state[1] == p);
                return m_ptr_ring->edge_expr_get_O(m_consts[i]);
            }
            if (is_variable_predicate(var)) {
                m_range_i_last = 0;
                m_triple_j = m_ranges_level[2][m_range_i_last][0];
                if (m_state[1] == o) {
                    return m_ptr_ring->map_OSP_to_POS(m_triple_j);
                }else {
                    return m_ptr_ring->map_SPO_to_POS(m_triple_j, m_consts[0]);
                }
            }
            return 0;

        }

        value_type seek_last_next(var_type var){
            if (!is_variable_predicate(var)) return 0; //no more triples
            ++m_triple_j;
            if (m_triple_j >m_ranges_level[2][m_range_i_last][1]) {
                if (m_range_i_last + 1 == m_ranges_level[2].size()) {
                    return 0; //No more triples
                }
                 m_triple_j = m_ranges_level[2][++m_range_i_last][0];
            }
            if (m_state[1] == o) {
                return m_ptr_ring->map_OSP_to_POS(m_triple_j);
            }else {
                return m_ptr_ring->map_SPO_to_POS(m_triple_j, m_consts[0]);
            }
        }

        /*void set_prop_value(var_type var, value_type value) {
            throw std::out_of_range("ltj_iterator_edge_expr::set_value_property");
        }

        value_type get_prop_value(var_type var, value_type c) {
            throw std::out_of_range("ltj_iterator_edge_expr::get_prop_value");
        }

        value_type compute_prop_value(var_type var, value_type c) {
            throw std::out_of_range("ltj_iterator_edge_expr::compute_prop_value");
        }*/

    };

}

#endif //RING_LTJ_ITERATOR_EDGE_EXPR_HPP