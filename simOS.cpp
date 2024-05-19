//Henry Tse


#include "SimOS.h"

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
        for(auto it = readyQueue_.begin(); it != readyQueue_.end();)
        {
            if (processes_[*it].PID == 0 || processes_[*it].isZombie)
            {
                it = readyQueue_.erase(it);
            }
            else
            {
                ++it;
            }
        }
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
    if (currentCPU_ == NO_PROCESS)
    {
        currentCPU_ = pid;
    }
    else{
        readyQueue_.push_back(pid);
        if (readyQueue_.size() == 0)
            readyQueue_.push_back(pid);
    }
}

void SimOS::DiskReadRequest( int diskNumber, std::string fileName)
{
    if(diskNumber >= diskQueues_.size())
    {
        throw std::out_of_range("Attempt to access out of bound disk index\n");
    }
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
    if(diskNumber >= diskQueues_.size())
    {
        throw std::out_of_range("Attempt to access out of bound disk index\n");
    }
    return currentIORequests_[diskNumber];
}

std::deque<FileReadRequest> SimOS::GetDiskQueue( int diskNumber )
{
    if(diskNumber >= diskQueues_.size())
    {
        throw std::out_of_range("Attempt to access out of bound disk index\n");
    }
    return diskQueues_[diskNumber];
}

void SimOS::DiskJobCompleted( int diskNumber )
{
    if(diskNumber >= diskQueues_.size())
    {
        throw std::out_of_range("Attempt to access out of bound disk index\n");
    }
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
    if (currentCPU_ == NO_PROCESS) {
        throw std::logic_error("Instruction require running process, but CPU is idle.\n");
    }

    int pid = currentPID_++;
    Process newFork{pid,currentCPU_,false,false};
    newFork.logicalMemory.resize(amountOfFrames_);
    processes_[pid] = newFork;
    processes_[currentCPU_].children.push_back(newFork.PID);
    UpdateCPU(pid);
}

void SimOS::TimerInterrupt()
{
    if (currentCPU_ == NO_PROCESS) {
        throw std::logic_error("Instruction require running process, but CPU is idle.\n");
    }
    readyQueue_.push_back(currentCPU_);
    UpdateCPU();
}

void SimOS::SimExit()
{
    if (currentCPU_ == NO_PROCESS) {
        throw std::logic_error("Instruction require running process, but CPU is idle.\n");
    }
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
    UpdateDisk();
}

void SimOS::SimWait()
{
    if (currentCPU_ == NO_PROCESS) {
        throw std::logic_error("Instruction require running process, but CPU is idle.\n");
    }
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
    if (currentCPU_ == NO_PROCESS) {
        throw std::logic_error("Instruction require running process, but CPU is idle.\n");
    }
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

void SimOS::UpdateDisk()
{
    for(int i = 0; i < currentIORequests_.size(); i++)
    {
        if (processes_[currentIORequests_[i].PID].PID == 0 || processes_[currentIORequests_[i].PID].isZombie)
        {
            currentIORequests_[i] = FileReadRequest();
        }
        for (auto it = diskQueues_[i].begin(); it != diskQueues_[i].end();)
        {
            if (processes_[it->PID].PID == 0 || processes_[it->PID].isZombie)
            {
                it = diskQueues_[i].erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (currentIORequests_[i].fileName == "" && !diskQueues_[i].empty())
        {
            currentIORequests_[i] = diskQueues_[i].front();
            diskQueues_[i].pop_front();
        }
    }   
}