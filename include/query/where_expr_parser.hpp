//
// Created by adrian on 13/11/25.
//

#ifndef WHERE_PARSER_HPP
#define WHERE_PARSER_HPP
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "constant_utils.hpp"

namespace ring {
    namespace query {
        enum enum_comp_where_type { EQ, NEQ, GT, GE, ST, SE, ISNULL, ISNOTNULL, WAND, WOR };
        std::array<enum_comp_where_type, 8> opposite_comp_where {EQ, NEQ, ST, SE, GT, GE, ISNULL, ISNOTNULL};

        class where_expr_parser {
        public:
            typedef uint32_t property_type;
            typedef int64_t value_type;

            typedef struct expr {
                enum_comp_where_type type = WAND;
                //used in AND and OR
                std::vector<expr> args;
                //used in comparisons
                std::array<bool, 2> is_var;
                std::array<value_type, 2> values;
                std::array<std::string, 2> strs = {"", ""}; //strings to compare if any
                std::array<property_type, 2> property_values = {0, 0};

                bool has_property() const {
                    return (is_var[0] && property_values[0]) || (is_var[1] && property_values[1]) ;
                }

                bool is_string_comp(uint p) const {
                    return (!is_var[p] && strs[p].empty());
                }

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
                        case ISNULL:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " IS NULL )";
                            break;
                        case ISNOTNULL:
                            std::cout << "(";
                            if (is_var[0]) std::cout << "?";
                            std::cout << values[0];
                            if (property_values[0]) std::cout << "." << property_values[0];
                            std::cout << " IS NOT NULL )";
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

            static size_t find_closing(size_t pos, const std::string &s) {
                int depth = 1; // we are after the opening parenthesis
                for (size_t i = pos + 1; i < s.size(); ++i) {
                    if (s[i] == '(') depth++;
                    else if (s[i] == ')') depth--;
                    if (depth == 0) return i;
                }
                return 0; // not found
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

            // Detects if the current position is a comparison operator or closing parenthesis
            static bool is_comp_op(const std::string& s, size_t pos, std::string& op) {
                static const std::vector<std::string> ops = {">=", "<=", "!=", "=", ">", "<"};
                for (const auto& candidate : ops) {
                    if (s.compare(pos, candidate.size(), candidate) == 0) {
                        op = candidate;
                        return true;
                    }
                }
                return false;
            }

            static expr_property_type parse_comp(size_t &pos, const std::string &s,
                                                 std::unordered_map<std::string, std::uint8_t> &ht) {
                skip_ws(pos, s);
                static const std::vector<std::string> ops = {">=", "<=", "!=", "=", ">", "<", "IS NULL", "IS NOT NULL"};
                size_t op_pos = std::string::npos;
                std::string found_op;
                // Find the first comparison operator using find
                for (const auto& op : ops) {
                    size_t p = s.find(op, pos);
                    if (p != std::string::npos && (op_pos == std::string::npos || p < op_pos)) {
                        op_pos = p;
                        found_op = op;
                    }
                }
                if (op_pos == std::string::npos)
                    throw std::runtime_error("No comparison operator found in expression: " + s.substr(pos));

                expr_property_type e;
                // Assign comparison type
                if (found_op == ">=") e.type = GE;
                else if (found_op == "<=") e.type = SE;
                else if (found_op == "!=") e.type = NEQ;
                else if (found_op == "=") e.type = EQ;
                else if (found_op == ">") e.type = GT;
                else if (found_op == "<") e.type = ST;
                else if (found_op == "IS NULL") e.type = ISNULL;
                else if (found_op == "IS NOT NULL") e.type = ISNOTNULL;
                else throw std::runtime_error("Unrecognized comparison operator: " + found_op);

                // Extract the first operand (trim trailing spaces)
                std::string op1_str = s.substr(pos, op_pos - pos);
                op1_str.erase(op1_str.find_last_not_of(" \t\n\r") + 1);
                parse_operand(op1_str, e.is_var[0], e.values[0], e.strs[0], e.property_values[0], ht);
                pos = op_pos + found_op.size();
                // Extract the second operand (trim leading and trailing spaces)
                if (e.type < ISNULL) {
                    //size_t op2_start = op_pos + found_op.size();
                    size_t op2_start = pos;
                    size_t op2_end = find_closing(op2_start, s);
                    if (op2_end == std::string::npos) op2_end = s.size();
                    std::string op2_str = s.substr(op2_start, op2_end - op2_start);
                    op2_str.erase(0, op2_str.find_first_not_of(" \t\n\r"));
                    op2_str.erase(op2_str.find_last_not_of(" \t\n\r") + 1);
                    parse_operand(op2_str, e.is_var[1], e.values[1], e.strs[1], e.property_values[1], ht);
                    pos = op2_end;
                }else {
                    e.is_var[1] = false;
                    e.values[1] = 0;
                }
                return e;
            }

            // parse_operand on exact string
            static void parse_operand(const std::string &s, bool &is_var, int64_t &value, std::string &str, uint32_t &prop,
                                      std::unordered_map<std::string, std::uint8_t> &ht) {
                is_var = false;
                value = 0;
                prop = 0;
                str.clear();
                size_t pos = 0;
                while (pos < s.size() && isspace(s[pos])) ++pos;
                if (pos < s.size() && s[pos] == '?') {
                    is_var = true;
                    ++pos;
                    size_t start = pos;
                    while (pos < s.size() && s[pos] != '.') ++pos;
                    value = ht[s.substr(start, pos - start)];
                    if (pos < s.size() && s[pos] == '.') {
                        ++pos;
                        size_t pstart = pos;
                        while (pos < s.size() && isdigit(s[pos])) ++pos;
                        prop = std::stoul(s.substr(pstart, pos - pstart));
                    }
                } else if (pos < s.size() && (s[pos] == '"' || s[pos] == '\'')) {
                    char quote_char = s[pos];
                    ++pos;
                    size_t start = pos;
                    size_t last = s.rfind(quote_char);
                    str = '"' + s.substr(start, last - start) + '"';
                } else if (pos < s.size()) {
                    size_t start = pos;
                    while (pos < s.size() && !isspace(s[pos])) ++pos;
                    std::string token = s.substr(start, pos - start);
                    std::int64_t aux;
                    if (constant::is_integer(token, aux)) {
                        value = aux;
                    } else if (constant::is_double(token, aux)) {
                        value = aux;
                    }else if (constant::is_date(token, aux)) {
                        value = aux;
                    }
                }
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
