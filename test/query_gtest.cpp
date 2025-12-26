#include <gtest/gtest.h>
#include "ring_pg.hpp"
#include "ltj_algorithm_pg.hpp"
#include "results_collector_test.hpp"
#include "test/query_checker.hpp"

// Variables globales para el dataset y el ring
std::vector<spo_triple> dataset_vec;
std::vector<std::vector<uint32_t>> node_labels;
std::vector<std::vector<std::pair<uint32_t, uint32_t>>> numeric_properties;
std::vector<std::vector<std::pair<uint32_t, std::string>>> string_properties;
std::unordered_map<uint32_t, std::pair<bool, uint32_t>> node_properties;
std::unordered_map<uint32_t, std::pair<bool, uint32_t>> edge_properties;
//typedef ring::ring_pg<> ring_type;
ring::ring_pg<> graph;

// Inicialización antes de los tests
class QueryTestEnvironment : public ::testing::Environment {

private:

    void read_input(vector<spo_triple> &vec, const std::string &triples_path, const std::string &label2nodes_path,
                    const std::string &base_nprop, const std::string &base_eprop) {
        {
            std::ifstream ifs(triples_path);
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


        {
            std::ifstream ifs(label2nodes_path);
            uint64_t label, size, node, max_node = 0;
            std::vector<std::vector<uint32_t>> label_nodes;
            do {
                ifs >> label;
                if(ifs.eof()) break;
                ifs >> size;
                std::vector<uint32_t> nodes;
                for (uint32_t i = 0; i < size; i++) {
                    ifs >> node;
                    if (node > max_node) max_node = node;
                    nodes.push_back(node);
                }
                label_nodes.push_back(nodes);
            } while (true);

            node_labels.resize(max_node);
            for (uint32_t l_i = 0; l_i < label_nodes.size(); l_i++) {
                for (const auto &n : label_nodes[l_i]) {
                    node_labels[n-1].push_back(l_i+1);
                }
            }
        }

        {
            uint32_t prop_id = 1;
            uint32_t node_id;
            std::string value;
            do {
                std::string file = base_nprop + std::to_string(prop_id);
                std::ifstream ifs(file);
                std::vector<std::pair<uint32_t, std::string>> values;
                if (!ifs.good()) break;
                do {
                    ifs >> node_id;
                    if(ifs.eof()) break;
                    std::getline(ifs, value);
                    value = value.substr(1); // remove leading space
                    if (!value.empty() && value.back() == '\r') value.pop_back();
                    values.emplace_back(node_id, value);
                } while (true);
                ifs.close();
                if (::ring::util::is_number(value)) {
                    std::vector<std::pair<uint32_t, uint32_t>> int_values;
                    for (const auto &n : values) {
                        int_values.emplace_back(n.first, std::stoi(n.second));
                    }
                    numeric_properties.emplace_back(int_values);
                    node_properties.insert({prop_id, {true, numeric_properties.size()-1}});
                }else {
                    string_properties.emplace_back(values);
                    node_properties.insert({prop_id, {false, string_properties.size()-1}});
                }
                ++prop_id;
            } while (true);
        }

        {
            uint32_t prop_id = 1;
            uint32_t edge_id;
            std::string value;
            do {
                std::string file = base_eprop + std::to_string(prop_id);
                std::ifstream ifs(file);
                std::vector<std::pair<uint32_t, std::string>> values;
                if (!ifs.good()) break;
                do {
                    ifs >> edge_id;
                    if(ifs.eof()) break;
                    std::getline(ifs, value);
                    value = value.substr(1); // remove leading space
                    if (!value.empty() && value.back() == '\r') value.pop_back();
                    values.emplace_back(edge_id, value);
                } while (true);
                ifs.close();
                if (::ring::util::is_number(value)) {
                    std::vector<std::pair<uint32_t, uint32_t>> int_values;
                    for (const auto &n : values) {
                        int_values.emplace_back(n.first, std::stoi(n.second));
                    }
                    numeric_properties.emplace_back(int_values);
                    edge_properties.insert({prop_id, {true, numeric_properties.size()-1}});
                }else {
                    string_properties.emplace_back(values);
                    edge_properties.insert({prop_id, {false, string_properties.size()-1}});
                }
                ++prop_id;
            } while (true);
        }
    }

public:
    void SetUp() override {
        // Cambia estos paths según lo necesites
        std::string dataset = "/mnt/movies/real/movies";
        std::string triples_path = dataset + ".triples";
        std::string label2nodes_path = dataset + ".label2nodes";
        std::string index_path = dataset + ".ring.pg";
        std::string base_nprop = dataset + ".nprop2values.";
        std::string base_eprop = dataset + ".eprop2values.";
        read_input(dataset_vec, triples_path, label2nodes_path, base_nprop, base_eprop);
        sdsl::load_from_file(graph, index_path);
    }
};

// Registrar el entorno de test
::testing::Environment* const query_env = ::testing::AddGlobalTestEnvironment(new QueryTestEnvironment);

void run_query_test(const std::string& s) {
    auto query = ring::query::pg_query(s);
    typedef ring::ltj_algorithm_pg<::util::results_collector_test<std::vector<uint64_t>>> algorithm_type;
    typedef algorithm_type::tuple_type tuple_type;
    algorithm_type ltj(&query, &graph);
    ::util::results_collector_test<tuple_type> res;
    ltj.join_v3(res, 0, 0);

    ring::test::query_checker q_c(&dataset_vec, &node_labels, &numeric_properties, &string_properties, &node_properties, &edge_properties, s);
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
        algorithm_type ltj(&query, &graph);
        ::util::results_collector_test<tuple_type> res;
        ltj.join_v3(res, 0, 0);

        ring::test::query_checker q_c(&dataset_vec, &node_labels, &numeric_properties, &string_properties, &node_properties, &edge_properties, s);
        q_c.run();
        res.sort(); q_c.sort();

        std::cout << "Query: " << s << "\n";
        std::cout << "Obtained results: " << res.size() << std::endl;

        // Print results
        for (auto i = 0; i < res.size(); ++i) {
            for (auto j = 0; j < res.results[i].size(); ++j) {
                std::cout << res.results[i][j] << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "Expected results: " << q_c.res.size() << std::endl;
        // Print results
        for (auto i = 0; i < q_c.res.size(); ++i) {
            for (auto j = 0; j < q_c.res[i].size(); ++j) {
                std::cout << q_c.res[i][j] << " ";
            }
            std::cout << std::endl;
        }


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

TEST(QueryTest, NodeLabels)
{
    std::vector<std::string> queries = {
        "(?k:2)-[?y]->(1)",
        "(?k:2)-[?y]->(1), (?k:2)-[?w]->(12)",
        "(?k:2)-[?y]->(1), (?k:2)-[?w]->(?z:1)",
        "(?k:2)-[?y:1]->(1), (?k:2)-[?w]->(?z:1)",
        "(?k:2)-[?y:1]->(1), (?k:2)-[?w]->(?z:(NOT 2))",
        "(?k:2)-[?y:1]->(1), (?k:2)-[?w]->(?z:(1 OR 2))",
        "(?k:2)-[?y:1]->(?z:1), (?k:2)-[?w]->(?z:1)",
        "(2)-[?y:1]->(?z:1), (?x)-[?w]->(?z:1)",
        "(?k:2)-[?y:1]->(1), (?k:2)-[?w]->(?z:(1 AND 2))",
        "(2)-[?y:1]->(?z:1), (?x)-[?w]->(?z:1), (?x)-[?v:2]->(?a:1)"
    };
    run_queries_test(queries);
}

TEST(QueryTest, NodeProperties)
{
    std::vector<std::string> queries = {
        "(?k:2)-[?y]->(?z) WHERE (?k.5 = 1964)",
        "(?k:2)-[?y]->(1) WHERE (?k.5 >= 1964)",
        "(?k:2)-[?y]->(1) WHERE (?k.5 >= 1964) AND (?k.5 <= 1967)",
        "(?k:2)-[?y]->(1) WHERE (?k.5 >= 1964) AND (?k.5 != 1967)",
        "(?k:2)-[?y]->(1), (?j:2)-[?w]->(30) WHERE (?j.5 > ?k.5)",
        "(?mx:2)-[?y]->(1), (?tg:2)-[?w]->(30) WHERE (?tg.5 > ?mx.5) AND (?tg.5 != 1962)",
        "(?tg:2)-[?w]->(30), (?mx:2)-[?y]->(1) WHERE (?tg.5 > ?mx.5) AND (?tg.5 != 1962)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?a1.5 != 1962)"
    };
    run_queries_test(queries);
}


/*TEST(QueryTest, EdgeProperties) {
    std::vector<std::string> queries = {
        "(?k:2)-[?y]->(?z) WHERE (?k.5 = 1964) AND (?y.1 = 2031963965)",
        "(?k:2)-[?y]->(?z) WHERE (?k.5 >= 1964) AND (?y.1 = 2031963965)",
        "(?k:2)-[?y:1]->(?z) WHERE (?y.1 = 2031963965)",
        "(?k:2)-[?y:1]->(1) WHERE (?y.1 = 2031963965)",
        "(?k:2)-[?y:(1 OR NOT 1)]->(1) WHERE (?y.1 = 2031963965)",
         "(2)-[?y:(1 OR NOT 1)]->(?z) WHERE (?y.1 = 2031963965)",
        "(?k:2)-[?y]->(1) WHERE (?y.1 = 2031963965)",
         "(2)-[?y:1]->(?z) WHERE (?y.1 = 2031963965)",
         "(2)-[?y]->(?z) WHERE (?y.1 = 2031963965)",
         "(2)-[?y]->(1) WHERE (?y.1 = 2031963965)",
         "(2)-[?y:1]->(1) WHERE (?y.1 = 2031963965)",
         "(?k)-[?y]->(1) WHERE (?y.1 = 2031963965)",
         "(?k)-[?y:1]->(1) WHERE (?y.1 = 2031963965)",
         "(?k)-[?y:(1 OR NOT 1)]->(1) WHERE (?y.1 = 2031963965)",
         "(?k)-[?y:(1 OR NOT 1)]->(1) WHERE (?y.1 >= 2031963965)",
         "(?k)-[?y:1]->(1) WHERE (?y.1 >= 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < 2031963965)",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != 2031963965)",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = 2031963965)",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= 2031963965)",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < 2031963965)",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != 2031963965)",


    };
    run_queries_test(queries);
}*/

TEST(QueryTest, EdgeProperties) {
    std::vector<std::string> queries = {
        "(?k:2)-[?y]->(?z) WHERE (?k.5 = 1964) AND (?y.1 = \"Neo\")",
        "(?k:2)-[?y]->(?z) WHERE (?k.5 >= 1964) AND (?y.1 = \"Neo\")",
        "(?k:2)-[?y:1]->(?z) WHERE (?y.1 = \"Neo\")",
        "(?k:2)-[?y:1]->(1) WHERE (?y.1 = \"Neo\")",
        "(?k:2)-[?y:(1 OR NOT 1)]->(1) WHERE (?y.1 = \"Neo\")",
         "(2)-[?y:(1 OR NOT 1)]->(?z) WHERE (?y.1 = \"Neo\")",
        "(?k:2)-[?y]->(1) WHERE (?y.1 = \"Neo\")",
         "(2)-[?y:1]->(?z) WHERE (?y.1 = \"Neo\")",
         "(2)-[?y]->(?z) WHERE (?y.1 = \"Neo\")",
         "(2)-[?y]->(1) WHERE (?y.1 = \"Neo\")",
         "(2)-[?y:1]->(1) WHERE (?y.1 = \"Neo\")",
         "(?k)-[?y]->(1) WHERE (?y.1 = \"Neo\")",
         "(?k)-[?y:1]->(1) WHERE (?y.1 = \"Neo\")",
         "(?k)-[?y:(1 OR NOT 1)]->(1) WHERE (?y.1 = \"Neo\")",
         "(?k)-[?y:(1 OR NOT 1)]->(1) WHERE (?y.1 >= \"Neo\")",
         "(?k)-[?y:1]->(1) WHERE (?y.1 >= \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < \"Neo\")",
        "(?a1:2)-[?w]->(?m), (?a2:2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != \"Neo\")",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = \"Neo\")",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= \"Neo\")",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 < \"Neo\")",
        "(?a1)-[?w]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != \"Neo\")"
    };
    run_queries_test(queries);
}

TEST(QueryTest, EdgeProperties2) {
    std::vector<std::string> queries = {
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 <= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:1]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 <= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 <= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(1 OR NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 <= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 2)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 <= ?w.1)",
        "(?a1)-[?w:1]->(?m), (?a2)-[?y:(NOT 1)]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != ?w.1)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 = ?w.1)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 >= ?w.1)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 <= ?w.1)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1.5 = ?a2.5) AND (?y.1 != ?w.1)"
    };
    run_queries_test(queries);
}

TEST(QueryTest, CompIds)
{
    std::vector<std::string> queries = {
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?y = ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?y <= ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?y >= ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?y < ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?y > ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 != ?a2)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 = ?a2)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 <= ?a2)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 >= ?a2)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 < ?a2)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 > ?a2)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 != ?a2) AND (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m) WHERE (?a1 < ?a2) AND (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m), (?m)-[?z]->(?k) WHERE (?a1 = ?a2) AND (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m), (?m)-[?z]->(?k) WHERE (?a1 != ?a2) AND (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m), (?m)-[?z]->(?k) WHERE (?a1 > ?a2) AND (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m), (?m)-[?z]->(?k) WHERE (?a1 < ?a2) AND (?y != ?w)",
        "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m), (?m)-[?z]->(?k) WHERE (?a1 <= ?a2) AND (?y != ?w)"
    };
    run_queries_test(queries);
}

TEST(QueryTest, Error)
{
     std::vector<std::string> queries = {
         "(?a1)-[?w]->(?m), (?a2)-[?y]->(?m), (?m)-[?z]->(?k) WHERE (?a1 != ?a2) AND (?y != ?w)",
    };
    run_queries_test(queries);
}