#include <gtest/gtest.h>
#include "ring_pg.hpp"
#include "ltj_algorithm_pg.hpp"
#include "results_collector_test.hpp"
#include "test/query_checker.hpp"

// Variables globales para el dataset y el ring
std::vector<spo_triple> dataset_vec;
//typedef ring::ring_pg<> ring_type;
ring::ring_pg<> graph;

// Inicialización antes de los tests
class QueryTestEnvironment : public ::testing::Environment {

private:

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

public:
    void SetUp() override {
        // Cambia estos paths según lo necesites
        std::string input_path = "/mnt/movies/real/movies.triples";
        std::string index_path = "/mnt/movies/real/movies.ring.pg";
        read_input(dataset_vec, input_path);
        sdsl::load_from_file(graph, index_path);
    }
};

// Registrar el entorno de test
::testing::Environment* const query_env = ::testing::AddGlobalTestEnvironment(new QueryTestEnvironment);

void run_query_test(const std::string& s) {
    auto query = ring::query::pg_query(s);
    typedef ring::ltj_algorithm_pg<::util::results_collector_test<std::vector<uint64_t>>> algorithm_type;
    typedef algorithm_type::tuple_type tuple_type;
    algorithm_type ltj(&query.patterns, &graph);
    ::util::results_collector_test<tuple_type> res;
    ltj.join_v3(res, 0, 0);

    ring::test::query_checker q_c(&dataset_vec, s);
    q_c.run();
    res.sort(); q_c.sort();

    ASSERT_EQ(q_c.res.size(), res.size());
    for (uint64_t i = 0; i < q_c.res.size(); ++i) {
        for (uint64_t j = 0; j < q_c.res[i].size(); ++j) {
            ASSERT_EQ(q_c.res[i][j], res.results[i][j]);
        }
    }
}

void run_queries_test(const std::vector<std::string>& queries) {
    typedef ring::ltj_algorithm_pg<::util::results_collector_test<std::vector<uint64_t>>> algorithm_type;
    typedef algorithm_type::tuple_type tuple_type;

    for (const auto& s : queries) {
        auto query = ring::query::pg_query(s);
        algorithm_type ltj(&query.patterns, &graph);
        ::util::results_collector_test<tuple_type> res;
        ltj.join_v3(res, 0, 0);

        ring::test::query_checker q_c(&dataset_vec, s);
        q_c.run();
        res.sort(); q_c.sort();

        std::cout << "Query: " << s << "\n";
        std::cout << "Obtained results: " << res.size() << std::endl;
        std::cout << "Expected results: " << q_c.res.size() << std::endl;

        ASSERT_EQ(q_c.res.size(), res.size()) << "Error in size. ";
        for (uint64_t i = 0; i < q_c.res.size(); ++i) {
            for (uint64_t j = 0; j < q_c.res[i].size(); ++j) {
                ASSERT_EQ(q_c.res[i][j], res.results[i][j]) << "Error in: i=" << i << " j=" << j;
            }
        }
    }
}

TEST(QueryTest, BasicIterator)
{
    std::vector<std::string> queries = {
        "(?a)-[?b]->(?c)",
        "(3)-[?b]->(?c)",
        "(?a)-[?b]->(10)",
        "(5)-[?b]->(11)"
    };
    run_queries_test(queries);
}

TEST(QueryTest, LabelIterator)
{
    std::vector<std::string> queries = {
        "(?a)-[?b:2]->(?c)",
        "(8)-[?y:3]->(?z)",
        "(?x)-[?y:1]->(1)",
        "(6)-[?y:2]->(1)"
    };
    run_queries_test(queries);
}

TEST(QueryTest, ExprIterator)
{
    std::vector<std::string> queries = {
        "(?a)-[?b:(1 OR 3)]->(?c)",
        "(?x)-[?y:(2 OR 3)]->(?z)",
        "(8)-[?y:(2 OR 3)]->(1)",
        "(8)-[?y:(2 OR 3)]->(?z)",
        "(?x)-[?y:(2 OR 3)]->(1)",
        "(?k)-[?y:((NOT 2) OR 3)]->(?z)"
    };
    run_queries_test(queries);
}

TEST(QueryTest, BGPs)
{
    std::vector<std::string> queries = {
        "(28)-[?y:(2 OR 3)]->(?z), (?v)-[?w:1]->(?z)",
        "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:1]->(?z)",
        "(?a)-[?b]->(?c), (?c)-[?d]->(?e)",
        "(6)-[?y:(2 OR 3)]->(?z), (?v)-[?w:(NOT 2 AND NOT 1)]->(?z)",
        "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:(NOT 2 AND NOT 1)]->(?z)",
        "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:1]->(?z), (?v)-[?u:(NOT 1)]->(?m)",
        "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:1]->(?z), (?v)-[?u:1]->(?m)"
    };
    run_queries_test(queries);
}

/*TEST(QueryTest, NodeLabels)
{
    std::vector<std::string> queries = {
        "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:1]->(?z)"
    };
    run_queries_test(queries);
}*/

TEST(QueryTest, Error)
{
    std::vector<std::string> queries = {
        "(?k)-[?y:(2 OR 3)]->(?z), (?v)-[?w:1]->(?z)"
    };
    run_queries_test(queries);
}