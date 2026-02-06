//
// Created by adrian on 26/1/26.
//

#ifndef TSV_HELPER_HPP
#define TSV_HELPER_HPP

#include <vector>
#include <string>
#include <sstream>

#include "cypher/cypher_create_helper.hpp"

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


}
#endif //TSV_HELPER_HPP
