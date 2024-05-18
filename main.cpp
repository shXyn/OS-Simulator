



#include "SimOS.h"
//#include "Process.h"
//#include "Drive.h"
//#include "RAM.h"

int main() {
	//TESTING ERROR HANDLING
	SimOS errorSim(1,10,1);

	std::cout<<"=== Error Handling ===\n";

	//Needs to print something if CPU = NO_PROCESS
	try {
		errorSim.SimFork();
	}
	catch (const std::logic_error& err) {
		std::cerr<<err.what();
	}

	//Needs to print something if disk index is out of bounds
	try {
		errorSim.GetDisk(10);
	}
	catch (const std::out_of_range& err_2) {
		std::cerr<<err_2.what();
	}
	
	std::cout<<"----- ----- ----- ----- -----\n";
	//To ensure code above is working, do not comment it out when testing code below

	std::cout<<"=== Function testing ====\n";

	//TESTING NEW, FORK, EXIT, WAIT, INTERUPT, DISK READ, DISK COMPLETE, ACCESS MEMORY, GET MEMORY 
	SimOS sim(1,10,1);
	bool passed = true;

	if (sim.GetCPU() != NO_PROCESS || sim.GetMemory().size() != 0 || sim.GetDisk(0).PID != NO_PROCESS) {
		std::cout<<"Failed to have idle OS (line 40)\n";
		passed = false;
	}

	sim.NewProcess();	//1
	if (sim.GetCPU() != 1) {
		std::cout<<"Failed to load process into CPU (line 46)\n";
		passed = false;
	}

	sim.NewProcess();	//2
	sim.NewProcess();	//3
	if (sim.GetCPU() != 1 || sim.GetReadyQueue().size() != 2) {
		std::cout<<"Failed to load processes in ready queue (line 53)\n";
		passed = false;
	}

	sim.SimFork();		//4
	if (sim.GetCPU() != 1 || sim.GetReadyQueue().size() != 3) {
		std::cout<<"Failed to fork current process (line 59)\n";
		passed = false;
	}

	sim.SimExit();		//CPU: 2 | Q: 3
	if (sim.GetCPU() != 2 || sim.GetReadyQueue().size() != 1) {
		std::cout<<"Failed to terminate process and decendants (line 65)\n";
		passed = false;
	}

	sim.SimFork();		//5
	sim.TimerInterrupt();	//CPU: 3 | Q: 5, 2
	if (sim.GetCPU() != 3) {
		std::cout<<"Failed to interrupt current process (line 72)\n";
		passed = false;
	}

	sim.SimFork();		//6
	sim.TimerInterrupt();	//CPU:5 | Q: 2, 6, 3
	sim.DiskReadRequest(0, "Shrek.mov");
	FileReadRequest file_1 = sim.GetDisk(0);
	if (sim.GetCPU() != 2 || file_1.fileName != "Shrek.mov" || file_1.PID != 5) {
		std::cout<<"Failed to request file (line 81)\n";
		passed = false;
	}

	sim.DiskJobCompleted(0);
	FileReadRequest file_2 = sim.GetDisk(0);
	if (sim.GetCPU() != 2 || file_2.fileName != "" || file_2.PID != NO_PROCESS) {
		std::cout<<"Failed to remove file from disk (line 88)\n";
		passed = false;
	}

	sim.TimerInterrupt();	//CPU 6
	sim.TimerInterrupt();	//CPU 3
	sim.TimerInterrupt();	//CPU 5
	sim.SimExit();		//5 turns to zombie, parent not waiting
	sim.SimWait();		//CPU 2
	if (sim.GetCPU() != 2) {
		std::cout<<"Failed to have 2 continue to use CPU since 5 was a zombie (line 98)\n";
		passed = false;
	}

	sim.SimExit();
	sim.SimExit();
	sim.SimExit();
	sim.NewProcess();	//7
	sim.NewProcess();	//8
	sim.SimFork();		//9
	sim.DiskReadRequest(0, "Cars 2");
	sim.DiskReadRequest(0, "Moana");
	sim.DiskReadRequest(0, "Jaws");
	FileReadRequest file_3 = sim.GetDisk(0);
	std::deque<FileReadRequest> file_q = sim.GetDiskQueue(0);
	if (sim.GetCPU() != NO_PROCESS || file_3.fileName != "Cars 2" || file_3.PID != 7 || file_q.size() != 2) {
		std::cout<<"Failed to load waiting processes in file queue (line 114)\n";
		passed = false;
	}

	sim.DiskJobCompleted(0);
	sim.DiskJobCompleted(0);
	sim.SimExit();
	if (sim.GetCPU() != 8 || sim.GetDisk(0).fileName != "" || sim.GetDisk(0).PID != NO_PROCESS) {
		std::cout<<"Faield to terminate child pending file request (line 122)\n";
		passed = false;
	}

	sim.SimExit();
	sim.NewProcess();	//10
	sim.SimFork();		//11
	sim.SimFork();		//12
	sim.NewProcess();	//13
	sim.DiskReadRequest(0, "1");
	sim.DiskReadRequest(0, "2");
	sim.DiskReadRequest(0, "3");
	sim.DiskReadRequest(0, "4");
	sim.DiskJobCompleted(0);
	sim.SimExit();
	if (sim.GetCPU() != NO_PROCESS || sim.GetDisk(0).PID != 13) {
		std::cout<<"Faield to update file reading queue (line 138)\n";
		passed = false;
	}

	sim.NewProcess();	//14
	sim.SimFork();		//15
	sim.TimerInterrupt();
	sim.SimFork();		//16
	sim.TimerInterrupt();
	sim.SimExit();		//Killing 14 and all its decendants and decendant decentants
	if (sim.GetCPU() != NO_PROCESS) {
		std::cout<<"Faield to perform cascade termination of children of children (line 149)\n";
		passed = false;
	}

	std::cout << sim.GetCPU() <<std::endl;
	sim.NewProcess();	//17
	sim.AccessMemoryAddress(3);
	MemoryUsage ram = sim.GetMemory();
	if (sim.GetCPU() != 17 || ram[0].PID != 17 || ram[0].pageNumber != 3) {
		std::cout<<"Failed to load process into RAM (line 157)\n";
		passed = false;
	}

	sim.NewProcess();	//18
	sim.TimerInterrupt();
	std::cout << sim.GetReadyQueue().size() <<std::endl;
	sim.AccessMemoryAddress(6);
	ram = sim.GetMemory();
	
	if (sim.GetCPU() != 18 || ram.size() != 2) {
		std::cout<<"Faield to load new process into RAM (line 166)\n";
		passed = false;
	}

	sim.TimerInterrupt();
	sim.AccessMemoryAddress(2);
	ram = sim.GetMemory();
	if (sim.GetCPU() != 17 || ram[0].PID != 17 || ram[2].PID != 17) {
		std::cout<<"Failed to load same process with different logical address into RAM (line 173)\n";
		passed = false;
	}

	sim.AccessMemoryAddress(2);
	ram = sim.GetMemory();
	if (sim.GetCPU() != 17 || ram.size() != 3) {
		std::cout<<"Faield to update RAM if its same process and logical address (line 181)\n";
		passed = false;
	}

	sim.SimExit();
	ram = sim.GetMemory();
	if (sim.GetCPU() != 18 || ram.size() != 1) {
		std::cout<<"Failed to remove terminated processes from RAM or not consider them as using RAM (line 188)\n";
		passed = false;
	}

	if (passed) std::cout << "These custom tests are passed" << std::endl;
}