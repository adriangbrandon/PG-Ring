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

    private:
        std::vector<triple_type> m_patterns;

    public:

        const std::vector<triple_type>& patterns = m_patterns;

        pg_query(const std::string& query) {
            std::unordered_map<std::string, uint8_t> ht;
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
                    m_patterns.push_back(triple_parser::parse(pat, ht));
                }
                start = pos + 1;
                while (start < query.size() && isspace(query[start])) ++start; //skip ws
            }
        }
    };


}
}

#endif // RING_CYPHER_QUERY_PARSER_HPP
