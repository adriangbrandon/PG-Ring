import pandas as pd

names = ['ring', 'c-ring', 'jena', 'jena-ltj', 'rdf3x', 'virtuoso', 'blazegraph']

def replace_comma_with_period(value):
    if isinstance(value, str):
        return value.replace(',', '.')
    return value

def write_columns_to_files(input_csv, field_separator=';'):
    try:
        df = pd.read_csv(input_csv, sep=field_separator, header=None)
        i = 0
        for column in df.columns:
            output_file = names[i]
            df[column] = df[column].apply(replace_comma_with_period)
            with open(output_file, "w") as file:
                file.write("\\begin{filecontents}{large-"+names[i]+".dat}\n")
                file.write("\n".join(map(str, df[column])))
                file.write("\n\\end{filecontents}")
            i = i+1

        print("Files created successfully.")
    except FileNotFoundError:
        print(f"Error: File '{input_csv}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    input_csv_file = "wikidata-large.csv"  # Replace with the path to your input CSV file
    write_columns_to_files(input_csv_file)
