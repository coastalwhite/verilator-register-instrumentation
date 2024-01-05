#ifndef _COVERAGE_H
#define _COVERAGE_H

#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

#define COVMAP_LOG2_SIZE (16)
#define COVMAP_SIZE (1 << COVMAP_LOG2_SIZE)
#define COVMAP_OFFSET_MASK (COVMAP_SIZE - 1)

class CoverageMap {
  private:
    uint8_t map[COVMAP_SIZE];
    uint64_t prev_location;

  public:
    CoverageMap();
    ~CoverageMap();

    void AddPoint(uint64_t cur_location);

    uint8_t* Compress();
};

extern CoverageMap __vri_covmap;

#endif // _COVERAGE_H