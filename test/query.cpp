//
// Created by adrian on 29/10/25.
//
#include <string>
#include <ring_pg.hpp>
#include <query/query_parser.hpp>
#include <ltj_algorithm_pg.hpp>

int main(int argc, char* argv[]) {


    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    //std::string s = "(8)-[?y:(2 OR 3)]->(1)";
    //std::string s = "(8)-[?y:(2 OR 3)]->(?z)";
    //std::string s = "(?x)-[?y:(2 OR 3)]->(1)";
    //std::string s = "(?x)-[?y:(2 OR 3)]->(?z)";
    std::string s = "(6)-[?y:(2 OR 3)]->(?z), (?v)-[?w:(NOT 2 AND NOT 1)]->(?z)";
    //std::string s = "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:(NOT 2 AND NOT 1)]->(?z)";

    std::cout << "Querying: " << s << std::endl;
    typedef ring::ring_pg<> ring_type;

    ring_type ring;
    sdsl::load_from_file(ring, path);

    auto query = ring::query::pg_query(s);
    typedef ring::ltj_algorithm_pg<> algorithm_type;
    algorithm_type ltj(&query.patterns, &ring);
    ring::ltj_algorithm_pg<>::results_type res;
    ltj.join_v3(res, 0, 0);
    //std::string s = "NOT (5 OR 3)"; //it cannot work
    //std::string s = "(1 OR 5 OR NOT 3)";



}
