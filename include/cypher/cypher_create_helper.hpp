/*
* Copyright (C) 2026 Author removed for double-blind evaluation
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
#ifndef CYPHER_CREATE_HELPER_HPP
#define CYPHER_CREATE_HELPER_HPP
#include <regex>
#include <string>
#include <vector>


namespace cypher_create_helper {
    typedef struct {
        std::string key;
        std::string value;
        bool is_string;
    } property_cypher_type;

    typedef struct {
        std::string variable;
        std::vector<std::string> labels;
        std::vector<property_cypher_type> properties;
    } node_cypher_type;

    typedef struct {
        std::string type;
        std::vector<property_cypher_type> properties;
        std::string from;
        std::string to;
    } edge_cypher_type;


    static std::string trim(const std::string &str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    static std::vector<property_cypher_type> extract_properties(const std::string &str) {
        std::vector<property_cypher_type> props;
        // Encontrar el contenido entre { }
        size_t openBrace = str.find('{');
        size_t closeBrace = str.rfind('}');

        if (openBrace == std::string::npos || closeBrace == std::string::npos) {
            return props;
        }

        std::string propsStr = str.substr(openBrace + 1, closeBrace - openBrace - 1);

        // Parsear propiedades:  key: value
        // Necesitamos manejar comas dentro de strings
        size_t pos = 0;
        while (pos < propsStr.length()) {
            // Buscar key:
            size_t colonPos = propsStr.find(':', pos);
            if (colonPos == std::string::npos) break;

            std::string key = trim(propsStr.substr(pos, colonPos - pos));

            // Buscar value (puede ser string con "", '', o valor simple)
            pos = colonPos + 1;

            // Skip whitespace
            while (pos < propsStr.length() && std::isspace(propsStr[pos])) pos++;

            std::string value;
            bool is_string = false;
            if (pos < propsStr.length()) {
                char startChar = propsStr[pos];

                if (startChar == '"') {
                    // String con comillas dobles - puede contener comillas simples
                    pos++; // skip "
                    size_t endPos = pos;
                    bool escaped = false;

                    while (endPos < propsStr.length()) {
                        if (escaped) {
                            escaped = false;
                        } else if (propsStr[endPos] == '\\') {
                            escaped = true;
                        } else if (propsStr[endPos] == '"') {
                            // Fin del string
                            break;
                        }
                        endPos++;
                    }

                    value = propsStr.substr(pos, endPos - pos);
                    pos = endPos + 1;
                    is_string = true;
                }
                else if (startChar == '\'') {
                    // String con comillas simples - puede contener comillas dobles
                    pos++; // skip '
                    size_t endPos = pos;
                    bool escaped = false;

                    while (endPos < propsStr. length()) {
                        if (escaped) {
                            escaped = false;
                        } else if (propsStr[endPos] == '\\') {
                            escaped = true;
                        } else if (propsStr[endPos] == '\'') {
                            // Fin del string
                            break;
                        }
                        endPos++;
                    }
                    value = propsStr.substr(pos, endPos - pos);
                    pos = endPos + 1;
                    is_string = true;
                } else {
                    // Valor simple (hasta la próxima coma)
                    size_t endPos = propsStr.find(',', pos);
                    if (endPos == std::string::npos) {
                        endPos = propsStr.length();
                    }
                    value = trim(propsStr.substr(pos, endPos - pos));
                    pos = endPos;
                }
            }

            // Guardar propiedad
            if (!key.empty()) {
                property_cypher_type prop;
                prop.key = key;
                prop.value = value;
                prop.is_string = is_string;
                props.push_back(prop);
            }

            // Skip comma
            while (pos < propsStr.length() && (propsStr[pos] == ',' || std::isspace(propsStr[pos]))) {
                pos++;
            }
        }
        return props;
    }

    static bool is_node(const std::string &query) {
        std::regex createPattern(R"(CREATE\s+)", std::regex::icase);
        std::string clean = std::regex_replace(query, createPattern, "");
        clean = trim(clean);

        // Si contiene relación: ()-[]->(...)
        std::regex relPattern(R"(\([^)]*\)\s*-\s*\[[^\]]*\]\s*->\s*\([^)]*\))");
        if (std::regex_search(clean, relPattern)) {
            return false; // Es relación
        }

        return true;
    }

    static node_cypher_type parse_node(const std::string &query) {
        node_cypher_type node;
        std::string nodeStr = query.substr(8); //remove create (
        nodeStr.pop_back(); //remove ending parenthesis

        // Variable
        std::regex varPattern(R"(^\s*(\w+))");
        std::smatch varMatch;
        if (std::regex_search(nodeStr, varMatch, varPattern)) {
            node.variable = varMatch[1].str();
        }

        // Labels
        size_t bracePos = nodeStr.find('{');
        std::string labelPart = (bracePos != std::string::npos)
                                    ? nodeStr.substr(0, bracePos)
                                    : nodeStr;

        std::regex labelPattern(R"(:(\w+))");
        std::smatch labelMatch;
        auto ls = labelPart.cbegin();
        while (std::regex_search(ls, labelPart.cend(), labelMatch, labelPattern)) {
            node.labels.push_back(labelMatch[1].str());
            ls = labelMatch.suffix().first;
        }

        // Properties
        node.properties = extract_properties(nodeStr);
        return node;
    }

    static edge_cypher_type parse_edge(const std::string &query) {
        edge_cypher_type edge;

        std::string edgeStr = query.substr(7); //remove create
        // Patrón:  (a)-[r:TYPE {props}]->(b)
        std::regex relPattern(R"(\((\w*)\)\s*-\s*\[([^\]]*)\]\s*->\s*\((\w*)\))");
        std::smatch relMatch;

        if (std::regex_search(query, relMatch, relPattern)) {
            edge.from = relMatch[1].str();
            std::string relContent = relMatch[2].str();
            edge.to = relMatch[3].str();

            // Tipo
            std::regex typePattern(R"(:(\w+))");
            std::smatch typeMatch;
            if (std::regex_search(relContent, typeMatch, typePattern)) {
                edge.type = typeMatch[1].str();
            }

            // Properties
            edge.properties = extract_properties(relContent);
        }

        return edge;
    }

    void print_node(const node_cypher_type &node) {
        std::cout << "Node" << std::endl;
        std::cout << " Variable: " << node.variable << std::endl;
        std::cout << " Labels: ";
        for (const auto &label : node.labels) {
            std::cout << label << " ";
        }
        std::cout << std::endl;
        std::cout << " Properties: " << std::endl;
        for (const auto &prop : node.properties) {
            std::cout << "  " << prop.key << ": " << prop.value << std::endl;
        }
    }

    void print_edge(const edge_cypher_type &edge) {
        std::cout << "Edge" << std::endl;
        std::cout << " From: " << edge.from << std::endl;
        std::cout << " To: " << edge.to << std::endl;
        std::cout << " Type: " << edge.type << std::endl;
        std::cout << " Properties: " << std::endl;
        for (const auto &prop : edge.properties) {
            std::cout << "  " << prop.key << ": " << prop.value << std::endl;
        }
    }
};

#endif //CYPHER_CREATE_HELPER_HPP
