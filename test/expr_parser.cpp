//
// Created by adrian on 23/10/25.
//


#include "../include/query/expr_parser.hpp"

#include <iostream>
#include <ostream>
#include <string>

int main() {

    std::string s = "(1 OR 2 OR NOT 4) AND NOT 3 AND NOT 2 AND 5";
     //std::string s = "NOT (5 OR 3)"; //it cannot work
    //std::string s = "(1 OR 5 OR NOT 3)";
    auto e = ring::query::expr_parser::parse(s);
    e.print();


}
