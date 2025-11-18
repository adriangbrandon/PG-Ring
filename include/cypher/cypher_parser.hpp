//
// Created by adrian on 22/10/25.
//

#ifndef CYPHER_PARSER_HPP
#define CYPHER_PARSER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <regex>
#include <fstream>
#include <sstream>

#include "configuration.hpp"

class cypher_parser {


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
    typedef std::vector<std::map<uint32_t, std::vector<uint32_t>>> properties_values_type; //para que se mostren ordenados por id de label
    properties_values_type m_properties_node_values;
    properties_values_type m_properties_edge_values;

    int get_or_add(const std::string& name, std::unordered_map<std::string, uint32_t>& set) {
        auto it = set.find(name);
        if (it != set.end()) return it->second;
        int id = set.size()+1;
        set[name] = id;
        return id;
    };

    void parse_properties(const std::string &prop, uint32_t id,  std::unordered_map<std::string, uint32_t> &map_properties, properties_values_type &pvs) {
        std::map<uint32_t, std::vector<uint32_t>> prop_map;
        std::regex prop_regex(R"((\w+)\s*:\s*(\[[^\]]*\]|'[^']*'|"[^"]*"|[^,}]+))");
        auto begin = std::sregex_iterator(prop.begin(), prop.end(), prop_regex);
        auto end = std::sregex_iterator();
        for (auto it = begin; it != end; ++it) {
            std::string key = (*it)[1];
            std::string value = (*it)[2];
            int prop_id = get_or_add(key, map_properties);
            std::vector<uint32_t> values;
            // Eliminar espacios al inicio y fin
            size_t first = value.find_first_not_of(" \t\n");
            size_t last = value.find_last_not_of(" \t\n");
            if (first != std::string::npos && last != std::string::npos)
                value = value.substr(first, last - first + 1);
            if (!value.empty() && value[0] == '[' && value.back() == ']') {
                std::string inner = value.substr(1, value.size() - 2);
                std::regex list_regex(R"((?:'[^']*'|"[^"]*"|[^,]+))");
                auto lbegin = std::sregex_iterator(inner.begin(), inner.end(), list_regex);
                auto lend = std::sregex_iterator();
                for (auto lit = lbegin; lit != lend; ++lit) {
                    std::string v = (*lit)[0];
                    size_t f = v.find_first_not_of(" \t\n'\"");
                    size_t l = v.find_last_not_of(" \t\n'\"");
                    if (f != std::string::npos && l != std::string::npos)
                        v = v.substr(f, l - f + 1);
                    std::hash<std::string> hasher;
                    values.push_back(static_cast<uint32_t>(hasher(v)));
                }
            } else {
                if (!value.empty() && (value[0] == '\'' || value[0] == '"')) {
                    value = value.substr(1, value.size() - 2);
                    std::hash<std::string> hasher;
                    values.push_back(static_cast<uint32_t>(hasher(value)));
                }else {
                    values.push_back(std::stoi(value));
                }

            }
            prop_map[prop_id] = values;
        }

        for (const auto &p : prop_map) {
            if (pvs.size() <= p.first) {
                pvs.resize(p.first + 1);
            }
            pvs[p.first][id] = p.second;
        }

    }

    // Métodos principales
    void parse_line(const std::string& line) {
        // Solo procesar si la línea contiene CREATE
        if (line.find("CREATE") == std::string::npos) return;
        // Extraer la parte después de CREATE
        size_t pos = line.find("CREATE");
        std::string content = line.substr(pos + 6);
        size_t first = content.find_first_not_of(" \t\n");
        if (first != std::string::npos) content = content.substr(first);
        // Regex para nodos y aristas
        static std::regex node_regex(R"(\((\w+)((?::\w+)*)(?:\s*\{(.*?)\})?\))");
        static std::regex edge_regex(R"(\((\w+)\)\s*-\s*\[:(\w+)(?:\s*\{(.*?)\})?\]\s*->\s*\((\w+)\))");
        // Primero buscar aristas
        auto edge_begin = std::sregex_iterator(content.begin(), content.end(), edge_regex);
        auto edge_end = std::sregex_iterator();
        bool found_edge = edge_begin != edge_end;
        if (found_edge) {
            for (auto it = edge_begin; it != edge_end; ++it) {
                std::string subj = (*it)[1];
                std::string pred = (*it)[2];
                std::string edge_props = (*it)[3];
                std::string obj = (*it)[4];
                uint32_t subj_id = get_or_add(subj, m_set_nodes);
                uint32_t pred_id = get_or_add(pred, m_set_label_edges);
                uint32_t obj_id = get_or_add(obj, m_set_nodes);
                std::cout << "Edge props: " << edge_props << std::endl;
                m_triples.emplace_back(subj_id, pred_id, obj_id);
                parse_properties(edge_props, m_triples.size(), m_properties_edge, m_properties_edge_values);
            }
        } else {
            // Si no hay aristas, buscar nodos
            auto node_begin = std::sregex_iterator(content.begin(), content.end(), node_regex);
            auto node_end = std::sregex_iterator();
            for (auto it = node_begin; it != node_end; ++it) {
                std::string node_name = (*it)[1];
                std::string labels_str = (*it)[2];
                std::string node_props = (*it)[3];
                uint32_t node_id = get_or_add(node_name, m_set_nodes);
                // Extraer todas las labels separadas por ':'
                size_t start = 0;
                while ((start = labels_str.find(':', start)) != std::string::npos) {
                    size_t end = labels_str.find(':', start + 1);
                    std::string label = labels_str.substr(start + 1, end - start - 1);
                    uint32_t label_id = get_or_add(label, m_set_label_nodes);
                    m_label_nodes_map[label_id].push_back(node_id);
                    if (end == std::string::npos) break;
                    start = end;
                }
                parse_properties(node_props, node_id, m_properties_node, m_properties_node_values);
            }
        }
    };

public:

    void parse_file(const std::string& filename) {
        std::ifstream file(filename);
        std::string line, buffer;
        bool in_create = false;
        while (std::getline(file, line)) {
            // Quitar comentarios
            size_t comment = line.find("//");
            if (comment != std::string::npos) line = line.substr(0, comment);
            // Quitar espacios al inicio y fin
            size_t first = line.find_first_not_of(" \t\n");
            size_t last = line.find_last_not_of(" \t\n");
            if (first == std::string::npos) continue;
            line = line.substr(first, last - first + 1);
            size_t create_pos = line.find("CREATE");
            if (create_pos != std::string::npos) {
                // Si ya estábamos acumulando, procesar el buffer anterior
                if (in_create && !buffer.empty()) {
                    parse_line(buffer);
                    buffer.clear();
                }
                in_create = true;
                buffer = line;
            } else if (in_create) {
                buffer += " " + line;
            }
        }
        // Procesar cualquier instrucción CREATE restante
        if (in_create && !buffer.empty()) {
            parse_line(buffer);
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
                    file << pair.first << " " << pair.second.size();
                    for (const auto& value : pair.second) {
                        file << " " << value;
                    }
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
                    file << pair.first << " " << pair.second.size();
                    for (const auto& value : pair.second) {
                        file << " " << value;
                    }
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



#endif //CYPHER_PARSER_HPP
