#!/usr/bin/env python3
"""
Script to extract selected lines from gql.ok.tsv based on line numbers in gql.ok.1000.lines
and save them to gql.ok.1000.tsv in the same order.
"""

def extract_selected_lines(lines_file, input_tsv, output_tsv):
    """
    Read line numbers from lines_file, extract those lines from input_tsv,
    and write them to output_tsv in the same order.

    Args:
        lines_file: Path to file containing line numbers (one per line)
        input_tsv: Path to input TSV file (gql.ok.tsv)
        output_tsv: Path to output TSV file (gql.ok.1000.tsv)
    """
    # Read line numbers
    print(f"Reading line numbers from {lines_file}...")
    with open(lines_file, "r", encoding="utf-8") as f:
        line_numbers = [int(line.strip()) for line in f if line.strip()]

    print(f"Found {len(line_numbers)} line numbers")

    # Read all lines from input TSV
    print(f"Reading {input_tsv}...")
    with open(input_tsv, "r", encoding="utf-8") as f:
        all_lines = f.readlines()

    print(f"Total lines in {input_tsv}: {len(all_lines)}")

    # Extract selected lines (convert to 0-based indexing)
    selected_lines = []
    for line_num in line_numbers:
        if 1 <= line_num <= len(all_lines):
            selected_lines.append(all_lines[line_num - 1])
        else:
            print(f"Warning: Line number {line_num} is out of range")

    # Write to output file
    print(f"Writing {len(selected_lines)} lines to {output_tsv}...")
    with open(output_tsv, "w", encoding="utf-8") as f:
        f.writelines(selected_lines)

    print(f"Done! Created {output_tsv}")


if __name__ == "__main__":
    extract_selected_lines(
        lines_file="gql.ok.1000.lines",
        input_tsv="gql.ok.tsv",
        output_tsv="gql.ok.1000.tsv"
    )

