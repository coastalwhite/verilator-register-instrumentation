#include "coverage.hpp"

#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>

CoverageMap::CoverageMap()
    : prev_location(0) {
    std::fill_n(this->map, COVMAP_SIZE, 0);
}
void CoverageMap::AddPoint(uint64_t cur_location) {
    uint64_t offset = cur_location ^ (prev_location >> 1);
    this->prev_location = cur_location;
    offset &= COVMAP_OFFSET_MASK;
    this->map[offset] += 1;
}
CoverageMap::~CoverageMap() {
    std::string file_name;
    const char* env_covmap_name = std::getenv("VRI_COVMAP_FILENAME");

    if (env_covmap_name == nullptr) {
        file_name = "vri-covmap";
    } else {
        file_name = env_covmap_name;
    };

    std::ofstream outfile(file_name, std::ios::out | std::ios::binary);
    if (!outfile) {
        std::cout << "Failed to open coverage file!" << std::endl;
        return;
    }

    outfile.write((char *)this->map, COVMAP_SIZE);
    outfile.close();

    if (!outfile.good()) {
        std::cout << "Error occurred while writing coverage!" << std::endl;
        return;
    }
}

CoverageMap __vri_covmap = CoverageMap();
