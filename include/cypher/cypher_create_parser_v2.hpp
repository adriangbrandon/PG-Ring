#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <unordered_map>

#include "configuration.hpp"
#include "cypher_create_helper.hpp"


class cypher_create_parser_v2 {

private:
    // Diccionarios
    std::unordered_map<std::string, uint32_t> m_set_nodes;   // Nombres de nodos
    std::unordered_map<std::string, uint32_t> m_set_label_nodes;  // Labels de nodos
    std::unordered_map<std::string, uint32_t> m_set_label_edges;  // Labels de aristas
    std::unordered_map<std::string, uint32_t> m_properties_node;  // Properties de nodos
    std::unordered_map<std::string, uint32_t> m_properties_edge;  // Properties de aristas
    // Triples: (sujeto, predicado, objeto)
    std::vector<std::tuple<int, int, int>> m_triples;
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

public:

    void parse_file(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        size_t p = 0;
        while (std::getline(file, line)) {
            ++p;
            if (p % 100000 == 0) {
                std::cout << "Processed " << p << " lines.\n";
            }
            if (line.empty()) continue;
            if (line.back() == ';') {
                line = line.substr(0, line.size() - 1);
            }
           // std::cout << "Line: " << line << std::endl;
            if (cypher_create_helper::is_node(line)) {
                auto node = cypher_create_helper::parse_node(line);
                //print_node(node);
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
                    if (prop.is_string) value = '"' + value + '"'; // add quotes to string values
                    m_properties_node_values[prop_id][node_id] = value;
                }
            }else {
                auto edge = cypher_create_helper::parse_edge(line);
                //print_edge(edge);
                uint32_t subj_id = get(edge.from, m_set_nodes);
                uint32_t obj_id = get(edge.to, m_set_nodes);
                if (!subj_id || !obj_id) continue;

                uint32_t pred_id = get_or_add(edge.type, m_set_label_edges);
                m_triples.emplace_back(subj_id, pred_id, obj_id);
                for (auto & prop : edge.properties) {
                    std::string key = prop.key;
                    std::string value = prop.value;
                    uint32_t prop_id = get_or_add(key, m_properties_edge);
                    if (m_properties_edge_values.size() <= prop_id) {
                        m_properties_edge_values.resize(prop_id + 1);
                    }
                    if (prop.is_string) value = '"' + value + '"'; // add quotes to string values
                    m_properties_edge_values[prop_id][m_triples.size()] = value;
                }
            }
        }
    }


    void write_file(const std::string& filename) {
        std::string nodes = filename + ".nodes.dict";
        std::string node_labels = filename + ".nlabels.dict";
        std::string edge_labels = filename + ".elabels.dict";
        std::string label2nodes = filename + ".label2nodes";
        std::string node_props = filename + ".nprops.dict";
        std::string edge_props = filename + ".eprops.dict";
        std::string nprop2values = filename + ".nprop2values";
        std::string eprop2values = filename + ".eprop2values";
        std::string triples = filename + ".triples";
        // Escribir nodos
        {
            std::ofstream file(nodes);
            for (const auto& pair : m_set_nodes) {
                file << pair.second << " " << pair.first << "\n";
            }
        }
        // Escribir labels de nodos
        {
            std::ofstream file(node_labels);
            for (const auto& pair : m_set_label_nodes) {
                file << pair.second << " " << pair.first << "\n";
            }
        }
        // Escribir labels de aristas
        {
            std::ofstream file(edge_labels);
            for (const auto& pair : m_set_label_edges) {
                file << pair.second << " " << pair.first << "\n";
            }
        }

        //Escribir node_labels map
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
        {
            std::ofstream file(node_props);
            for (const auto& pair : m_properties_node) {
                file << pair.second << " " << pair.first << "\n";
            }
        }

        //Escribir properties de aristas
        {
            std::ofstream file(edge_props);
            for (const auto& pair : m_properties_edge) {
                file << pair.second << " " << pair.first << "\n";
            }
        }

        //Escribir valores de cada property
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

        //Escribir valores de cada property de aristas
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

};

