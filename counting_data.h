#ifndef __COUNTING_DATA_H
#define __COUNTING_DATA_H

#include "verilated.h"
#include <iostream>

#ifndef __COUNTING_IS_THREADED
#error "It is not known whether the instrumentation counting is threaded or not."
#endif

#if __COUNTING_IS_THREADED == 1
#include <atomic>

class GlobalCounter ;
class ThreadCounter ;

extern GlobalCounter __bf_glob_counter;

class GlobalCounter {
private:
	std::atomic_uint64_t value;
	std::atomic_uint num_written;
public:
    GlobalCounter() :  value(0), num_written(0) {}
    GlobalCounter& operator+= (const uint64_t& rhs)
    {
        this->value += rhs;
		this->num_written += 1;
        return *this;
    }
    
    ~GlobalCounter() {
		std::cout << "Counter: " << this->value << std::endl;
    }
};

class ThreadCounter {
private:
    uint64_t value;
public:
    ThreadCounter() : value(0) {}
    ThreadCounter& operator+= (const uint64_t& rhs)
    {
        this->value += rhs;
        return *this;
    }
    
    ~ThreadCounter() {
		__bf_glob_counter += this->value;
    }
};

GlobalCounter __bf_glob_counter = GlobalCounter();
thread_local ThreadCounter __bf_counter = ThreadCounter();

#else
class Counter {
private:
    uint64_t value;
public:
    Counter() : value(0) {}
    Counter& operator+= (const uint64_t& addition)
    {
        this->value += addition;
        return *this;
    }
    
    ~Counter() {
        std::cout << "Counter: " << this->value << std::endl;
    }
};

static Counter __bf_counter = Counter();
#endif

#include <bits/stdc++.h>
#include <stdint.h>
#include <iostream>

class CountingCData {
private:
    CData data;
public:
    CountingCData() : data(0) {}
    CountingCData(CData i) : data(i) {}
    operator CData() const { return (CData)this->data; }
    CountingCData& operator=(const CountingCData& other)
    {
        __bf_counter += __builtin_popcount(this->data ^ other.data);
        data = other.data;
        return *this;
    }
};

class CountingSData {
private:
    SData data;
public:
    CountingSData() : data(0) {}
    CountingSData(SData i) : data(i) {}
    operator SData() const { return (SData)this->data; }
    CountingSData& operator=(const CountingSData& other)
    {
        __bf_counter += __builtin_popcount(this->data ^ other.data);
        data = other.data;
        return *this;
    }
};

class CountingIData {
private:
    IData data;
public:
    CountingIData() : data(0) {}
    CountingIData(IData i) : data(i) {}
    operator IData() const { return (IData)this->data; }
    CountingIData& operator=(const CountingIData& other)
    {
        __bf_counter += __builtin_popcount(this->data ^ other.data);
        data = other.data;
        return *this;
    }
};

class CountingQData {
private:
    QData data;
public:
    CountingQData() : data(0) {}
    CountingQData(QData i) : data(i) {}
    operator QData() const { return (QData)this->data; }
    CountingQData& operator=(const CountingQData& other)
    {
        __bf_counter += __builtin_popcount(this->data ^ other.data);
        data = other.data;
        return *this;
    }
};

#endif // __COUNTING_DATA_H