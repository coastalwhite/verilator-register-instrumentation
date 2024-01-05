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
uint8_t* CoverageMap::Compress() {
    for (int i = 0; i < COVMAP_SIZE / 2; i++) {
        uint8_t lhs = this->map[i*2];
        uint8_t rhs = this->map[i*2+1];

        lhs = (lhs == 0) ? 0b1111 : __builtin_clz(lhs);
        rhs = (rhs == 0) ? 0b1111 : __builtin_clz(rhs);
        
        this->map[i] = (lhs << 4) | rhs;
    }

    return (uint8_t*) this->map;
}

CoverageMap::~CoverageMap() {
    const char* env_covmap_name = std::getenv("VRI_COVMAP_FILENAME");

    if (env_covmap_name == nullptr) {
        return;
    }

    std::string file_name;
    file_name = env_covmap_name;

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
