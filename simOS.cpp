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
    Process newProcess;
    newProcess.PID = pid;
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
        Process newZombie{currentCPU_,process.parentPID,false,true};
        TerminateProcess(currentCPU_);
        process = newZombie;
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
    for(auto it = process.children.begin(); it != process.children.begin(); ++it)
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
    if (processes_[pid].children.empty())
        processes_[pid] = Process();
    else{
        for(int child : processes_[pid].children)
        {
            TerminateProcess(child);
        }
        processes_[pid] = Process();
    }

}

void SimOS::AccessMemoryAddress(unsigned long long address)
{

}