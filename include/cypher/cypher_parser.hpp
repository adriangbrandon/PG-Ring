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

class cypher_parser {


private:
    // Diccionarios
    std::unordered_map<std::string, int> m_set_nodes;   // Nombres de nodos
    std::unordered_map<std::string, int> m_set_label_nodes;  // Labels de nodos
    std::unordered_map<std::string, int> m_set_label_edges;  // Labels de aristas
    // Triples: (sujeto, predicado, objeto)
    std::vector<std::tuple<int, int, int>> m_triples;

    int get_or_add(const std::string& name, std::unordered_map<std::string, int>& set) {
        auto it = set.find(name);
        if (it != set.end()) return it->second;
        int id = set.size()+1;
        set[name] = id;
        return id;
    };

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
        static std::regex node_regex(R"(\(([\w_]+)((?::[\w_]+)*)(?:\s*(\{.*?\}))?\))");
        static std::regex edge_regex(R"(\(([\w_]+)\)\s*-\s*\[:([\w_]+)\s*(\{.*?\})?\]\s*->\s*\(([\w_]+)\))");
        // Primero buscar aristas
        auto edge_begin = std::sregex_iterator(content.begin(), content.end(), edge_regex);
        auto edge_end = std::sregex_iterator();
        bool found_edge = edge_begin != edge_end;
        if (found_edge) {
            for (auto it = edge_begin; it != edge_end; ++it) {
                std::string subj = (*it)[1];
                std::string pred = (*it)[2];
                std::string obj = (*it)[4];
                int subj_id = get_or_add(subj, m_set_nodes);
                int pred_id = get_or_add(pred, m_set_label_edges);
                int obj_id = get_or_add(obj, m_set_nodes);
                m_triples.emplace_back(subj_id, pred_id, obj_id);
            }
        } else {
            // Si no hay aristas, buscar nodos
            auto node_begin = std::sregex_iterator(content.begin(), content.end(), node_regex);
            auto node_end = std::sregex_iterator();
            for (auto it = node_begin; it != node_end; ++it) {
                std::string node_name = (*it)[1];
                std::string labels_str = (*it)[2];
                get_or_add(node_name, m_set_nodes);
                // Extraer todas las labels separadas por ':'
                size_t start = 0;
                while ((start = labels_str.find(':', start)) != std::string::npos) {
                    size_t end = labels_str.find(':', start + 1);
                    std::string label = labels_str.substr(start + 1, end - start - 1);
                    get_or_add(label, m_set_label_nodes);
                    if (end == std::string::npos) break;
                    start = end;
                }
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
        std::string nodes = filename + ".nodes";
        std::string node_labels = filename + ".nlabels";
        std::string edge_labels = filename + ".elabels";
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
