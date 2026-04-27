/*
 * Copyright (C) 2026 Author removed for double-blind evaluation
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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <sys/stat.h>

// Check if directory exists (C++11 compatible)
bool dir_exists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

// Parse a simple CSV line (IDs only, no properties)
std::vector<std::string> parse_csv_line(const std::string& line, char delimiter = '|') {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string field;

    while (std::getline(ss, field, delimiter)) {
        result.push_back(field);
    }

    return result;
}

// Process node files (they contain just IDs, one per line)
void process_node_file(const std::string& filepath,
                       const std::string& label,
                       const std::string& node_type_prefix,
                       std::ofstream& output) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "Warning: Could not open " << filepath << std::endl;
        return;
    }

    std::string line;
    int count = 0;
    bool first_line = true;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        // Skip header line
        if (first_line) {
            first_line = false;
            continue;
        }

        // Remove any whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        // LSQB format: just the ID
        // Ring TSV format: variable\tlabels
        // Add prefix to make IDs globally unique
        output << node_type_prefix << "_" << line << "\t" << label << "\n";
        count++;
    }

    std::cout << "  Processed " << count << " " << label << " nodes" << std::endl;
}

// Process node files with multiple labels (e.g., Message:Comment)
void process_node_file_multi_label(const std::string& filepath,
                                   const std::vector<std::string>& labels,
                                   const std::string& node_type_prefix,
                                   std::ofstream& output) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "Warning: Could not open " << filepath << std::endl;
        return;
    }

    // Build label string
    std::string label_str;
    for (size_t i = 0; i < labels.size(); ++i) {
        if (i > 0) label_str += ":";
        label_str += labels[i];
    }

    std::string line;
    int count = 0;
    bool first_line = true;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        // Skip header line
        if (first_line) {
            first_line = false;
            continue;
        }

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        // Add prefix to make IDs globally unique
        output << node_type_prefix << "_" << line << "\t" << label_str << "\n";
        count++;
    }

    std::cout << "  Processed " << count << " " << label_str << " nodes" << std::endl;
}

// Process edge files (they contain from|to, one pair per line)
void process_edge_file(const std::string& filepath,
                      const std::string& edge_type,
                      const std::string& from_node_type,
                      const std::string& to_node_type,
                      std::ofstream& output) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "Warning: Could not open " << filepath << std::endl;
        return;
    }

    std::string line;
    int count = 0;
    bool first_line = true;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        // Skip header line
        if (first_line) {
            first_line = false;
            continue;
        }

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        // Parse from|to
        auto fields = parse_csv_line(line, '|');
        if (fields.size() < 2) {
            std::cerr << "Warning: Invalid edge line: " << line << std::endl;
            continue;
        }

        // Ring TSV format: from\ttype\tto
        // Add prefixes to make IDs globally unique
        output << from_node_type << "_" << fields[0] << "\t"
               << edge_type << "\t"
               << to_node_type << "_" << fields[1] << "\n";
        count++;
    }

    std::cout << "  Processed " << count << " " << edge_type << " edges" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <lsqb_data_dir> <output_prefix>" << std::endl;
        std::cout << "Converts LSQB projected-fk format to Ring TSV format" << std::endl;
        std::cout << std::endl;
        std::cout << "Input: <lsqb_data_dir>/*.csv (LSQB projected-fk format)" << std::endl;
        std::cout << "Output:" << std::endl;
        std::cout << "  - <output_prefix>-nodes.tsv" << std::endl;
        std::cout << "  - <output_prefix>-edges.tsv" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " /path/to/social-network-sf0.003-projected-fk lsqb-sf0.003" << std::endl;
        return 1;
    }

    std::string input_dir = argv[1];
    std::string output_prefix = argv[2];

    // Check if input directory exists
    if (!dir_exists(input_dir)) {
        std::cerr << "Error: Input directory does not exist: " << input_dir << std::endl;
        return 1;
    }

    std::string nodes_output = output_prefix + "-nodes.tsv";
    std::string edges_output = output_prefix + "-edges.tsv";

    std::ofstream nodes_file(nodes_output);
    std::ofstream edges_file(edges_output);

    if (!nodes_file.is_open()) {
        std::cerr << "Error: Could not create output file: " << nodes_output << std::endl;
        return 1;
    }

    if (!edges_file.is_open()) {
        std::cerr << "Error: Could not create output file: " << edges_output << std::endl;
        return 1;
    }

    std::cout << "Converting LSQB data from: " << input_dir << std::endl;
    std::cout << "Output prefix: " << output_prefix << std::endl;
    std::cout << std::endl;

    // Process nodes (in the order used by Neo4j loader for consistency)
    std::cout << "Processing nodes..." << std::endl;

    process_node_file(input_dir + "/Continent.csv", "Continent", "Continent", nodes_file);
    process_node_file(input_dir + "/Country.csv", "Country", "Country", nodes_file);
    process_node_file(input_dir + "/City.csv", "City", "City", nodes_file);
    process_node_file(input_dir + "/University.csv", "University", "University", nodes_file);
    process_node_file(input_dir + "/Company.csv", "Company", "Company", nodes_file);
    process_node_file(input_dir + "/TagClass.csv", "TagClass", "TagClass", nodes_file);
    process_node_file(input_dir + "/Tag.csv", "Tag", "Tag", nodes_file);
    process_node_file(input_dir + "/Forum.csv", "Forum", "Forum", nodes_file);
    process_node_file(input_dir + "/Person.csv", "Person", "Person", nodes_file);

    // Message:Comment and Message:Post have multiple labels
    process_node_file_multi_label(input_dir + "/Comment.csv", {"Message", "Comment"}, "Comment", nodes_file);
    process_node_file_multi_label(input_dir + "/Post.csv", {"Message", "Post"}, "Post", nodes_file);

    std::cout << std::endl;
    std::cout << "Processing edges..." << std::endl;

    // Process edges (relationships)
    process_edge_file(input_dir + "/Country_isPartOf_Continent.csv", "IS_PART_OF", "Country", "Continent", edges_file);
    process_edge_file(input_dir + "/City_isPartOf_Country.csv", "IS_PART_OF", "City", "Country", edges_file);
    process_edge_file(input_dir + "/TagClass_isSubclassOf_TagClass.csv", "IS_SUBCLASS_OF", "TagClass", "TagClass", edges_file);
    process_edge_file(input_dir + "/University_isLocatedIn_City.csv", "IS_LOCATED_IN", "University", "City", edges_file);
    process_edge_file(input_dir + "/Company_isLocatedIn_Country.csv", "IS_LOCATED_IN", "Company", "Country", edges_file);
    process_edge_file(input_dir + "/Tag_hasType_TagClass.csv", "HAS_TYPE", "Tag", "TagClass", edges_file);
    process_edge_file(input_dir + "/Comment_hasCreator_Person.csv", "HAS_CREATOR", "Comment", "Person", edges_file);
    process_edge_file(input_dir + "/Comment_isLocatedIn_Country.csv", "IS_LOCATED_IN", "Comment", "Country", edges_file);
    process_edge_file(input_dir + "/Comment_replyOf_Comment.csv", "REPLY_OF", "Comment", "Comment", edges_file);
    process_edge_file(input_dir + "/Comment_replyOf_Post.csv", "REPLY_OF", "Comment", "Post", edges_file);
    process_edge_file(input_dir + "/Comment_hasTag_Tag.csv", "HAS_TAG", "Comment", "Tag", edges_file);
    process_edge_file(input_dir + "/Post_hasCreator_Person.csv", "HAS_CREATOR", "Post", "Person", edges_file);
    process_edge_file(input_dir + "/Post_isLocatedIn_Country.csv", "IS_LOCATED_IN", "Post", "Country", edges_file);
    process_edge_file(input_dir + "/Post_hasTag_Tag.csv", "HAS_TAG", "Post", "Tag", edges_file);
    process_edge_file(input_dir + "/Forum_containerOf_Post.csv", "CONTAINER_OF", "Forum", "Post", edges_file);
    process_edge_file(input_dir + "/Forum_hasMember_Person.csv", "HAS_MEMBER", "Forum", "Person", edges_file);
    process_edge_file(input_dir + "/Forum_hasModerator_Person.csv", "HAS_MODERATOR", "Forum", "Person", edges_file);
    process_edge_file(input_dir + "/Forum_hasTag_Tag.csv", "HAS_TAG", "Forum", "Tag", edges_file);
    process_edge_file(input_dir + "/Person_hasInterest_Tag.csv", "HAS_INTEREST", "Person", "Tag", edges_file);
    process_edge_file(input_dir + "/Person_isLocatedIn_City.csv", "IS_LOCATED_IN", "Person", "City", edges_file);
    process_edge_file(input_dir + "/Person_knows_Person_bidirectional.csv", "KNOWS", "Person", "Person", edges_file);
    process_edge_file(input_dir + "/Person_likes_Comment.csv", "LIKES", "Person", "Comment", edges_file);
    process_edge_file(input_dir + "/Person_likes_Post.csv", "LIKES", "Person", "Post", edges_file);
    process_edge_file(input_dir + "/Person_studyAt_University.csv", "STUDY_AT", "Person", "University", edges_file);
    process_edge_file(input_dir + "/Person_workAt_Company.csv", "WORK_AT", "Person", "Company", edges_file);

    nodes_file.close();
    edges_file.close();

    std::cout << std::endl;
    std::cout << "Conversion completed successfully!" << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  - " << nodes_output << std::endl;
    std::cout << "  - " << edges_output << std::endl;

    return 0;
}

