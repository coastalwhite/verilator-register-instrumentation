#ifndef __COUNTING_DATA_H
#define __COUNTING_DATA_H

#ifndef __COUNTING_IS_THREADED
#error                                                                         \
    "It is not known whether the instrumentation counting is threaded or not."
#endif

#include "verilated.h"

#include <bits/stdc++.h>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

static std::time_t output_file_timestamp = std::time(nullptr);

// Output the `value` with a `name` to the output file.
void output_counter_value(const std::string name, const uint64_t value) {
    std::ofstream outfile;

    std::stringstream path;
    path << "./countout-";
    path << output_file_timestamp;

    std::string path_str;
    path >> path_str;

    outfile.open(path_str, std::ios_base::app);
    outfile << name << ": " << value << std::endl;
}

#if __COUNTING_IS_THREADED == 1
#include <atomic>

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

#else
class Counter {
  private:
    uint64_t value;

  public:
    Counter() : value(0) {}
    Counter &operator+=(const uint64_t &addition) {
        this->value += addition;
        return *this;
    }

    ~Counter() { output_counter_value("flipflop toggles", this->value); }
};

static Counter __bf_counter = Counter();
#endif

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
            __bf_counter += __builtin_popcount(bitdiff);                       \
            data = other.data;                                                 \
            return *this;                                                      \
        }                                                                      \
    };

DEFINE_COUNTING_TYPE(CountingCData, CData)
DEFINE_COUNTING_TYPE(CountingSData, SData)
DEFINE_COUNTING_TYPE(CountingIData, IData)
DEFINE_COUNTING_TYPE(CountingQData, QData)

#endif // __COUNTING_DATA_H