#ifndef __COUNTING_H
#define __COUNTING_H

#include "verilated.h"

#include <bits/stdc++.h>
#include <stdint.h>
#include <atomic>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

// Output the `value` with a `name` to the output file.
void output_counter_value(const std::string name, const uint64_t value) {
    static std::time_t output_file_timestamp = std::time(nullptr);
    
    std::ofstream outfile;

    std::stringstream path;
    path << "./countout-";
    path << output_file_timestamp;

    std::string path_str;
    path >> path_str;

    outfile.open(path_str, std::ios_base::app);
    outfile << name << ": " << value << std::endl;
}

// We have both a program wide counter and a thread local counter.
//
// This allows additions to be really cheap and not need atomic operations,
// while we can just sync it at the end into the global counter with
// destructors.
class GlobalCounter;
class ThreadCounter;

extern GlobalCounter __bf_glob_counter;

class GlobalCounter {
  private:
    std::atomic_uint64_t value;

  public:
    GlobalCounter() : value(0) {}
    GlobalCounter &operator+=(const uint64_t &rhs) {
        this->value += rhs;
        return *this;
    }

    ~GlobalCounter() { output_counter_value("flipflop toggles", this->value); }
};

class ThreadCounter {
  private:
    uint64_t value;

  public:
    ThreadCounter() : value(0) {}
    ThreadCounter &operator+=(const uint64_t &rhs) {
        this->value += rhs;
        return *this;
    }

    ~ThreadCounter() { __bf_glob_counter += this->value; }
};

GlobalCounter __bf_glob_counter = GlobalCounter();
thread_local ThreadCounter __bf_counter = ThreadCounter();

// This macro defines all the CountingXData types.
//
// All this logic is the same between types. It adds the Hamming distance with
// the previous value to the thread counter when an assignment is done.
#define DEFINE_COUNTING_TYPE(NEW_TYPE, BASE_TYPE)                              \
    class NEW_TYPE {                                                           \
      private:                                                                 \
        BASE_TYPE data;                                                        \
                                                                               \
      public:                                                                  \
        NEW_TYPE() : data(0) {}                                                \
        NEW_TYPE(BASE_TYPE i) : data(i) {}                                     \
        operator BASE_TYPE() const { return (BASE_TYPE)this->data; }           \
        NEW_TYPE &operator=(const NEW_TYPE &other) {                           \
            BASE_TYPE bitdiff = this->data ^ other.data;                       \
            uint64_t hamming_distance = __builtin_popcount(bitdiff);           \
            __bf_counter += hamming_distance;                                  \
            data = other.data;                                                 \
            return *this;                                                      \
        }                                                                      \
    };

DEFINE_COUNTING_TYPE(CountingCData, CData)
DEFINE_COUNTING_TYPE(CountingSData, SData)
DEFINE_COUNTING_TYPE(CountingIData, IData)
DEFINE_COUNTING_TYPE(CountingQData, QData)

#endif // __COUNTING_H