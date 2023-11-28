#include "read_binary.hpp"

int main(int argc, char ** argv) {
    if (argc < 3) {
        std::cout << "USAGE: ./reader file.bin <number of data points>" << std::endl;
        return 0;
    }

    std::vector<float> cloud_data = readFloatBinaryFile(argv[1], atoi(argv[2]));
    if (cloud_data.empty()) return 1;

    std::cout << "Successfully read " << cloud_data.size() << " data points from " << argv[1] << std::endl;
    return 0;
}