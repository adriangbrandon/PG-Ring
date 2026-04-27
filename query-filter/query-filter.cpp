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

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>

#include "query/transform.hpp"


std::unordered_map<std::string, uint32_t> m_set_nodes;   // Nombres de nodos
std::unordered_map<std::string, uint32_t> m_set_label_nodes;  // Labels de nodos
std::unordered_map<std::string, uint32_t> m_set_label_edges;  // Labels de aristas
std::unordered_map<std::string, uint32_t> m_properties_node;  // Properties de nodos
std::unordered_map<std::string, uint32_t> m_properties_edge;  // Properties de aristas

void read_nodes_dict(const std::string& prefix) {;
    std::ifstream file(prefix + ".nodes.dict");
    uint32_t id;
    std::string data;
    while (file >> id >> data) {
        m_set_nodes.insert({data, id});
    }
}

void read_nlabels_dict(const std::string& prefix) {;
    std::ifstream file(prefix + ".nlabels.dict");
    uint32_t id;
    std::string data;
    while (file >> id >> data) {
        m_set_label_nodes.insert({data, id});
    }
}

void read_elabels_dict(const std::string& prefix) {;
    std::ifstream file(prefix + ".elabels.dict");
    uint32_t id;
    std::string data;
    while (file >> id >> data) {
        m_set_label_edges.insert({data, id});
    }
}

void read_nprops_dict(const std::string& prefix) {;
    std::ifstream file(prefix + ".nprops.dict");
    uint32_t id;
    std::string data;
    while (file >> id >> data) {
        m_properties_node.insert({data, id});
    }
}

void read_eprops_dict(const std::string& prefix) {;
    std::ifstream file(prefix + ".eprops.dict");
    uint32_t id;
    std::string data;
    while (file >> id >> data) {
        m_properties_edge.insert({data, id});
    }
}

int main(int argc, char* argv[]) {

    //std::string q = "(?v:(P1 AND P2 AND ( P3 OR  P4)))-[]->()";
    //std::string q = "(?v:( NOT P1 AND P2 AND ( P3 OR  NOT P4) ))-[]->()";
    //std::string q = "(?v:((NOT P1 AND P2) OR ( P3 AND  NOT P4)))-[?e0:( NOT P5)]->(?v2:Q31), (?v2)-[?e1]->(?v3:Q32) WHERE (?e0.prop1 > 5) AND (?e0.prop2 = 'abc') AND (?e1.prop3 < 10) AND (?v2.prop4 = 'xyz')";
    std::string q = "(?v0)-[?e0:P131]->(Q801) WHERE ( ?v0.P625lat IS NOT NULL ) AND ( ?v0.P625long IS NOT NULL )";

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << "<prefix> <query> [distinct]" << std::endl;
        return 1;
    }
    bool distinct = false;
    if (argc == 4) {
        distinct = std::stoi(argv[3]);
    }

    std::string prefix = argv[1];
    std::string query_file = argv[2];

    ring::query::transform t;
    t.run(query_file, prefix, distinct);

     return 0;
}