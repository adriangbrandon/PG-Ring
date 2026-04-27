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
    std::ofstream ofs(tsv_prfix+ "-nodes.cypher");
    std::string line;
    size_t pos = 0;
    tsv_parser tp;
    while (std::getline(nodes, line)) {
        auto node = tsv_helper::parse_node(line);
        std::string cypher_node = tsv_helper::node_to_cypher(node);
        ofs  << cypher_node << ";\n";
    }
    nodes.close();
    ofs.close();
    ofs.open(tsv_prfix + "-edges.cypher");
    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);
        std::string cypher_edge = tsv_helper::edge_to_cypher(edge);
        ofs  << cypher_edge << ";\n";
    }
    edges.close();
    ofs.close();

}
