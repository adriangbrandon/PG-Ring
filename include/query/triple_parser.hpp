/*
 * triple_pattern.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#ifndef RING_TRIPLE_PATTERN_PG_HPP
#define RING_TRIPLE_PATTERN_PG_HPP
#include <cstdint>

#include <query/expr_parser.hpp>
#include <unordered_map>


namespace ring {

    namespace query {

        class triple_parser {

        public:
            typedef struct node {
                uint32_t const_value = 0;
                uint32_t var_value = 0;
                expr_parser::expr_type expr;

                bool is_var() const {
                    return var_value != 0;
                }

                bool is_empty() const {
                    return expr.type == EMPTY;
                }

            } node_type;

            typedef struct edge {
                uint32_t const_value = 0;
                uint32_t var_value = 0;
                expr_parser::expr_type expr;

                bool is_var() const {
                    return var_value != 0;
                }

                bool is_label() const {
                    return expr.type == LAB;
                }

                bool is_empty() const {
                    return expr.type == EMPTY;
                }

                uint32_t get_label() const {
                    return expr.label;
                }




            } edge_type;

            typedef struct triple {
                node subj;
                edge edge;
                node obj;
            } triple_type;

        private:

            static void skip_ws(size_t& p, const std::string& str) { while (p < str.size() && isspace(str[p])) ++p; }


            static uint32_t parse_constant(size_t& p, const std::string& str) {
                skip_ws(p, str);
                std::string val;
                while (p < str.size() && isdigit(str[p])) val += str[p++];
                if (val.empty()) throw std::runtime_error("Expected constant value (digits only)");
                return static_cast<uint32_t>(std::stoul(val));
            }

            static uint8_t parse_variable(size_t& p, const std::string& str, std::unordered_map<std::string, uint8_t> &ht) {
                ++p; //skip ?
                std::string var;
                while (p < str.size() && (isalnum(str[p]) || str[p] == '_')) var += str[p++];
                if (var.empty()) return 0;
                auto it = ht.find(var);
                if (it != ht.end()) {
                    return it->second;
                } else {
                    uint8_t id = ht.size() + 1;
                    ht[var] = id;
                    return id;
                }
            }

            static node_type parse_node(size_t& p, const std::string& str, std::unordered_map<std::string, std::uint8_t> &ht) {
                node_type n;
                skip_ws(p, str);
                if (str[p] != '(') throw std::runtime_error("Expected '('");
                ++p;
                skip_ws(p, str);
                // Variable
                if ((str[p] == '?')) {
                    n.var_value = parse_variable(p, str, ht);
                    n.const_value = 0;
                }else if (str[p] != ':') {
                    n.var_value = 0;
                    n.const_value = parse_constant(p, str);
                }

                skip_ws(p, str);
                // Label or expression
                if (p < str.size() && str[p] == ':') {
                    ++p;
                    skip_ws(p, str);
                    // It can be an expression between parentheses or a single word
                    std::string expr_str;
                    if (p < str.size() && str[p] == '(') {
                        int par = 1; expr_str += str[p++];
                        while (p < str.size() && par > 0) {
                            expr_str += str[p];
                            if (str[p] == '(') par++;
                            if (str[p] == ')') par--;
                            ++p;
                        }
                    } else {
                        while (p < str.size() && (isalnum(str[p]) || str[p] == '_')) expr_str += str[p++];
                    }
                    n.expr = expr_parser::parse(expr_str);
                }
                skip_ws(p, str);
                if (str[p] != ')') throw std::runtime_error("Expected ')' in node");
                ++p;
                return n;
            }

            static edge_type parse_edge(size_t& p, const std::string& str, std::unordered_map<std::string, std::uint8_t> &ht) {
                edge_type e;
                skip_ws(p, str);
                if (str[p] != '[') throw std::runtime_error("Expected '['");
                ++p;
                skip_ws(p, str);
                // Variable
                if ((str[p] == '?')) {
                    e.var_value = parse_variable(p, str, ht);
                    e.const_value = 0;
                }else if (str[p] != ':') {
                    e.var_value = 0;
                    e.const_value = parse_constant(p, str);
                }
                skip_ws(p, str);
                // Etiqueta o expresión
                if (p < str.size() && str[p] == ':') {
                    ++p;
                    skip_ws(p, str);
                    std::string expr_str;
                    if (p < str.size() && str[p] == '(') {
                        int par = 1; expr_str += str[p++];
                        while (p < str.size() && par > 0) {
                            expr_str += str[p];
                            if (str[p] == '(') par++;
                            if (str[p] == ')') par--;
                            ++p;
                        }
                    } else {
                        while (p < str.size() && (isalnum(str[p]) || str[p] == '_')) expr_str += str[p++];
                    }
                    e.expr = expr_parser::parse(expr_str);
                }
                skip_ws(p, str);
                if (str[p] != ']') throw std::runtime_error("Expected ']' in edge");
                ++p;
                return e;
            }

        public:
            static triple_type parse(const std::string& str, std::unordered_map<std::string, std::uint8_t> &ht) {
                // Ejemplo de str: (a:Person)-[p:(Direct OR ACT)]->(b)
                triple_type t;
                size_t pos = 0;
                t.subj = parse_node(pos, str, ht);
                skip_ws(pos, str);
                if (str.substr(pos, 1) != "-") throw std::runtime_error("Expected '-' after subject");
                ++pos;
                skip_ws(pos, str);
                t.edge = parse_edge(pos, str, ht);
                skip_ws(pos, str);
                if (str.substr(pos, 2) != "->") throw std::runtime_error("Expected '->' after edge");
                pos += 2;
                skip_ws(pos, str);
                t.obj = parse_node(pos, str, ht);
                return t;
            }

            static void print(const triple_type& t) {
                std::cout << "(";
                if (t.subj.var_value)
                    std::cout << "?" << static_cast<int>(t.subj.var_value);
                else if (t.subj.const_value)
                    std::cout << t.subj.const_value;
                if (t.subj.expr.type != EMPTY) {
                    std::cout << ":";
                    t.subj.expr.print();
                }
                std::cout << ")-";
                std::cout << "[";
                if (t.edge.var_value)
                    std::cout << "?" << static_cast<int>(t.edge.var_value);
                else if (t.edge.const_value)
                    std::cout << t.edge.const_value;
                if (t.edge.expr.type != EMPTY) {
                    std::cout << ":";
                    t.edge.expr.print();
                }
                std::cout << "]->(";
                if (t.obj.var_value)
                    std::cout << "?" << static_cast<int>(t.obj.var_value);
                else if (t.obj.const_value)
                    std::cout << t.obj.const_value;
                if (t.obj.expr.type != EMPTY) {
                    std::cout << ":";
                    t.obj.expr.print();
                }
                std::cout << ")" << std::endl;
            }

        };






    }

}


#endif //RING_TRIPLE_PATTERN_HPP
