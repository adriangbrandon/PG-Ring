//
// Created by adrian on 22/10/25.
//


#include <iostream>
#include <string>
#include <cypher/parser.hpp>

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage:" << argv[0] << " <cypher_file> <output_file>" << std::endl;
        return 0;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    cypher_parser parser;
    parser.parse_file(input_file);
    parser.write_file(output_file);

}
