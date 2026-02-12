//
// Created by adrian on 9/2/26.
//

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
        std::cerr << "Usage: " << argv[0] << "<prefix> <query>" << std::endl;
        return 1;
    }

    std::string prefix = argv[1];
    std::string query_file = argv[2];

    ring::query::transform t;
    t.run(query_file, prefix);

     return 0;
}