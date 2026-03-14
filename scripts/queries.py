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

QUERY_FILE = "gql.ok.1000.cypher"
REPEAT = 5
MAX_RESULTS = 1000
TIMEOUT = 600  # seconds
OUTPUT_DIR = Path("results")

OUTPUT_DIR.mkdir(exist_ok=True)
SUMMARY_FILE = OUTPUT_DIR / "summary.tsv"

# ==========================
# LOAD QUERIES
# ==========================
def load_queries(file_path):
    queries = []

    for line in Path(file_path).read_text(encoding="utf-8").splitlines():
        q = line.strip()
        if not q:
            continue

        # if MAX_RESULTS == 0 → no limit applied
        if MAX_RESULTS == 0:
            queries.append(q)
            continue

        # avoid adding LIMIT twice
        if " LIMIT " in q.upper():
            queries.append(q)
        else:
            queries.append(f"{q} LIMIT {MAX_RESULTS}")

    return queries

# ==========================
# TIMED EXECUTION
# ==========================
def timed_run(session, query):
    start = time.perf_counter()
    try:
        result = session.run(query, timeout = TIMEOUT)
        list(result)  # force full execution
        end = time.perf_counter()
        return (end - start) * 1000, "OK"
    except Exception as e:
        end = time.perf_counter()
        elapsed = (end - start) * 1000
        error_msg = str(e).lower()

        # Check if it's a timeout
        if "timeout" in error_msg or "timed out" in error_msg:
            print(f" Query timeout after {elapsed:.2f}ms: {e}")
            return elapsed, "TIMEOUT"
        else:
            print(f" Query error: {e}")
            return None, "ERROR"

def count_run(session, query):
    try:
        result = session.run(query, timeout= TIMEOUT)  # Longer timeout for warmup
        count = sum(1 for _ in result)
        return count
    except Exception as e:
        print(f" Query aborted (timeout or error): {e}")
        return None

# ==========================
# EXPORT RESULTS (NO TIMING)
# ==========================
def export_results(session, query, output_file):
    result = session.run(query, timeout = TIMEOUT)
    keys = result.keys()

    count = 0
    with output_file.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, delimiter="\t")

        for i, record in enumerate(result):
            count += 1
            row = [record.get(k, "") for k in keys]
            writer.writerow(row)
    return count

# ==========================
# BENCHMARK + EXPORT
# ==========================
def benchmark_and_export(queries):
    driver = GraphDatabase.driver(URI, auth=(USERNAME, PASSWORD), notifications_disabled_categories=["DEPRECATION"])

    summary_f = SUMMARY_FILE.open("w", newline="", encoding="utf-8")
    summary_writer = csv.writer(summary_f, delimiter="\t")

    with driver.session() as session:
        for idx, query in enumerate(queries, start=1):
            print(f"\nQuery {idx}")

            #In memory warmup (not timed)
            c = count_run(session, query)
            print(f" Warmup complete (not timed): {c} results")

            # ---- TIMED RUNS ----
            times = []
            statuses = []
            for r in range(REPEAT):
                elapsed, status = timed_run(session, query)
                if status == "OK" and elapsed / 1000 >= TIMEOUT:
                    status = "TIMEOUT"
                    elapsed = TIMEOUT * 1000  # Cap time at timeout limit
                times.append(elapsed
                statuses.append(status)

                if status == "OK":
                    print(f"  Run {r+1}: {elapsed:.6f}ms")
                elif status == "TIMEOUT":
                    print(f"  Run {r+1}: TIMEOUT after {elapsed:.6f}ms")
                    break  # Stop further runs on timeout
                else:  # ERROR
                    print(f"  Run {r+1}: ERROR")
                    break  # Stop further runs on error

            # ---- CALCULATE SUMMARY ----
            # If any run had an error (not timeout), mark as 'E'
            if "ERROR" in statuses:
                time_result = "E"
                print(f"  Result: ERROR")
            # If timeout, use the time it was running
            elif "TIMEOUT" in statuses:
                # Use the first timeout time
                timeout_times = [t for t, s in zip(times, statuses) if s == "TIMEOUT"]
                time_result = f"{timeout_times[0]:.6f}"
                print(f"  Result: TIMEOUT at {timeout_times[0]:.6f}ms")
            else:
                # All OK - calculate average
                valid_times = [t for t in times if t is not None]
                avg_time = sum(valid_times) / len(valid_times)
                time_result = f"{avg_time:.6f}"
                print(f"  Avg: {avg_time:.6f}ms")
                print(f"  Min: {min(valid_times):.6f}ms")
                print(f"  Max: {max(valid_times):.6f}ms")

            # ---- EXPORT RUN (NOT TIMED) ----
            #out_file = OUTPUT_DIR / f"query_{idx:05d}.tsv"
            #c = export_results(session, query, out_file)
            #print(f"  → Exported {c} rows to {out_file}")

            # ---- SUMMARY LINE ----
            summary_writer.writerow([
                idx,
                c if c is not None else "E",
                time_result
            ])

    driver.close()
    summary_f.close()

# ==========================
# MAIN
# ==========================
if __name__ == "__main__":
    queries = load_queries(QUERY_FILE)
    print(f"Loaded {len(queries)} queries")

    benchmark_and_export(queries)

    print("\nDone. Files written to:", OUTPUT_DIR.resolve())
