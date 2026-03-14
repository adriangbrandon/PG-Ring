//
// Created by adrian on 10/3/26.
// Lexicographic TSV Parser - Optimized for minimum memory usage
//

#ifndef TSV_LEX_PARSER_HPP
#define TSV_LEX_PARSER_HPP

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include "tsv_helper.hpp"

/**
 * @brief TSV parser that creates dictionaries with lexicographic IDs
 *
 * Memory-optimized: uses std::map for automatic sorting, deduplication, and ID storage.
 * IDs are assigned in lexicographic order: first string alphabetically gets ID 1.
 *
 * Strategy (same as tsv_parser):
 * 1. Process NODES first: collect, assign IDs, write dictionaries and mappings
 * 2. Clear node-specific data to free memory
 * 3. Process EDGES: collect, assign IDs, write dictionaries and mappings
 */
class tsv_lex_parser {

private:
    // Maps maintain lexicographic order AND store IDs
    std::map<std::string, uint32_t> m_nodes;
    std::map<std::string, uint32_t> m_nlabels;
    std::map<std::string, uint32_t> m_elabels;
    std::map<std::string, uint32_t> m_nprops;
    std::map<std::string, uint32_t> m_eprops;

    // Triples: (subject, predicate, object, properties)
    typedef std::tuple<int, int, int, std::vector<tsv_helper::property_tsv_type>> triple_type;
    std::vector<triple_type> m_triples;

    // Label to nodes mapping (sorted by label id)
    typedef std::map<uint32_t, std::vector<uint32_t>> label_nodes_map_type;
    label_nodes_map_type m_label_nodes_map;

    // Property values (sorted by property id)
    typedef std::vector<std::map<uint32_t, std::string>> properties_values_type;
    properties_values_type m_properties_node_values;
    properties_values_type m_properties_edge_values;

    // File paths
    std::string m_node_file;
    std::string m_edge_file;

    /**
     * @brief Pass 1: Collect unique strings from nodes
     */
    void collect_node_strings() {
        std::cout << "Pass 1: Collecting node strings..." << std::endl;

        // Clear node-related maps
        m_nodes.clear();
        m_nlabels.clear();
        m_nprops.clear();

        std::ifstream file(m_node_file);
        std::string line;
        size_t pos = 0;

        while (std::getline(file, line)) {
            auto node = tsv_helper::parse_node(line);
            if (pos % 10000 == 0) {
                std::cout << "  Nodes: " << pos << "\r";
                std::cout.flush();
            }

            m_nodes[node.variable] = 0;  // Insert with temp ID
            for (const auto& label : node.labels) {
                m_nlabels[label] = 0;
            }
            for (const auto& prop : node.properties) {
                m_nprops[prop.key] = 0;
            }
            ++pos;
        }
        std::cout << "  Nodes: " << pos << " (unique: " << m_nodes.size() << ")" << std::endl;
        std::cout << "  Unique node labels: " << m_nlabels.size() << std::endl;
        std::cout << "  Unique node properties: " << m_nprops.size() << std::endl;
    }

    /**
     * @brief Pass 2: Assign lexicographic IDs to node-related maps
     */
    void assign_node_ids() {
        std::cout << "Pass 2: Assigning node IDs..." << std::endl;

        assign_ids_to_map(m_nodes);
        assign_ids_to_map(m_nlabels);
        assign_ids_to_map(m_nprops);

        std::cout << "  ✓ Node IDs assigned" << std::endl;
    }

    /**
     * @brief Helper: assign sequential IDs to map (map already sorted)
     */
    void assign_ids_to_map(std::map<std::string, uint32_t>& map) {
        uint32_t id = 1;
        for (auto& [str, value] : map) {
            value = id++;
        }
    }

    /**
     * @brief Write dictionary file from map
     */
    void write_dict(const std::string& filename, const std::map<std::string, uint32_t>& dict) {
        std::ofstream file(filename);
        for (const auto& [str, id] : dict) {
            file << id << " " << str << "\n";
        }
    }

    /**
     * @brief Pass 3: Write node dictionaries
     */
    void write_node_dictionaries(const std::string& output_prefix) {
        std::cout << "Pass 3: Writing node dictionaries..." << std::endl;

        write_dict(output_prefix + ".nodes.dict", m_nodes);
        std::cout << "  ✓ nodes.dict (" << m_nodes.size() << " entries)" << std::endl;

        write_dict(output_prefix + ".nlabels.dict", m_nlabels);
        std::cout << "  ✓ nlabels.dict (" << m_nlabels.size() << " entries)" << std::endl;

        write_dict(output_prefix + ".nprops.dict", m_nprops);
        std::cout << "  ✓ nprops.dict (" << m_nprops.size() << " entries)" << std::endl;
    }

    /**
     * @brief Pass 3b: Re-parse nodes to write label2nodes and node properties
     */
    void write_node_mappings(const std::string& output_prefix) {
        std::cout << "  Writing node mappings..." << std::endl;

        std::ifstream file(m_node_file);
        std::string line;
        size_t pos = 0;

        while (std::getline(file, line)) {
            auto node = tsv_helper::parse_node(line);
            if (pos % 10000 == 0) {
                std::cout << "    Processing: " << pos << "\r";
                std::cout.flush();
            }

            auto node_it = m_nodes.find(node.variable);
            if (node_it == m_nodes.end()) continue;

            uint32_t node_id = node_it->second;

            // Collect label->nodes
            for (const auto& label : node.labels) {
                auto label_it = m_nlabels.find(label);
                if (label_it != m_nlabels.end()) {
                    uint32_t label_id = label_it->second;
                    m_label_nodes_map[label_id].push_back(node_id);
                }
            }

            // Collect node properties
            for (const auto& prop : node.properties) {
                auto prop_it = m_nprops.find(prop.key);
                if (prop_it != m_nprops.end()) {
                    uint32_t prop_id = prop_it->second;
                    if (m_properties_node_values.size() <= prop_id) {
                        m_properties_node_values.resize(prop_id + 1);
                    }
                    m_properties_node_values[prop_id][node_id] = prop.value;
                }
            }

            ++pos;
        }
        std::cout << "    Processing: " << pos << std::endl;

        // Write label2nodes
        {
            std::ofstream out(output_prefix + ".label2nodes");
            for (auto& [label_id, node_ids] : m_label_nodes_map) {
                std::sort(node_ids.begin(), node_ids.end());
                out << label_id << " " << node_ids.size();
                for (uint32_t node_id : node_ids) {
                    out << " " << node_id;
                }
                out << "\n";
            }
            std::cout << "  ✓ label2nodes" << std::endl;
        }

        // Write node properties
        for (uint32_t prop_id = 1; prop_id < m_properties_node_values.size(); ++prop_id) {
            std::string filename = output_prefix + ".nprop2values." + std::to_string(prop_id);
            std::ofstream out(filename);
            const auto& prop_map = m_properties_node_values[prop_id];
            for (const auto& [node_id, value] : prop_map) {
                out << node_id << " " << value << "\n";
            }
        }
        std::cout << "  ✓ nprop2values written" << std::endl;
    }

    /**
     * @brief Clear node-specific data to free memory
     */
    void clear_node_data() {
        std::cout << "  Clearing node-specific data from memory..." << std::endl;
        m_nlabels.clear();
        m_nprops.clear();
        m_label_nodes_map.clear();
        m_properties_node_values.clear();
        std::cout << "  ✓ Memory freed" << std::endl;
    }

    /**
     * @brief Pass 4: Collect unique strings from edges
     */
    void collect_edge_strings() {
        std::cout << "Pass 4: Collecting edge strings..." << std::endl;

        // Clear edge-related maps
        m_elabels.clear();
        m_eprops.clear();

        std::ifstream file(m_edge_file);
        std::string line;
        size_t pos = 0;

        while (std::getline(file, line)) {
            auto edge = tsv_helper::parse_edge(line);
            if (pos % 10000 == 0) {
                std::cout << "  Edges: " << pos << "\r";
                std::cout.flush();
            }

            m_elabels[edge.type] = 0;

            for (const auto& prop : edge.properties) {
                m_eprops[prop.key] = 0;
            }
            ++pos;
        }
        std::cout << "  Edges: " << pos << std::endl;
        std::cout << "  Unique nodes (total): " << m_nodes.size() << std::endl;
        std::cout << "  Unique edge labels: " << m_elabels.size() << std::endl;
        std::cout << "  Unique edge properties: " << m_eprops.size() << std::endl;
    }

    /**
     * @brief Pass 5: Assign lexicographic IDs to edge-related maps
     */
    void assign_edge_ids() {
        std::cout << "Pass 5: Assigning edge IDs..." << std::endl;

        // Re-assign node IDs (may have new nodes from edges)
        assign_ids_to_map(m_nodes);
        assign_ids_to_map(m_elabels);
        assign_ids_to_map(m_eprops);

        std::cout << "  ✓ Edge IDs assigned" << std::endl;
    }

    /**
     * @brief Pass 6: Write edge dictionaries
     */
    void write_edge_dictionaries(const std::string& output_prefix) {
        std::cout << "Pass 6: Writing edge dictionaries..." << std::endl;

        write_dict(output_prefix + ".elabels.dict", m_elabels);
        std::cout << "  ✓ elabels.dict (" << m_elabels.size() << " entries)" << std::endl;

        write_dict(output_prefix + ".eprops.dict", m_eprops);
        std::cout << "  ✓ eprops.dict (" << m_eprops.size() << " entries)" << std::endl;
    }

    /**
     * @brief Pass 7: Re-parse edges to write triples and edge properties
     */
    void write_edge_mappings(const std::string& output_prefix) {
        std::cout << "Pass 7: Writing edge mappings..." << std::endl;

        // Clear previous triples and edge properties
        m_triples.clear();
        m_properties_edge_values.clear();

        std::ifstream file(m_edge_file);
        std::string line;
        size_t pos = 0;

        while (std::getline(file, line)) {
            auto edge = tsv_helper::parse_edge(line);
            if (pos % 10000 == 0) {
                std::cout << "  Processing: " << pos << "\r";
                std::cout.flush();
            }

            auto from_it = m_nodes.find(edge.from);
            auto to_it = m_nodes.find(edge.to);
            auto type_it = m_elabels.find(edge.type);

            if (from_it == m_nodes.end() || to_it == m_nodes.end() ||
                type_it == m_elabels.end()) {
                ++pos;
                continue;
            }

            int subj_id = static_cast<int>(from_it->second);
            int pred_id = static_cast<int>(type_it->second);
            int obj_id = static_cast<int>(to_it->second);

            // Store triple
            m_triples.emplace_back(subj_id, pred_id, obj_id, edge.properties);

            ++pos;
        }
        std::cout << "  Processing: " << pos << std::endl;

        // Sort triples (same order as tsv_parser)
        std::cout << "  Sorting triples..." << std::flush;
        std::sort(m_triples.begin(), m_triples.end(), [](const triple_type& a, const triple_type& b) {
            if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
            if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) < std::get<2>(b);
            return std::get<0>(a) < std::get<0>(b);
        });
        std::cout << " done." << std::endl;

        // Write triples
        {
            std::ofstream triples_file(output_prefix + ".triples");
            for (const auto& triple : m_triples) {
                triples_file << std::get<0>(triple) << " "
                            << std::get<1>(triple) << " "
                            << std::get<2>(triple) << "\n";
            }
            std::cout << "  ✓ triples (" << m_triples.size() << " triples)" << std::endl;
        }

        // Collect and write edge properties
        std::cout << "  Setting properties of edges..." << std::flush;
        for (uint64_t i = 0; i < m_triples.size(); ++i) {
            for (const auto& prop : std::get<3>(m_triples[i])) {
                std::string key = prop.key;
                std::string value = prop.value;
                auto prop_it = m_eprops.find(key);
                if (prop_it != m_eprops.end()) {
                    uint32_t prop_id = prop_it->second;
                    if (m_properties_edge_values.size() <= prop_id) {
                        m_properties_edge_values.resize(prop_id + 1);
                    }
                    m_properties_edge_values[prop_id][i + 1] = value;
                }
            }
        }
        std::cout << " done." << std::endl;

        // Write edge properties
        for (uint32_t prop_id = 1; prop_id < m_properties_edge_values.size(); ++prop_id) {
            std::string filename = output_prefix + ".eprop2values." + std::to_string(prop_id);
            std::ofstream out(filename);
            const auto& prop_map = m_properties_edge_values[prop_id];
            for (const auto& [edge_id, value] : prop_map) {
                out << edge_id << " " << value << "\n";
            }
        }
        std::cout << "  ✓ eprop2values written" << std::endl;
    }

public:
    /**
     * @brief Parse TSV files and create dictionaries with lexicographic IDs
     * @param tsv_prefix Prefix for input files (e.g., "data/graph")
     * @param output_prefix Prefix for output files (e.g., "data/graph.lex")
     *
     * Reads: <tsv_prefix>-clean-nodes.tsv and <tsv_prefix>-clean-edges.tsv
     * Creates dictionaries where IDs follow lexicographic order (first alphabetically = ID 1)
     *
     * Process flow (same as tsv_parser):
     * 1. NODES: collect -> assign IDs -> write dicts + mappings -> clear memory
     * 2. EDGES: collect -> assign IDs -> write dicts + mappings
     */
    void parse(const std::string& tsv_prefix, const std::string& output_prefix) {
        m_node_file = tsv_prefix + "-clean-nodes.tsv";
        m_edge_file = tsv_prefix + "-clean-edges.tsv";

        // ========== PROCESS NODES ==========

        // Pass 1: Collect unique strings from nodes
        collect_node_strings();

        // Pass 2: Assign lexicographic IDs to node-related maps
        assign_node_ids();

        // Pass 3: Write node dictionaries and mappings
        write_node_dictionaries(output_prefix);
        write_node_mappings(output_prefix);

        // Clear node-specific data to free memory (like tsv_parser)
        clear_node_data();

        // ========== PROCESS EDGES ==========

        // Pass 4: Collect unique strings from edges
        collect_edge_strings();

        // Pass 5: Assign lexicographic IDs to edge-related maps
        assign_edge_ids();

        // Pass 6: Write edge dictionaries
        write_edge_dictionaries(output_prefix);

        // Pass 7: Write edge mappings (triples and properties)
        write_edge_mappings(output_prefix);

        std::cout << "\n✓ All dictionaries created successfully!" << std::endl;
    }

    /**
     * @brief Get statistics about collected data
     */
    void print_stats() const {
        std::cout << "\n=== Statistics ===" << std::endl;
        std::cout << "Unique nodes: " << m_nodes.size() << std::endl;
        std::cout << "Unique node labels: " << m_nlabels.size() << std::endl;
        std::cout << "Unique edge labels: " << m_elabels.size() << std::endl;
        std::cout << "Unique node properties: " << m_nprops.size() << std::endl;
        std::cout << "Unique edge properties: " << m_eprops.size() << std::endl;
        std::cout << "==================" << std::endl;
    }
};

#endif //TSV_LEX_PARSER_HPP

