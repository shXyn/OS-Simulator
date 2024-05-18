#ifndef PROCESS_H
#define PROCESS_H

#include<iostream>
#include<algorithm>
#include<vector>
#include<deque>
#include<unordered_map>
#include <iterator>
#include <stdexcept>
#include "SimOS.h"

class Process
{
    private:
    int PID {0};
    int parentPID {0};
    bool isWaiting = false;
    bool isZombie = false;
    std::vector<int> children;
    MemoryUsage logicalMemory;
};

#endif