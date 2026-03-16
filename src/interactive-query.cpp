/*
 * interactive-query.cpp
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
#include <string>
#include <chrono>
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
    std::cout << "Usage: " << program_name << " <index_file> <max_results> <timeout> <output_file>" << std::endl;
    std::cout << "  <index_file>   : Path to the .ring.pg index file" << std::endl;
    std::cout << "  <max_results>  : Maximum number of results to return (0 = unlimited)" << std::endl;
    std::cout << "  <timeout>      : Timeout in seconds" << std::endl;
    std::cout << "  <output_file>  : File to save query results" << std::endl;
}

int main(int argc, char **argv)
{
    if(argc != 5) {
        print_usage(argv[0]);
        return 1;
    }

    std::string index_file = argv[1];
    uint64_t max_results = std::stoull(argv[2]);
    uint64_t timeout = std::stoull(argv[3]);
    std::string output_file = argv[4];

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

    // Open output file
    std::ofstream ofs(output_file, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open output file: " << output_file << std::endl;
        return 1;
    }

    std::cout << "\n=== Interactive Query System ===" << std::endl;
    std::cout << "Results limit: " << (max_results == 0 ? "unlimited" : std::to_string(max_results)) << std::endl;
    std::cout << "Output file: " << output_file << std::endl;
    std::cout << "Type 'exit' or 'quit' to exit\n" << std::endl;

    std::string query_string;
    uint64_t query_counter = 0;

    while (true) {
        std::cout << "\nQuery [" << query_counter + 1 << "]> ";

        // Read complete line
        if (!std::getline(std::cin, query_string)) {
            break; // EOF or error
        }

        // Remove leading and trailing whitespace
        size_t beg = query_string.find_first_not_of(" \t\r\n");
        size_t end = query_string.find_last_not_of(" \t\r\n");

        if (beg == std::string::npos) {
            continue; // Empty line
        }

        query_string = query_string.substr(beg, end - beg + 1);

        // Check exit commands
        if (query_string == "exit" || query_string == "quit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        if (query_string.empty()) {
            continue;
        }

        query_counter++;

        try {

            // Execute the query
            typedef ring::ltj_algorithm_pg<::util::results_collector_test<std::vector<uint64_t>>> algorithm_type;
            typedef algorithm_type::tuple_type tuple_type;

            ::util::results_collector_test<tuple_type> res;

            auto start = timer::now();
            // Parse the query
            auto query = ring::query::pg_query(query_string);
            // Translate string identifiers to uint32_t using dictionaries
            query.translate(&graph);
            algorithm_type ltj(&query, &graph);
            ltj.join_v3(res, max_results, timeout);
            auto stop = timer::now();

            auto time_ns = duration_cast<nanoseconds>(stop - start).count();
            auto time_ms = duration_cast<milliseconds>(stop - start).count();

            // Display results in console
            std::cout << "\nResults: " << res.size() << " tuples found";
            std::cout << " (Time: " << time_ms << " ms)" << std::endl;

            // Save to file
            ofs << "=== Query " << query_counter << " ===" << std::endl;
            ofs << "Query: " << query_string << std::endl;
            ofs << "Results: " << res.size() << std::endl;
            ofs << "Time (ms): " << time_ms << std::endl;
            ofs << "Time (ns): " << time_ns << std::endl;

            if (res.size() > 0) {
                ofs << "Tuples:" << std::endl;

                // Determine the number of variables in the query
                size_t num_vars = query.ht.size();

                // Display some results in console (limited to 10)
                size_t display_limit = std::min(static_cast<size_t>(res.size()), static_cast<size_t>(10));
                if (display_limit > 0) {
                    std::cout << "\nFirst " << display_limit << " tuples:" << std::endl;
                    for (size_t i = 0; i < display_limit; ++i) {
                        std::cout << "(";
                        for (size_t j = 0; j < res.results[i].size(); ++j) {
                            std::cout << res.results[i][j];
                            if (j < res.results[i].size() - 1) std::cout << ", ";
                        }
                        std::cout << ")" << std::endl;
                    }
                    if (res.size() > display_limit) {
                        std::cout << "... (and " << (res.size() - display_limit) << " more)" << std::endl;
                    }
                }

                // Save all results to file
                for (size_t i = 0; i < res.size(); ++i) {
                    ofs << "(";
                    for (size_t j = 0; j < res.results[i].size(); ++j) {
                        ofs << res.results[i][j];
                        if (j < res.results[i].size() - 1) ofs << ", ";
                    }
                    ofs << ")" << std::endl;
                }
            }

            ofs << std::endl;
            ofs.flush(); // Ensure immediate write

        } catch (const std::exception& e) {
            std::cerr << "Error executing query: " << e.what() << std::endl;

            ofs << "=== Query " << query_counter << " ===" << std::endl;
            ofs << "Query: " << query_string << std::endl;
            ofs << "Error: " << e.what() << std::endl;
            ofs << std::endl;
            ofs.flush();
        }
    }

    ofs.close();
    std::cout << "\nTotal queries executed: " << query_counter << std::endl;
    std::cout << "Results saved to: " << output_file << std::endl;

    return 0;
}

