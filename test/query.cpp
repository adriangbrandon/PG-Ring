//
// Created by adrian on 29/10/25.
//
#include <string>
#include <ring_pg.hpp>
#include <query/query_parser.hpp>
#include <ltj_algorithm_pg.hpp>

#include <results_collector_test.hpp>
#include "test/query_checker.hpp"

void read_input(vector<spo_triple> &vec, const std::string &input_path) {
    std::ifstream ifs(input_path);
    uint64_t s, p , o;
    do {
        ifs >> s >> p >> o;
        if(ifs.eof()) break;
        vec.push_back(spo_triple(s, p, o));
    } while (true);

    sort(vec.begin(), vec.end(), [](const spo_triple& a, const spo_triple& b) {
                                    if (std::get<1>(a) == std::get<1>(b)) {
                                        if (std::get<2>(a) == std::get<2>(b)) {
                                            return std::get<0>(a) < std::get<0>(b);
                                        };
                                        return std::get<2>(a) < std::get<2>(b);
                                    }
                                    return std::get<1>(a) < std::get<1>(b);
                        });
}







int main(int argc, char* argv[]) {


    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <index> <input>" << std::endl;
        return 1;
    }

    std::string index_path = argv[1];
    std::string input_path = argv[2];

    vector<spo_triple> dataset;
    read_input(dataset, input_path);



    typedef ring::ring_pg<> ring_type;

    ring_type ring;
    sdsl::load_from_file(ring, index_path);

    //std::string s = "(8)-[?y:(2 OR 3)]->(1)";
    //std::string s = "(8)-[?y:(2 OR 3)]->(?z)";
    //std::string s = "(?x)-[?y:(2 OR 3)]->(1)";
    //std::string s = "(?x)-[?y:(2 OR 3)]->(?z)";
    //std::string s = "(6)-[?y:(2 OR 3)]->(?z), (?v)-[?w:(NOT 2 AND NOT 1)]->(?z)";
    //std::string s = "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:(NOT 2 AND NOT 1)]->(?z)";
    //std::string s = "(?k)-[?y:((NOT 2) OR 3)]->(?z)";
    //Normal iterator
    //std::string s = "(?k)-[?y]->(?z)";
    //std::string s = "(?k)-[?y:3]->(?z)";
    //std::string s = "(?k)-[?y:1]->(?z)";
    //std::string s = "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:3]->(?z)";
    std::string s = "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w]->(?z)";
    std::cout << "Querying: " << s << std::endl;
    auto query = ring::query::pg_query(s);
    typedef ring::ltj_algorithm_pg<util::results_collector_test<std::vector<uint64_t>>> algorithm_type;
    typedef algorithm_type::tuple_type tuple_type;
    algorithm_type ltj(&query.patterns, &ring);
    util::results_collector_test<tuple_type> res;
    ltj.join_v3(res, 0, 0);
    std::cout << res.size() << std::endl;

    ring::test::query_checker q_c(&dataset, s);
    q_c.run();
    res.sort(); q_c.sort();
    std::cout << "Checked results: " << q_c.res.size() << std::endl;

    assert(q_c.res.size() == res.size());

    for (uint64_t i = 0; i < q_c.res.size(); ++i) {
        for (uint64_t j = 0; j < q_c.res[i].size(); ++j) {
            assert(q_c.res[i][j] == res.results[i][j]);
        }
    }


    //std::string s = "NOT (5 OR 3)"; //it cannot work
    //std::string s = "(1 OR 5 OR NOT 3)";



}
