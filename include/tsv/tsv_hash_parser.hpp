//
// Created by adrian on 10/3/26.
// Hash-based TSV Parser - IDs assigned by hash value (same as LibCSD)
//

#ifndef TSV_HASH_PARSER_HPP
#define TSV_HASH_PARSER_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include "tsv_helper.hpp"
#include "dict/string_dictionary.hpp"

/**
 * @brief TSV parser that creates dictionaries with hash-based IDs (LibCSD compatible)
 *
 * Uses LibCSD to generate correct hash-based IDs, then writes text-based .dict files.
 * IDs match LibCSD's HASHRPF dictionary behavior.
 *
 * Strategy:
 * 1. Process NODES first: collect, create LibCSD dict, transform to IDs, store dicts
 * 2. Clear node-specific data to free memory
 * 3. Process EDGES: collect, create LibCSD dict, transform to IDs, store dicts
 */
class tsv_hash_parser {

private:
    // Dictionaries
    ring::string_dictionary m_nodes;
    ring::string_dictionary m_nlabels;
    ring::string_dictionary m_elabels;
    ring::string_dictionary m_nprops;
    ring::string_dictionary m_eprops;

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
     * @brief Creates a temporary LibCSD dictionary
     * @param strings Vector of strings (will be consumed/moved)
     * @param out Output dictionary to populate with string->ID mappings
     */
    void create_dictionary(std::vector<std::string>& strings, ring::string_dictionary &out) {
        if (strings.empty()) return;

        // Create temporary LibCSD dictionary (uses HASHRPF with hash-based IDs)
        // This consumes the strings vector
        auto t0 = std::chrono::high_resolution_clock::now();
        out = ring::string_dictionary(std::move(strings),
                                          ring::string_dictionary::dict_type::HASHRPF,
                                          20, 32);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = t1 - t0;
        std::cout << "    Created LibCSD dictionary with " << out.size() << " entries in " << elapsed.count() << " seconds." << std::endl;
    }

    /**
     * @brief Pass 1: Collect unique strings from nodes
     */
    void collect_node_strings(std::vector<std::string>& nodes,
                              std::vector<std::string>& nlabels,
                              std::vector<std::string>& nprops) {
        std::cout << "Pass 1: Collecting node strings..." << std::endl;

        std::map<std::string, bool> seen_nodes, seen_nlabels, seen_nprops;

        std::ifstream file(m_node_file);
        std::string line;
        size_t pos = 0;

        while (std::getline(file, line)) {
            auto node = tsv_helper::parse_node(line);
            if (pos % 10000 == 0) {
                std::cout << "  Nodes: " << pos << "\r";
                std::cout.flush();
            }

            if (seen_nodes.find(node.variable) == seen_nodes.end()) {
                nodes.push_back(node.variable);
                seen_nodes[node.variable] = true;
            }

            for (const auto& label : node.labels) {
                if (seen_nlabels.find(label) == seen_nlabels.end()) {
                    nlabels.push_back(label);
                    seen_nlabels[label] = true;
                }
            }

            for (const auto& prop : node.properties) {
                if (seen_nprops.find(prop.key) == seen_nprops.end()) {
                    nprops.push_back(prop.key);
                    seen_nprops[prop.key] = true;
                }
            }
            ++pos;
        }
        std::cout << "  Nodes: " << pos << " (unique: " << nodes.size() << ")" << std::endl;
        std::cout << "  Unique node labels: " << nlabels.size() << std::endl;
        std::cout << "  Unique node properties: " << nprops.size() << std::endl;
    }

    /**
     * @brief Pass 2: Use LibCSD to assign hash-based IDs to node-related strings
     */
    void assign_node_ids(std::vector<std::string>& nodes,
                        std::vector<std::string>& nlabels,
                        std::vector<std::string>& nprops) {
        std::cout << "Pass 2: Using LibCSD to create the dictionaries of nodes..." << std::endl;

        create_dictionary(nodes, m_nodes);
        create_dictionary(nlabels, m_nlabels);
        create_dictionary(nprops, m_nprops);

        std::cout << "  ✓ Node IDs assigned (LibCSD hash-based)" << std::endl;
    }

    /**
     * @brief Write dictionary file from map
     */
    void write_dict(const std::string& filename, ring::string_dictionary &dict) {
        std::ofstream file(filename);
        for (uint32_t id = 1; id <= dict.size(); ++id) {
            std::string str = dict.extract(id);
            file << id << " " << str << "\n";
        }
    }

    /**
     * @brief Pass 3: Write node dictionaries
     */
    void write_node_dictionaries(const std::string& output_prefix) {
        std::cout << "Pass 3: Writing node dictionaries..." << std::endl;

        write_dict(output_prefix + ".nodes.dict", m_nodes);
        sdsl::store_to_file(m_nodes, output_prefix + ".nodes.csd");
        std::cout << "  ✓ nodes.dict (" << m_nodes.size() << " entries)" << std::endl;

        write_dict(output_prefix + ".nlabels.dict", m_nlabels);
        sdsl::store_to_file(m_nlabels, output_prefix + ".nlabels.csd");
        std::cout << "  ✓ nlabels.dict (" << m_nlabels.size() << " entries)" << std::endl;

        write_dict(output_prefix + ".nprops.dict", m_nprops);
        sdsl::store_to_file(m_nprops, output_prefix + ".nprops.csd");
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

            uint32_t node_id = m_nodes.locate(node.variable);

            // Collect label->nodes
            for (const auto& label : node.labels) {
                uint32_t label_id = m_nlabels.locate(label);
                m_label_nodes_map[label_id].push_back(node_id);
            }

            // Collect node properties
            for (const auto& prop : node.properties) {
                uint32_t prop_id = m_nprops.locate(prop.key);
                if (m_properties_node_values.size() <= prop_id) {
                    m_properties_node_values.resize(prop_id + 1);
                }
                m_properties_node_values[prop_id][node_id] = prop.value;
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
        sdsl::util::clear(m_nlabels);
        sdsl::util::clear(m_nprops);
        m_label_nodes_map.clear();
        m_properties_node_values.clear();
        std::cout << "  ✓ Memory freed" << std::endl;
    }

    /**
     * @brief Pass 4: Collect unique strings from edges
     */
    void collect_edge_strings(std::vector<std::string>& elabels,
                              std::vector<std::string>& eprops) {
        std::cout << "Pass 4: Collecting edge strings..." << std::endl;

        std::map<std::string, bool> seen_elabels, seen_eprops;

        std::ifstream file(m_edge_file);
        std::string line;
        size_t pos = 0;

        while (std::getline(file, line)) {
            auto edge = tsv_helper::parse_edge(line);
            if (pos % 10000 == 0) {
                std::cout << "  Edges: " << pos << "\r";
                std::cout.flush();
            }

            if (seen_elabels.find(edge.type) == seen_elabels.end()) {
                elabels.push_back(edge.type);
                seen_elabels[edge.type] = true;
            }

            for (const auto& prop : edge.properties) {
                if (seen_eprops.find(prop.key) == seen_eprops.end()) {
                    eprops.push_back(prop.key);
                    seen_eprops[prop.key] = true;
                }
            }
            ++pos;
        }
        std::cout << "  Edges: " << pos << std::endl;
        std::cout << "  Unique nodes (total): " << m_nodes.size() << std::endl;
        std::cout << "  Unique edge labels: " << elabels.size() << std::endl;
        std::cout << "  Unique edge properties: " << eprops.size() << std::endl;
    }

    /**
     * @brief Pass 5: Use LibCSD to create the dictionaries
     */
    void assign_edge_ids(std::vector<std::string>& elabels,
                        std::vector<std::string>& eprops) {
        std::cout << "Pass 5: Using LibCSD to create the dictionaries of edges..." << std::endl;

        create_dictionary(elabels, m_elabels);
        create_dictionary(eprops, m_eprops);

        std::cout << "  ✓ Edge IDs assigned (LibCSD hash-based)" << std::endl;
    }

    /**
     * @brief Pass 6: Write edge dictionaries
     */
    void write_edge_dictionaries(const std::string& output_prefix) {
        std::cout << "Pass 6: Writing edge dictionaries..." << std::endl;

        write_dict(output_prefix + ".elabels.dict", m_elabels);
        sdsl::store_to_file( m_elabels, output_prefix+".elabels.csd");
        std::cout << "  ✓ elabels.dict (" << m_elabels.size() << " entries)" << std::endl;

        write_dict(output_prefix + ".eprops.dict", m_eprops);
        sdsl::store_to_file( m_eprops, output_prefix + ".eprops.csd");
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

            auto from_id = m_nodes.locate(edge.from);
            auto to_id = m_nodes.locate(edge.to);
            auto type_id = m_elabels.locate(edge.type);
            if (!from_id || !to_id || !type_id) continue;

            // Store triple
            m_triples.emplace_back(from_id, type_id, to_id, edge.properties);

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
                uint32_t prop_id = m_eprops.locate(key);
                if (m_properties_edge_values.size() <= prop_id) {
                    m_properties_edge_values.resize(prop_id + 1);
                }
                m_properties_edge_values[prop_id][i + 1] = value;
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
     * @brief Parse TSV files and create dictionaries with hash-based IDs (LibCSD compatible)
     * @param tsv_prefix Prefix for input files (e.g., "data/graph")
     * @param output_prefix Prefix for output files (e.g., "data/graph")
     *
     * Reads: <tsv_prefix>-clean-nodes.tsv and <tsv_prefix>-clean-edges.tsv
     * Creates dictionaries where IDs are based on hash values (same as LibCSD)
     */
    void parse(const std::string& tsv_prefix, const std::string& output_prefix) {
        m_node_file = tsv_prefix + "-clean-nodes.tsv";
        m_edge_file = tsv_prefix + "-clean-edges.tsv";

        // ========== PROCESS NODES ==========

        std::vector<std::string> nodes, nlabels, nprops;

        // Pass 1: Collect unique strings from nodes
        collect_node_strings(nodes, nlabels, nprops);

        // Pass 2: Assign hash-based IDs to node-related strings
        assign_node_ids(nodes, nlabels, nprops);

        // Clear temporary vectors
        nodes.clear(); nlabels.clear(); nprops.clear();

        // Pass 3: Write node dictionaries and mappings
        write_node_dictionaries(output_prefix);
        write_node_mappings(output_prefix);

        // Clear node-specific data to free memory
        clear_node_data();

        // ========== PROCESS EDGES ==========

        std::vector<std::string> elabels, eprops;

        // Pass 4: Collect unique strings from edges
        collect_edge_strings(elabels, eprops);

        // Pass 5: Assign hash-based IDs to edge-related strings
        assign_edge_ids(elabels, eprops);

        // Clear temporary vectors
        elabels.clear(); eprops.clear();

        // Pass 6: Write edge dictionaries
        write_edge_dictionaries(output_prefix);

        // Pass 7: Write edge mappings (triples and properties)
        write_edge_mappings(output_prefix);

        std::cout << "\n✓ All dictionaries created successfully!" << std::endl;
    }

};

#endif //TSV_HASH_PARSER_HPP

