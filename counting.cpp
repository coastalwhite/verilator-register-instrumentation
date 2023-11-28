#include "counting.hpp"

#include <bits/stdc++.h>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <endian.h>
#include <string>

BitFlipCounter::BitFlipCounter() : value(0) {}

BitFlipCounter::~BitFlipCounter() {
    const char *env_bitflips_name = std::getenv("VRI_BITFLIP_FILENAME");
    std::string file_name;

    if (env_bitflips_name == nullptr) {
        file_name = "vri-bitflips";
    } else {
        file_name = env_bitflips_name;
    };

    std::ofstream outfile(file_name, std::ios::out | std::ios::binary);
    if (!outfile) {
        std::cout << "Failed to open counting output file!" << std::endl;
        return;
    }

	uint64_t value = htobe64(this->value);
    outfile.write((const char *)&value, sizeof(uint64_t));
    outfile.close();

    if (!outfile.good()) {
        std::cout << "Error occurred while writing counting output!"
                  << std::endl;
        return;
    }
}

BitFlipCounter __vri_bfcntr = BitFlipCounter();