//
// Created by adrian on 26/1/26.
//

#include "tsv/tsv_parser.hpp"

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <tsv_prefix> <output>" << std::endl;
        std::cout << "Reads <tsv_prefix>-nodes.tsv and <tsv_prefix>-edges.tsv" << std::endl;
        std::cout << "Outputs <output> file" << std::endl;
        return 0;
    }

    std::string tsv_prefix = argv[1];
    std::string out_file  = argv[2];

    std::ifstream nodes(tsv_prefix + "-nodes.tsv");
    std::ifstream edges(tsv_prefix + "-edges.tsv");
    std::ofstream ofs(out_file);
    std::string line;

    // Process nodes
    while (std::getline(nodes, line)) {
        auto node = tsv_helper::parse_node(line);
        std::string milldb_node = tsv_helper::node_to_milldb(node);
        ofs << milldb_node << ";\n";
    }
    nodes.close();

    // Process edges
    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);
        std::string milldb_edge = tsv_helper::edge_to_milldb(edge);
        ofs << milldb_edge << ";\n";
    }
    edges.close();
    ofs.close();

    std::cout << "MillenniumDB file generated: " << out_file << std::endl;
    return 0;
}
