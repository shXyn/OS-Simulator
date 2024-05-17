#include "simOS.hpp"

SimOS::SimOS( int numberOfDisks, unsigned long long amountOfRAM, unsigned int pageSize)
:amountOfFrames_{amountOfRAM/pageSize},pageSize_{pageSize},currentPID_{1},currentCPU_{NO_PROCESS},diskQueues_(numberOfDisks),currentIORequests_(numberOfDisks)
{
    memory_.resize(amountOfFrames_);
    for (unsigned long long i = 0; i < amountOfFrames_; ++i)
    {
        memory_[i].pageNumber = i;
    }
}

void SimOS::NewProcess()
{
    int pid =  currentPID_++;
    pChildren_[pid].push_back(0);
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
    pChildren_[pid];
    pChildren_[currentCPU_].push_back(pid);
    UpdateCPU(pid);
}

void SimOS::TimerInterrupt()
{
    readyQueue_.push_back(currentCPU_);
    UpdateCPU();
}

//UNFINISHED
void SimOS::SimExit()
{
    //If parent has not called wait 
        //process becomes zombie, removed from pChildren_
    //If parent has called wait, removed from pChildren_ and pChildren_[parent]
    UpdateCPU();
}

void SimOS::SimWait()
{

}

void SimOS::AccessMemoryAddress(unsigned long long address)
{
}