/*
 * build-index.cpp
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
#include <sdsl/construct.hpp>
#include <utils.hpp>

#include "ring_pg.hpp"

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;



/*template<class ring>
void build_index(const std::string &dataset, const std::string &output){
    vector<spo_triple> triples;
    vector<vector<uint32_t>> label2nodes;
    std::vector<std::vector<std::pair<uint32_t, std::string>>> nprop2values;
    std::vector<std::vector<std::pair<uint32_t, std::string>>> eprop2values;
    std::vector<bool> nprop_numeric;
    std::vector<bool> eprop_numeric;

    std::string triples_file = dataset + ".triples";
    std::string labels_map = dataset + ".label2nodes";
    std::string base_nprop = dataset + ".nprop2values.";
    std::string base_eprop = dataset + ".eprop2values.";

    {
        std::ifstream ifs(triples_file);
        uint64_t s, p , o;
        do {
            ifs >> s >> p >> o;
            if(ifs.eof()) break;
            triples.emplace_back(spo_triple(s, p, o));
        } while (true);
    }

    {
        std::ifstream ifs(labels_map);
        uint32_t node, size, label;
        do {
            ifs >> label;
            if(ifs.eof()) break;
            ifs >> size;
            label2nodes.emplace_back();
            for (uint32_t i = 0; i < size; i++) {
                ifs >> node;
                label2nodes.back().push_back(node);
            }
        } while (true);
    }

    {
        uint32_t prop_id = 1;
        uint32_t node_id;
        std::string value;
        do {
            std::string file = base_nprop + std::to_string(prop_id);
            std::ifstream ifs(file);
            if (!ifs.good()) break;
            do {
                ifs >> node_id;
                if(ifs.eof()) break;
                if (prop_id > nprop2values.size()) {
                    nprop2values.emplace_back();
                }
                std::getline(ifs, value);
                value = value.substr(1); // remove leading space
                if (!value.empty() && value.back() == '\r') value.pop_back();
                nprop2values[prop_id-1].emplace_back(node_id, value);
            } while (true);
            ifs.close();
            nprop_numeric.push_back(::ring::util::is_number(value));
            ++prop_id;
        } while (true);
    }

    {
        uint32_t prop_id = 1;
        uint32_t edge_id;
        std::string value;
        do {
            std::string file = base_eprop + std::to_string(prop_id);
            std::ifstream ifs(file);
            if (!ifs.good()) break;
            do {
                ifs >> edge_id;
                if(ifs.eof()) break;
                if (prop_id > eprop2values.size()) {
                    eprop2values.emplace_back();
                }
                std::getline(ifs, value);
                value = value.substr(1); // remove leading space
                if (!value.empty() && value.back() == '\r') value.pop_back();
                eprop2values[prop_id-1].emplace_back(edge_id, value);
            } while (true);
            eprop_numeric.push_back(::ring::util::is_number(value));
            ++prop_id;
        } while (true);
    }


    triples.shrink_to_fit();
    cout << "--Indexing " << triples.size() << " triples" << endl;
    memory_monitor::start();
    auto start = timer::now();

    ring A(triples, label2nodes, nprop2values, eprop2values, nprop_numeric, eprop_numeric);
    auto stop = timer::now();
    memory_monitor::stop();
    cout << "  Index built  " << sdsl::size_in_bytes(A) << " bytes" << endl;

    sdsl::store_to_file(A, output);
    cout << "Index saved" << endl;
    cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
    cout << memory_monitor::peak() << " bytes." << endl;

}*/

int main(int argc, char **argv)
{

    if(argc != 3){
        std::cout << "Usage: " << argv[0] << " <dataset> [ring|c-ring|ring-sel|ring-muthu|c-ring-muthu|ring-sel-muthu]" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string type    = argv[2];
    if (type == "pg") {
        std::string index_name = dataset + ".ring.pg";
        memory_monitor::start();
        auto start = timer::now();
        ring::ring_pg<> A(dataset);
        auto stop = timer::now();
        memory_monitor::stop();
        cout << "  Index built  " << sdsl::size_in_bytes(A) << " bytes" << endl;
        sdsl::store_to_file(A, index_name);
        cout << "Index saved" << endl;
        cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        cout << memory_monitor::peak() << " bytes." << endl;
    }
   /* if(type == "ring"){
        std::string index_name = dataset + ".ring";
        build_index<ring::ring<>>(dataset, index_name);
    }else if (type == "c-ring"){
        std::string index_name = dataset + ".c-ring";
        build_index<ring::c_ring>(dataset, index_name);
    }else if (type == "ring-sel"){ //TODO: usar este para property graphs
        std::string index_name = dataset + ".ring-sel";
        build_index<ring::ring_sel>(dataset, index_name);
    }else if (type == "ring-muthu"){
        std::string index_name = dataset + ".ring-muthu";
        build_index<ring::ring_muthu<>>(dataset, index_name);
    }else if (type == "c-ring-muthu"){
        std::string index_name = dataset + ".c-ring-muthu";
        build_index<ring::c_ring_muthu>(dataset, index_name);
    }else if (type == "ring-sel-muthu"){
        std::string index_name = dataset + ".ring-sel-muthu";
        build_index<ring::ring_sel_muthu>(dataset, index_name);
    }else{
        std::cout << "Usage: " << argv[0] << " <dataset> [ring|c-ring|ring-sel|ring-muthu|c-ring-muthu|ring-sel-muthu]" << std::endl;
    }*/

    return 0;
}

