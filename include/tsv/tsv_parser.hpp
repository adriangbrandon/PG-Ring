//
// Created by adrian on 26/1/26.
//

#ifndef TSV_PARSER_HPP
#define TSV_PARSER_HPP


#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <set>
#include <unordered_map>
#include <sdsl/util.hpp>

#include "tsv_helper.hpp"

class tsv_parser {

private:
    // Diccionarios
    std::unordered_map<std::string, uint32_t> m_set_nodes;   // Nombres de nodos
    std::unordered_map<std::string, uint32_t> m_set_label_nodes;  // Labels de nodos
    std::unordered_map<std::string, uint32_t> m_set_label_edges;  // Labels de aristas
    std::unordered_map<std::string, uint32_t> m_properties_node;  // Properties de nodos
    std::unordered_map<std::string, uint32_t> m_properties_edge;  // Properties de aristas
    // Triples: (sujeto, predicado, objeto)
    typedef std::tuple<int, int, int,  std::vector<tsv_helper::property_tsv_type>> triple_type;
    std::vector<triple_type> m_triples;
    typedef std::map<uint32_t, std::vector<uint32_t>> label_nodes_map_type; //para que se mostren ordenados por id de label
    label_nodes_map_type m_label_nodes_map;
    typedef std::vector<std::map<uint32_t, std::string>> properties_values_type; //para que se mostren ordenados por id de label
    properties_values_type m_properties_node_values;
    properties_values_type m_properties_edge_values;

    int get_or_add(const std::string& name, std::unordered_map<std::string, uint32_t>& set) {
        auto it = set.find(name);
        if (it != set.end()) return it->second;
        int id = set.size()+1;
        set[name] = id;
        return id;
    };

    int get(const std::string& name, std::unordered_map<std::string, uint32_t>& set) {
        auto it = set.find(name);
        if (it != set.end()) return it->second;
        return 0;
    };


    void get_nodes(const std::string &filename, std::set<std::string> &nodes) {
        std::ifstream file(filename);
        std::string line;
        size_t pos = 0;
        while (std::getline(file, line)) {
            auto node = tsv_helper::parse_node(line);
            //print_node(node);
            if (pos % 10000 == 0) {
                std::cout << "Parsed nodes: " << pos << "\r";
                std::cout.flush();
            }
            nodes.insert(node.variable);
            ++pos;
        }
        std::cout << "Parsed nodes: " << pos << std::endl;
    }

    void get_invalid_triples(const std::string &filename, const std::string &output,
                             std::set<std::string> &nodes, std::set<std::string> &used_nodes) {
        std::ifstream file(filename);
        std::ofstream out(output);
        std::string line;
        size_t pos = 0;
        while (std::getline(file, line)) {
            auto edge = tsv_helper::parse_edge(line);
            if (pos % 10000 == 0) {
                std::cout << "Parsed edges: " << pos << "\r";
                std::cout.flush();
            }
            if (nodes.find(edge.to) != nodes.end()) {
                used_nodes.insert(edge.from);
                used_nodes.insert(edge.to);
                out << line << "\n";
            }
            ++pos;
        }
        std::cout << "Nodes: " << (nodes.find("Q12406") == nodes.end()) << std::endl;
        std::cout << "Nodes: " << (nodes.find("Q686") == nodes.end()) << std::endl;
    }

    void clean_nodes(const std::string &input, const std::string &output, const std::set<std::string> &used_nodes) {
        std::ifstream file(input);
        std::ofstream out(output);
        std::string line;
        std::cout << "Used nodes: " << (used_nodes.find("Q12406") == used_nodes.end()) << std::endl;
        std::cout << "Used nodes: " << (used_nodes.find("Q686") == used_nodes.end()) << std::endl;
        size_t pos = 0;
        while (std::getline(file, line)) {
            auto node = tsv_helper::parse_node(line);
            //print_node(node);
            if (pos % 10000 == 0) {
                std::cout << "Parsed nodes: " << pos << "\r";
                std::cout.flush();
            }
            if (used_nodes.find(node.variable) != used_nodes.end()) {
                out << line << "\n";
            }
            ++pos;
        }
        std::cout << "Parsed nodes: " << pos << std::endl;
    }



    void parse_file_nodes(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        size_t pos = 0;
        while (std::getline(file, line)) {
            auto node = tsv_helper::parse_node(line);
            //print_node(node);
            if (pos % 10000 == 0) {
                std::cout << "Parsed nodes: " << pos << "\r";
                std::cout.flush();
            }

            uint32_t node_id = get_or_add(node.variable, m_set_nodes);
            for (auto & label : node.labels) {
                uint32_t label_id = get_or_add(label, m_set_label_nodes);
                m_label_nodes_map[label_id].push_back(node_id);
            }
            for (auto & prop : node.properties) {
                std::string key = prop.key;
                std::string value = prop.value;
                uint32_t prop_id = get_or_add(key, m_properties_node);
                if (m_properties_node_values.size() <= prop_id) {
                    m_properties_node_values.resize(prop_id + 1);
                }
                m_properties_node_values[prop_id][node_id] = value;
            }
            ++pos;
        }
        std::cout << "Parsed nodes: " << pos << std::endl;
    }

    void parse_file_edges(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        size_t pos = 0;
        while (std::getline(file, line)) {
            auto edge = tsv_helper::parse_edge(line);
            if (pos % 10000 == 0) {
                std::cout << "Parsed edges: " << pos << "\r";
                std::cout.flush();
            }

            uint32_t subj_id = get(edge.from, m_set_nodes);
            uint32_t obj_id = get(edge.to, m_set_nodes);

            if (!subj_id || !obj_id) continue;

            uint32_t pred_id = get_or_add(edge.type, m_set_label_edges);
            m_triples.emplace_back(subj_id, pred_id, obj_id, edge.properties);
            ++pos;
        }
        std::cout << "Parsed edges: " << pos << std::endl;
        std::cout << "Sorting edges..." << std::flush;
        std::sort(m_triples.begin(), m_triples.end(), [](const triple_type& a, const triple_type& b) {
            if (std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
            if (std::get<2>(a) != std::get<2>(b)) return std::get<2>(a) < std::get<2>(b);
            return std::get<0>(a) < std::get<0>(b);
        });
        std::cout << " done." << std::endl;

        std::cout << "Setting properties of edges..." << std::flush;
        for (uint64_t i = 0; i < m_triples.size(); ++i) {
            for (auto & prop : std::get<3>(m_triples[i])) {
                std::string key = prop.key;
                std::string value = prop.value;
                uint32_t prop_id = get_or_add(key, m_properties_edge);
                if (m_properties_edge_values.size() <= prop_id) {
                    m_properties_edge_values.resize(prop_id + 1);
                }
                m_properties_edge_values[prop_id][i+1] = value;
            }
        }
        std::cout << " done." << std::endl;




    }

    void write_file_nodes(const std::string& filename) {
        std::cout << "Writing nodes data to " << filename << std::endl;

        std::string nodes = filename + ".nodes.dict";
        std::string node_labels = filename + ".nlabels.dict";
        std::string label2nodes = filename + ".label2nodes";
        std::string node_props = filename + ".nprops.dict";
        std::string nprop2values = filename + ".nprop2values";

        // Escribir nodos
        std::cout << "Writing nodes data to " << filename << std::endl;
        {
            std::ofstream file(nodes);
            for (const auto& pair : m_set_nodes) {
                file << pair.second << " " << pair.first << "\n";
            }
        }
        // Escribir labels de nodos
        std::cout << " - Writing node labels" << std::endl;
        {
            std::ofstream file(node_labels);
            for (const auto& pair : m_set_label_nodes) {
                file << pair.second << " " << pair.first << "\n";
            }
        }

        //Escribir node_labels map
        std::cout << " - Writing label to nodes map" << std::endl;
        {
            std::ofstream file(label2nodes);
            for (const auto& pair : m_label_nodes_map) {
                file << pair.first << " " << pair.second.size();
                for (const auto& node_id : pair.second) {
                    file << " " << node_id;
                }
                file << "\n";
            }
        }

        //Escribir properties de nodos
        std::cout << " - Writing node properties" << std::endl;
        {
            std::ofstream file(node_props);
            for (const auto& pair : m_properties_node) {
                file << pair.second << " " << pair.first << "\n";
            }
        }

        //Escribir valores de cada property
        std::cout << " - Writing node property values" << std::endl;
        {
            for (uint32_t p_id = 1; p_id < m_properties_node_values.size(); p_id++) {
                std::string file_name = nprop2values + "." + std::to_string(p_id);
                std::ofstream file(file_name);
                const auto& prop_map = m_properties_node_values[p_id];
                for (const auto& pair : prop_map) {
                    /*file << pair.first << " " << pair.second.size();
                    for (const auto& value : pair.second) {
                        file << " " << value;
                    }*/
                    file << pair.first << " " << pair.second;
                    file << "\n";
                }
            }
        }
    }

    void write_file_edges(const std::string& filename) {
        std::cout << "Writing edges data to " << filename << std::endl;

        std::string edge_labels = filename + ".elabels.dict";
        std::string edge_props = filename + ".eprops.dict";
        std::string eprop2values = filename + ".eprop2values";
        std::string triples = filename + ".triples";
        std::cout << " - Writing edge labels" << std::endl;
        // Escribir labels de aristas
        {
            std::ofstream file(edge_labels);
            for (const auto& pair : m_set_label_edges) {
                file << pair.second << " " << pair.first << "\n";
            }
        }

        //Escribir properties de aristas
        std::cout << " - Writing edge properties" << std::endl;
        {
            std::ofstream file(edge_props);
            for (const auto& pair : m_properties_edge) {
                file << pair.second << " " << pair.first << "\n";
            }
        }



        //Escribir valores de cada property de aristas
        std::cout << " - Writing edge property values" << std::endl;
        {
            for (uint32_t p_id = 1; p_id < m_properties_edge_values.size(); p_id++) {
                std::string file_name = eprop2values + "." + std::to_string(p_id);
                std::ofstream file(file_name);
                const auto& prop_map = m_properties_edge_values[p_id];
                for (const auto& pair : prop_map) {
                    /*file << pair.first << " " << pair.second.size();
                    for (const auto& value : pair.second) {
                        file << " " << value;
                    }*/
                    file << pair.first  << " " << pair.second;
                    file << "\n";
                }
            }
        }

        std::cout << " - Writing triples" << std::endl;
        // Escribir triples
        {
            std::ofstream file(triples);
            for (const auto& triple : m_triples) {
                file << std::get<0>(triple) << " "
                     << std::get<1>(triple) << " "
                     << std::get<2>(triple) << "\n";
            }
        }
    }

    void clear_data_nodes() {
        sdsl::util::clear(m_set_label_nodes);
        sdsl::util::clear(m_label_nodes_map);
        sdsl::util::clear(m_properties_node);
        sdsl::util::clear(m_properties_node_values);
    }

public:

    void clean(const std::string &filename) {
        std::string node_file = filename + "-nodes.tsv";
        std::string edge_file = filename + "-edges.tsv";

        std::string node_file_clean = filename + "-clean-nodes.tsv";
        std::string edge_file_clean = filename + "-clean-edges.tsv";

        std::set<std::string> nodes, used_nodes;
        get_nodes(node_file, nodes);
        get_invalid_triples(edge_file, edge_file_clean, nodes, used_nodes);
        clean_nodes(node_file, node_file_clean, used_nodes);
    }

    void parse(const std::string& filename) {
        std::string node_file = filename + "-clean-nodes.tsv";
        std::string edge_file = filename + "-clean-edges.tsv";

        std::string data = filename + ".data";
        parse_file_nodes(node_file);
        write_file_nodes(data);
        clear_data_nodes();
        parse_file_edges(edge_file);
        write_file_edges(data);
    }


};

#endif //TSV_PARSER_HPP
