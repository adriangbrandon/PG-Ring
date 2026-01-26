//
// Created by adrian on 26/1/26.
//

#include "tsv/tsv_parser.hpp"

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Usage:" << argv[0] << " <tsv_prefix>" << std::endl;
        std::cout << "Reads <tsv_prefix>-nodes.tsv and <tsv_prefix>-edges.tsv" << std::endl;
        std::cout << "Ouputs <tsv_prefix>.data.<extension> files" << std::endl;
        return 0;
    }

    std::string tsv_prfix = argv[1];
    tsv_parser parser;
    parser.parse(tsv_prfix);

}
