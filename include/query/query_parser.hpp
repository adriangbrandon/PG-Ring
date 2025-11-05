#ifndef RING_CYPHER_QUERY_PARSER_HPP
#define RING_CYPHER_QUERY_PARSER_HPP
#include <string>
#include <vector>
#include <query/triple_parser.hpp>

namespace ring {
namespace query {


    class pg_query {


    public:
        typedef triple_parser::triple_type triple_type;
        typedef std::vector<triple_type> patterns_type;

    private:
        patterns_type m_patterns;
        std::unordered_map<std::string, uint8_t> m_ht;

        void copy(const pg_query &o) {
            m_patterns = o.m_patterns;
            m_ht = o.m_ht;
        }

    public:

        const patterns_type& patterns = m_patterns;
        const std::unordered_map<std::string, uint8_t>& ht = m_ht;

        pg_query() = default;

        pg_query(const std::string& query) {
            size_t start = 0;
            while (start < query.size()) {
                // Find the next comma outside parentheses/brackets
                size_t pos = start;
                int p = 0, b = 0;
                while (pos < query.size()) {
                    if (query[pos] == '(') p++;
                    if (query[pos] == ')') p--;
                    if (query[pos] == '[') b++;
                    if (query[pos] == ']') b--;
                    if (query[pos] == ',' && p == 0 && b == 0) break;
                    ++pos;
                }
                std::string pat = query.substr(start, pos - start);
                if (!pat.empty()) {
                    m_patterns.push_back(triple_parser::parse(pat, m_ht));
                }
                start = pos + 1;
                while (start < query.size() && isspace(query[start])) ++start; //skip ws
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
                m_ht = std::move(o.m_ht);
            }
            return *this;
        }

    };


}
}

#endif // RING_CYPHER_QUERY_PARSER_HPP
