/*
 * ring.hpp
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

#ifndef RING_RING_PG
#define RING_RING_PG

#include <cstdint>
#include "bwt.hpp"
#include "bwt_interval.hpp"
#include <queue>
#include "ranges_util.hpp"
#include <sdsl/succ_support_v.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <property_grid_v2.hpp>

#include "string_mgr.hpp"
#include "query/where_expr_parser.hpp"

namespace ring {

    template <class bwt_so_t = bwt_plain, class bwt_p_t = bwt_plain>
    class ring_pg {
    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef bwt_so_t bwt_type;
        typedef bwt_p_t bwt_p_type;
        typedef std::tuple<uint32_t, uint32_t, uint32_t> spo_triple_type;
        typedef sdsl::bit_vector bit_vector_type;
        typedef typename bit_vector_type::succ_0_type succ_0_type;
        typedef typename bit_vector_type::succ_1_type succ_1_type;


    private:
        bwt_type m_bwt_s;   //POS
        bwt_p_type m_bwt_p; //OSP
        bwt_type m_bwt_o;   //SPO

        size_type m_max_s;
        size_type m_max_p;
        size_type m_max_o;
        size_type m_n_triples;  // number of triples

        //labels of nodes
        std::vector<bit_vector_type> m_bvts;
        std::vector<size_type> m_cnt_labels;
        std::vector<succ_0_type> m_succs0;
        std::vector<succ_1_type> m_succs1;

        //properties
        std::vector<property_grid_v2<>> m_node_properties;
        std::vector<property_grid_v2<>> m_edge_properties;

        //manager of strings
        string_mgr m_string_mgr;

        void copy(const ring_pg &o) {
            m_bwt_s = o.m_bwt_s;
            m_bwt_p = o.m_bwt_p;
            m_bwt_o = o.m_bwt_o;
            m_max_s = o.m_max_s;
            m_max_p = o.m_max_p;
            m_max_o = o.m_max_o;
            m_n_triples = o.m_n_triples;

            m_bvts = o.m_bvts;
            m_cnt_labels = o.m_cnt_labels;
            m_succs0 = o.m_succs0;
            m_succs1 = o.m_succs1;
            for (size_t i = 0; i < m_bvts.size(); i++) {
                m_succs0[i].set_vector(&m_bvts[i]);
                m_succs1[i].set_vector(&m_bvts[i]);
            }

            m_node_properties = o.m_node_properties;
            m_edge_properties = o.m_edge_properties;

            m_string_mgr = o.m_string_mgr;
        }

    public:

        const bwt_type &s_spo = m_bwt_s; //POS
        const bwt_p_type &p_spo = m_bwt_p; //OSP
        const bwt_type &o_spo = m_bwt_o; //SPO
        const size_type &n_triples = m_n_triples; //SPO

        const size_type& max_s = m_max_s;
        const size_type& max_p = m_max_p;
        const size_type& max_o = m_max_o;

        ring_pg() = default;

        // Assumes the triples have been stored in a vector<spo_triple>
        ring_pg(vector<spo_triple_type> &D, std::vector<std::vector<uint32_t>>& label2nodes,
                std::vector<std::vector<std::pair<value_type, std::string>>>& node_properties,
                std::vector<std::vector<std::pair<value_type, std::string>>>& edge_properties,
                std::vector<bool> &nprop_numeric, std::vector<bool> &eprop_numeric) {

            uint64_t i, pos_c;
            vector<spo_triple>::iterator it, triple_begin = D.begin(), triple_end = D.end();
            uint64_t U, n = m_n_triples = D.size();

            {
                m_max_p = std::get<1>(D[0]), U = std::get<0>(D[0]);
                if (std::get<2>(D[0]) > U)
                    U = std::get<2>(D[0]);

                for (i = 1; i < n; i++) {
                    if (std::get<1>(D[i]) > m_max_p)
                        m_max_p = std::get<1>(D[i]);

                    if (std::get<0>(D[i]) > U)
                        U = std::get<0>(D[i]);

                    if (std::get<2>(D[i]) > U)
                        U = std::get<2>(D[i]);
                }

            }
            uint64_t alphabet_SO = U;
            m_max_s = m_max_o = alphabet_SO;

            std::vector<uint32_t> M_O, M_S, M_P;

            M_S.resize(alphabet_SO+1, 0);
            M_S.shrink_to_fit();

            for (it = triple_begin, i=0; i<n; i++, it++)
                M_S[std::get<0>(*it)]++;

            // Sorts the triples lexycographically
            sort(triple_begin, triple_end);

            // First O
            {
                uint64_t c;
                vector<uint64_t> new_C_O;
                uint64_t cur_pos = 1;
                new_C_O.push_back(0); // Dummy value
                new_C_O.push_back(cur_pos);
                for (c = 2; c <= alphabet_SO; c++) {
                    cur_pos += M_S[c-1];
                    new_C_O.push_back(cur_pos);
                }
                new_C_O.push_back(n+1);
                new_C_O.shrink_to_fit();

                M_S.clear();
                M_S.shrink_to_fit();

                int_vector<> new_O(n+1);
                new_O[0] = 0;
                for (i=1; i<=n; i++)
                    new_O[i] = std::get<2>(D[i-1]);

                sdsl::util::bit_compress(new_O);
                // builds the WT for BWT(O)
                m_bwt_o = bwt_type(new_O, new_C_O);
            }

            M_O.resize(alphabet_SO+1, 0);
            M_O.shrink_to_fit();

            for (it = triple_begin, i=0; i<n; i++, it++)
                M_O[std::get<2>(*it)]++;

            stable_sort(D.begin(), D.end(), [](const spo_triple& a,
                    const spo_triple& b) {return std::get<2>(a) < std::get<2>(b);});
            {
                uint64_t c;
                vector<uint64_t> new_C_P;

                uint64_t cur_pos = 1;
                new_C_P.push_back(0);  // Dummy value
                new_C_P.push_back(cur_pos);
                for (c = 2; c <= alphabet_SO; c++) {
                    cur_pos += M_O[c-1];
                    new_C_P.push_back(cur_pos);
                }
                new_C_P.push_back(n+1);
                new_C_P.shrink_to_fit();

                M_O.clear();

                int_vector<> new_P(n+1);
                new_P[0] = 0;
                for (i=1; i<=n; i++)
                    new_P[i] = std::get<1>(D[i-1]);

                sdsl::util::bit_compress(new_P);
                m_bwt_p = bwt_p_type(new_P, new_C_P);
            }

            M_P.resize(m_max_p+1, 0);
            M_P.shrink_to_fit();

            for (it = triple_begin, i=0; i<n; i++, it++)
                M_P[std::get<1>(*it)]++;

            stable_sort(D.begin(), D.end(), [](const spo_triple& a,
                    const spo_triple& b) {return std::get<1>(a) < std::get<1>(b); });
            // Builds BWT_S
            {
                uint64_t c;
                vector<uint64_t> new_C_S;

                uint64_t cur_pos = 1;
                new_C_S.push_back(0);  // Dummy value
                new_C_S.push_back(cur_pos);
                for (c = 2; c <= m_max_p; c++) {
                    cur_pos += M_P[c-1];
                    new_C_S.push_back(cur_pos);
                }
                new_C_S.push_back(n+1);
                new_C_S.shrink_to_fit();

                M_P.clear();

                int_vector<> new_S(n+1);
                new_S[0] = 0;
                for (i=1; i<=n; i++)
                    new_S[i] = std::get<0>(D[i-1]);
                sdsl::util::bit_compress(new_S);

                m_bwt_s = bwt_type(new_S, new_C_S);
            }

            cout << "-- BWTs built successfully" << endl;

            {
                m_bvts.resize(label2nodes.size());
                m_cnt_labels.resize(label2nodes.size());
                m_succs0.resize(label2nodes.size());
                m_succs1.resize(label2nodes.size());
                for (i = 0; i < label2nodes.size(); i++) {
                    sdsl::bit_vector bvt(alphabet_SO + 2, 0);
                    m_cnt_labels[i] = label2nodes[i].size();
                    for (uint32_t j = 0; j < label2nodes[i].size(); j++) {
                        bvt[label2nodes[i][j]] = 1;
                    }
                    bvt[alphabet_SO+1] = 1; // sentinel
                    m_bvts[i] = bit_vector_type(bvt);
                    sdsl::util::init_support(m_succs0[i], &m_bvts[i]);
                    sdsl::util::init_support(m_succs1[i], &m_bvts[i]);
                }
            }

            {
                std::set<std::string> strings_in_prop;
                for (i = 0; i < node_properties.size(); i++) {
                    if (!nprop_numeric[i]) {
                        for (auto &p : node_properties[i]) {
                            strings_in_prop.insert(p.second);
                        }
                    }
                }

                for (i = 0; i < edge_properties.size(); i++) {
                    if (!eprop_numeric[i]) {
                        for (auto &p : edge_properties[i]) {
                            strings_in_prop.insert(p.second);
                        }
                    }
                }

                m_string_mgr = string_mgr(strings_in_prop);

                for (i = 0; i < node_properties.size(); i++) {
                    if (!nprop_numeric[i]) {
                        for (auto &p : node_properties[i]) {
                            p.second = std::to_string(m_string_mgr.find(p.second));
                        }
                    }
                }

                for (i = 0; i < edge_properties.size(); i++) {
                    if (!eprop_numeric[i]) {
                        for (auto &p : edge_properties[i]) {
                            p.second = std::to_string(m_string_mgr.find(p.second));
                        }
                    }
                }
            }


            {


                m_node_properties.resize(node_properties.size());
                for (i = 0; i < node_properties.size(); i++) {
                    m_node_properties[i] = property_grid_v2<>(node_properties[i], m_max_s);
                }

                m_edge_properties.resize(edge_properties.size());
                for (i = 0; i < edge_properties.size(); i++) {
                    m_edge_properties[i] = property_grid_v2<>(edge_properties[i], m_n_triples);
                }

            }

            cout << "-- Grids built successfully" << endl;

            cout << "-- Index constructed successfully" << endl;
        };


        //! Copy constructor
        ring_pg(const ring_pg &o) {
            copy(o);
        }

        //! Move constructor
        ring_pg(ring_pg &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ring_pg &operator=(const ring_pg &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ring_pg &operator=(ring_pg &&o) {
            if (this != &o) {
                m_bwt_s = std::move(o.m_bwt_s);
                m_bwt_p = std::move(o.m_bwt_p);
                m_bwt_o = std::move(o.m_bwt_o);
                m_max_s = o.m_max_s;
                m_max_p = o.m_max_p;
                m_max_o = o.m_max_o;
                m_n_triples = o.m_n_triples;
                m_bvts = std::move(o.m_bvts);
                m_cnt_labels = std::move(o.m_cnt_labels);
                m_succs0 = std::move(o.m_succs0);
                m_succs1 = std::move(o.m_succs1);
                for (size_t i = 0; i < m_bvts.size(); i++) {
                    m_succs0[i].set_vector(&m_bvts[i]);
                    m_succs1[i].set_vector(&m_bvts[i]);
                }
                m_node_properties = std::move(o.m_node_properties);
                m_edge_properties = std::move(o.m_edge_properties);
                m_string_mgr = std::move(o.m_string_mgr);

            }
            return *this;
        }

        void swap(ring_pg &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_bwt_s, o.m_bwt_s);
            std::swap(m_bwt_p, o.m_bwt_p);
            std::swap(m_bwt_o, o.m_bwt_o);
            std::swap(m_max_s, o.m_max_s);
            std::swap(m_max_p, o.m_max_p);
            std::swap(m_max_o, o.m_max_o);
            std::swap(m_n_triples, o.m_n_triples);
            std::swap(m_bvts, o.m_bvts);
            std::swap(m_cnt_labels, o.m_cnt_labels);
            for (size_t i = 0; i < m_bvts.size(); i++) {
                sdsl::util::swap_support(m_succs0[i], o.m_succs0[i], &m_bvts[i], &o.m_bvts[i]);
                sdsl::util::swap_support(m_succs1[i], o.m_succs1[i], &m_bvts[i], &o.m_bvts[i]);
            }
            m_node_properties.swap(o.m_node_properties);
            m_edge_properties.swap(o.m_edge_properties);
            m_string_mgr.swap(o.m_string_mgr);
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_bwt_s.serialize(out, child, "bwt_s");
            written_bytes += m_bwt_p.serialize(out, child, "bwt_p");
            written_bytes += m_bwt_o.serialize(out, child, "bwt_o");
            written_bytes += sdsl::write_member(m_max_s, out, child, "max_s");
            written_bytes += sdsl::write_member(m_max_p, out, child, "max_p");
            written_bytes += sdsl::write_member(m_max_o, out, child, "max_o");
            written_bytes += sdsl::write_member(m_n_triples, out, child, "n_triples");
            sdsl::write_member(m_bvts.size(), out,  child, "labels");
            written_bytes += sdsl::serialize_vector(m_bvts, out, child, "bvts");
            written_bytes += sdsl::serialize_vector(m_cnt_labels, out, child, "cnt_labels");
            written_bytes += sdsl::serialize_vector(m_succs0, out, child, "succs0");
            written_bytes += sdsl::serialize_vector(m_succs1, out, child, "succs1");
            sdsl::write_member(m_node_properties.size(), out,  child, "node_prop_size");
            written_bytes += sdsl::serialize_vector(m_node_properties, out, child, "node_properties");
            sdsl::write_member(m_edge_properties.size(), out,  child, "edge_prop_size");
            written_bytes += sdsl::serialize_vector(m_edge_properties, out, child, "edge_properties");
            written_bytes += sdsl::serialize(m_string_mgr, out, child, "string_mgr");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_bwt_s.load(in);
            m_bwt_p.load(in);
            m_bwt_o.load(in);
            sdsl::read_member(m_max_s, in);
            sdsl::read_member(m_max_p, in);
            sdsl::read_member(m_max_o, in);
            sdsl::read_member(m_n_triples, in);
            size_t labels_size;
            sdsl::read_member(labels_size, in);
            m_bvts.resize(labels_size);
            m_cnt_labels.resize(labels_size);
            m_succs0.resize(labels_size);
            m_succs1.resize(labels_size);
            sdsl::load_vector(m_bvts, in);
            sdsl::load_vector(m_cnt_labels, in);
            sdsl::load_vector(m_succs0, in);
            sdsl::load_vector(m_succs1, in);
            for (size_t i = 0; i < m_bvts.size(); i++) {
                m_succs0[i].set_vector(&m_bvts[i]);
                m_succs1[i].set_vector(&m_bvts[i]);
            }
            size_t node_prop_size;
            sdsl::read_member(node_prop_size, in);
            m_node_properties.resize(node_prop_size);
            sdsl::load_vector(m_node_properties, in);
            size_t edge_prop_size;
            sdsl::read_member(edge_prop_size, in);
            m_edge_properties.resize(edge_prop_size);
            sdsl::load_vector(m_edge_properties, in);
            sdsl::load(m_string_mgr, in);


            std::cout << "--- SPO ---" << std::endl;
            m_bwt_o.print_size();
            std::cout << "--- OSP ---" << std::endl;
            m_bwt_p.print_size();
            std::cout << "--- POS ---" << std::endl;
            m_bwt_s.print_size();
        }


        //Given a Suffix returns its range in BWT O
        pair<uint64_t, uint64_t> init_S(uint64_t S) const {
            return m_bwt_o.backward_search_1_interval(S);
        }

        //Given a Predicate returns its range in BWT S
        pair<uint64_t, uint64_t> init_P(uint64_t P) const {
            return m_bwt_s.backward_search_1_interval(P);
            //return {I.first + m_n_triples, I.second + m_n_triples};
        }

        //Given an Object returns its range in BWT P
        pair<uint64_t, uint64_t> init_O(uint64_t O) const {
            return m_bwt_p.backward_search_1_interval(O);
            //return {I.first + 2 * m_n_triples, I.second + 2 * m_n_triples};
        }


        //POS m_bwt_s
        //OSP m_bwt_p
        //SPO m_bwt_o

        //POS -> SPO
        pair<uint64_t, uint64_t> init_SP(uint64_t S, uint64_t P) const {
            auto I = m_bwt_s.backward_search_1_rank(P, S); //POS
            return m_bwt_o.backward_search_2_interval(S, I); //SPO
        }

        //SPO -> OSP
        pair<uint64_t, uint64_t> init_SO(uint64_t S, uint64_t O) const {
            auto I = m_bwt_o.backward_search_1_rank(S, O); //SPO
            return m_bwt_p.backward_search_2_interval(O, I); //OSP
           // return {I.first + 2 * m_n_triples, I.second + 2 * m_n_triples};
        }


        //OSP -> POS
        pair<uint64_t, uint64_t> init_PO(uint64_t P, uint64_t O) const {
            auto I = m_bwt_p.backward_search_1_rank(O, P); //OSP
            return m_bwt_s.backward_search_2_interval(P, I); //POS
            //return {I.first + m_n_triples, I.second + m_n_triples};
        }

        //OSP -> POS -> SPO
        pair<uint64_t, uint64_t> init_SPO(uint64_t S, uint64_t P, uint64_t O) const {
            auto I = m_bwt_p.backward_search_1_rank(O, P); //OSP
            I = m_bwt_s.backward_search_2_rank(P, S, I); //POS
            return m_bwt_o.backward_search_2_interval(S, I); //SPO
        }

        /**********************************/
        // Functions for PSO
        //

        bwt_interval open_PSO() {
            //return bwt_interval(2 * m_n_triples + 1, 3 * m_n_triples);
            return bwt_interval( 1, m_n_triples);
        }

        /**********************************/
        // P->S  (simulates going down in the trie)
        // Returns an interval within m_bwt_o
        bwt_interval down_P_S(bwt_interval &p_int, uint64_t s) {
            auto I = m_bwt_s.backward_step(p_int.left(), p_int.right(), s);
            uint64_t c = m_bwt_o.get_C(s);
            return bwt_interval(I.first + c, I.second + c);
        }

        uint64_t min_O_in_S(bwt_interval &I) {
            return I.begin(m_bwt_o);
        }

        uint64_t next_O_in_S(bwt_interval &I, uint64_t O) {
            if (O > m_max_o) return 0;
            return I.next_value(O, m_bwt_o);
        }

        bool there_are_O_in_S(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        uint64_t min_O_in_PS(bwt_interval &I) {
            return I.begin(m_bwt_o);
        }

        uint64_t next_O_in_PS(bwt_interval &I, uint64_t O) {
            if (O > m_max_o) return 0;
            return I.next_value(O, m_bwt_o);
        }

        bool there_are_O_in_PS(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        std::vector<uint64_t>
        all_O_in_range(bwt_interval &I) {
            return m_bwt_o.values_in_range(I.left(), I.right());
        }

        /**********************************/
        // Functions for OPS
        //

        bwt_interval open_OPS() {
            return bwt_interval(1, m_n_triples);
        }


        /**********************************/
        // O->P  (simulates going down in the trie)
        // Returns an interval within m_bwt_s
        bwt_interval down_O_P(bwt_interval &o_int, uint64_t p) {
            auto I = m_bwt_p.backward_step(o_int.left(), o_int.right(), p);
            uint64_t c = m_bwt_s.get_C(p);
            return bwt_interval(I.first + c, I.second + c);
        }

        uint64_t min_S_in_OP(bwt_interval &I) {
            return I.begin(m_bwt_s);
        }

        uint64_t next_S_in_OP(bwt_interval &I, uint64_t s_value) {
            if (s_value > m_max_s) return 0;
            return I.next_value(s_value, m_bwt_s);
        }

        bool there_are_S_in_OP(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        uint64_t min_S_in_P(bwt_interval &I) {
            return I.begin(m_bwt_s);
        }

        uint64_t next_S_in_P(bwt_interval &I, uint64_t s_value) {
            if (s_value > m_max_s) return 0;
            return I.next_value(s_value, m_bwt_s);
        }

        bool there_are_S_in_P(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        std::vector<uint64_t>
        all_S_in_range(bwt_interval &I) {
            return m_bwt_s.values_in_range(I.left(), I.right());
        }


        /**********************************/
        // Function for SOP
        //

        bwt_interval open_SOP() {
            return bwt_interval(1,  m_n_triples);
        }



        /**********************************/
        // S->O  (simulates going down in the trie)
        // Returns an interval within m_bwt_p
        bwt_interval down_S_O(bwt_interval &s_int, uint64_t o) {
            pair<uint64_t, uint64_t> I = m_bwt_o.backward_step(s_int.left(), s_int.right(), o);
            uint64_t c = m_bwt_p.get_C(o);
            return bwt_interval(I.first + c, I.second + c);
        }

        uint64_t min_P_in_SO(bwt_interval &I) {
            return I.begin(m_bwt_p);
        }

        uint64_t next_P_in_SO(bwt_interval &I, uint64_t p_value) {
            if (p_value > m_max_p) return 0;
            return I.next_value(p_value, m_bwt_p);
        }

        bool there_are_P_in_SO(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        uint64_t min_P_in_O(bwt_interval &I) {
            return I.begin(m_bwt_p);
        }

        uint64_t next_P_in_O(bwt_interval &I, uint64_t p_value) {
            if (p_value > m_max_p) return 0;
            return I.next_value(p_value, m_bwt_p);
        }

        bool there_are_P_in_O(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        std::vector<uint64_t>
        all_P_in_range(bwt_interval &I) {
            return m_bwt_p.values_in_range(I.left(), I.right());
        }




        /**********************************/
        // Functions for SPO
        //
        bwt_interval open_SPO() {
            return bwt_interval(1, m_n_triples);
        }

        uint64_t min_S(bwt_interval &I) {
            return I.begin(m_bwt_s);
        }

        uint64_t next_S(bwt_interval &I, uint64_t s_value) {
            if (s_value > m_max_s) return 0;

            return I.next_value(s_value, m_bwt_s);
        }

        bwt_interval down_S(uint64_t s_value) {
            pair<uint64_t, uint64_t> i = init_S(s_value);
            return bwt_interval(i.first, i.second);
        }


        // S->P  (simulates going down in the trie, for the order SPO)
        // Returns an interval within m_bwt_p
        bwt_interval down_S_P(bwt_interval &s_int, uint64_t s_value, uint64_t p_value) {
            std::pair<uint64_t, uint64_t> q = s_int.get_stored_values();
            uint64_t b = q.first;
            if (q.first == (uint64_t) -1) {
                q = m_bwt_s.select_next(p_value, s_value, m_bwt_o.nElems(s_value));
                b = m_bwt_s.bsearch_C(q.first) - 1;
            }
            uint64_t nE = m_bwt_s.rank(b + 1, s_value) - m_bwt_s.rank(b, s_value);
            uint64_t start = q.second;

            return bwt_interval(s_int.left() + start, s_int.left() + start + nE - 1);
        }

        uint64_t min_P_in_S(bwt_interval &I, uint64_t s_value) {
            std::pair<uint64_t, uint64_t> q;
            q = m_bwt_s.select_next(1, s_value, m_bwt_o.nElems(s_value));
            uint64_t b = m_bwt_s.bsearch_C(q.first) - 1;
            I.set_stored_values(b, q.second);
            return b;
        };


        uint64_t next_P_in_S(bwt_interval &I, uint64_t s_value, uint64_t p_value) {
            if (p_value > m_max_p) return 0;

            std::pair<uint64_t, uint64_t> q;
            q = m_bwt_s.select_next(p_value, s_value, m_bwt_o.nElems(s_value));
            if (q.first == 0 && q.second == 0) {
                return 0;
            }

            uint64_t b = m_bwt_s.bsearch_C(q.first) - 1;
            I.set_stored_values(b, q.second);
            return b;
        };

        uint64_t min_O_in_SP(bwt_interval &I) {
            return I.begin(m_bwt_o);
        }

        uint64_t next_O_in_SP(bwt_interval &I, uint64_t O) {
            if (O > m_max_o) return 0;
            return I.next_value(O, m_bwt_o);
        }

        bool there_are_O_in_SP(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        /**********************************/
        // Functions for POS
        //

        bwt_interval open_POS() {
            return bwt_interval( 1, m_n_triples);
        }

        uint64_t min_P(bwt_interval &I) {
            //bwt_interval I_aux(I.left() - 2 * m_n_triples, I.right() - 2 * m_n_triples);
            //return I_aux.begin(m_bwt_p);
            return I.begin(m_bwt_p);
        }

        uint64_t next_P(bwt_interval &I, uint64_t p_value) {
            if (p_value > m_max_p) return 0;

            //bwt_interval I_aux(I.left() - 2 * m_n_triples, I.right() - 2 * m_n_triples);
            //uint64_t nextv = I_aux.next_value(p_value, m_bwt_p);
            return I.next_value(p_value, m_bwt_p);
        }

        bwt_interval down_P(uint64_t p_value) {
            pair<uint64_t, uint64_t> i = init_P(p_value);
            return bwt_interval(i.first, i.second);
        }

        // P->O  (simulates going down in the trie, for the order POS)
        // Returns an interval within m_bwt_p
        bwt_interval down_P_O(bwt_interval &p_int, uint64_t p_value, uint64_t o_value) {
            std::pair<uint64_t, uint64_t> q = p_int.get_stored_values();
            uint64_t b = q.first;
            if (q.first == (uint64_t) -1) {
                q = m_bwt_p.select_next(o_value, p_value, m_bwt_s.nElems(p_value));
                b = m_bwt_p.bsearch_C(q.first) - 1;
            }
            uint64_t nE = m_bwt_p.rank(b + 1, p_value) - m_bwt_p.rank(b, p_value);
            uint64_t start = q.second;

            return bwt_interval(p_int.left() + start, p_int.left() + start + nE - 1);
        }

        uint64_t min_O_in_P(bwt_interval &p_int, uint64_t p_value) {
            std::pair<uint64_t, uint64_t> q;
            q = m_bwt_p.select_next(1, p_value, m_bwt_s.nElems(p_value));
            uint64_t b = m_bwt_p.bsearch_C(q.first) - 1;
            p_int.set_stored_values(b, q.second);
            return b;
        }

        uint64_t next_O_in_P(bwt_interval &I, uint64_t p_value, uint64_t o_value) {
            if (o_value > m_max_o) return 0;

            std::pair<uint64_t, uint64_t> q;
            q = m_bwt_p.select_next(o_value, p_value, m_bwt_s.nElems(p_value));
            if (q.first == 0 && q.second == 0)
                return 0;

            uint64_t b = m_bwt_p.bsearch_C(q.first) - 1;
            I.set_stored_values(b, q.second);
            return b;
        }

        uint64_t min_S_in_PO(bwt_interval &I) {
           // bwt_interval I_aux = bwt_interval(I.left() - m_n_triples, I.right() - m_n_triples);
            //return I_aux.begin(m_bwt_s);
            return I.begin(m_bwt_s);
        }

        uint64_t next_S_in_PO(bwt_interval &I, uint64_t s_value) {
            if (s_value > m_max_s) return 0;
            return I.next_value(s_value, m_bwt_s);
        }

        bool there_are_S_in_PO(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

        /**********************************/
        // Functions for OSP
        //

        bwt_interval open_OSP() {
            return bwt_interval(1, m_n_triples);
        }

        uint64_t min_O(bwt_interval &I) {
            return I.begin(m_bwt_o);
        }

        uint64_t next_O(bwt_interval &I, uint64_t o_value) {
            if (o_value > m_max_o) return 0;

            uint64_t nextv = I.next_value(o_value, m_bwt_o);
            if (nextv == 0) return 0;
            else return nextv;
        }

        bwt_interval down_O(uint64_t o_value) {
            pair<uint64_t, uint64_t> i = init_O(o_value);
            return bwt_interval(i.first, i.second);
        }

        // P->O  (simulates going down in the trie, for the order OSP)
        // Returns an interval within m_bwt_p
        bwt_interval down_O_S(bwt_interval &o_int, uint64_t o_value, uint64_t s_value) {
            std::pair<uint64_t, uint64_t> q = o_int.get_stored_values();
            uint64_t b = q.first;
            if (q.first == (uint64_t) -1) {
                q = m_bwt_o.select_next(s_value, o_value, m_bwt_p.nElems(o_value));
                b = m_bwt_o.bsearch_C(q.first) - 1;
            }
            uint64_t nE = m_bwt_o.rank(b + 1, o_value) - m_bwt_o.rank(b, o_value);
            uint64_t start = q.second;

            return bwt_interval(o_int.left() + start, o_int.left() + start + nE - 1);
        }

        uint64_t min_S_in_O(bwt_interval &o_int, uint64_t o_value) {
            std::pair<uint64_t, uint64_t> q;
            q = m_bwt_o.select_next(1, o_value, m_bwt_p.nElems(o_value));
            uint64_t b = m_bwt_o.bsearch_C(q.first) - 1;
            o_int.set_stored_values(b, q.second);
            return b;
        };

        uint64_t next_S_in_O(bwt_interval &I, uint64_t o_value, uint64_t s_value) {
            if (s_value > m_max_s) return 0;

            std::pair<uint64_t, uint64_t> q;
            q = m_bwt_o.select_next(s_value, o_value, m_bwt_p.nElems(o_value));
            if (q.first == 0 && q.second == 0)
                return 0;

            uint64_t b = m_bwt_o.bsearch_C(q.first) - 1;
            I.set_stored_values(b, q.second);
            return b;
        };

        uint64_t min_P_in_OS(bwt_interval &I) {
            return I.begin(m_bwt_p);
        }

        uint64_t next_P_in_OS(bwt_interval &I, uint64_t p_value) {
            if (p_value > m_max_p) return 0;
            return I.next_value(p_value, m_bwt_p);
        }

        bool there_are_P_in_OS(bwt_interval &I) {
            return I.get_cur_value() != I.end();
        }

       /************** Functions needed for Property Graphs ********************/


        value_type edge_expr_get_O(size_type pos_i) {
            auto r_s = m_bwt_s.inverse_select(pos_i);
            auto spo_i = m_bwt_o.get_C(r_s.second) + r_s.first;
            return m_bwt_o.get_value(spo_i);
        }

        value_type edge_expr_get_S(size_type pos_i) {
            return m_bwt_s.get_value(pos_i);
        }

        range_type edge_expr_down_P_S(range_type &s_int, uint64_t s) {
            pair<uint64_t, uint64_t> I = m_bwt_s.backward_step(s_int[0], s_int[1], s);
            uint64_t c = m_bwt_o.get_C(s);
            return range_type{I.first + c, I.second + c};
        }


        range_type edge_expr_down_S_O(range_type &s_int, uint64_t o) {
            pair<uint64_t, uint64_t> I = m_bwt_o.backward_step(s_int[0], s_int[1], o);
            uint64_t c = m_bwt_p.get_C(o);
            return range_type{I.first + c, I.second + c};
        }

        range_type edge_expr_down_O_P(range_type &o_int, uint64_t p) {
            pair<uint64_t, uint64_t> I = m_bwt_p.backward_step(o_int[0], o_int[1], p);
            uint64_t c = m_bwt_s.get_C(p);
            return range_type{I.first + c, I.second + c};
        }

        std::vector<uint64_t> edge_expr_all_P_in_range(range_type &r, std::vector<range_type> &sigma_ranges) {
            return m_bwt_p.values_in_range(r[0], r[1], sigma_ranges);
        }

        value_type edge_expr_min_S_in_P(std::vector<range_type> &ranges) {
            return m_bwt_s.range_min_value(ranges).first;
        }

        value_type edge_expr_min_O_in_P(std::vector<range_type> &ranges) {
            uint64_t pos = m_bwt_p.select_next(1, ranges);
            uint64_t b = m_bwt_p.bsearch_C(pos) - 1;
            return b;
        }

        value_type edge_expr_min_O_in_SP(std::vector<range_type> &ranges) {
            return m_bwt_o.range_min_value(ranges).first;
        }

        value_type edge_expr_min_E_in_SP(std::vector<range_type> &ranges, value_type current_s) {
            return m_bwt_s.select_next_ranges(ranges, current_s);
        }

        value_type min_E_in_SP(const bwt_interval &interval, value_type current_s) {
            std::vector<range_type> aux = {{interval.left(), interval.right()}};
            return m_bwt_s.select_next_ranges(aux, current_s);
        }

        value_type edge_expr_min_S_in_PO(std::vector<range_type> &ranges) {
            return m_bwt_s.range_min_value(ranges).first;
        }

        value_type edge_expr_next_S_in_P(std::vector<range_type> &ranges, uint64_t val) {
            return m_bwt_s.range_next_value(val, ranges).first;
        }

        value_type edge_expr_next_O_in_P(std::vector<range_type> &ranges, uint64_t val) {
            uint64_t pos = m_bwt_p.select_next(val, ranges);
            uint64_t b = m_bwt_p.bsearch_C(pos) - 1;
            return b;
        }

        value_type edge_expr_next_O_in_SP(std::vector<range_type> &ranges, uint64_t val) {
            return m_bwt_o.range_next_value(val, ranges).first;
        }

        value_type next_E_in_SP(const bwt_interval &interval, value_type current_s, value_type current_e) {
            if (current_e > m_n_triples) return 0;
            std::vector<range_type> aux;
            if (interval.right() < current_e) return 0;
            if (interval.left() < current_e) {
                aux.push_back({current_e, interval.right()});
            }else {
                aux.push_back({interval.left(), interval.right()});
            }
            return m_bwt_s.select_next_ranges(aux, current_s);
        }

        value_type edge_expr_next_E_in_SP(const std::vector<range_type> &ranges, value_type current_s, value_type current_e) {
            if (current_e > m_n_triples) return 0;
            std::vector<range_type> aux;
            for (const auto &r : ranges) {
                if (r[1] < current_e) continue;
                if (r[0] < current_e) {
                    aux.push_back({current_e, r[1]});
                }else {
                    aux.push_back({r[0], r[1]});
                }
            }
            return m_bwt_s.select_next_ranges(aux, current_s);
        }

        value_type edge_expr_next_S_in_PO(std::vector<range_type> &ranges, uint64_t val) {
            return m_bwt_s.range_next_value(val, ranges).first;
        }


        value_type min_E_in_OS(const bwt_interval &interval) {
            auto ranges = std::vector<range_type>{{interval.left(), interval.right()}};
            auto s_r = m_bwt_p.range_min_value(ranges);
            return m_bwt_s.get_C(s_r.first) + s_r.second;
        }

        value_type edge_expr_min_E_in_OS(std::vector<range_type> &ranges) {
            auto s_r = m_bwt_p.range_min_value(ranges);
            return m_bwt_s.get_C(s_r.first) + s_r.second;
        }

        value_type next_E_in_O(const bwt_interval &interval, uint64_t c) {
            if (c > m_n_triples) return 0;
            auto val_c = m_bwt_s.bsearch_C(c) - 1;
            auto rnk_c = c - m_bwt_s.get_C(val_c) + 1;
            auto j = m_bwt_p.select(rnk_c, val_c);
            if (interval.left() <= j && j <= interval.right()) {
                return c;
            }
            if (j > interval.right()) ++val_c; //must go to next predicate
            auto ranges = std::vector<range_type>{{interval.left(), interval.right()}};
            auto s_r = m_bwt_p.range_next_value(val_c, ranges);
            return m_bwt_s.get_C(s_r.first) + s_r.second;
        }
        //TODO: checking this
        value_type next_E_in_OS(const bwt_interval &interval, uint64_t c) {
            return next_E_in_O(interval, c);
        }

        value_type edge_expr_next_E_in_OS(std::vector<range_type> &ranges, uint64_t c) {
            if (c > m_n_triples) return 0;
            auto val_c = m_bwt_s.bsearch_C(c) - 1;
            auto rnk_c = c - m_bwt_s.get_C(val_c) + 1;
            auto j = m_bwt_p.select(rnk_c, val_c);

            size_type r_i = 0;
            while (r_i < ranges.size() && j > ranges[r_i][1]) ++r_i; //first range that can cover j
            if (r_i < ranges.size()) {
                if (j >= ranges[r_i][0]) return c; //j is inside the range

                std::vector<range_type> aux;
                for (++r_i; r_i < ranges.size(); ++r_i) {
                    aux.emplace_back(ranges[r_i]); //all the rest of ranges
                }
                auto s_r = m_bwt_p.range_next_value(val_c, aux);
                return m_bwt_s.get_C(s_r.first) + s_r.second;
            }else {
                ++val_c;
                auto s_r = m_bwt_p.range_next_value(val_c, ranges);
                return m_bwt_s.get_C(s_r.first) + s_r.second;
            }
        }

        //SPO->OSP->POS
        size_type map_OSP_to_POS(const size_type osp_i) {
            auto r_s = m_bwt_p.inverse_select(osp_i);
            return m_bwt_s.get_C(r_s.second) + r_s.first; //OSP -> POS
        }

        size_type map_SPO_to_POS(const size_type spo_i, const value_type o) {
            auto osp_i = m_bwt_p.get_C(o) + m_bwt_o.ranky(spo_i, o);
            auto r_s = m_bwt_p.inverse_select(osp_i);
            return m_bwt_s.get_C(r_s.second) + r_s.first; //OSP -> POS
        }


        value_type next_node_label(uint32_t label, value_type node) {
            auto n = m_succs1[label-1](node);
            if (n > m_max_s) return 0;
            return n;
        }

        value_type next_node_neg_label(uint32_t label, value_type node) {
            auto n =  m_succs0[label-1](node);
            if (n > m_max_s) return 0;
            return n;
        }

        size_type node_label_cnt(uint32_t label) {
            return m_cnt_labels[label-1];
        }

        size_type node_neg_label_cnt(uint32_t label) {
            return m_max_s - m_cnt_labels[label-1];
        }

        std::pair<value_type, value_type> next_node_in_property(const value_type prop_id, const value_type node_id) {
            return m_node_properties[prop_id-1].next_exists(node_id);
        }

        std::pair<value_type, value_type> next_edge_in_property(const value_type prop_id, const value_type node_id) {
            return m_edge_properties[prop_id-1].next_exists(node_id);
        }

        std::pair<int32_t, int32_t> get_node_property_range(const value_type prop_id) {
            return {m_node_properties[prop_id-1].min_val, m_node_properties[prop_id-1].max_val};
        }

        std::pair<int32_t, int32_t> get_edge_property_range(const value_type prop_id) {
            return {m_edge_properties[prop_id-1].min_val, m_edge_properties[prop_id-1].max_val};
        }

        std::pair<value_type, value_type> next_node_property(const value_type prop_id, const value_type node_id, const value_type value,
                                     const query::enum_comp_where_type op) {
            std::cout << "Next node property called: prop_id=" << prop_id << " node_id=" << node_id << " value=" << value << " op=" << op << std::endl;
            switch (op) {
                case query::EQ:
                    return m_node_properties[prop_id-1].next_eq(node_id, value);
                case query::ST:
                    //if (value == 1) return {0,0};
                    return m_node_properties[prop_id-1].next_se(node_id, value-1);
                case query::SE:
                    return m_node_properties[prop_id-1].next_se(node_id, value);
                case query::GT:
                    //if (value+1 > m_max_s) return {0,0};
                    return m_node_properties[prop_id-1].next_ge(node_id, value+1);
                case query::GE:
                    return m_node_properties[prop_id-1].next_ge(node_id, value);
                case query::NEQ:
                    return m_node_properties[prop_id-1].next_not_eq(node_id, value);
                default:
                    throw std::runtime_error("Unsupported operator in property graph queries");
            }
         }
 
         std::pair<value_type, value_type> next_edge_property(const value_type prop_id, const value_type node_id, const value_type value,
                                      const query::enum_comp_where_type op) {
            switch (op) {
                case query::EQ:
                    return m_edge_properties[prop_id-1].next_eq(node_id, value);
                case query::ST:
                    //if (value == 1) return {0,0};
                    return m_edge_properties[prop_id-1].next_se(node_id, value-1);
                case query::SE:
                    return m_edge_properties[prop_id-1].next_se(node_id, value);
                case query::GT:
                    //if (value+1 > m_max_p) return {0,0};
                    return m_edge_properties[prop_id-1].next_ge(node_id, value+1);
                case query::GE:
                    return m_edge_properties[prop_id-1].next_ge(node_id, value);
                case query::NEQ:
                    return m_edge_properties[prop_id-1].next_not_eq(node_id, value);
                default:
                    throw std::runtime_error("Unsupported operator in property graph queries");
            }
         }

        value_type get_node_property_value(const value_type prop_id, const value_type node_id) {
            return m_node_properties[prop_id-1][node_id];
        };

        value_type get_edge_property_value(const value_type prop_id, const value_type edge_id) {
            return m_edge_properties[prop_id-1][edge_id];
        };

        value_type get_string_id(const std::string &s, query::enum_comp_where_type op) {
            return m_string_mgr.get_id(s, op);
        }
    };

    typedef ring_pg<bwt_rrr, bwt_rrr> c_ring_pg;

}

#endif
