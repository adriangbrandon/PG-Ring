//
// Created by adrian on 23/10/25.
//

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
                while (pos < s.size() && isdigit(s[pos])) ++pos;
                if (start == pos) throw std::runtime_error("Expected number");
                uint32_t lab = std::stoul(s.substr(start, pos - start));
                expr_label_type e;
                e.type = t;
                e.label = lab;
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
