#ifndef RING_CYPHER_QUERY_PARSER_HPP
#define RING_CYPHER_QUERY_PARSER_HPP
#include <string>
#include <set>
#include <vector>
#include <query/triple_parser.hpp>

#include "where_expr_parser.hpp"

namespace ring {
namespace query {


    class pg_query {


    public:
        typedef triple_parser::triple_type triple_type;
        typedef std::vector<triple_type> patterns_type;
        typedef where_expr_parser::expr where_type;

    private:
        patterns_type m_patterns;
        where_type m_where;

        std::unordered_map<std::string, uint8_t> m_ht;
        std::vector<bool> m_vnodes; // to know if the variable is in a node or in an edge

        void copy(const pg_query &o) {
            m_patterns = o.m_patterns;
            m_ht = o.m_ht;
            m_where = o.m_where;
            m_vnodes = o.m_vnodes;
        }

        static void skip_ws(size_t& pos, const std::string& s) {
            while (pos < s.size() && isspace(s[pos])) ++pos;
        }

        static bool match(const std::string &tok, size_t& pos, const std::string& s) {
            skip_ws(pos, s);
            size_t len = tok.size();
            if (s.substr(pos, len) == tok) {
                return true;
            }
            return false;
        }


    public:

        const patterns_type& patterns = m_patterns;
        const where_type& where = m_where;
        const std::unordered_map<std::string, uint8_t>& ht = m_ht;
        const std::vector<bool>& vnodes = m_vnodes;

        pg_query() = default;

        pg_query(const std::string& query) {
            m_vnodes.resize(50); //initial size
            size_t start = 0; bool in_where = false;
            while (start < query.size() && !in_where) {
                // Find the next comma outside parentheses/brackets
                size_t pos = start;
                int p = 0, b = 0;
                while (pos < query.size()) {
                    if (query[pos] == '(') p++;
                    if (query[pos] == ')') p--;
                    if (query[pos] == '[') b++;
                    if (query[pos] == ']') b--;
                    if (query[pos] == ',' && p == 0 && b == 0) break;
                    in_where = match("WHERE", pos, query);
                    if (in_where) break;
                    ++pos;
                }
                std::string pat = query.substr(start, pos - start);
                if (!pat.empty()) {
                    m_patterns.push_back(triple_parser::parse(pat, m_ht, m_vnodes));
                }
                if (!in_where) start = pos + 1;
                else start = pos + 6; // length of "WHERE" + 1

                skip_ws(start, query);
            }
            m_vnodes.resize(m_ht.size()+1);
            if (start < query.size()) {
                std::string where_str = query.substr(start);
                m_where = where_expr_parser::parse(where_str, m_ht);
            }
        }

        pg_query(const pg_query &o) {
            copy(o);
        }

        pg_query(pg_query &&o) {
            *this = std::move(o);
        }

        pg_query& operator=(const pg_query &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        pg_query& operator=(pg_query &&o) {
            if (this != &o) {
                m_patterns = std::move(o.m_patterns);
                m_where = std::move(o.m_where);
                m_ht = std::move(o.m_ht);
                m_vnodes = std::move(o.m_vnodes);
            }
            return *this;
        }

        // Translate all string identifiers to uint32_t using ring dictionaries
        template<typename ring_type>
        void translate(const ring_type* ring_ptr) {
            // Translate node constants
            for (auto& pattern : m_patterns) {
                // Subject node
                if (!pattern.subj.const_str.empty()) {
                    pattern.subj.const_value = ring_ptr->dict_nodes.locate(pattern.subj.const_str);
                    if (pattern.subj.const_value == 0) {
                        throw std::runtime_error("Node not found in dictionary: " + pattern.subj.const_str);
                    }
                }
                // Object node
                if (!pattern.obj.const_str.empty()) {
                    pattern.obj.const_value = ring_ptr->dict_nodes.locate(pattern.obj.const_str);
                    if (pattern.obj.const_value == 0) {
                        throw std::runtime_error("Node not found in dictionary: " + pattern.obj.const_str);
                    }
                }

                // Translate node label expressions
                translate_label_expr(pattern.subj.expr, ring_ptr->dict_label_nodes);
                translate_label_expr(pattern.obj.expr, ring_ptr->dict_label_nodes);
                // Translate edge label expressions
                translate_label_expr(pattern.edge.expr, ring_ptr->dict_label_edges);
            }

            // Translate where clause properties
            translate_where_expr(m_where, ring_ptr);
        }

    private:
        template<typename dict_type>
        void translate_label_expr(label_expr_parser::expr_label_type& expr, const dict_type& dict) {
            if (expr.type == LAB || expr.type == NEG) {
                if (!expr.label_str.empty()) {
                    expr.label = dict.locate(expr.label_str);
                    if (expr.label == 0) {
                        throw std::runtime_error("Label not found in dictionary: " + expr.label_str);
                    }
                }
            } else if (expr.type == AND || expr.type == OR) {
                for (auto& arg : expr.args) {
                    translate_label_expr(arg, dict);
                }
            }
        }

        template<typename ring_type>
        void translate_where_expr(where_expr_parser::expr& expr, const ring_type* ring_ptr) {
            if (expr.type == WAND || expr.type == WOR) {
                for (auto& arg : expr.args) {
                    translate_where_expr(arg, ring_ptr);
                }
            } else {
                // Translate property names to IDs
                for (int i = 0; i < 2; ++i) {
                    if (!expr.property_strs[i].empty()) {
                        // Determine if it's a node or edge property based on the variable
                        bool is_node = (expr.is_var[i] && expr.values[i] > 0 &&
                                       expr.values[i] <= m_vnodes.size() &&
                                       m_vnodes[expr.values[i]]);

                        if (is_node) {
                            expr.property_values[i] = ring_ptr->dict_prop_nodes.locate(expr.property_strs[i]);
                        } else {
                            expr.property_values[i] = ring_ptr->dict_prop_edges.locate(expr.property_strs[i]);
                        }

                        if (expr.property_values[i] == 0) {
                            throw std::runtime_error("Property not found in dictionary: " + expr.property_strs[i]);
                        }
                    }
                }
            }
        }

    public:

    };}
}

#endif // RING_CYPHER_QUERY_PARSER_HPP
