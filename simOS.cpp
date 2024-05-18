#include "simOS.h"

SimOS::SimOS( int numberOfDisks, unsigned long long amountOfRAM, unsigned int pageSize)
:amountOfFrames_{amountOfRAM/pageSize},pageSize_{pageSize},currentPID_{1},currentCPU_{NO_PROCESS},diskQueues_(numberOfDisks),currentIORequests_(numberOfDisks),recencyCount_{1}
{
    memoryCounter_.resize(amountOfFrames_);
    physicalMemory_.resize(amountOfFrames_);
    for (unsigned long long i = 0; i < amountOfFrames_; ++i)
    {
        physicalMemory_[i].frameNumber = i;
    }
}

void SimOS::NewProcess()
{
    int pid =  currentPID_++;
    Process newProcess;
    newProcess.PID = pid;
    newProcess.logicalMemory.resize(amountOfFrames_);
    processes_[pid] = newProcess;
    UpdateCPU(pid);

}

int SimOS::GetCPU()
{
    return currentCPU_;
}

void SimOS::UpdateCPU()
{
    if (!readyQueue_.empty())
    {
        currentCPU_ = readyQueue_.front();
        readyQueue_.pop_front();
    }
    else
    {
        currentCPU_ = NO_PROCESS;
    }
}

void SimOS::UpdateCPU(int pid)
{
    if (GetCPU() == NO_PROCESS)
        currentCPU_ = pid;
    else
        readyQueue_.push_back(pid);
}

void SimOS::DiskReadRequest( int diskNumber, std::string fileName)
{
    FileReadRequest newRequest {currentCPU_,fileName};

    if(currentIORequests_[diskNumber].PID == 0)
    {
        currentIORequests_[diskNumber] = newRequest;
    }
    else
    {
        diskQueues_[diskNumber].push_back(newRequest);
    }
    UpdateCPU();
}

FileReadRequest SimOS::GetDisk( int diskNumber )
{
    return currentIORequests_[diskNumber];
}

std::deque<FileReadRequest> SimOS::GetDiskQueue( int diskNumber )
{
    return diskQueues_[diskNumber];
}

void SimOS::DiskJobCompleted( int diskNumber )
{
    UpdateCPU(currentIORequests_[diskNumber].PID);
    if (diskQueues_[diskNumber].empty())
        currentIORequests_[diskNumber] = FileReadRequest();
    else
    {
        currentIORequests_[diskNumber] = diskQueues_[diskNumber].front();
        diskQueues_[diskNumber].pop_front();
    }
}

std::deque<int> SimOS::GetReadyQueue()
{
    return readyQueue_;
}

void SimOS::SimFork()
{
    int pid = currentPID_++;
    Process newFork{pid,currentCPU_,false,false};
    newFork.logicalMemory.resize(amountOfFrames_);
    processes_[pid] = newFork;
    processes_[currentCPU_].children.push_back(newFork.PID);
    UpdateCPU(pid);
}

void SimOS::TimerInterrupt()
{
    readyQueue_.push_back(currentCPU_);
    UpdateCPU();
}

void SimOS::SimExit()
{
    auto& process = processes_[currentCPU_];
    
    if (process.parentPID == 0)
    {
        TerminateProcess(currentCPU_);
    }
    else if(!processes_[process.parentPID].isWaiting)
    {
        process.isZombie = true;
        process.logicalMemory.clear();
        for(auto child : process.children)
        {
            TerminateProcess(currentCPU_);
        }
    }
    else
    {
        UpdateCPU(process.parentPID);
        TerminateProcess(currentCPU_);
    }
    UpdateCPU();
}

void SimOS::SimWait()
{
    auto& process = processes_[currentCPU_];
    if (process.children.empty())
    {
        return;
    }
    for(auto it = process.children.begin(); it != process.children.end(); ++it)
    {
        if(processes_[*it].isZombie)
        {
            TerminateProcess(*it);
            process.children.erase(it);
            return;
        }
    }
    process.isWaiting = true;
    UpdateCPU();
}

void SimOS::TerminateProcess(int pid)
{
    auto& process = processes_[pid];
    if (!processes_[pid].children.empty())
    {
        auto childrenCopy = process.children;
        for(int child : childrenCopy)
        {
            TerminateProcess(child);
        }
    }
    
    if (!process.logicalMemory.empty())
    {
        for (auto& logItem : process.logicalMemory) {
            
            {
                physicalMemory_[logItem.frameNumber] = MemoryItem();
                memoryCounter_[logItem.frameNumber] = 1;
            }
        }
    }
    process = Process();
}

void SimOS::AccessMemoryAddress(unsigned long long address)
{
    unsigned long long processPage = address/pageSize_;
    auto& selectProcess = processes_[currentCPU_].logicalMemory[processPage];

    if(selectProcess.PID != 0 && physicalMemory_[selectProcess.frameNumber].PID == selectProcess.PID)
    {
        memoryCounter_[selectProcess.frameNumber] = recencyCount_++;
    }

    else{
        unsigned long long processFrame=0;
        auto leastRecent = std::min_element(memoryCounter_.begin(),memoryCounter_.end());
        if (leastRecent != memoryCounter_.end())
            processFrame = std::distance(memoryCounter_.begin(),leastRecent);
        MemoryItem newItem{processPage,processFrame,currentCPU_};

        memoryCounter_[processFrame] = recencyCount_++;
        physicalMemory_[newItem.frameNumber] = newItem;
        selectProcess = newItem;
    }
}

MemoryUsage SimOS::GetMemory()
{
    MemoryUsage output;
    for(auto it = physicalMemory_.begin(); it != physicalMemory_.end(); ++it)
    {
        if(it->PID != 0 && !processes_[it->PID].isZombie)
        {
            output.push_back(*it);
        }
    }
    return output;
}