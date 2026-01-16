//
// Created by adrian on 5/11/25.
//

#ifndef CHECK_HPP
#define CHECK_HPP

#include <configuration.hpp>
#include <query/query_parser.hpp>

namespace ring {
    namespace test {
        class query_checker {
        private:
            std::vector<spo_triple> *m_ptr_triples; //should be sorted by POS
            std::vector<std::vector<uint32_t> > *m_ptr_node_labels; //adjlist of node->labels
            std::vector<std::vector<std::pair<uint32_t, uint32_t> > > *m_ptr_numeric_properties;
            std::vector<std::vector<std::pair<uint32_t, std::string> > > *m_ptr_string_properties;
            std::unordered_map<uint32_t, std::pair<bool, uint32_t> > *m_ptr_node_properties;
            std::unordered_map<uint32_t, std::pair<bool, uint32_t> > *m_ptr_edge_properties;
            query::pg_query m_query;
            std::vector<std::vector<uint32_t> > m_res;


            bool check_expr_edge_and(const query::label_expr_parser::expr_label_type &expr, const spo_triple &triple) {
                for (const auto &arg: expr.args) {
                    if (arg.type == ring::query::LAB) {
                        if (std::get<1>(triple) != arg.label) return false;
                    } else if (arg.type == ring::query::NEG) {
                        if (std::get<1>(triple) == arg.label) return false;
                    } else if (arg.type == ring::query::OR) {
                        if (!check_expr_edge_or(arg, triple)) return false;
                    } else if (arg.type == ring::query::AND) {
                        if (!check_expr_edge_and(arg, triple)) return false;
                    }
                }
                return true;
            }

            bool check_expr_edge_or(const query::label_expr_parser::expr_label_type &expr, const spo_triple &triple) {
                for (const auto &arg: expr.args) {
                    if (arg.type == ring::query::LAB) {
                        if (std::get<1>(triple) == arg.label) return true;
                    } else if (arg.type == ring::query::NEG) {
                        if (std::get<1>(triple) != arg.label) return true;
                    } else if (arg.type == ring::query::OR) {
                        if (check_expr_edge_or(arg, triple)) return true;
                    } else if (arg.type == ring::query::AND) {
                        if (check_expr_edge_and(arg, triple)) return true;
                    }
                }
                return false;
            }


            bool check_expr_edge(const query::label_expr_parser::expr_label_type &expr, const spo_triple &triple) {
                if (expr.type == ring::query::LAB) {
                    return (std::get<1>(triple) == expr.label);
                } else if (expr.type == ring::query::NEG) {
                    return (std::get<1>(triple) != expr.label);
                } else if (expr.type == ring::query::OR) {
                    return (check_expr_edge_or(expr, triple));
                } else if (expr.type == ring::query::AND) {
                    return (check_expr_edge_and(expr, triple));
                }
                return true;
            }

            bool check_expr_node_and(const query::label_expr_parser::expr_label_type &expr, const uint64_t id) {
                for (const auto &arg: expr.args) {
                    if (arg.type == ring::query::LAB) {
                        if (m_ptr_node_labels->at(id - 1).end() == std::find(
                                m_ptr_node_labels->at(id - 1).begin(), m_ptr_node_labels->at(id - 1).end(),
                                arg.label)) {
                            return false;
                        }
                    } else if (arg.type == ring::query::NEG) {
                        if (m_ptr_node_labels->at(id - 1).end() != std::find(
                                m_ptr_node_labels->at(id - 1).begin(), m_ptr_node_labels->at(id - 1).end(),
                                arg.label)) {
                            return false;
                        }
                    } else if (arg.type == ring::query::OR) {
                        if (!check_expr_node_or(arg, id)) return false;
                    } else if (arg.type == ring::query::AND) {
                        if (!check_expr_node_and(arg, id)) return false;
                    }
                }
                return true;
            }


            bool check_expr_node_or(const query::label_expr_parser::expr_label_type &expr, const uint64_t id) {
                for (const auto &arg: expr.args) {
                    if (arg.type == ring::query::LAB) {
                        if (m_ptr_node_labels->at(id - 1).end() != std::find(
                                m_ptr_node_labels->at(id - 1).begin(), m_ptr_node_labels->at(id - 1).end(),
                                arg.label)) {
                            return true;
                        }
                    } else if (arg.type == ring::query::NEG) {
                        if (m_ptr_node_labels->at(id - 1).end() == std::find(
                                m_ptr_node_labels->at(id - 1).begin(), m_ptr_node_labels->at(id - 1).end(),
                                arg.label)) {
                            return true;
                        }
                    } else if (arg.type == ring::query::OR) {
                        if (check_expr_node_or(arg, id)) return true;
                    } else if (arg.type == ring::query::AND) {
                        if (check_expr_node_and(arg, id)) return true;
                    }
                }
                return false;
            }

            bool check_expr_node(const query::label_expr_parser::expr_label_type &expr, const uint64_t id) {
                if (expr.type == ring::query::LAB) {
                    return (m_ptr_node_labels->at(id - 1).end() != std::find(
                                m_ptr_node_labels->at(id - 1).begin(), m_ptr_node_labels->at(id - 1).end(),
                                expr.label));
                } else if (expr.type == ring::query::NEG) {
                    return (m_ptr_node_labels->at(id - 1).end() == std::find(
                                m_ptr_node_labels->at(id - 1).begin(), m_ptr_node_labels->at(id - 1).end(),
                                expr.label));
                } else if (expr.type == ring::query::OR) {
                    return check_expr_node_or(expr, id);
                } else if (expr.type == ring::query::AND) {
                    return check_expr_node_and(expr, id);
                }
                return true;
            }

            bool check(const query::triple_parser::triple &pattern, const spo_triple &triple) {
                // Check subject
                if (!pattern.subj.is_var()) {
                    if (std::get<0>(triple) != pattern.subj.const_value) return false;
                } else {
                    if (!check_expr_node(pattern.subj.expr, std::get<0>(triple))) return false;
                }

                // Check predicate
                if (!pattern.edge.is_var()) {
                    if (std::get<1>(triple) != pattern.edge.const_value) return false;
                } else {
                    // Check expression
                    if (!check_expr_edge(pattern.edge.expr, triple)) return false;
                }

                // Check object
                if (!pattern.obj.is_var()) {
                    if (std::get<2>(triple) != pattern.obj.const_value) return false;
                } else {
                    if (!check_expr_node(pattern.obj.expr, std::get<2>(triple))) return false;
                }

                return true;
            }

            bool check_bindings(const query::triple_parser::triple &pattern, const spo_triple &triple,
                                const std::vector<uint32_t> &tuple) {
                if (pattern.subj.is_var()) {
                    if (tuple[pattern.subj.var_value - 1] && std::get<0>(triple) != tuple[pattern.subj.var_value - 1])
                        return false;
                }
                if (pattern.obj.is_var()) {
                    if (tuple[pattern.obj.var_value - 1] && std::get<2>(triple) != tuple[pattern.obj.var_value - 1])
                        return false;
                }
                return true;
            }

            std::tuple<bool, bool, bool> bind(const query::triple_parser::triple &pattern, const spo_triple &triple,
                                              uint64_t t_i, std::vector<uint32_t> &tuple) {
                std::tuple<bool, bool, bool> bind = {false, false, false};
                if (pattern.subj.is_var() && !tuple[pattern.subj.var_value - 1]) {
                    tuple[pattern.subj.var_value - 1] = std::get<0>(triple);
                    std::get<0>(bind) = true;
                }
                if (pattern.edge.is_var() && !tuple[pattern.edge.var_value - 1]) {
                    tuple[pattern.edge.var_value - 1] = t_i;
                    std::get<1>(bind) = true;
                }
                if (pattern.obj.is_var() && !tuple[pattern.obj.var_value - 1]) {
                    tuple[pattern.obj.var_value - 1] = std::get<2>(triple);
                    std::get<2>(bind) = true;
                }
                return bind;
            }

            void unbind(const std::tuple<bool, bool, bool> &b, const query::triple_parser::triple &pattern,
                        std::vector<uint32_t> &tuple) {
                if (std::get<0>(b)) {
                    tuple[pattern.subj.var_value - 1] = 0;
                }
                if (std::get<1>(b)) {
                    tuple[pattern.edge.var_value - 1] = 0;
                }
                if (std::get<2>(b)) {
                    tuple[pattern.obj.var_value - 1] = 0;
                }
            }

            void run(uint64_t p_i, uint64_t t_i, std::vector<uint32_t> &tuple) {
                if (p_i == m_query.patterns.size()) {
                    m_res.push_back(tuple);
                    return;
                }
                if (t_i > m_ptr_triples->size()) {
                    return;
                }
                const auto &pattern = m_query.patterns[p_i];
                const auto &triple = (*m_ptr_triples)[t_i - 1];
                if (check(pattern, triple)) {
                    if (check_bindings(pattern, triple, tuple)) {
                        auto b = bind(pattern, triple, t_i, tuple);
                        run(p_i + 1, 1, tuple);
                        unbind(b, pattern, tuple);
                    }
                }
                run(p_i, t_i + 1, tuple);
            }

            bool get_node_property_value_numeric(uint32_t prop_id, uint32_t node_id, uint32_t &value) {
                auto it = m_ptr_node_properties->find(prop_id);
                auto pos = it->second.second;
                const auto &vec = m_ptr_numeric_properties->at(pos);
                for (const auto &pair: vec) {
                    if (pair.first == node_id) {
                        value = pair.second;
                        return true;
                    }
                }
                return false;
            }

            bool get_node_property_value_string(uint32_t prop_id, uint32_t node_id, std::string &value) {
                auto it = m_ptr_node_properties->find(prop_id);
                auto pos = it->second.second;
                const auto &vec = m_ptr_string_properties->at(pos);
                for (const auto &pair: vec) {
                    if (pair.first == node_id) {
                        value = pair.second;
                        return true;
                    }
                }
                return false;
            }

            bool get_edge_property_value_numeric(uint32_t prop_id, uint32_t node_id, uint32_t &value) {
                auto it = m_ptr_edge_properties->find(prop_id);
                auto pos = it->second.second;
                const auto &vec = m_ptr_numeric_properties->at(pos);
                for (const auto &pair: vec) {
                    if (pair.first == node_id) {
                        value = pair.second;
                        return true;
                    }
                }
                return false;
            }

            bool get_edge_property_value_string(uint32_t prop_id, uint32_t node_id, std::string &value) {
                auto it = m_ptr_edge_properties->find(prop_id);
                auto pos = it->second.second;
                const auto &vec = m_ptr_string_properties->at(pos);
                for (const auto &pair: vec) {
                    if (pair.first == node_id) {
                        value = pair.second;
                        return true;
                    }
                }
                return false;
            }

            bool is_property_numeric(bool is_node, uint32_t prop_id) {
                if (is_node) {
                    auto it = m_ptr_node_properties->find(prop_id);
                    if (it != m_ptr_node_properties->end()) {
                        return it->second.first;
                    }
                    return false;
                } else {
                    auto it = m_ptr_edge_properties->find(prop_id);
                    if (it != m_ptr_edge_properties->end()) {
                        return it->second.first;
                    }
                    return false;
                }
            }


            template<class T>
            struct compare {
                bool operator()(const T &a, const T &b, query::where_expr_parser::expr &expr) const {
                    switch (expr.type) {
                        case query::EQ: return a == b;
                        case query::NEQ: return a != b;
                        case query::ST: return a < b;
                        case query::GT: return a > b;
                        case query::SE: return a <= b;
                        case query::GE: return a >= b;
                        default: return false;
                    }
                }
            };


            bool check_expr_cmp(query::where_expr_parser::expr expr, const std::vector<uint32_t> &tuple) {
                uint32_t e0, e1;
                std::string s0, s1;
                if (expr.has_property()) {
                    if (expr.is_var[0] && expr.is_var[1]) {
                        if (m_query.vnodes[expr.values[0]]) {
                            if (is_property_numeric(true, expr.property_values[0]) && is_property_numeric(
                                    true, expr.property_values[1])) {
                                auto ok0 = get_node_property_value_numeric(
                                    expr.property_values[0], tuple[expr.values[0] - 1], e0);
                                auto ok1 = get_node_property_value_numeric(
                                    expr.property_values[1], tuple[expr.values[1] - 1], e1);
                                if (!(ok0 && ok1)) return false;
                                compare<uint32_t> cmp;
                                return cmp(e0, e1, expr);
                            } else {
                                auto ok0 = get_node_property_value_string(
                                    expr.property_values[0], tuple[expr.values[0] - 1], s0);
                                auto ok1 = get_node_property_value_string(
                                    expr.property_values[1], tuple[expr.values[1] - 1], s1);
                                if (!(ok0 && ok1)) return false;
                                compare<std::string> cmp;
                                return cmp(s0, s1, expr);
                            }
                        } else {
                            if (is_property_numeric(true, expr.property_values[0]) && is_property_numeric(
                                    true, expr.property_values[1])) {
                                auto ok0 = get_edge_property_value_numeric(
                                    expr.property_values[0], tuple[expr.values[0] - 1], e0);
                                auto ok1 = get_edge_property_value_numeric(
                                    expr.property_values[1], tuple[expr.values[1] - 1], e1);
                                if (!(ok0 && ok1)) return false;
                                compare<uint32_t> cmp;
                                return cmp(e0, e1, expr);
                            } else {
                                auto ok0 = get_edge_property_value_string(
                                    expr.property_values[0], tuple[expr.values[0] - 1], s0);
                                auto ok1 = get_edge_property_value_string(
                                    expr.property_values[1], tuple[expr.values[1] - 1], s1);
                                if (!(ok0 && ok1)) return false;
                                compare<std::string> cmp;
                                return cmp(s0, s1, expr);
                            }
                        }
                        // c++
                    } else if (!expr.is_var[0] && expr.is_var[1]) {
                        if (m_query.vnodes[expr.values[1]]) {
                            if (is_property_numeric(true, expr.property_values[1])) {
                                if (!get_node_property_value_numeric(expr.property_values[1], tuple[expr.values[1] - 1],
                                                                     e0)) return false;
                                compare<uint32_t> cmp;
                                return cmp(expr.values[0], e0, expr);
                            } else {
                                if (!get_node_property_value_string(expr.property_values[1], tuple[expr.values[1] - 1],
                                                                    s0)) return false;
                                // Nota: si expr.values[0] es constante string, usarlo aquí; si es numérico, convertir a string si aplica.
                                compare<std::string> cmp;
                                return cmp(expr.strs[0], s0, expr);
                            }
                        } else {
                            if (is_property_numeric(false, expr.property_values[1])) {
                                if (!get_edge_property_value_numeric(expr.property_values[1], tuple[expr.values[1] - 1],
                                                                     e0)) return false;
                                compare<uint32_t> cmp;
                                return cmp(expr.values[0], e0, expr);
                            } else {
                                if (!get_edge_property_value_string(expr.property_values[1], tuple[expr.values[1] - 1],
                                                                    s0)) return false;
                                compare<std::string> cmp;
                                return cmp(expr.strs[0], s0, expr);
                            }
                        }
                    } else if (expr.is_var[0] && !expr.is_var[1]) {
                        if (m_query.vnodes[expr.values[0]]) {
                            if (is_property_numeric(true, expr.property_values[0])) {
                                if (!get_node_property_value_numeric(expr.property_values[0], tuple[expr.values[0] - 1],
                                                                     e0)) return false;
                                compare<uint32_t> cmp;
                                return cmp(e0, expr.values[1], expr);
                            } else {
                                if (!get_node_property_value_string(expr.property_values[0], tuple[expr.values[0] - 1],
                                                                    s0)) return false;
                                compare<std::string> cmp;
                                return cmp(s0, expr.strs[1], expr);
                            }
                        } else {
                            if (is_property_numeric(false, expr.property_values[0])) {
                                if (!get_edge_property_value_numeric(expr.property_values[0], tuple[expr.values[0] - 1],
                                                                     e0)) return false;
                                compare<uint32_t> cmp;
                                return cmp(e0, expr.values[1], expr);
                            } else {
                                if (!get_edge_property_value_string(expr.property_values[0], tuple[expr.values[0] - 1],
                                                                    s0)) return false;
                                compare<std::string> cmp;
                                return cmp(s0, expr.strs[1], expr);
                            }
                        }
                    }
                } else {
                    if (expr.is_var[0] && expr.is_var[1]) {
                        compare<uint32_t> cmp;
                        return cmp(tuple[expr.values[0] - 1], tuple[expr.values[1] - 1], expr);
                    } else if (!expr.is_var[0] && expr.is_var[1]) {
                        compare<uint32_t> cmp;
                        return cmp(expr.values[0], tuple[expr.values[1] - 1], expr);
                    } else if (expr.is_var[0] && !expr.is_var[1]) {
                        compare<uint32_t> cmp;
                        return cmp(tuple[expr.values[0] - 1], expr.values[1], expr);
                    }
                }
                return false;
            }

            bool check_expr(const query::where_expr_parser::expr &expr, const std::vector<uint32_t> &tuple) {
                if (expr.type == query::WAND) {
                    for (const auto &e: expr.args) {
                        if (!check_expr_cmp(e, tuple)) return false;
                    }
                    return true;
                } else if (expr.type == query::WOR) {
                    for (const auto &e: expr.args) {
                        if (check_expr_cmp(e, tuple)) return true;
                    }
                    return false;
                } else {
                    return check_expr_cmp(expr, tuple);
                }
            }

            void filter() {
                std::vector<std::vector<uint32_t> > res_filtered;
                for (const auto &tuple: m_res) {
                    if (check_expr(m_query.where, tuple)) {
                        res_filtered.push_back(tuple);
                    }
                }
                m_res = std::move(res_filtered);
            }

        public:
            const std::vector<std::vector<uint32_t> > &res = m_res;

            query_checker(std::vector<spo_triple> *ptr_triples, std::vector<std::vector<uint32_t> > *ptr_node_labels,
                          std::vector<std::vector<std::pair<uint32_t, uint32_t> > > *ptr_numeric_properties,
                          std::vector<std::vector<std::pair<uint32_t, std::string> > > *ptr_string_properties,
                          std::unordered_map<uint32_t, std::pair<bool, uint32_t>>  *ptr_node_properties,
                          std::unordered_map<uint32_t, std::pair<bool, uint32_t>> *ptr_edge_properties,
                          const std::string &query) {
                m_ptr_triples = ptr_triples;
                m_ptr_node_labels = ptr_node_labels;
                m_ptr_numeric_properties = ptr_numeric_properties;
                m_ptr_string_properties = ptr_string_properties;
                m_ptr_node_properties = ptr_node_properties;
                m_ptr_edge_properties = ptr_edge_properties;
                m_query = query::pg_query(query);
            };

            void run() {
                std::vector<uint32_t> tuple(m_query.ht.size(), 0);
                run(0, 1, tuple);
                filter();
            }

            void sort() {
                std::sort(m_res.begin(), m_res.end());
            }
        };
    }
}

#endif //CHECK_HPP
