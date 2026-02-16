//
// Created by adrian on 26/1/26.
//

#include "tsv/tsv_parser.hpp"

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage:" << argv[0] << " <tsv_prefix> <output>" << std::endl;
        std::cout << "Reads <tsv_prefix>-nodes.tsv and <tsv_prefix>-edges.tsv" << std::endl;
        std::cout << "Ouputs <output> file" << std::endl;
        return 0;
    }

    std::string tsv_prfix = argv[1];
    std::string out_file  = argv[2];

    std::ifstream nodes(tsv_prfix + "-nodes.tsv");
    std::ifstream edges(tsv_prfix + "-edges.tsv");
    std::ofstream ofs(out_file);
    std::string line;
    size_t pos = 0;
    tsv_parser tp;
    while (std::getline(nodes, line)) {
        auto node = tsv_helper::parse_node(line);
        std::string cypher_node = tsv_helper::node_to_cypher(node);
        ofs  << cypher_node << ";\n";
    }
    nodes.close();
    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);
        std::string cypher_edge = tsv_helper::edge_to_cypher(edge);
        ofs  << cypher_edge << ";\n";
    }
    edges.close();
    ofs.close();

}
