#ifndef __COUNTING_H
#define __COUNTING_H

#include <stdint.h>

class BitFlipCounter {
  public:
    uint64_t value;
    BitFlipCounter();
    ~BitFlipCounter();
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
extern BitFlipCounter __bf_counter;

#endif // __COUNTING_H