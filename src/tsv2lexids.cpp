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

#include "tsv/tsv_hash_parser.hpp"

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <tsv_prefix> <output_prefix>" << std::endl;
        std::cout << "Reads <tsv_prefix>-clean-nodes.tsv and <tsv_prefix>-clean-edges.tsv" << std::endl;
        std::cout << "Outputs <output_prefix>.*.dict files with hash-based IDs" << std::endl;
        std::cout << "  - IDs are assigned based on hash values (LibCSD compatible)" << std::endl;
        std::cout << "  - Uses same bitwisehash function as LibCSD" << std::endl;
        return 0;
    }

    std::string tsv_prefix = argv[1];
    std::string output_prefix = argv[2];

    tsv_hash_parser parser;
    parser.parse(tsv_prefix, output_prefix);

    std::cout << "\n✓ Dictionary creation complete!" << std::endl;

    return 0;
}

