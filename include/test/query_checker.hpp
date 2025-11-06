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
            std::vector<spo_triple>* m_ptr_triples; //should be sorted by POS
            query::pg_query m_query;
            std::vector<std::vector<uint32_t>> m_res;


            bool check_expr_and(const query::expr_parser::expr_type &expr, const spo_triple &triple) {
                bool match = false;
                for (const auto& arg : expr.args) {
                    if (arg.type == ring::query::LAB) {
                        if (std::get<1>(triple) != arg.label) return false;
                    } else if (arg.type == ring::query::NEG) {
                        if (std::get<1>(triple) == arg.label) return false;
                    } else if (arg.type == ring::query::OR) {
                        if (!check_expr_or(arg, triple)) return false;
                    } else if (arg.type == ring::query::AND) {
                        if (!check_expr_and(arg, triple)) return false;
                    }
                }
                return true;
            }

            bool check_expr_or(const query::expr_parser::expr_type &expr, const spo_triple &triple) {
                for (const auto& arg : expr.args) {
                    if (arg.type == ring::query::LAB) {
                        if (std::get<1>(triple) == arg.label) return true;
                    } else if (arg.type == ring::query::NEG) {
                        if (std::get<1>(triple) != arg.label) return true;
                    } else if (arg.type == ring::query::OR) {
                        if (check_expr_or(arg, triple)) return true;
                    } else if (arg.type == ring::query::AND) {
                        if (check_expr_and(arg, triple)) return true;
                    }
                }
                return false;
            }

            bool check_expr(const query::expr_parser::expr_type &expr, const spo_triple &triple) {
                if (expr.type == ring::query::LAB) {
                    return (std::get<1>(triple) == expr.label);
                } else if (expr.type == ring::query::NEG) {
                    return (std::get<1>(triple) != expr.label);
                } else if (expr.type == ring::query::OR) {
                    return (check_expr_or(expr, triple));
                } else if (expr.type == ring::query::AND) {
                    return (check_expr_and(expr, triple));
                }
                return true;
            }

            bool check(const query::triple_parser::triple &pattern, const spo_triple &triple) {

                // Check subject
                if (!pattern.subj.is_var()) {
                    if (std::get<0>(triple) != pattern.subj.const_value) return false;
                }

                // Check predicate
                if (!pattern.edge.is_var()) {
                    if (std::get<1>(triple) != pattern.edge.const_value) return false;
                } else {
                    // Check expression
                    if (!check_expr(pattern.edge.expr, triple)) return false;
                }

                // Check object
                if (!pattern.obj.is_var()) {
                    if (std::get<2>(triple) != pattern.obj.const_value) return false;
                }

                return true;
            }

            bool check_bindings(const query::triple_parser::triple &pattern, const spo_triple &triple, const std::vector<uint32_t> &tuple) {
                if (pattern.subj.is_var()) {
                    if (tuple[pattern.subj.var_value-1] && std::get<0>(triple) != tuple[pattern.subj.var_value-1]) return false;
                }
                if (pattern.obj.is_var()) {
                    if (tuple[pattern.obj.var_value-1] && std::get<2>(triple) != tuple[pattern.obj.var_value-1]) return false;
                }
                return true;
            }

            std::tuple<bool, bool, bool> bind(const query::triple_parser::triple &pattern, const spo_triple &triple, uint64_t t_i, std::vector<uint32_t> &tuple) {
                std::tuple<bool, bool, bool> bind = {false, false, false};
                if (pattern.subj.is_var() && !tuple[pattern.subj.var_value-1]) {
                    tuple[pattern.subj.var_value-1] = std::get<0>(triple);
                    std::get<0>(bind) = true;
                }
                if (pattern.edge.is_var() && !tuple[pattern.edge.var_value-1]) {
                    tuple[pattern.edge.var_value-1] = t_i;
                    std::get<1>(bind) = true;
                }
                if (pattern.obj.is_var() && !tuple[pattern.obj.var_value-1]) {
                    tuple[pattern.obj.var_value-1] = std::get<2>(triple);
                    std::get<2>(bind) = true;
                }
                return bind;
            }

            void unbind(const std::tuple<bool, bool, bool> &b, const query::triple_parser::triple &pattern, std::vector<uint32_t> &tuple) {
                if (std::get<0>(b)) {
                    tuple[pattern.subj.var_value-1] = 0;
                }
                if (std::get<1>(b)) {
                    tuple[pattern.edge.var_value-1] = 0;
                }
                if (std::get<2>(b)) {
                    tuple[pattern.obj.var_value-1] = 0;
                }
            }

            void run(uint64_t p_i, uint64_t t_i, std::vector<uint32_t>& tuple) {
                if (p_i == m_query.patterns.size()) {
                    m_res.push_back(tuple);
                    return;
                }
                if (t_i > m_ptr_triples->size()) {
                    return;
                }
                const auto& pattern = m_query.patterns[p_i];
                const auto& triple = (*m_ptr_triples)[t_i-1];
                if (check(pattern, triple)) {
                    if (check_bindings(pattern, triple, tuple)) {
                        auto b = bind(pattern, triple, t_i, tuple);
                        run(p_i+1, 1, tuple);
                        unbind(b, pattern, tuple);
                    }
                }
                run(p_i, t_i + 1, tuple);

            }

        public:

            const std::vector<std::vector<uint32_t>>& res = m_res;

            query_checker(std::vector<spo_triple>* ptr_triples, const std::string& query) {
                m_ptr_triples = ptr_triples;
                m_query = query::pg_query(query);
            };

            void run() {
                std::vector<uint32_t> tuple(m_query.ht.size(), 0);
                run(0, 1, tuple);
            }

            void sort() {
                std::sort(m_res.begin(), m_res.end());
            }

        };

    }
}

#endif //CHECK_HPP
