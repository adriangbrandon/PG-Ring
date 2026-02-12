//
// Created by adrian on 24/10/25.
//

#include <query/query_parser.hpp>

int main() {

    //std::string s = "(?1:5)-[?3:2]->(3), (3)-[?4]->(?5:(5 OR NOT 2)), (?5)-[:(NOT 6)]->(?1)";
    //std::string s = "(?1:5)-[?3:2]->(3), (3)-[?4]->(?5:(5 OR NOT 2)), (?5)-[:(NOT 6)]->(?1) WHERE ?1.1 = 10 AND ?5.2 >= 20";
    std::string s = "(?v1)-[?e0:124]->(?v2) WHERE (?v1.183 = 21275344520.) AND (?v1.176 IS NOT NULL)";
    //std::string s = "NOT (5 OR 3)"; //it cannot work
    //std::string s = "(1 OR 5 OR NOT 3)";
    auto e = ring::query::pg_query(s);
    for (const auto &p :  e.patterns) {
        std::cout << "Pattern:" << std::endl;
        ring::query::triple_parser::print(p);
    }
    e.where.print();


}