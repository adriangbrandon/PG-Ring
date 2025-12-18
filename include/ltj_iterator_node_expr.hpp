/*
 * ltj_iterator.hpp
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

#ifndef RING_LTJ_ITERATOR_NODE_EXPR_HPP
#define RING_LTJ_ITERATOR_NODE_EXPR_HPP

#define VERBOSE 0


#include <ltj_iterator_base.hpp>
#include <query/query_parser.hpp>

namespace ring {
    template<class ring_t, class var_t, class cons_t>
    class ltj_iterator_node_expr : public ltj_iterator_base<var_t, cons_t> {
    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef uint64_t size_type;
        typedef query::triple_parser::triple_type pattern_type;
        typedef query::label_expr_parser::expr_label_type expr_type;
        typedef wt_range_iterator<typename ring_type::bwt_type::wm_type> wt_so_iterator_type;
        typedef wt_range_iterator<typename ring_type::bwt_p_type::wm_type> wt_p_iterator_type;
        //std::vector<value_type> leap_result_type;

    private:
        const expr_type *m_expr;
        ring_type *m_ptr_ring; //TODO: should be const
        bool m_is_empty = false;
        bool m_is_subject = false;
        value_type m_value = 0;


        void copy(const ltj_iterator_node_expr &o) {
            m_is_empty = o.m_is_empty;
            m_is_subject = o.m_is_subject;
            m_expr = o.m_expr;
            m_ptr_ring = o.m_ptr_ring;
            m_value = o.m_value;
        }

        value_type next_node_rec(value_type c, const expr_type* expr) {
            if (expr->type == query::LAB) {
                return m_ptr_ring->next_node_label(expr->label, c);
            }else if (expr->type == query::NEG) {
                return m_ptr_ring->next_node_neg_label(expr->label, c);
            }else if (expr->type == query::OR) {
                value_type ans = -1ULL;
                for (size_type i = 0; i < expr->args.size(); ++i) {
                    value_type val = next_node_rec(c, &expr->args[i]);
                    if (val != 0) ans = std::min(ans, val);
                }
                return (ans != -1ULL) ? ans : 0;
            }else if (expr->type == query::AND) {
                value_type ans = c, aux;
                size_type ok = 0;
                size_type i = 0;
                while (ans) {
                    aux = next_node_rec(ans, &expr->args[i]);
                    if (aux == 0) return 0;
                    if (aux == ans) {
                        ++ok;
                    }else {
                        ok = 1;
                        ans = aux;
                    }
                    if (ok == expr->args.size()) return ans;
                    i = ++i % expr->args.size();
                }
            }
            return 0;
        }

        value_type next_node(value_type c) {
            return next_node_rec(c, m_expr);
        }

    public:
        //const bool &is_empty = m_is_empty;

        ltj_iterator_node_expr() = default;

        ltj_iterator_node_expr(const expr_type *expr, ring_type *ring, bool is_subject) {
            m_is_subject = is_subject;
            m_expr = expr;
            m_ptr_ring = ring;
        }

        //! Copy constructor
        ltj_iterator_node_expr(const ltj_iterator_node_expr &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_node_expr(ltj_iterator_node_expr &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_node_expr &operator=(const ltj_iterator_node_expr &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_node_expr &operator=(ltj_iterator_node_expr &&o) {
            if (this != &o) {
                m_expr = std::move(o.m_expr);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_is_empty = o.m_is_empty;
                m_value = o.m_value;
                m_is_subject = o.m_is_subject;
            }
            return *this;
        }

        void swap(ltj_iterator_node_expr &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_expr, o.m_expr);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_value, o.m_value);
            std::swap(m_is_subject, o.m_is_subject);
        }


        inline bool is_variable_subject(var_type var) {
            return m_is_subject;
        }

        inline bool is_variable_predicate(var_type var) {
            return false;
        }

        inline bool is_variable_object(var_type var) {
            return !m_is_subject;
        }


        inline bool is_empty() {
            return m_is_empty;
        }

        void down(var_type var, size_type c) {
            //Go down in the trie
            m_value = c;
        };

        void down(var_type var, size_type c, size_type k) {
            down(var, c);
        };


        void up(var_type var) {
            //Go up in the trie
            m_value = 0;
        };

        value_type leap(var_type var) {
            return next_node(1);
        };

        value_type leap(var_type var, size_type c) {
            return next_node(c);
        };

        inline bool in_last_level() {
            return false;
        }

        inline size_type interval_length() const {
            //TODO: complicado saber o numero de triples xa que a expresión pode ser complexa
            return 1; //TODO: fix this depending on the priority
        }

        value_type seek_last(var_type var) {
            return 0;
        }

        value_type seek_last_next(var_type var) {
            return 0;
        }

       /* void set_prop_value(var_type var, value_type value) {
            throw std::out_of_range("ltj_iterator_node_expr::set_value_property");
        }

        value_type get_prop_value(var_type var, value_type c) {
            throw std::out_of_range("ltj_iterator_node_expr::get_prop_value");
        }

        value_type compute_prop_value(var_type var, value_type c) {
            throw std::out_of_range("ltj_iterator_node_expr::compute_prop_value");
        }*/
    };
}

#endif //RING_LTJ_ITERATOR_NODE_EXPR_HPP
