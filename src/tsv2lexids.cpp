//
// Created by adrian on 10/3/26.
//

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

