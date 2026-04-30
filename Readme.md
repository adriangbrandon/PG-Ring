# PG-Ring

Query engine for graphs based on the work presented in "Uplifting the Superpowers of Worst-Case-Optimal Join Algorithms"

## Build

### Requirements

- CMake 2.8.7 or higher
- C++11 compiler (GCC 4.7+ or Clang 3.2+)
- [SDSL-lite](https://github.com/adriangbrandon/sdsl-lite) (extended version)
- [libCSD](https://github.com/migumar2/libCSD) (adapted and included in this project)

### Instructions

```bash
mkdir build
cd build
cmake ..
make -DCMAKE_BUILD_TYPE=Release
```

## Main Executables

### build-index

Builds indexes from graph data.

```bash
./build-index <data-path>
```

**Arguments:**
- `<data-path>`: Path to input dataset

**Output:** Creates a `.ring.pg` index file in the same directory as the input data.

### query

Executes queries on the built indexes.

```bash
./query <index-path> <queries-file> [max-results] [repeat] [timeout]
```

**Arguments:**
- `<index-path>`: Path to the `.ring.pg` index file
- `<queries-file>`: File containing queries (one per line)
- `[max-results]`: Maximum number of results (0 = unlimited, default)
- `[repeat]`: Number of times to execute each query (default: 1)
- `[timeout]`: Timeout limit in seconds

**Output:** For each query prints: `<query-number>;<num-results>;<time-ms>`

## Query Format

Queries follow a graph pattern syntax:

```
(?var:Label)-[?edge:TYPE]->(?var2:Label2), ...
```

**Example:**
```
(?p:Person)-[?e1:KNOWS]->(?p2:Person), (?p2:Person)-[?e2:LIKES]->(?m:Message)
```

Queries can include `WHERE` clauses for filters:
```
... WHERE (?var1 != ?var2) AND ...
```

## Project Structure

### Main Folders

- **`include/`** - Header files with algorithms and iterators
  - `ring_pg.hpp` - Main implementation of the Ring engine
  - `ltj_algorithm_pg.hpp` - Leapfrog Triejoin algorithm
  - `ltj_iterator_*.hpp` - Iterators for different pattern types
  - `veo_*.hpp` - Variable evaluation order optimizers
  - `query/` - Query parser components

- **`libCSD/`** - Compact string dictionaries library

- **`Queries/`** - Benchmark queries
  - `lsqb/` - LSQB queries
  - `wd/` - Wikidata queries

- **`res/`** - Results and experimental data

## Libraries

- **SDSL** - Succinct Data Structure Library (extended version)
- **libCSD** - Compact String Dictionaries

## License

This software is distributed under the GNU General Public License v3.0.
