/*
 * query.cpp
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

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <sys/stat.h>
#include <sdsl/construct.hpp>
#include <utils.hpp>

#include "ring_pg.hpp"
#include "ltj_algorithm_pg.hpp"
#include "results_collector_test.hpp"
#include "query/query_parser.hpp"

using namespace std;
using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

typedef ring::ring_pg<> ring_type;

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <index_file> <queries_file> <max_results> <repeat> <timeout>" << std::endl;
    std::cout << "  <index_file>   : Path to the .ring.pg index file" << std::endl;
    std::cout << "  <queries_file> : File containing queries (one per line)" << std::endl;
    std::cout << "  <max_results>  : Maximum number of results to return (0 = unlimited)" << std::endl;
    std::cout << "  <repeat>       : Number of times to execute each query" << std::endl;
    std::cout << "  <timeout>      : Timeout limit" << std::endl;
}

bool create_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(path.c_str(), 0755) == 0;
}

int main(int argc, char **argv)
{
    if(argc < 5) {
        print_usage(argv[0]);
        return 1;
    }

    std::string index_file = argv[1];
    std::string queries_file = argv[2];
    uint64_t max_results = std::stoull(argv[3]);
    uint64_t repeat = std::stoull(argv[4]);
    uint64_t timeout_sec = 600;
    if (argc > 5) timeout_sec = std::stoull(argv[5]);

    if (repeat == 0) {
        std::cerr << "Error: REPEAT must be at least 1" << std::endl;
        return 1;
    }

    // Load the index
    ring_type graph;
    std::cout << "Loading index from: " << index_file << std::endl;

    try {
        sdsl::load_from_file(graph, index_file);
        std::cout << "Index loaded successfully (" << sdsl::size_in_bytes(graph) << " bytes)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading index: " << e.what() << std::endl;
        return 1;
    }

    // Read queries from file
    std::ifstream queries_stream(queries_file);
    if (!queries_stream.is_open()) {
        std::cerr << "Error: Cannot open queries file: " << queries_file << std::endl;
        return 1;
    }

    std::vector<std::string> queries;
    std::string line;
    while (std::getline(queries_stream, line)) {
        // Remove leading and trailing whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");

        if (start != std::string::npos) {
            line = line.substr(start, end - start + 1);
            if (!line.empty()) {
                queries.push_back(line);
            }
        }
    }
    queries_stream.close();

    if (queries.empty()) {
        std::cerr << "Error: No queries found in file" << std::endl;
        return 1;
    }

    std::cout << "Loaded " << queries.size() << " queries" << std::endl;
    std::cout << "Results limit: " << (max_results == 0 ? "unlimited" : std::to_string(max_results)) << std::endl;
    std::cout << "Repeat count: " << repeat << std::endl;
    std::cout << "Timeout: " << timeout_sec << " seconds" << std::endl;

    // Create results directory
    std::string results_dir = "results";
    if (!create_directory(results_dir)) {
        std::cerr << "Error: Cannot create results directory" << std::endl;
        return 1;
    }

    // Open summary file
    std::string summary_file = results_dir + "/summary.tsv";
    std::ofstream summary_stream(summary_file);
    if (!summary_stream.is_open()) {
        std::cerr << "Error: Cannot create summary file: " << summary_file << std::endl;
        return 1;
    }

    // Process each query
    typedef ring::ltj_algorithm_pg<> algorithm_type;
    typedef algorithm_type::tuple_type tuple_type;
    ::util::results_collector<tuple_type> res;
    for (size_t query_id = 0; query_id < queries.size(); ++query_id) {
        const std::string& query_string = queries[query_id];

        std::cout << "\nProcessing Query " << (query_id + 1) << "/" << queries.size() << std::endl;
        std::cout << "Query: " << query_string << std::endl;

        // Create individual result file for this query with zero-padded numbering
        std::ostringstream filename;
        filename << results_dir << "/query_" << std::setfill('0') << std::setw(5) << (query_id + 1) << ".tsv";
        std::string query_result_file = filename.str();
        std::ofstream query_stream(query_result_file);
        if (!query_stream.is_open()) {
            std::cerr << "Warning: Cannot create result file for query " << (query_id + 1) << std::endl;
            continue;
        }

        try {
            // Parse the query once
            auto query = ring::query::pg_query(query_string);

            std::vector<uint64_t> execution_times;
            uint64_t num_results = 0;
            // Execute the query REPEAT times
            for (uint64_t run = 0; run < repeat; ++run) {
                res.clear();
                auto start = timer::now();
                algorithm_type ltj(&query, &graph);
                ltj.join_v3(res, max_results, timeout_sec);
                auto stop = timer::now();

                auto time_ns = duration_cast<nanoseconds>(stop - start).count();
                execution_times.push_back(time_ns);

                std::cout << "  Run " << (run + 1) << "/" << repeat
                          << ": " << res.size() << " results, "
                          << time_ns << " ns" << std::endl;

            }


            // Calculate average time
            uint64_t total_time_ns = 0;
            for (uint64_t t : execution_times) {
                total_time_ns += t;
            }
            double avg_time_ns = static_cast<double>(total_time_ns) / repeat;
            double avg_time_ms = avg_time_ns / 1e6;

            num_results = res.size();
            // Write results (from last execution)
            if (num_results > 0) {
                for (size_t i = 0; i < num_results; ++i) {
                    for (size_t j = 0; j < res[i].size(); ++j) {
                        query_stream << res[i][j];
                        if (j < res[i].size() - 1) query_stream << "\t";
                    }
                    query_stream << std::endl;
                }
            }

            query_stream.close();

            // Write to summary
            summary_stream << (query_id + 1) << "\t"
                          << res.size() << "\t"
                          << std::fixed << std::setprecision(6) << avg_time_ms << std::endl;

            std::cout << "  Average time: " << std::fixed << std::setprecision(6) << avg_time_ms << " ms." << std::endl;
            std::cout << "  Results: " << res.size() << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error executing query " << (query_id + 1) << ": " << e.what() << std::endl;

            query_stream << "Error: " << e.what() << std::endl;
            query_stream.close();

            // Write error to summary
            summary_stream << (query_id + 1) << "\t"
                          << "0" << "\t"
                          << "0.000000" << std::endl;
        }
    }

    summary_stream.close();

    std::cout << "\n=== Execution Complete ===" << std::endl;
    std::cout << "Total queries processed: " << queries.size() << std::endl;
    std::cout << "Summary saved to: " << summary_file << std::endl;
    std::cout << "Individual results saved in: " << results_dir << "/" << std::endl;

    return 0;
}

