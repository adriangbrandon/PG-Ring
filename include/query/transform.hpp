//
// Created by adrian on 9/2/26.
//

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP
#include <array>
#include <string>
#include <vector>

#include "configuration.hpp"
#include "constant_utils.hpp"
#include "label_expr_parser.hpp"

namespace ring {

    namespace query {
        class transform {
        public:
            typedef struct {
                std::string key;
                std::string value;
            } prop_type;

            typedef struct expr {
                enum_expr_label_type type = EMPTY;
                std::string value; //only for LAB and NEG, the label name
                std::vector<expr> args;
            } expr_label_type;

            typedef struct {
                bool is_variable;
                std::string value;
                expr_label_type expr_label;
            } node_type;

            typedef struct {
                std::string value;
                expr_label_type expr_label;
                node_type from;
                node_type to;
            } edge_type;


            typedef struct {
                bool is_var;
                std::string value; //either variable name or constant value
                bool has_property;
                std::string property;
            } expr_where_type;


            typedef struct {
                std::string comp;
                std::array<expr_where_type, 2> args;
            } op_where_type;


            typedef struct {
                std::vector<edge_type> patterns;
                std::vector<op_where_type> where_exprs;
            } query_type;

        private:

            std::unordered_map<std::string, uint32_t> m_set_nodes;   // Nombres de nodos
            std::unordered_map<std::string, uint32_t> m_set_label_nodes;  // Labels de nodos
            std::unordered_map<std::string, uint32_t> m_set_label_edges;  // Labels de aristas
            std::unordered_map<std::string, uint32_t> m_properties_node;  // Properties de nodos
            std::unordered_map<std::string, uint32_t> m_properties_edge;  // Properties de aristas

            void read_nodes_dict(const std::string& prefix) {;
                std::ifstream file(prefix + ".nodes.dict");
                uint32_t id;
                std::string data;
                while (file >> id >> data) {
                    m_set_nodes.insert({data, id});
                }
            }

            void read_nlabels_dict(const std::string& prefix) {;
                std::ifstream file(prefix + ".nlabels.dict");
                uint32_t id;
                std::string data;
                while (file >> id >> data) {
                    m_set_label_nodes.insert({data, id});
                }
            }

            void read_elabels_dict(const std::string& prefix) {;
                std::ifstream file(prefix + ".elabels.dict");
                uint32_t id;
                std::string data;
                while (file >> id >> data) {
                    m_set_label_edges.insert({data, id});
                }
            }

            void read_nprops_dict(const std::string& prefix) {;
                std::ifstream file(prefix + ".nprops.dict");
                uint32_t id;
                std::string data;
                while (file >> id >> data) {
                    m_properties_node.insert({data, id});
                }
            }

            void read_eprops_dict(const std::string& prefix) {;
                std::ifstream file(prefix + ".eprops.dict");
                uint32_t id;
                std::string data;
                while (file >> id >> data) {
                    m_properties_edge.insert({data, id});
                }
            }

            size_t find_close(char open, char close, size_t pos, const std::string& str) {
                if (str[pos] != open) throw std::runtime_error(std::string("Expected '") + open + "' at position " + std::to_string(pos));
                int count = 1;
                for (size_t i = pos+1; i < str.size(); ++i) {
                    if (str[i] == open) count++;
                    else if (str[i] == close) count--;
                    if (count == 0) return i;
                }
                throw std::runtime_error(std::string("No matching closing '") + close + "' found for '" + open + "'");
            }

            size_t end_string(char open, char close, size_t pos, const std::string& str) {
                if (str[pos] != open) throw std::runtime_error(std::string("Expected '") + open + "' at position " + std::to_string(pos));
                for (size_t i = pos+1; i < str.size(); ++i) {
                    if (str[i] == '\\') { // skip escaped characters
                        ++i;
                        continue;
                    }
                    if (str[i] == close) return i;
                }
            }

            void skip_ws(size_t& pos, const std::string& s) {
                while (pos < s.size() && isspace(s[pos])) ++pos;
            }


            bool contains_char(const std::string& str, char c, size_t pos, size_t end) {
                for (size_t i = pos; i < end; ++i) {
                    if (str[i] == c) return true;
                }
                return false;
            }

            bool match(const std::string &tok, size_t& pos, const std::string& s) {
                skip_ws(pos, s);
                size_t len = tok.size();
                if (s.substr(pos, len) == tok) {
                    pos += len;
                    return true;
                }
                return false;
            }

            bool startswith(const std::string &tok, size_t pos, const std::string& s) {
                skip_ws(pos, s);
                for (size_t i = 0; i < tok.size(); ++i) {
                    if (tok[i] != s[i + pos]) return false;
                }
                return true;
            }

            expr_label_type parse_expr(const std::string& s, size_t &pos, size_t end) {
                skip_ws(pos, s);
                return parse_or(s, pos, end);
            }

            expr_label_type parse_or(const std::string& s, size_t &pos, size_t end) {
                std::vector<expr_label_type> children;
                children.push_back(parse_and(s, pos, end));
                skip_ws(pos, s);
                while (match("OR", pos, s)) {
                    children.push_back(parse_and(s, pos, end));
                    skip_ws(pos, s);
                }
                if (children.size() == 1) return std::move(children[0]);
                expr_label_type e;
                e.type = OR;
                e.args = std::move(children);
                return e;
            }

            expr_label_type parse_and(const std::string& s, size_t &pos, size_t end) {
                std::vector<expr_label_type> children;
                children.push_back(parse_factor(s, pos, end));
                skip_ws(pos, s);
                while (match("AND", pos, s)) {
                    children.push_back(parse_factor(s, pos, end));
                    skip_ws(pos, s);
                }
                if (children.size() == 1) return std::move(children[0]);
                expr_label_type e;
                e.type = AND;
                e.args = std::move(children);
                return e;
            }

            expr_label_type parse_factor(const std::string& s, size_t &pos, size_t end) {
                skip_ws(pos, s);
                if (match("(", pos, s)) {
                    expr_label_type e = parse_expr(s, pos, end);
                    skip_ws(pos, s);
                    if (!match(")", pos, s)) throw std::runtime_error("Missing ')' in expression");
                    return e;
                } else if (match("NOT", pos, s)) {
                    return parse_label(s, pos, end, NEG);
                } else {
                    return parse_label(s, pos, end);
                }
            }

            expr_label_type parse_label(const std::string& s, size_t &pos, size_t end, enum_expr_label_type t = LAB) {
                skip_ws(pos, s);
                size_t start = pos;
                expr_label_type e;
                e.type = t;
                e.value = "";
                while (pos < end && !isspace(s[pos]) && s[pos] != ')') {
                    e.value += s[pos];
                    ++pos;
                }
                return e;
            }



            node_type parse_node(const std::string& str, size_t &pos, size_t end) {
                ++pos; // skip '('
                skip_ws(pos, str);
                node_type node;
                // Parse variable or constant
                if (str[pos] == '?') {
                    node.is_variable = true;
                    ++pos;
                    node.value = "";
                    while (pos < end && str[pos] != ':') {
                        node.value += str[pos++];
                    }
                    // parse label expression if any
                    if (pos < end && str[pos] == ':') {
                        ++pos; // skip ':'
                        skip_ws(pos, str);
                        if (!contains_char(str, '&', pos, end)) { //normal expression without '&'
                            expr_label_type e = parse_expr(str, pos, end);
                            node.expr_label = e;
                        }else { // Aidan format (&)
                            expr_label_type main_expr, aux_expr;
                            main_expr.type = AND;
                            while (pos < end) {
                                skip_ws(pos, str);
                                aux_expr.type = LAB;
                                aux_expr.value = "";
                                while (pos < end && str[pos] != '&') {
                                    aux_expr.value += str[pos];
                                    ++pos;
                                }
                                main_expr.args.push_back(aux_expr);
                                ++pos;
                            }
                            node.expr_label = main_expr;
                        }
                    }
                }
                else {
                    node.is_variable = false;
                    node.value = str.substr(pos, end-pos);
                }
                return node;
            }

            edge_type parse_edge(const std::string& str, size_t &pos, size_t end) {
                ++pos; // skip '['
                skip_ws(pos, str);
                edge_type edge;
                // Parse variable or constant
                if (str[pos] == '?') {
                    ++pos;
                    edge.value = "";
                    while (pos < end && str[pos] != ':') {
                        edge.value += str[pos++];
                    }
                    // parse label expression if any
                    if (pos < end && str[pos] == ':') {
                        ++pos; // skip ':'
                        skip_ws(pos, str);
                        if (!contains_char(str, '&', pos, end)) { //normal expression without '&'
                            expr_label_type e = parse_expr(str, pos, end);
                            edge.expr_label = e;
                        }else { // Aidan format: <l1>&<l2>&...
                            expr_label_type main_expr, aux_expr;
                            main_expr.type = AND;
                            while (pos < end) {
                                skip_ws(pos, str);
                                aux_expr.type = LAB;
                                aux_expr.value = "";
                                while (pos < end && str[pos] != '&') {
                                    aux_expr.value += str[pos];
                                    ++pos;
                                }
                                main_expr.args.push_back(aux_expr);
                                ++pos;
                            }
                            edge.expr_label = main_expr;
                        }
                    }
                }
                else {
                    throw std::runtime_error("Unexpected character: " + str.substr(pos, end-pos));
                }
                return edge;
            }


            void parse_operand(const std::string& str, size_t &pos, size_t end, op_where_type &op, size_t arg_i) {
                skip_ws(pos, str);
                if (str[pos] == '?') {
                    op.args[arg_i].is_var = true;
                    ++pos;
                    op.args[arg_i].value = "";
                    while (pos < end && !isspace(str[pos]) && str[pos] != '.' && str[pos] != ')') {
                        op.args[arg_i].value += str[pos++];
                    }
                    if (pos < end && str[pos] == '.') {
                        ++pos; // skip '.'
                        op.args[arg_i].has_property = true;
                        while (pos < end && !isspace(str[pos]) && str[pos] != ')') op.args[arg_i].property += str[pos++];
                    }
                }else {
                    op.args[arg_i].is_var = false;
                    op.args[arg_i].has_property = false;
                    if (startswith("ZONED_DATETIME", pos, str)) {
                        auto z_b = pos + 14; // skip "ZONED_DATETIME"
                        auto z_e = find_close('(', ')', z_b, str); // find closing parenthesis of ZONED_DATETIME
                        op.args[arg_i].value = str.substr(pos, z_e-pos+1); // include closing parenthesis
                    }else if (str[pos] == '"') {
                        end_string('"', '"', pos, str);
                        op.args[arg_i].value = str.substr(pos, end-pos);
                        normalize_date(op.args[arg_i].value); // normalize date format if it's a date
                    }else if (str[pos] == '\'') {
                        end_string('\'', '\'', pos, str);
                        op.args[arg_i].value = str.substr(pos, end-pos);
                    }else { // number
                        while (pos < end && !isspace(str[pos])) {
                            op.args[arg_i].value += str[pos++];
                        }
                    }
                }
            }

            op_where_type parse_op_where(const std::string& str, size_t &pos, size_t end) {
                ++pos; // skip '('
                op_where_type op;
                skip_ws(pos, str);
                // Parse operand
                parse_operand(str, pos, end, op, 0);
                skip_ws(pos, str);
                // Parse operator
                if (match(">=", pos, str)) {
                    op.comp = ">=";
                } else if (match("<=", pos, str)) {
                    op.comp = "<=";
                } else if (match("!=", pos, str)) {
                    op.comp = "!=";
                } else if (match(">", pos, str)) {
                    op.comp = ">";
                } else if (match("<", pos, str)) {
                    op.comp = "<";
                } else if (match("=", pos, str)) {
                    op.comp = "=";
                } else if (match("IS NULL", pos, str)) {
                    op.comp = "IS NULL";
                } else if (match("IS NOT NULL", pos, str)) {
                    op.comp = "IS NOT NULL";
                }
                if (op.comp != "IS NULL" && op.comp != "IS NOT NULL") {
                    //pos += op.comp.size();
                    skip_ws(pos, str);
                    // Parse second operand
                    parse_operand(str, pos, end, op, 1);
                }
                return op;
            }

        public:
            query_type parse_query(const std::string& query) {
                query_type result;
                size_t pos = 0;
                // Parse patterns until "WHERE"
                while (pos < query.size()) {
                    skip_ws(pos, query);
                    auto c_n1 = find_close('(', ')', pos, query);
                    auto src = parse_node(query, pos, c_n1);
                    pos = c_n1 + 2; // skip ")-
                    auto c_edge = find_close('[', ']', pos, query);
                    auto edge = parse_edge(query, pos, c_edge);
                    pos = c_edge + 3; // skip "]->"
                    auto c_n2 = find_close('(', ')', pos, query);
                    auto tgt = parse_node(query, pos, c_n2);
                    pos = c_n2 + 1; // skip ')'

                    edge.from = src;
                    edge.to = tgt;
                    result.patterns.push_back(edge);

                    skip_ws(pos, query);
                    if (query[pos] == ',') {
                        ++pos; // skip ','
                    } else if (match("WHERE", pos, query)) {
                        //pos += 5; // skip "WHERE"
                        break;
                    } else if (pos < query.size()) {
                        throw std::runtime_error("Expected ',' or 'WHERE' at position " + std::to_string(pos));
                    }
                }
                // Parse WHERE expressions
                while (pos < query.size()) {
                    skip_ws(pos, query);
                    auto c_op = find_close('(', ')', pos, query);
                    op_where_type op = parse_op_where(query, pos, c_op);
                    if (!op.args[0].is_var && !op.args[1].is_var) throw std::runtime_error("At least one operand must be a variable in WHERE expression.");
                    result.where_exprs.push_back(op);
                    pos = c_op + 1;
                    skip_ws(pos, query);
                    if (pos < query.size()) {
                        if (!match("AND", pos, query)) {
                            throw std::runtime_error("Expected AND at position " + std::to_string(pos));
                        }
                    }
                }
                return result;

            }


            std::string str_expr(const expr_label_type& expr) {
                std::string result;
                if (expr.type == LAB || expr.type == NEG) {
                    if (expr.type == NEG) result += "(NOT ";
                    result += expr.value;
                    if (expr.type == NEG) result += ")";
                } else {
                    result += "(";
                    for (size_t i = 0; i < expr.args.size(); ++i) {
                        result += str_expr(expr.args[i]);
                        if (i < expr.args.size() - 1) {
                            if (expr.type == AND) result += " AND ";
                            else if (expr.type == OR) result += " OR ";
                        }
                    }
                    result += ")";
                }
                return result;
            }

            std::string str_pattern(const edge_type& edge) {
                std::string result;
                result += "(";
                if (edge.from.is_variable)
                    result += "?" + edge.from.value;
                else
                    result += edge.from.value;
                if (edge.from.expr_label.type != EMPTY) {
                    result += ":";
                    // print label expression
                    result += str_expr(edge.from.expr_label);
                }
                result += ")-[";
                if (!edge.value.empty())
                    result += "?" + edge.value;
                if (edge.expr_label.type != EMPTY) {
                    result += ":";
                    // print label expression
                    result += str_expr(edge.expr_label);
                }
                result += "]->(";
                if (edge.to.is_variable)
                    result += "?" + edge.to.value;
                else
                    result += edge.to.value;
                if (edge.to.expr_label.type != EMPTY) {
                    result += ":";
                    // print label expression
                    result += str_expr(edge.to.expr_label);
                }
                result += ")";
                return result;
            }

            std::string cypher_where_labels_expr(const std::string &var, const expr_label_type& expr) {
                std::string result;
                result += "(";
                if (expr.type == LAB || expr.type == NEG) {
                    if (expr.type == NEG) result += "(NOT ";
                    result += var + ":" + expr.value;
                    if (expr.type == NEG) result += ")";
                } else {
                    result += "(";
                    for (size_t i = 0; i < expr.args.size(); ++i) {
                        result += str_expr(expr.args[i]);
                        if (i < expr.args.size() - 1) {
                            if (expr.type == AND) result += " AND ";
                            else if (expr.type == OR) result += " OR ";
                        }
                    }
                    result += ")";
                }
                result += ")";
                return result;
            }

            std::string cypher_str_expr(const std::string &var, const expr_label_type& expr, std::string &where_str) {
                std::string result;
                if (expr.type == LAB) {
                    return ":" + expr.value;
                }else if (expr.type == AND) {
                    // check if all children are LAB, then we can print them as :L1:L2:...
                    bool all_lab = true;
                    for (const auto& arg: expr.args) {
                        if (arg.type != LAB) {
                            all_lab = false;
                            break;
                        }
                    }
                    if (all_lab) {
                        for (const auto& arg: expr.args) {
                            result += ":" + arg.value;
                        }
                        return result;
                    }else {
                        if (!where_str.empty()) where_str += " AND ";
                        where_str += cypher_where_labels_expr(var, expr);
                    }
                }
                return result;
            }

            std::string cypher_pattern(const edge_type& edge, std::string &where_str, uint &ids) {
                std::string result;
                result += "(";
                if (edge.from.is_variable) {
                    result += edge.from.value + ":Entity";
                } else {
                    result += "cid" + std::to_string(ids) + ":Entity { qid: \"" + edge.from.value + "\"}";
                    ids++;
                }
                if (edge.from.expr_label.type != EMPTY) {
                    // print label expression
                    result += cypher_str_expr(edge.from.value, edge.from.expr_label, where_str);
                }
                result += ")-[";
                if (!edge.value.empty())
                    result += edge.value;
                if (edge.expr_label.type != EMPTY) {
                    // print label expression
                    result += cypher_str_expr(edge.value, edge.expr_label, where_str);
                }
                result += "]->(";
                if (edge.to.is_variable) {
                    result += edge.to.value + ":Entity";
                } else {
                    result += "cid" + std::to_string(ids) + ":Entity { qid: \"" + edge.to.value + "\"}";
                    ids++;
                }
                if (edge.to.expr_label.type != EMPTY) {
                    // print label expression
                    result += cypher_str_expr(edge.to.value, edge.to.expr_label, where_str);
                }
                result += ")";
                return result;
            }

            void normalize_date(std::string &value) {

                // Detectar si es una fecha en formato ISO (contiene dígitos y guiones)
                // Formatos soportados:
                // "1988-01-01T00:00:00Z"
                // "1988-01-01T00:00:00"
                // "1988-01-01T"
                // "1988-01-01"

                // Verificar si el string tiene comillas al inicio y final
                size_t start = 0;
                size_t end = value.length();
                bool has_quotes = false;

                if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
                    has_quotes = true;
                    start = 1;
                    end = value.length() - 1;
                }

                // Verificar formato de fecha: al menos 10 caracteres + los índices de guiones
                if ((end - start) >= 10 && value[start + 4] == '-' && value[start + 7] == '-') {
                    // Verificar que los primeros 4 caracteres son dígitos (año)
                    bool is_date = true;
                    for (size_t i = start; i < start + 4; i++) {
                        if (!std::isdigit(value[i])) {
                            is_date = false;
                            break;
                        }
                    }

                    if (is_date) {
                        std::string date_part;
                        std::string time_part = "00:00:00";

                        // Extraer la parte de fecha (YYYY-MM-DD) sin comillas
                        std::string date_str = value.substr(start, end - start);
                        size_t t_pos = date_str.find('T');

                        if (t_pos != std::string::npos) {
                            date_part = date_str.substr(0, t_pos);
                            // Si hay parte de tiempo después de T
                            if (t_pos + 1 < date_str.length()) {
                                size_t z_pos = date_str.find('Z', t_pos);
                                if (z_pos != std::string::npos) {
                                    time_part = date_str.substr(t_pos + 1, z_pos - t_pos - 1);
                                } else {
                                    time_part = date_str.substr(t_pos + 1);
                                }
                            }
                        } else {
                            // Solo fecha, sin parte de tiempo
                            date_part = date_str.substr(0, 10);
                        }

                        // Construir fecha normalizada y actualizar el valor
                        value = "ZONED_DATETIME('" + date_part + "T" + time_part + "Z')";
                    }
                }
            }

            std::string cypher_value(const std::string &value) {
                const std::string from = "ZONED_DATETIME";
                const std::string to   = "datetime";
                if (startswith(from, 0, value)) {
                    std::string r = value;
                    r.replace(0, from.length(), to);
                    auto month = r.find_first_of('-')+1;
                    if (r[month] == '0' && r[month+1] == '0') {
                        r[month+1] = '1';
                    }
                    auto day = month+3;
                    if (r[day] == '0' && r[day+1] == '0') {
                        r[day+1] = '1';
                    }
                    return r;
                }
                int64_t d;
                if (constant::is_double(value, d)) {
                    if (value.back() == '.') {
                        return value + "0";
                    }
                }

                return value;
            }

            std::string to_string(query_type &q) {
                std::string result;
                for (size_t i = 0; i < q.patterns.size(); ++i) {
                    result += str_pattern(q.patterns[i]);
                    if (i < q.patterns.size() - 1) result += ", ";
                }
                if (!q.where_exprs.empty()) {
                    result += " WHERE ";
                    for (size_t i = 0; i < q.where_exprs.size(); ++i) {
                        auto &op = q.where_exprs[i];
                        result += "(";
                        if (op.args[0].is_var) result += "?";
                        result += op.args[0].value;
                        if (op.args[0].has_property) result += "." + op.args[0].property;

                        if (op.comp != "IS NULL" && op.comp != "IS NOT NULL") {
                            result += " " + op.comp + " ";
                            if (op.args[1].is_var) result += "?";
                            result += op.args[1].value;
                            if (op.args[1].has_property) result += "." + op.args[1].property;
                        }else {
                            result += " " + op.comp;
                        }
                        result += ")";
                        if (i < q.where_exprs.size() - 1) result += " AND ";
                    }
                }
                return result;
            }

            std::string to_cypher(query_type &q) {
                std::string result = "MATCH ";
                std::string where_str = "";
                uint ids = 1;
                for (size_t i = 0; i < q.patterns.size(); ++i) {
                    result += cypher_pattern(q.patterns[i], where_str, ids);
                    if (i < q.patterns.size() - 1) result += ", ";
                }
                if (!q.where_exprs.empty()) {
                    result += " WHERE ";
                    result += where_str;
                    if (!where_str.empty() && !q.where_exprs.empty()) {
                        result += " AND ";
                    }
                    for (size_t i = 0; i < q.where_exprs.size(); ++i) {
                        auto &op = q.where_exprs[i];
                        result += "(";
                        result += cypher_value(op.args[0].value);
                        if (op.args[0].has_property) result += "." + op.args[0].property;

                        if (op.comp != "IS NULL" && op.comp != "IS NOT NULL") {
                            if (op.comp == "!=") result += " <> ";
                            else result += " " + op.comp + " ";
                            result += cypher_value(op.args[1].value);
                            if (op.args[1].has_property) result += "." + op.args[1].property;
                        }else {
                            result += " " + op.comp;
                        }
                        result += ")";
                        if (i < q.where_exprs.size() - 1) result += " AND ";
                    }
                }
                result += " RETURN ";
                std::vector<std::pair<std::string, bool>> vars;
                std::set<std::string> var_set;
                for (size_t i = 0; i < q.patterns.size(); ++i) {
                    if (q.patterns[i].from.is_variable && var_set.find(q.patterns[i].from.value) == var_set.end()) {
                        vars.emplace_back(q.patterns[i].from.value, true);
                        var_set.insert(q.patterns[i].from.value);
                    }
                    vars.emplace_back(q.patterns[i].value, false);
                    if (q.patterns[i].to.is_variable && var_set.find(q.patterns[i].to.value) == var_set.end()) {
                        vars.emplace_back(q.patterns[i].to.value, true);
                        var_set.insert(q.patterns[i].to.value);
                    }
                }
                for (size_t i = 0; i < vars.size(); ++i) {
                    if (vars[i].second) result += vars[i].first + ".qid";
                    else result += "id(" + vars[i].first + ")";
                    if (i < vars.size() - 1) result += ", ";
                }

                return result;
            }

            // Generate MQL query for MillenniumDB
            std::string to_mql(query_type &q) {
                std::string result = "MATCH ";

                // Generate patterns
                for (size_t i = 0; i < q.patterns.size(); ++i) {
                    result += mql_pattern(q.patterns[i]);
                    if (i < q.patterns.size() - 1) result += ", ";
                }

                // Generate WHERE clause
                if (!q.where_exprs.empty()) {
                    result += " WHERE ";
                    for (size_t i = 0; i < q.where_exprs.size(); ++i) {
                        auto &op = q.where_exprs[i];
                        result += "(";

                        // First argument
                        if (op.args[0].is_var) result += "?";
                        result += mql_value(op.args[0].value);
                        if (op.args[0].has_property) result += "." + op.args[0].property;

                        // Operator
                        if (op.comp != "IS NULL" && op.comp != "IS NOT NULL") {
                            result += " " + op.comp + " ";

                            // Second argument
                            if (op.args[1].is_var) result += "?";
                            result += mql_value(op.args[1].value);
                            if (op.args[1].has_property) result += "." + op.args[1].property;
                        } else {
                            result += " " + op.comp;
                        }
                        result += ")";
                        if (i < q.where_exprs.size() - 1) result += " AND ";
                    }
                }

                // Generate RETURN clause
                result += " RETURN ";
                std::vector<std::pair<std::string, bool>> vars;
                std::set<std::string> var_set;

                // Collect all variables
                for (size_t i = 0; i < q.patterns.size(); ++i) {
                    if (q.patterns[i].from.is_variable && var_set.find(q.patterns[i].from.value) == var_set.end()) {
                        vars.emplace_back(q.patterns[i].from.value, true);
                        var_set.insert(q.patterns[i].from.value);
                    }
                    if (!q.patterns[i].value.empty()) {
                        vars.emplace_back(q.patterns[i].value, false);
                    }
                    if (q.patterns[i].to.is_variable && var_set.find(q.patterns[i].to.value) == var_set.end()) {
                        vars.emplace_back(q.patterns[i].to.value, true);
                        var_set.insert(q.patterns[i].to.value);
                    }
                }

                // Output variables with ? prefix
                for (size_t i = 0; i < vars.size(); ++i) {
                    result += "?";
                    result += vars[i].first;
                    if (i < vars.size() - 1) result += ", ";
                }

                return result;
            }

            // Generate MQL pattern (node)-[edge]->(node)
            std::string mql_pattern(const edge_type& edge) {
                std::string result;

                // From node
                result += "(";
                if (edge.from.is_variable) {
                    result += "?" + edge.from.value;
                    if (edge.from.expr_label.type != EMPTY) {
                        result += mql_label_expr(edge.from.expr_label);
                    }
                } else {
                    // Constant node ID (like Q65363)
                    result += edge.from.value;
                }
                result += ")";

                // Edge
                result += "-[";
                if (!edge.value.empty()) {
                    result += "?" + edge.value;
                }
                if (edge.expr_label.type != EMPTY) {
                    result += mql_label_expr(edge.expr_label);
                }
                result += "]->";

                // To node
                result += "(";
                if (edge.to.is_variable) {
                    result += "?" + edge.to.value;
                    if (edge.to.expr_label.type != EMPTY) {
                        result += mql_label_expr(edge.to.expr_label);
                    }
                } else {
                    // Constant node ID (like Q65363)
                    result += edge.to.value;
                }
                result += ")";

                return result;
            }

            // Generate MQL label expression
            std::string mql_label_expr(const expr_label_type& expr) {
                std::string result;

                if (expr.type == LAB) {
                    result = " :" + expr.value;
                } else if (expr.type == NEG) {
                    // NOT label - MQL might not support this directly, use filter
                    result = " /* NOT " + expr.value + " */";
                } else if (expr.type == AND) {
                    // Multiple labels: :L1:L2:L3
                    for (const auto& arg: expr.args) {
                        if (arg.type == LAB) {
                            result += " :" + arg.value;
                        }
                    }
                } else if (expr.type == OR) {
                    // OR is complex, might need WHERE clause
                    result = " /* OR expression */";
                }

                return result;
            }

            // Format value for MQL
            std::string mql_value(const std::string &value) {
                // Convert ZONED_DATETIME to dateTimeStamp
                const std::string zoned_prefix = "ZONED_DATETIME('";
                const std::string zoned_suffix = "')";

                if (value.find(zoned_prefix) == 0 && value.find(zoned_suffix) == value.length() - zoned_suffix.length()) {
                    // Extract date string
                    std::string date_str = value.substr(zoned_prefix.length(),
                                                       value.length() - zoned_prefix.length() - zoned_suffix.length());
                    // Remove leading + for positive years
                    if (!date_str.empty() && date_str[0] == '+') {
                        date_str = date_str.substr(1);
                    }
                    return "dateTimeStamp(\"" + date_str + "\")";
                }

                return value;
            }


            void transform_expr(expr_label_type &expr, bool is_node) {
                if (expr.type == LAB || expr.type == NEG) {
                    if (is_node) {
                        auto it = m_set_label_nodes.find(expr.value);
                        if (it != m_set_label_nodes.end()) {
                            expr.value = std::to_string(it->second);
                        } else {
                            throw std::runtime_error("Unknown label: " + expr.value);
                        }
                    }else {
                        auto it = m_set_label_edges.find(expr.value);
                        if (it != m_set_label_edges.end()) {
                            expr.value = std::to_string(it->second);
                        } else {
                            throw std::runtime_error("Unknown label: " + expr.value);
                        }
                    }
                } else {
                    for (auto &arg: expr.args) {
                        transform_expr(arg, is_node);
                    }
                }
            }

            void transform_node(node_type &node) {
                if (!node.is_variable) {
                    auto it = m_set_nodes.find(node.value);
                    if (it == m_set_nodes.end()) throw std::runtime_error("Unknown node: " + node.value);
                    node.value = std::to_string(it->second);
                }else {
                    transform_expr(node.expr_label, true);
                }
            }

            void transform_edge(edge_type &edge) {
                transform_node(edge.from);
                transform_expr(edge.expr_label, false);
                transform_node(edge.to);
            }

            void transform_op_where(op_where_type &op, std::set<std::string> &node_vars, std::set<string> &edge_vars) {
                size_t args = 2;
                if (op.comp == "IS NULL" || op.comp == "IS NOT NULL") args = 1;
                for (size_t i = 0; i < args; ++i) {
                    if (op.args[i].is_var) {
                        if (op.args[i].has_property) {
                            if (node_vars.find(op.args[i].value) != node_vars.end()) {
                                auto it_prop = m_properties_node.find(op.args[i].property);
                                if (it_prop != m_properties_node.end()) {
                                    op.args[i].property = std::to_string(it_prop->second);
                                }else {
                                    throw std::runtime_error("Unknown node property: " + op.args[i].property);
                                }
                            }else {
                                if (edge_vars.find(op.args[i].value) != edge_vars.end()) {
                                    auto it_prop = m_properties_edge.find(op.args[i].property);
                                    if (it_prop != m_properties_edge.end()) {
                                        op.args[i].property = std::to_string(it_prop->second);
                                    }else {
                                        throw std::runtime_error("Unknown edge property: " + op.args[i].property);
                                    }
                                } else {
                                    throw std::runtime_error("Unknown variable: " + op.args[i].value);
                                }

                            }
                        } else {
                            if (node_vars.find(op.args[i].value) == node_vars.end() && edge_vars.find(op.args[i].value) == edge_vars.end()) { throw std::runtime_error("Unknown variable: " + op.args[i].value); }
                        }
                    }else {
                        // constant can be a ID or any literal
                        int64_t res;

                        if (!(constant::is_date(op.args[i].value, res) || constant::is_integer(op.args[i].value, res) || constant::is_double(op.args[i].value, res) || constant::is_string(op.args[i].value))) {
                            bool is_node = node_vars.find(op.args[(i+1) % 2].value) != node_vars.end();
                            if (is_node) {
                                auto it_node = m_set_nodes.find(op.args[i].value);
                                if (it_node != m_set_nodes.end()) {
                                    op.args[i].value = std::to_string(it_node->second);
                                }else {
                                    throw std::runtime_error("Unknown node: " + op.args[i].value);
                                }
                            }else {
                                throw std::runtime_error("Edge shouldn't have an ID");
                            }

                        }

                    }
                }
            }

            // Validation functions - check existence without transformation

            void validate_expr(const expr_label_type &expr, bool is_node) const {
                if (expr.type == LAB || expr.type == NEG) {
                    if (is_node) {
                        auto it = m_set_label_nodes.find(expr.value);
                        if (it == m_set_label_nodes.end()) {
                            throw std::runtime_error("Unknown node label: " + expr.value);
                        }
                    } else {
                        auto it = m_set_label_edges.find(expr.value);
                        if (it == m_set_label_edges.end()) {
                            throw std::runtime_error("Unknown edge label: " + expr.value);
                        }
                    }
                } else {
                    for (const auto &arg: expr.args) {
                        validate_expr(arg, is_node);
                    }
                }
            }

            void validate_node(const node_type &node) const {
                if (!node.is_variable) {
                    auto it = m_set_nodes.find(node.value);
                    if (it == m_set_nodes.end()) {
                        throw std::runtime_error("Unknown node: " + node.value);
                    }
                } else {
                    validate_expr(node.expr_label, true);
                }
            }

            void validate_edge(const edge_type &edge) const {
                validate_node(edge.from);
                validate_expr(edge.expr_label, false);
                validate_node(edge.to);
            }

            void validate_op_where(const op_where_type &op, const std::set<std::string> &node_vars, const std::set<std::string> &edge_vars) const {
                size_t args = 2;
                if (op.comp == "IS NULL" || op.comp == "IS NOT NULL") args = 1;
                for (size_t i = 0; i < args; ++i) {
                    if (op.args[i].is_var) {
                        if (op.args[i].has_property) {
                            if (node_vars.find(op.args[i].value) != node_vars.end()) {
                                auto it_prop = m_properties_node.find(op.args[i].property);
                                if (it_prop == m_properties_node.end()) {
                                    throw std::runtime_error("Unknown node property: " + op.args[i].property);
                                }
                            } else {
                                if (edge_vars.find(op.args[i].value) != edge_vars.end()) {
                                    auto it_prop = m_properties_edge.find(op.args[i].property);
                                    if (it_prop == m_properties_edge.end()) {
                                        throw std::runtime_error("Unknown edge property: " + op.args[i].property);
                                    }
                                } else {
                                    throw std::runtime_error("Unknown variable: " + op.args[i].value);
                                }
                            }
                        } else {
                            if (node_vars.find(op.args[i].value) == node_vars.end() && edge_vars.find(op.args[i].value) == edge_vars.end()) {
                                throw std::runtime_error("Unknown variable: " + op.args[i].value);
                            }
                        }
                    } else {
                        // constant can be a ID or any literal
                        int64_t res;

                        if (!(constant::is_date(op.args[i].value, res) || constant::is_integer(op.args[i].value, res) || constant::is_double(op.args[i].value, res) || constant::is_string(op.args[i].value))) {
                            bool is_node = node_vars.find(op.args[(i+1) % 2].value) != node_vars.end();
                            if (is_node) {
                                auto it_node = m_set_nodes.find(op.args[i].value);
                                if (it_node == m_set_nodes.end()) {
                                    throw std::runtime_error("Unknown node: " + op.args[i].value);
                                }
                            } else {
                                throw std::runtime_error("Edge shouldn't have an ID");
                            }
                        }
                    }
                }
            }

            void run(const std::string &queries, const std::string &prefix, bool distinct) {

                //extract extension
                auto p = queries.rfind('.');
                std::string name = queries.substr(0, p);

                std::string ok = name + ".ok.tsv";
                std::string ok_cypher = name + ".ok.cypher";
                std::string ok_mql = name + ".ok.mql";
                std::string err = name + ".err.tsv";
                std::ofstream out(ok);
                std::ofstream out_cypher(ok_cypher);
                std::ofstream out_mql(ok_mql);
                std::ofstream err_out(err);

                read_nodes_dict(prefix);
                read_nlabels_dict(prefix);
                read_elabels_dict(prefix);
                read_nprops_dict(prefix);
                read_eprops_dict(prefix);

                std::string query;
                std::ifstream ifs(queries);
                uint i = 1;
                while (std::getline(ifs, query)) {
                    try {
                        std::set<std::string> node_vars, edge_vars;
                        auto q = parse_query(query);
                        std::string cypher = to_cypher(q); //before transforming the values
                        std::string mql = to_mql(q); //generate MQL for MillenniumDB
                        for (auto &edge: q.patterns) {
                            if (edge.from.is_variable) node_vars.insert(edge.from.value);
                            if (edge.to.is_variable) node_vars.insert(edge.to.value);
                            if (!edge.value.empty()) edge_vars.insert(edge.value);
                            if (edge.from.is_variable && edge.to.is_variable && edge.from.value == edge.to.value) {
                                throw std::runtime_error("Variable " + edge.from.value + " cannot be both source and target of the same edge.");
                            }
                        }

                        for (auto &ev : edge_vars) {
                            if (node_vars.find(ev) != node_vars.end()) {
                                throw std::runtime_error("Conflicting type with node: " + ev);
                            }
                        }

                        for (auto &nv : node_vars) {
                            if (edge_vars.find(nv) != edge_vars.end()) {
                                throw std::runtime_error("Conflicting type with edge: " + nv);
                            }
                        }

                        //Usado para transformar a ids (ahora no lo necesito, porque ya se encarga el sistema de hacer la traduccion)
                        for (auto &edge: q.patterns) {
                            validate_edge(edge);
                        }
                        for (auto &op: q.where_exprs) {
                            validate_op_where(op, node_vars, edge_vars);
                        }
                        if (distinct && edge_vars.size() > 1) {
                            std::vector<std::string> vars;
                            vars.insert(vars.end(), edge_vars.begin(), edge_vars.end());
                            for (uint b = 0; b < vars.size()-1; ++b) {
                                for (uint k = b+1; k < vars.size(); ++k) {
                                    op_where_type op_where;
                                    op_where.comp = "!=";
                                    op_where.args[0].is_var = true;
                                    op_where.args[0].value = vars[b];
                                    op_where.args[0].has_property = false;
                                    op_where.args[1].is_var = true;
                                    op_where.args[1].value = vars[k];
                                    op_where.args[1].has_property = false;
                                    q.where_exprs.push_back(op_where);
                                }
                            }
                        }
                        out << to_string(q) << "\n";


                        out_cypher << cypher << "\n";
                        out_mql << mql << "\n";
                        std::cout << "[" << i << "] Processed query: " << query << "\n";
                        ++i;
                    }catch (std::exception &e) {
                        err_out << "Error processing query: " << query << " Exception: " << e.what() << "\n";
                    }
                }

                out.close(); err_out.close(); out_cypher.close(); out_mql.close();
            }
        };



    }


}


#endif //TRANSFORM_HPP
