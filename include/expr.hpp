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
    enum enum_expr_type { LAB, NEG, AND, OR };

    class expr_parser {
    public:
        typedef uint32_t label_type;
        typedef struct expr {
            enum_expr_type type;
            std::vector<expr> args;
            uint32_t label = 0; //only for LAB and NEG

            void print() {
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
                }
            }

        } expr_type;

    private:
        const std::string &m_s;
        size_t m_pos;

        void skip_ws() { while (m_pos < m_s.size() && isspace(m_s[m_pos])) ++m_pos; }

        bool match(const std::string &tok) {
            skip_ws();
            size_t len = tok.size();
            if (m_s.substr(m_pos, len) == tok) {
                m_pos += len;
                return true;
            }
            return false;
        }

        bool peek(const std::string &tok) {
            skip_ws();
            return m_s.substr(m_pos, tok.size()) == tok;
        }

        bool end() {
            skip_ws();
            return m_pos >= m_s.size();
        }

        expr_type parse_expr() {
            skip_ws();
            return parse_or();
        }

        expr_type parse_or() {
            std::vector<expr> children;
            children.push_back(parse_and());
            skip_ws();
            while (match("OR")) {
                children.push_back(parse_and());
                skip_ws();
            }
            if (children.size() == 1) return std::move(children[0]);
            expr_type e;
            e.type = OR;
            e.args = std::move(children);
            return e;
        }

        expr_type parse_and() {
            std::vector<expr> children;
            children.push_back(parse_factor());
            skip_ws();
            while (match("AND")) {
                children.push_back(parse_factor());
                skip_ws();
            }
            if (children.size() == 1) return std::move(children[0]);
            expr_type e;
            e.type = AND;
            e.args = std::move(children);
            return e;
        }

        expr_type parse_factor() {
            skip_ws();
            if (match("(")) {
                expr_type e = parse_expr();
                skip_ws();
                if (!match(")")) throw std::runtime_error("Missing ')' in expression");
                return e;
            } else if (match("NOT")) {
                //NOT cannot contain more than one element
                /*expr e;
                e.type = NEG;
                e.args.push_back(parse_factor());
                return e;*/
                return parse_not();
            } else {
                return parse_label();
            }
        }

        expr_type parse_not() {
            skip_ws();
            size_t start = m_pos;
            while (m_pos < m_s.size() && isdigit(m_s[m_pos])) ++m_pos;
            if (start == m_pos) throw std::runtime_error("Expected number");
            uint32_t lab = std::stoul(m_s.substr(start, m_pos - start));
            expr_type e;
            e.type = NEG;
            e.label = lab;
            return e;
        }

        expr_type parse_label() {
            skip_ws();
            size_t start = m_pos;
            while (m_pos < m_s.size() && isdigit(m_s[m_pos])) ++m_pos;
            if (start == m_pos) throw std::runtime_error("Expected number");
            uint32_t lab = std::stoul(m_s.substr(start, m_pos - start));
            expr_type e;
            e.type = LAB;
            e.label = lab;
            return e;
        }

    public:
        explicit expr_parser(const std::string &str) : m_s(str), m_pos(0) {}

        expr_type parse() {
            return parse_expr();
        }
    };
}

#endif //EXPR_HPP
