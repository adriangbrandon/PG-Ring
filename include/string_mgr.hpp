//
// Created by adrian on 17/12/25.
//

#ifndef STRING_MGR_HPP
#define STRING_MGR_HPP
#include <set>
#include <string>
#include <vector>
#include <sdsl/io.hpp>

#include "query/where_expr_parser.hpp"

namespace ring {

    class string_mgr {

    public:
        typedef std::string::size_type size_type;
        typedef std::string value_type;
        
    private:

        std::vector<std::string> m_strings;
        


        size_type first_ge(const std::string &x) {
            auto it = std::lower_bound(m_strings.begin(), m_strings.end(), x);
            return std::distance(m_strings.begin(), it)+1;
        }

        size_type last_se(const std::string &x) {
            auto it = std::lower_bound(m_strings.begin(), m_strings.end(), x);
            if (it != m_strings.end() && *it == x) {
                return std::distance(m_strings.begin(), it)+1;
            }
            //*it > x
            if (it == m_strings.begin()) return 0; //the first is greater than x
            return std::distance(m_strings.begin(), it); //the previous
        }

        void copy(const string_mgr &o) {
            m_strings = o.m_strings;
        }

    public:

        string_mgr() = default;

        string_mgr(std::set<std::string> &strings) {
            m_strings.assign(strings.begin(), strings.end());
        }

        string_mgr(std::vector<std::string> &strings) {
            m_strings = std::move(strings);
        }

        size_type find(const std::string &x){
            auto it = std::lower_bound(m_strings.begin(), m_strings.end(), x);
            size_type id = std::distance(m_strings.begin(), it)+1;
            if (it != m_strings.end() && *it == x) {
                return id;
            }
            return 0;
        }

        size_type get_id(const std::string &s, query::enum_comp_where_type comp_type) {
            switch (comp_type) {
                case query::NEQ:
                case query::EQ:
                    return find(s);
                case query::GE:
                case query::ST:
                    return first_ge(s);
                case query::GT:
                case query::SE:
                    return last_se(s);
                default:
                    return 0;
            }
        }

        string_mgr(const string_mgr &o) {
            copy(o);
        }

        string_mgr(string_mgr &&o) noexcept {
            *this = std::move(o);
        }

        string_mgr& operator=(const string_mgr &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        string_mgr& operator=(string_mgr &&o) noexcept {
            if (this != &o) {
                m_strings = std::move(o.m_strings);
            }
            return *this;
        }

        void swap(string_mgr &o) noexcept{
            std::swap(m_strings, o.m_strings);
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += sdsl::write_member(m_strings.size(), out,  child, "size");
            for (const auto &s : m_strings) {
                written_bytes += sdsl::write_member(s.size(), out, child, "length");
                written_bytes += sdsl::write_member(s, out, child, "data");
            }
            return written_bytes;
        }

        void load(std::istream &in) {
            size_type size, length;
            std::string data;
            sdsl::read_member(size, in);
            m_strings.resize(size);
            for (size_type i = 0; i < size; ++i) {
                sdsl::read_member(length, in);
                data.resize(length);
                sdsl::read_member(data, in);
                m_strings[i] = std::move(data);
            }
        }

    };
    
}
#endif //STRING_MGR_HPP
