#ifndef __COUNTING_H
#define __COUNTING_H

#include "verilated.h"

#include <atomic>
#include <bits/stdc++.h>
#include <ctime>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>

// Output the `value` with a `name` to the output file.
void output_counter_value(const std::string name, const uint64_t value) {
    // Define one timestamp per program, but only when the first output happens.
    // This way, we can fork all we want before the first output and files will
    // end up having a different file name.
    static std::time_t output_file_timestamp = std::time(nullptr);

    std::stringstream path;
    std::string path_str;

    path << "./countout-" << output_file_timestamp;
    path >> path_str;

    std::ofstream outfile;
    outfile.open(path_str, std::ios_base::app);
    outfile << name << ": " << value << std::endl;
}

// Program wide counter that will upon destruction output its total to the
// output file.
class GlobalCounter {
  public:
    std::atomic_uint64_t value;
    GlobalCounter() : value(0) {}
    ~GlobalCounter() { output_counter_value("flipflop toggles", this->value); }
};

// Thread local counter that will upon destruction add its total to the
// GlobalCounter
class ThreadCounter {
  private:
    GlobalCounter &global_counter;

  public:
    uint64_t value;
    ThreadCounter(GlobalCounter &global_counter)
        : global_counter(global_counter), value(0) {}
    ~ThreadCounter() { this->global_counter.value += this->value; }
};

// We have both a program wide counter and a thread local counter.
//
// This allows additions to be really cheap and not need atomic operations,
// while we can just sync it at the end into the global counter with
// destructors.
//
// Note that we are guarenteed that the thread_local destructors are called
// before the static destructor.
//
// > Destructors for initialized objects with thread storage duration within a
// > given thread are called as a result of returning from the initial function
// > of that thread and as a result of that thread calling std::exit. The
// > completions of the destructors for all initialized objects with thread
// > storage duration within that thread are sequenced before the initiation of
// > the destructors of any object with static storage duration.
static GlobalCounter __bf_glob_counter = GlobalCounter();
thread_local ThreadCounter __bf_counter = ThreadCounter(__bf_glob_counter);

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
            __bf_counter.value += hamming_distance;                            \
            data = other.data;                                                 \
            return *this;                                                      \
        }                                                                      \
    };

DEFINE_COUNTING_TYPE(CountingCData, CData)
DEFINE_COUNTING_TYPE(CountingSData, SData)
DEFINE_COUNTING_TYPE(CountingIData, IData)
DEFINE_COUNTING_TYPE(CountingQData, QData)

#endif // __COUNTING_H