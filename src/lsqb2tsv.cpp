//
// Created by adrian on 24/3/26.
//
// Converter from LSQB format (projected-fk) to Ring TSV format
//

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
        output << line << "\t" << label << "\n";
        count++;
    }

    std::cout << "  Processed " << count << " " << label << " nodes" << std::endl;
}

// Process node files with multiple labels (e.g., Message:Comment)
void process_node_file_multi_label(const std::string& filepath,
                                   const std::vector<std::string>& labels,
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

        output << line << "\t" << label_str << "\n";
        count++;
    }

    std::cout << "  Processed " << count << " " << label_str << " nodes" << std::endl;
}

// Process edge files (they contain from|to, one pair per line)
void process_edge_file(const std::string& filepath,
                      const std::string& edge_type,
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
        output << fields[0] << "\t" << edge_type << "\t" << fields[1] << "\n";
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

    process_node_file(input_dir + "/Continent.csv", "Continent", nodes_file);
    process_node_file(input_dir + "/Country.csv", "Country", nodes_file);
    process_node_file(input_dir + "/City.csv", "City", nodes_file);
    process_node_file(input_dir + "/University.csv", "University", nodes_file);
    process_node_file(input_dir + "/Company.csv", "Company", nodes_file);
    process_node_file(input_dir + "/TagClass.csv", "TagClass", nodes_file);
    process_node_file(input_dir + "/Tag.csv", "Tag", nodes_file);
    process_node_file(input_dir + "/Forum.csv", "Forum", nodes_file);
    process_node_file(input_dir + "/Person.csv", "Person", nodes_file);

    // Message:Comment and Message:Post have multiple labels
    process_node_file_multi_label(input_dir + "/Comment.csv", {"Message", "Comment"}, nodes_file);
    process_node_file_multi_label(input_dir + "/Post.csv", {"Message", "Post"}, nodes_file);

    std::cout << std::endl;
    std::cout << "Processing edges..." << std::endl;

    // Process edges (relationships)
    process_edge_file(input_dir + "/Country_isPartOf_Continent.csv", "IS_PART_OF", edges_file);
    process_edge_file(input_dir + "/City_isPartOf_Country.csv", "IS_PART_OF", edges_file);
    process_edge_file(input_dir + "/TagClass_isSubclassOf_TagClass.csv", "IS_SUBCLASS_OF", edges_file);
    process_edge_file(input_dir + "/University_isLocatedIn_City.csv", "IS_LOCATED_IN", edges_file);
    process_edge_file(input_dir + "/Company_isLocatedIn_Country.csv", "IS_LOCATED_IN", edges_file);
    process_edge_file(input_dir + "/Tag_hasType_TagClass.csv", "HAS_TYPE", edges_file);
    process_edge_file(input_dir + "/Comment_hasCreator_Person.csv", "HAS_CREATOR", edges_file);
    process_edge_file(input_dir + "/Comment_isLocatedIn_Country.csv", "IS_LOCATED_IN", edges_file);
    process_edge_file(input_dir + "/Comment_replyOf_Comment.csv", "REPLY_OF", edges_file);
    process_edge_file(input_dir + "/Comment_replyOf_Post.csv", "REPLY_OF", edges_file);
    process_edge_file(input_dir + "/Comment_hasTag_Tag.csv", "HAS_TAG", edges_file);
    process_edge_file(input_dir + "/Post_hasCreator_Person.csv", "HAS_CREATOR", edges_file);
    process_edge_file(input_dir + "/Post_isLocatedIn_Country.csv", "IS_LOCATED_IN", edges_file);
    process_edge_file(input_dir + "/Post_hasTag_Tag.csv", "HAS_TAG", edges_file);
    process_edge_file(input_dir + "/Forum_containerOf_Post.csv", "CONTAINER_OF", edges_file);
    process_edge_file(input_dir + "/Forum_hasMember_Person.csv", "HAS_MEMBER", edges_file);
    process_edge_file(input_dir + "/Forum_hasModerator_Person.csv", "HAS_MODERATOR", edges_file);
    process_edge_file(input_dir + "/Forum_hasTag_Tag.csv", "HAS_TAG", edges_file);
    process_edge_file(input_dir + "/Person_hasInterest_Tag.csv", "HAS_INTEREST", edges_file);
    process_edge_file(input_dir + "/Person_isLocatedIn_City.csv", "IS_LOCATED_IN", edges_file);
    process_edge_file(input_dir + "/Person_knows_Person.csv", "KNOWS", edges_file);
    process_edge_file(input_dir + "/Person_likes_Comment.csv", "LIKES", edges_file);
    process_edge_file(input_dir + "/Person_likes_Post.csv", "LIKES", edges_file);
    process_edge_file(input_dir + "/Person_studyAt_University.csv", "STUDY_AT", edges_file);
    process_edge_file(input_dir + "/Person_workAt_Company.csv", "WORK_AT", edges_file);

    nodes_file.close();
    edges_file.close();

    std::cout << std::endl;
    std::cout << "Conversion completed successfully!" << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  - " << nodes_output << std::endl;
    std::cout << "  - " << edges_output << std::endl;

    return 0;
}

