/*
* Copyright (C) 2026 Author removed for double-blind evaluation
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

#ifndef EXPR_HPP
#define EXPR_HPP
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ring {
    namespace query {

        enum enum_expr_label_type { EMPTY, LAB, NEG, AND, OR };

        class label_expr_parser {
        public:
            typedef uint32_t label_type;
            typedef struct expr {
                enum_expr_label_type type = EMPTY;
                std::vector<expr> args;
                uint32_t label = 0; //only for LAB and NEG
                std::string label_str; // temporal string before translation

                void print() const {
                    switch (type) {
                        case LAB:
                            std::cout << label;
                            break;
                        case NEG:
                            std::cout << "NOT(";
                            std::cout << label;
                            std::cout << ")";
                            break;
                        case AND:
                            std::cout << "(";
                            for (size_t i = 0; i < args.size(); ++i) {
                                if (i > 0) std::cout << " AND ";
                                args[i].print();
                            }
                            std::cout << ")";
                            break;
                        case OR:
                            std::cout << "(";
                            for (size_t i = 0; i < args.size(); ++i) {
                                if (i > 0) std::cout << " OR ";
                                args[i].print();
                            }
                            std::cout << ")";
                            break;
                        default:
                            std::cout << "EMPTY";
                    }
                }

            } expr_label_type;

        private:
            static void skip_ws(size_t& pos, const std::string& s) {
                while (pos < s.size() && isspace(s[pos])) ++pos;
            }

            static bool match(const std::string &tok, size_t& pos, const std::string& s) {
                skip_ws(pos, s);
                size_t len = tok.size();
                if (s.substr(pos, len) == tok) {
                    pos += len;
                    return true;
                }
                return false;
            }

            static expr_label_type parse_expr(size_t& pos, const std::string& s) {
                skip_ws(pos, s);
                return parse_or(pos, s);
            }

            static expr_label_type parse_or(size_t& pos, const std::string& s) {
                std::vector<expr_label_type> children;
                children.push_back(parse_and(pos, s));
                skip_ws(pos, s);
                while (match("OR", pos, s)) {
                    children.push_back(parse_and(pos, s));
                    skip_ws(pos, s);
                }
                if (children.size() == 1) return std::move(children[0]);
                expr_label_type e;
                e.type = OR;
                e.args = std::move(children);
                return e;
            }

            static expr_label_type parse_and(size_t& pos, const std::string& s) {
                std::vector<expr_label_type> children;
                children.push_back(parse_factor(pos, s));
                skip_ws(pos, s);
                while (match("AND", pos, s)) {
                    children.push_back(parse_factor(pos, s));
                    skip_ws(pos, s);
                }
                if (children.size() == 1) return std::move(children[0]);
                expr_label_type e;
                e.type = AND;
                e.args = std::move(children);
                return e;
            }

            static expr_label_type parse_factor(size_t& pos, const std::string& s) {
                skip_ws(pos, s);
                if (match("(", pos, s)) {
                    expr_label_type e = parse_expr(pos, s);
                    skip_ws(pos, s);
                    if (!match(")", pos, s)) throw std::runtime_error("Missing ')' in expression");
                    return e;
                } else if (match("NOT", pos, s)) {
                    return parse_label(pos, s, NEG);
                } else {
                    return parse_label(pos, s);
                }
            }

            static expr_label_type parse_label(size_t& pos, const std::string& s, enum_expr_label_type t = LAB) {
                skip_ws(pos, s);
                size_t start = pos;
                while (pos < s.size() && (isalnum(s[pos]) || s[pos] == '_')) ++pos;
                if (start == pos) throw std::runtime_error("Expected label");
                std::string lab_str = s.substr(start, pos - start);
                expr_label_type e;
                e.type = t;
                e.label_str = lab_str; // store string
                e.label = 0; // will be translated later
                return e;
            }

        public:

            static expr_label_type parse(const std::string& str) {
                size_t pos = 0;
                return parse_expr(pos, str);
            }
        };
    }
}
#endif //EXPR_HPP
