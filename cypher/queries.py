from neo4j import GraphDatabase
import time
import csv
from pathlib import Path

# ==========================
# CONFIG
# ==========================
URI = "bolt://localhost:7687"
USERNAME = "neo4j"
PASSWORD = "password"

QUERY_FILE = "gql.ok.cypher"
REPEAT = 5
MAX_RESULTS = 1000
OUTPUT_DIR = Path("results")

OUTPUT_DIR.mkdir(exist_ok=True)

# ==========================
# LOAD QUERIES
# ==========================
def load_queries(file_path):
    return [
        line.strip()
        for line in Path(file_path).read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]

# ==========================
# TIMED EXECUTION
# ==========================
def timed_run(session, query):
    start = time.perf_counter()
    result = session.run(query)
    list(result)  # force full execution
    end = time.perf_counter()
    return (end - start) * 1000

# ==========================
# EXPORT RESULTS (NO TIMING)
# ==========================
def export_results(session, query, output_file):
    result = session.run(query)
    keys = result.keys()

    with output_file.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, delimiter="\t")

        for i, record in enumerate(result):
            if i >= MAX_RESULTS:
                break
            row = [record.get(k, "") for k in keys]
            writer.writerow(row)

# ==========================
# BENCHMARK + EXPORT
# ==========================
def benchmark_and_export(queries):
    driver = GraphDatabase.driver(URI, auth=(USERNAME, PASSWORD))

    with driver.session() as session:
        for idx, query in enumerate(queries, start=1):
            print(f"\nQuery {idx}")

            # ---- TIMED RUNS ----
            times = []
            for r in range(REPEAT):
                elapsed = timed_run(session, query)
                times.append(elapsed)
                print(f"  Run {r+1}: {elapsed:.6f}ms")

            print(f"  Avg: {sum(times)/len(times):.6f}ms")
            print(f"  Min: {min(times):.6f}ms")
            print(f"  Max: {max(times):.6f}ms")

            # ---- EXPORT RUN (NOT TIMED) ----
            out_file = OUTPUT_DIR / f"query_{idx:03d}.tsv"
            export_results(session, query, out_file)
            print(f"  → Exported first {MAX_RESULTS} rows to {out_file}")

    driver.close()

# ==========================
# MAIN
# ==========================
if __name__ == "__main__":
    queries = load_queries(QUERY_FILE)
    print(f"Loaded {len(queries)} queries")

    benchmark_and_export(queries)

    print("\nDone. Files written to:", OUTPUT_DIR.resolve())
