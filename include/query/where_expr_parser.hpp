//
// Created by adrian on 13/11/25.
//

#ifndef WHERE_PARSER_HPP
#define WHERE_PARSER_HPP
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

namespace ring {
    namespace query {
        enum enum_expr_property_type { EQ, NEQ, GT, GE, ST, SE, WAND, WOR };
        std::array<enum_expr_property_type, 6> opposite_expr_property {EQ, NEQ, ST, SE, GT, GE};

        class where_expr_parser {
        public:
            typedef uint32_t property_type;
            typedef uint32_t value_type;

            typedef struct expr {
                enum_expr_property_type type = WAND;
                //used in AND and OR
                std::vector<expr> args;
                //used in comparisons
                std::array<bool, 2> is_var;
                std::array<value_type, 2> values;
                std::array<property_type, 2> property_values = {0, 0};


                void print() const {
                    switch (type) {
                        case EQ:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " = ";
                            if (is_var[1]) std::cout << "?";
                            std::cout << values[1];
                            if (property_values[1]) std::cout << "." << property_values[1];
                            std::cout << ")";
                            break;
                        case NEQ:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " != ";
                            if (is_var[1]) std::cout << "?";
                            std::cout << values[1];
                            if (property_values[1]) std::cout << "." << property_values[1];
                            std::cout << ")";
                            break;
                        case GT:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " > ";
                            if (is_var[1]) std::cout << "?";
                            std::cout << values[1];
                            if (property_values[1]) std::cout << "." << property_values[1];
                            std::cout << ")";
                            break;
                        case GE:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " >= ";
                            if (is_var[1]) std::cout << "?";
                            std::cout << values[1];
                            if (property_values[1]) std::cout << "." << property_values[1];
                            std::cout << ")";
                            break;
                        case ST:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " < ";
                            if (is_var[1]) std::cout << "?";
                            std::cout << values[1];
                            if (property_values[1]) std::cout << "." << property_values[1];
                            std::cout << ")";
                            break;
                        case SE:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " <= ";
                            if (is_var[1]) std::cout << "?";
                            std::cout << values[1];
                            if (property_values[1]) std::cout << "." << property_values[1];
                            std::cout << ")";
                            break;
                        case WAND:
                            std::cout << "(";
                            for (size_t i = 0; i < args.size(); ++i) {
                                if (i > 0) std::cout << " AND ";
                                args[i].print();
                            }
                            std::cout << ")";
                            break;
                        case WOR:
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
            } expr_property_type;

        private:
            static void skip_ws(size_t &pos, const std::string &s) {
                while (pos < s.size() && isspace(s[pos])) ++pos;
            }

            static bool match(const std::string &tok, size_t &pos, const std::string &s) {
                skip_ws(pos, s);
                size_t len = tok.size();
                if (s.substr(pos, len) == tok) {
                    pos += len;
                    return true;
                }
                return false;
            }

            static expr_property_type parse_expr(size_t &pos, const std::string &s,
                                                 std::unordered_map<std::string, std::uint8_t> &ht) {
                skip_ws(pos, s);
                return parse_or(pos, s, ht);
            }

            static expr_property_type parse_and(size_t &pos, const std::string &s,
                                                std::unordered_map<std::string, std::uint8_t> &ht) {
                std::vector<expr_property_type> children;
                children.push_back(parse_factor(pos, s, ht));
                skip_ws(pos, s);
                while (match("AND", pos, s)) {
                    children.push_back(parse_factor(pos, s, ht));
                    skip_ws(pos, s);
                }
                if (children.size() == 1) return std::move(children[0]);
                expr_property_type e;
                e.type = WAND;
                e.args = std::move(children);
                return e;
            }

            static expr_property_type parse_or(size_t &pos, const std::string &s,
                                               std::unordered_map<std::string, std::uint8_t> &ht) {
                std::vector<expr_property_type> children;
                children.push_back(parse_and(pos, s, ht));
                skip_ws(pos, s);
                while (match("OR", pos, s)) {
                    children.push_back(parse_and(pos, s, ht));
                    skip_ws(pos, s);
                }
                if (children.size() == 1) return std::move(children[0]);
                expr_property_type e;
                e.type = WOR;
                e.args = std::move(children);
                return e;
            }

            static expr_property_type parse_factor(size_t &pos, const std::string &s,
                                                   std::unordered_map<std::string, std::uint8_t> &ht) {
                skip_ws(pos, s);
                if (match("(", pos, s)) {
                    expr_property_type e = parse_expr(pos, s, ht);
                    skip_ws(pos, s);
                    if (!match(")", pos, s)) throw std::runtime_error("Missing ')' in expression");
                    return e;
                } else {
                    return parse_comp(pos, s, ht);
                }
            }

            static void parse_operand(size_t &pos, const std::string &s, bool &is_var, uint32_t &value, uint32_t &prop,
                                      std::unordered_map<std::string, std::uint8_t> &ht) {
                is_var = false;
                value = 0;
                prop = 0;
                if (s[pos] == '?') {
                    is_var = true;
                    ++pos;
                    size_t start = pos;
                    while (pos < s.size() && s[pos] != '.' && !isspace(s[pos])) ++pos;
                    //value = std::stoul(s.substr(start, pos - start));
                    value = ht[s.substr(start, pos - start)];
                    if (pos < s.size() && s[pos] == '.') {
                        ++pos;
                        size_t pstart = pos;
                        while (pos < s.size() && isdigit(s[pos])) ++pos;
                        prop = std::stoul(s.substr(pstart, pos - pstart));
                    }
                } else {
                    is_var = false;
                    size_t start = pos;
                    while (pos < s.size() && isdigit(s[pos])) ++pos;
                    value = std::stoul(s.substr(start, pos - start));
                }
            }

            static expr_property_type parse_comp(size_t &pos, const std::string &s,
                                                 std::unordered_map<std::string, std::uint8_t> &ht) {
                skip_ws(pos, s);
                expr_property_type e;
                parse_operand(pos, s, e.is_var[0], e.values[0], e.property_values[0], ht);
                skip_ws(pos, s);
                //parse operator
                if (match(">=", pos, s)) e.type = GE;
                else if (match("<=", pos, s)) e.type = SE;
                else if (match("!=", pos, s)) e.type = NEQ;
                else if (match("=", pos, s)) e.type = EQ;
                else if (match(">", pos, s)) e.type = GT;
                else if (match("<", pos, s)) e.type = ST;
                else throw std::runtime_error("Expected comparison operator");
                skip_ws(pos, s);
                parse_operand(pos, s, e.is_var[1], e.values[1], e.property_values[1], ht);
                return e;
            }

        public:
            static expr_property_type parse(const std::string &str, std::unordered_map<std::string, std::uint8_t> &ht) {
                size_t pos = 0;
                return parse_expr(pos, str, ht);
            }
        };
    };
}

#endif //WHERE_PARSER_HPP
