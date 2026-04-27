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
#include <sdsl/construct.hpp>
#include <utils.hpp>

#include "ring_pg.hpp"

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;




int main(int argc, char **argv)
{

    if(argc !=2 && argc != 3){
        std::cout << "Usage: " << argv[0] << " <dataset> [pg]" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string type = "pg";
    if(argc == 3){ type = argv[2]; }
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

    return 0;
}

