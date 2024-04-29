import struct
import os
import struct

# function that creates a binary file with floats 1 to n
def create_binary_file(n, file_name):
    file_path = os.path.join(os.path.dirname(__file__), file_name)
    with open(file_path, 'wb') as file:
        for i in range(n):
            binary_data = struct.pack('f', float(i))
            file.write(binary_data)

def main():
    for i in range(20):
        create_binary_file(2**(i+1), f'binaryFile{i}.bin')

if __name__ == '__main__':
    main()