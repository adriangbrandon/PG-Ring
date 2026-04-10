//
// Created by adrian on 26/1/26.
//

#ifndef TSV_HELPER_HPP
#define TSV_HELPER_HPP

#include <vector>
#include <string>
#include <sstream>

#include "cypher/cypher_create_helper.hpp"
#include "query/constant_utils.hpp"

namespace tsv_helper {

    typedef struct {
        std::string key;
        std::string value;
    } property_tsv_type;

    typedef struct {
        std::string variable;
        std::vector<std::string> labels;
        std::vector<property_tsv_type> properties;
    } node_tsv_type;

    typedef struct {
        std::string type;
        std::vector<property_tsv_type> properties;
        std::string from;
        std::string to;
    } edge_tsv_type;


    static std::string trim(std::string &str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    static std::vector<std::string> parse_labels(const std::string &labels_str) {
        std::vector<std::string> labels;
        std::stringstream ss(labels_str);
        std::string label;
        while (std::getline(ss, label, ':')) {
            if (!label.empty()) {
                labels.push_back(trim(label));
            }
        }
        return labels;
    }

    static property_tsv_type parse_property(const std::string &prop_str) {
        std::stringstream ss(prop_str);
        std::string key_str, val_str;
        std::getline(ss, key_str, ':');
        std::getline(ss, val_str);
        property_tsv_type prop;
        prop.key = trim(key_str);
        prop.value = trim(val_str);
        return prop;
    }

    static node_tsv_type parse_node(const std::string &line) {
        node_tsv_type node;

        std::stringstream ss(line);
        std::string var_str, labels_str, prop_str;
        std::getline(ss, var_str, '\t');
        std::getline(ss, labels_str, '\t');
        node.variable = trim(var_str);
        node.labels = parse_labels(labels_str);
        while (std::getline(ss, prop_str, '\t')) {
            if (!prop_str.empty()) node.properties.push_back(parse_property(prop_str));
        }
        return node;
    }

    static edge_tsv_type parse_edge(const std::string &line) {
        edge_tsv_type edge;

        std::stringstream ss(line);
        std::string from_str, to_str, type_str, prop_str;
        std::getline(ss, from_str, '\t');
        std::getline(ss, type_str, '\t');
        std::getline(ss, to_str, '\t');
        edge.from = trim(from_str);
        edge.to = trim(to_str);
        edge.type = trim(type_str);
        while (std::getline(ss, prop_str, '\t')) {
            edge.properties.push_back(parse_property(prop_str));
        }
        return edge;
    }


    static std::string fix_date_cypher(const std::string &value) {
        //avoid months 00 and days 00
        std::string fixed = value;
        uint64_t beg = value.find('-', 1);
        if (value[beg + 1] == '0' && value[beg + 2] == '0') {
            fixed[beg + 2] = '1';
        }
        if (value[beg + 4] == '0' && value[beg + 5] == '0') {
            fixed[beg + 5] = '1';
        }
        return fixed;
    }

    static std::string node_to_cypher(const node_tsv_type &node) {
        std::string res;
        res += "CREATE (" + node.variable;
        for (const auto &label: node.labels) {
            res += ":" + label;
        }
        res += " { qid: \"" + node.variable + "\"";
        if (!node.properties.empty()) {
            res += ", ";
            for (size_t i = 0; i < node.properties.size(); ++i) {
                int64_t value;
                if (ring::query::constant::is_date(node.properties[i].value, value)) {
                    res += node.properties[i].key + ": datetime('" + fix_date_cypher(node.properties[i].value) + "')";
                }else {
                    res += node.properties[i].key + ": " + node.properties[i].value;
                }

                if (i < node.properties.size() - 1) res += ", ";
            }
        }
        res += "}";
        res += ")";
        return res;
    }


    static std::string node_cypher(const std::string &name) {
        bool fix = false;
        for (size_t i = 0; i < name.size(); ++i) {
            if (!std::isalpha(name[i])) {
                fix = true;
            }
        }
        if (!fix) return name;
        std::string res = "`" + name + "`";
        return res;
    }



    static std::string edge_to_cypher(const edge_tsv_type &edge) {
        std::string res;
        res += "MATCH (a {qid:\"" + edge.from + "\"}), (b {qid:\"" + edge.to + "\"}) ";
        res += "CREATE (a)-[:" + edge.type;
        if (!edge.properties.empty()) {
            res += " {";
            for (size_t i = 0; i < edge.properties.size(); ++i) {
                int64_t value;
                if (ring::query::constant::is_date(edge.properties[i].value, value)) {
                    res += edge.properties[i].key + ": datetime('" + fix_date_cypher(edge.properties[i].value) + "')";
                }else {
                    res += edge.properties[i].key + ": " + edge.properties[i].value;
                }
                if (i < edge.properties.size() - 1) res += ", ";
            }
            res += "}";
        }
        res += "]->(b)";
        return res;
    }


    static std::string format_date_milldb(const std::string &date_str) {
        // Remove the leading '+' if present (MillenniumDB parser doesn't accept it for positive years)
        std::string formatted = date_str;
        if (!formatted.empty() && formatted[0] == '+') {
            formatted = formatted.substr(1);
        }
        return "dateTimeStamp(\"" + formatted + "\")";
    }


    static std::string node_to_milldb(const node_tsv_type &node) {
        std::string res;
        // ID
        res += node.variable;

        // Labels
        for (const auto &label: node.labels) {
            res += " :" + label;
        }

        // Properties
        for (size_t i = 0; i < node.properties.size(); ++i) {
            int64_t value;
            res += " " + node.properties[i].key + ":";
            // Check if it's a string first (to avoid misdetecting quoted values as dates)
            if (ring::query::constant::is_string(node.properties[i].value)) {
                res += node.properties[i].value;
            } else if (ring::query::constant::is_date(node.properties[i].value, value)) {
                res += format_date_milldb(node.properties[i].value);
            } else {
                res += node.properties[i].value;
            }
        }

        return res;
    }


    static std::string edge_to_milldb(const edge_tsv_type &edge) {
        std::string res;
        // IDs: from->to
        res += edge.from + "->" + edge.to;

        // Edge type (label)
        res += " :" + edge.type;

        // Properties
        for (size_t i = 0; i < edge.properties.size(); ++i) {
            int64_t value;
            res += " " + edge.properties[i].key + ":";
            // Check if it's a string first (to avoid misdetecting quoted values as dates)
            if (ring::query::constant::is_string(edge.properties[i].value)) {
                res += edge.properties[i].value;
            } else if (ring::query::constant::is_date(edge.properties[i].value, value)) {
                res += format_date_milldb(edge.properties[i].value);
            } else {
                res += edge.properties[i].value;
            }
        }

        return res;
    }


}
#endif //TSV_HELPER_HPP
