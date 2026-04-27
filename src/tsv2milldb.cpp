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
        ofs << milldb_node << "\n";
    }
    nodes.close();

    // Process edges
    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);
        std::string milldb_edge = tsv_helper::edge_to_milldb(edge);
        ofs << milldb_edge << "\n";
    }
    edges.close();
    ofs.close();

    std::cout << "MillenniumDB file generated: " << out_file << std::endl;
    return 0;
}
