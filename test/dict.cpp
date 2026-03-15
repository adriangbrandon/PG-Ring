//
// Created by adrian on 14/3/26.
//

#include <dict/string_dictionary.hpp>
#include <string>

// LibCSD's bitwisehash function for testing
size_t bitwisehash(const std::string& word, size_t htsize) {
    uint32_t h = 4294967279u;
    for (size_t i = 0; i < word.length(); i++) {
        int c = static_cast<unsigned char>(word[i]);
        h = (((h << 15) + h) + static_cast<uint32_t>(c)) % htsize;
    }
    return static_cast<size_t>(h % htsize);
}

void build_dict(const std::string &file, ring::string_dictionary &dict) {
    std::ifstream ifs(file);
    std::string key;
    int value;
    std::vector<std::string> input;
    do {
        ifs >> value >> key;
        if(ifs.eof()) break;
        input.emplace_back(key);
    } while (true);
    std::cout << "Building dictionary from " << file << " with " << input.size() << " entries" << std::endl;
    dict = ring::string_dictionary(std::move(input), ring::string_dictionary::dict_type::HASHRPF, 20, 32);
}

int main(int argc, char* argv[]) {

    std::string s = argv[1];
    ring::string_dictionary dict;
    build_dict(s, dict);
    std::cout << "Done building " << s << std::endl;

    // Test: Check if IDs match hash order
    std::cout << "\n=== Testing ID assignment order (should match hash order) ===" << std::endl;
    std::vector<std::string> test_strings = {"Q10129", "Q100", "Q1000", "Q10000", "Q1"};

    // Compute hash table size (same as LibCSD: elements * 1.2 for 20% overhead)
    size_t htsize = static_cast<size_t>(dict.size() * 1.2);

    for (const auto& str : test_strings) {
        auto id = dict.locate(str);
        auto hash = bitwisehash(str, htsize);
        std::cout << "String: " << str << " -> Hash: " << hash << " -> ID: " << id << std::endl;
    }

    std::cout << "\n=== Testing extractRank ===" << std::endl;
    // extractRank gives the string at alphabetical position k
    for (const auto& str : test_strings) {
        auto id = dict.locate(str);
        auto hash = bitwisehash(str, htsize);
        std::cout << "String=" << str << ", Hash=" << hash << ", ID=" << id << std::endl;
    }


}
