import sys

def replace_zeros(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    with open(file_path, 'w') as file:
        for line in lines:
            if '0\n' == line:
                line = '0.001\n'
            file.write(line)

# Replace zeros in a file
file_path = sys.argv[1]
replace_zeros(file_path)
