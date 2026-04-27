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


#include <iostream>
#include <string>
#include <cypher/cypher_create_parser_v2.hpp>

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage:" << argv[0] << " <cypher_file> <output_file>" << std::endl;
        return 0;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    cypher_create_parser_v2 parser;
    parser.parse_file(input_file);
    parser.write_file(output_file);

}
