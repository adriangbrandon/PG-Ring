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
    typedef std::vector<std::map<uint32_t, std::vector<std::string>>> properties_values_type; //para que se mostren ordenados por id de label
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

        std::cout << "Parsing properties for id " << id << ": " << prop << std::endl;

        std::map<uint32_t, std::vector<std::string>> prop_map;

        // Regex mejorado para capturar propiedades en diferentes formatos
        // Grupo 1: clave con comillas "key"
        // Grupo 2: clave sin comillas key
        // Grupo 3: array [...]
        // Grupo 4: valor con comillas dobles "value"
        // Grupo 5: valor con comillas simples 'value'
        // Grupo 6: valor sin comillas (número, booleano, URL)
        std::regex prop_regex(R"delim((?:"([^"]+)"|([a-zA-Z0-9_-]+))\s*:\s*(?:(\[[^\]]*\])|"([^"]*)"|'([^']*)'|([^\s,}]+)))delim");

        auto begin = std::sregex_iterator(prop.begin(), prop.end(), prop_regex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            // La clave puede estar en el grupo 1 (con comillas) o grupo 2 (sin comillas)
            std::string key = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();

            // El valor puede estar en los grupos 3-6
            std::string value;
            for (int i = 3; i <= 6; ++i) {
                if ((*it)[i].matched) {
                    value = (*it)[i].str();
                    break;
                }
            }

            int prop_id = get_or_add(key, map_properties);
            std::vector<std::string> values;

            // Eliminar espacios al inicio y fin
            size_t first = value.find_first_not_of(" \t\n");
            size_t last = value.find_last_not_of(" \t\n");
            if (first != std::string::npos && last != std::string::npos)
                value = value.substr(first, last - first + 1);

            if (!value.empty() && value[0] == '[' && value.back() == ']') {
                // Manejar arrays
                std::string inner = value.substr(1, value.size() - 2);
                std::regex list_regex(R"delim('([^']*)'|"([^"]*)"|([^,]+))delim");
                auto lbegin = std::sregex_iterator(inner.begin(), inner.end(), list_regex);
                auto lend = std::sregex_iterator();
                for (auto lit = lbegin; lit != lend; ++lit) {
                    std::string v;
                    if ((*lit)[1].matched) v = (*lit)[1].str();
                    else if ((*lit)[2].matched) v = (*lit)[2].str();
                    else v = (*lit)[3].str();

                    size_t f = v.find_first_not_of(" \t\n");
                    size_t l = v.find_last_not_of(" \t\n");
                    if (f != std::string::npos && l != std::string::npos)
                        v = v.substr(f, l - f + 1);
                    if (!v.empty())
                        values.push_back(v);
                }
            } else {
                // Valor simple
                if (!value.empty())
                    values.push_back(value);
            }

            if (!values.empty())
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

        // Extraer la parte después de CREATE y quitar punto y coma final
        size_t pos = line.find("CREATE");
        std::string content = line.substr(pos + 6);
        size_t first = content.find_first_not_of(" \t\n");
        if (first != std::string::npos) content = content.substr(first);

        // Remover punto y coma final si existe
        size_t semicolon = content.find(';');
        if (semicolon != std::string::npos) content = content.substr(0, semicolon);

        // Regex para nodos y aristas (permitir guiones, guiones bajos y otros caracteres en nombres)
        static std::regex node_regex(R"delim(\(([a-zA-Z0-9_-]+)((?::[a-zA-Z0-9_-]+)*)(?:\s*\{(.*)\})?\s*\))delim");
        static std::regex edge_regex(R"delim(\(([a-zA-Z0-9_-]+)\)\s*-\s*\[:([a-zA-Z0-9_-]+)(?:\s*\{(.*)\})?\]\s*->\s*\(([a-zA-Z0-9_-]+)\))delim");

        // Comprobar si es una arista (contiene ->)
        if (content.find("->") != std::string::npos) {
            // Procesar arista
            std::smatch match;
            if (std::regex_search(content, match, edge_regex)) {
                std::string subj = match[1];
                std::string pred = match[2];
                std::string edge_props = match[3];
                std::string obj = match[4];

                uint32_t subj_id = get_or_add(subj, m_set_nodes);
                uint32_t pred_id = get_or_add(pred, m_set_label_edges);
                uint32_t obj_id = get_or_add(obj, m_set_nodes);

                m_triples.emplace_back(subj_id, pred_id, obj_id);

                if (!edge_props.empty()) {
                    parse_properties(edge_props, m_triples.size(), m_properties_edge, m_properties_edge_values);
                }
            }
        } else {
            // Procesar nodo
            std::smatch match;
            if (std::regex_search(content, match, node_regex)) {
                std::string node_name = match[1];
                std::string labels_str = match[2];
                std::string node_props = match[3];

                uint32_t node_id = get_or_add(node_name, m_set_nodes);

                // Extraer todas las labels separadas por ':'
                if (!labels_str.empty()) {
                    size_t start = 0;
                    while ((start = labels_str.find(':', start)) != std::string::npos) {
                        size_t end = labels_str.find(':', start + 1);
                        std::string label = labels_str.substr(start + 1, end - start - 1);
                        if (!label.empty()) {
                            uint32_t label_id = get_or_add(label, m_set_label_nodes);
                            m_label_nodes_map[label_id].push_back(node_id);
                        }
                        if (end == std::string::npos) break;
                        start = end;
                    }
                }

                if (!node_props.empty()) {
                    parse_properties(node_props, node_id, m_properties_node, m_properties_node_values);
                }
            }
        }
    };

public:

    void parse_file(const std::string& filename) {
        std::ifstream file(filename);
        std::string line, buffer;
        while (std::getline(file, line)) {
            parse_line(line);
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
                    file << pair.first << " " << pair.second[0];
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
                    file << pair.first  << " " << pair.second[0];
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
