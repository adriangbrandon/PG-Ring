//
// Created by adrian on 29/1/26.
//

#ifndef LABEL_NODES_HPP
#define LABEL_NODES_HPP
#include <cstdint>
#include <sdsl/sd_vector.hpp>
#include <succ_support_sd.hpp>

namespace ring {

    //pre: length at most 255
    template <uint64_t length = 64>
    class label_nodes {

    public:

        typedef uint64_t size_type;
        typedef uint64_t value_type;
        typedef sdsl::sd_vector<> bv_type;
        typedef sdsl::succ_support_sd<> succ_1_type;
        typedef typename bv_type::rank_1_type rank_1_type;
        typedef typename sdsl::int_vector<64> int_vector_type;
        typedef std::vector<size_type>::const_iterator const_iterator;
        typedef std::vector<size_type>::iterator iterator;

    private:


        size_type m_size;
        bv_type m_bv_nodes; //bitvector to know which nodes have a label
        succ_1_type m_succ_node; //successor structure over m_bv_nodes
        bv_type m_bv_runs;
        rank_1_type m_rank_runs;
        int_vector_type m_next_0; //to know the next zeroe


        void copy(const label_nodes &rhs) {
            m_size = rhs.m_size;
            m_bv_nodes = rhs.m_bv_nodes;
            m_succ_node = rhs.m_succ_node;
            m_bv_runs = rhs.m_bv_runs;
            m_rank_runs = rhs.m_rank_runs;
            m_next_0 = rhs.m_next_0;
            m_succ_node.set_vector(&m_bv_nodes);
            m_rank_runs.set_vector(&m_bv_runs);
        }


    public:

        const size_type &size = m_size;

        label_nodes() = default;

        label_nodes(iterator begin, iterator end) {
            m_bv_nodes = bv_type(begin, end);
            sdsl::util::init_support(m_succ_node, &m_bv_nodes);

            //compute starting positions of runs
            m_size = std::distance(begin, end)-1; //removing sentinel
            std::vector<size_type> beg_runs, next_0;
            auto p1 = begin;
            auto p2 = begin;
            ++p2;
            size_type beg = *p1, l = 1;
            while (p2 != end) {
                if (*p2 == *p1 + 1) {
                    ++l;
                }else {
                    if (l >= length) {
                        beg_runs.push_back(beg);
                        next_0.push_back(*p1 + 1);
                    }
                    l = 1;
                    beg = *p2;
                }
                ++p1; ++p2;
            }
            beg_runs.push_back(*p1); //sentinel
            m_bv_runs = bv_type(beg_runs.begin(), beg_runs.end());
            sdsl::util::init_support(m_rank_runs, &m_bv_runs);
            m_next_0.resize(m_bv_runs.size());
            for (size_type i = 0; i < next_0.size(); ++i) {
                m_next_0[i] = next_0[i];
            }
        }

        label_nodes(const label_nodes &rhs) {
            copy(rhs);
        }

        label_nodes(label_nodes &&rhs) {
            *this = std::move(rhs);
        }

        label_nodes &operator=(const label_nodes &rhs) {
            if (this != &rhs) {
                copy(rhs);
            }
            return *this;
        }

        label_nodes &operator=(label_nodes &&rhs) {
            if (this != &rhs) {
                m_size = rhs.m_size;
                m_bv_nodes = std::move(rhs.m_bv_nodes);
                m_succ_node = std::move(rhs.m_succ_node);
                m_succ_node.set_vector(&m_bv_nodes);
                m_bv_runs = std::move(rhs.m_bv_runs);
                m_rank_runs = std::move(rhs.m_rank_runs);
                m_rank_runs.set_vector(&m_bv_runs);
                m_next_0 = std::move(rhs.m_next_0);
            }
            return *this;
        }

        void swap(label_nodes &rhs) noexcept{
            std::swap(m_size, rhs.m_size);
            m_bv_nodes.swap(rhs.m_bv_nodes);
            sdsl::util::swap_support(m_succ_node, rhs.m_succ_node, &m_bv_nodes, &rhs.m_bv_nodes);
            m_bv_runs.swap(rhs.m_bv_runs);
            sdsl::util::swap_support(m_rank_runs, rhs.m_rank_runs, &m_bv_runs, &rhs.m_bv_runs);
            m_next_0.swap(rhs.m_next_0);
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_size, out, child, "nodes");
            written_bytes += m_bv_nodes.serialize(out, child, "bv_nodes");
            written_bytes += m_succ_node.serialize(out, child, "succ_node");
            written_bytes += m_bv_runs.serialize(out, child, "bv_runs");
            written_bytes += m_rank_runs.serialize(out, child, "rank_runs");
            written_bytes += m_next_0.serialize(out, child, "lengths");
            return written_bytes;
        }

        void load(std::istream &in) {
            sdsl::read_member(m_size, in);
            m_bv_nodes.load(in);
            m_succ_node.load(in, &m_bv_nodes);
            m_bv_runs.load(in);
            m_rank_runs.load(in, &m_bv_runs);
            m_next_0.load(in);
        }

        size_type next_node(size_type node_id) {
            if (node_id >= m_bv_nodes.size()) return 0;
            return  m_succ_node.succ(node_id);
        }

        size_type next_neg_node(size_type node_id) {
            if (node_id >= m_bv_nodes.size()) return 0;
            size_type aux = 0 , tries = 0;
            while (tries < length) {
                aux = m_succ_node(node_id);
                if (aux > node_id) return node_id;
                ++node_id;
                ++tries;
            }
            auto p = m_rank_runs(node_id); //which run
            return m_next_0[p-1]; //next zero after the run
        }

    };


}

#endif //LABEL_NODES_HPP
