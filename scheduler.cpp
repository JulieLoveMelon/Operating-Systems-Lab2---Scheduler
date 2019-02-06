#include <iomanip>
#include <iostream>
#include <fstream>
#include <string.h>
#include <list>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

typedef enum {
	TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT
} process_state_t;

int offset;
int rnum;
int* randvals;

static const char *optString = "s:?";

class Process{
public:
	/* static parameters */
	int AT;
	int TC;
	int CB;
	int IO;
	int prio;
	int pid;
	int quantum;

	/* dynamic parameters */
	int STATE_TS; // time starting the previous state
	int timeInPrevState; // time in previous state
	int rem; // remaining cpu time
	int current_prio;
	int remCB;

	/* final output info */
	int FT; // finishing time
	int TT; // turnaround time
	int CW; // CPU waiting time
	int IT; // I/O time
	int IT1; // only this process is doing IO

	Process();
	Process(int arrivaltime, int totalcpu, int cpuburst, int ioburst, int staticprio, int processid);
};

Process::Process(){}
Process::Process(int arrivaltime, int totalcpu, int cpuburst, int ioburst, int staticprio, int processid){
	AT = arrivaltime;
	TC = totalcpu;
	CB = cpuburst;
	IO = ioburst;
	prio = staticprio;
	pid = processid;
}

class EVENT{
public:
	process_state_t transition;
	Process* evtProcess;
	int evtTimeStamp;

	EVENT(process_state_t pst, Process* proc, int evtTS);
	EVENT();
};

EVENT::EVENT(){}
EVENT::EVENT(process_state_t pst, Process* proc, int evtTS){
	transition = pst;
	evtProcess = proc;
	evtTimeStamp = evtTS;
}

list<EVENT*> eventqueue;
list<Process*> readyqueue;
list<Process*> finish;
list<Process*> prio_readyqueue[2][4];
list<Process*> *active = &prio_readyqueue[0][0];
list<Process*> *expire = &prio_readyqueue[1][0];
Process* CURRENT_RUNNING_PROCESS = nullptr;

class SCHEDULER{
public:
	virtual Process* get_next_process();
	virtual void add_to_queue(Process* p);
	//virtual void put_event(EVENT* evt);
};

class FCFS: public SCHEDULER{
public:
	Process* get_next_process();
	void add_to_queue(Process* p);
	//void put_event(EVENT* evt);
};

class LCFS: public SCHEDULER{
public:
	Process* get_next_process();
	void add_to_queue(Process* p);
	//void put_event(EVENT* evt);
};

class SRTF: public SCHEDULER{
public:
	Process* get_next_process();
	void add_to_queue(Process* p);
};

class RR:public SCHEDULER{
public:
	Process* get_next_process();
	void add_to_queue(Process* p);
};

class PRIO: public SCHEDULER{
public:
	Process* get_next_process();
	void add_to_queue(Process* p);
};

class PREPRIO: public SCHEDULER{
public:
	Process* get_next_process();
	void add_to_queue(Process* p);
};

int myrandom(int burst){
	if(offset == rnum - 1)
		offset = 0;
	int rand = 1 + (randvals[offset]%burst);
	offset ++;
	return rand;
}

Process* SCHEDULER::get_next_process(){ 
	Process* p = new Process();
	return p;
}
void SCHEDULER::add_to_queue(Process* p){}
//void SCHEDULER::put_event(EVENT* evt){}

Process* FCFS::get_next_process(){
	if(readyqueue.empty())
		return nullptr;
	return readyqueue.front();
}

void FCFS::add_to_queue(Process* p){
	// add event to readyqueue
	//cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": ";
	/*
	if(p->STATE_TS == p->AT){
		// this event was just created
		cout << "CREATED -> READY" << endl;
	}
	else{
		// must come from BLOCKED or PREEMPT
		cout << "BLOCK -> READY" << endl;
	}
	*/
	p->STATE_TS += p->timeInPrevState;
	readyqueue.push_back(p);
}

EVENT* get_event(){
	// get event from eventqueue
	if(eventqueue.empty())
		return nullptr;
	else{
		EVENT* evt = eventqueue.front();
		eventqueue.pop_front();
		return evt;
	}
}

void put_event(EVENT* evt){
	// put event to eventqueue according to timestamp
	if(eventqueue.empty()){
		eventqueue.push_back(evt);
		return;
	}
	list<EVENT*>::iterator iter = eventqueue.begin();
	while(iter != eventqueue.end()){
		if((*iter)->evtTimeStamp <= evt->evtTimeStamp)
			iter++;
		else{
			eventqueue.insert(iter, evt);
			break;
		}
	}
	if(iter == eventqueue.end())
		eventqueue.push_back(evt);
}

Process* LCFS::get_next_process(){
	if(readyqueue.empty())
		return nullptr;
	return readyqueue.front();
}

void LCFS::add_to_queue(Process* p){
	// add event to readyqueue
	cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": ";
	/*
	if(p->STATE_TS == p->AT){
		// this event was just created
		cout << "CREATED -> READY" << endl;
	}
	else{
		// must come from BLOCKED or PREEMPT
		cout << "BLOCK -> READY" << endl;
	}
	*/
	p->STATE_TS += p->timeInPrevState;
	readyqueue.push_front(p);
}

Process* SRTF::get_next_process(){
	if(readyqueue.empty())
		return nullptr;
	else
		return readyqueue.front();
}

void SRTF::add_to_queue(Process* p){
	// put process to readyqueue according to remaining time
	if(readyqueue.empty()){
		readyqueue.push_back(p);
		return;
	}
	list<Process*>::iterator iter = readyqueue.begin();
	while(iter != readyqueue.end()){
		if((*iter)->rem <= p->rem)
			iter++;
		else{
			readyqueue.insert(iter, p);
			break;
		}
	}
	if(iter == readyqueue.end())
		readyqueue.push_back(p);
}

Process* RR::get_next_process(){
	if(readyqueue.empty())
		return nullptr;
	else
		return readyqueue.front();
}

void RR::add_to_queue(Process* p){
	/*
	// p was just created 
	if(p->STATE_TS + p->timeInPrevState == p->AT)
		cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "CREATED -> READY" << endl;
	// p comes from block 
	else if(p->remCB == 0){
		cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "BLOCK -> READY" << endl;
		p->timeInPrevState = 0;
	}
	// p comes from preempt
	else{
		cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "RUNNG -> READY" 
			<< " cb=" << p->remCB << " rem=" << p->rem << " prio=" << p->current_prio << endl;
	}
	*/
	if(p->remCB == 0)
		p->timeInPrevState = 0;
	p->STATE_TS += p->timeInPrevState;
	readyqueue.push_back(p);
}

Process* PRIO::get_next_process(){
	int i = 3;	
	for(i = 3; i >= 0; i--){
		if(!active[i].empty())
			return active[i].front();
	}
	/* active queue is empty, switch active and expire */
	list<Process*> *temp = active;
	active = expire;
	expire = temp;
	temp = nullptr;
	for(i = 3; i >= 0; i--){
		if(!active[i].empty())
			return active[i].front();
	}
	return nullptr;
}

void PRIO::add_to_queue(Process* p){
	/* add process to readyqueue according to its priority */
	/* p was just created, add to active prioqueue */
	if(p->STATE_TS + p->timeInPrevState == p->AT){
		// cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "CREATED -> READY" << endl;
		p->STATE_TS += p->timeInPrevState;
		active[p->current_prio].push_back(p);
	}
	/* p comes from block */
	else if(p->remCB == 0){
		// cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "BLOCK -> READY" << endl;
		p->current_prio = p->prio - 1;
		p->timeInPrevState = 0;
		active[p->current_prio].push_back(p);
	}
	/* p comes from preempt */
	else{
		//cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "RUNNG -> READY" 
		//	<< " cb=" << p->remCB << " rem=" << p->rem << " prio=" << p->current_prio << endl;
		p->timeInPrevState = p->quantum;
		if(p->current_prio == 0){
			p->current_prio = p->prio - 1;
			expire[p->current_prio].push_back(p);
		}
		else{
			p->current_prio--;
			active[p->current_prio].push_back(p);
		}
	}
}

Process* PREPRIO::get_next_process(){
	int i = 3;	
	for(i = 3; i >= 0; i--){
		if(!active[i].empty())
			return active[i].front();
	}
	/* active queue is empty, switch active and expire */
	list<Process*> *temp = active;
	active = expire;
	expire = temp;
	temp = nullptr;
	for(i = 3; i >= 0; i--){
		if(!active[i].empty())
			return active[i].front();
	}
	return nullptr;
}

void PREPRIO::add_to_queue(Process* p){
	/* add process to readyqueue according to its priority */
	/* p was just created, add to active prioqueue */
	if(p->STATE_TS + p->timeInPrevState == p->AT){
		// cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "CREATED -> READY" << endl;
		p->STATE_TS += p->timeInPrevState;
		/* test if this unblocking process would preempt the currently running process*/ 
		if(CURRENT_RUNNING_PROCESS != nullptr){
			if(CURRENT_RUNNING_PROCESS->current_prio < p->current_prio){
				/* if priority of this unblocking process is higher than that of the currently running process */
				if(CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS > p->STATE_TS + p->timeInPrevState){
					/* if the currently running process doesn't have a pending event for the same time stamp */
					/* remove the future event for the currently running process */
					list<EVENT*>::iterator iter = eventqueue.begin();
					list<EVENT*>::iterator next = eventqueue.end();
					process_state_t st;
					for(; iter != eventqueue.end(); iter = next){
						next = ++iter;
						--iter;
						if((*iter)->evtProcess->pid == CURRENT_RUNNING_PROCESS->pid){
							st = (*iter)->transition;
							eventqueue.erase(iter);
							break;
						}
					}
					/* create an event due to preemption */
					EVENT* evt = new EVENT();
					CURRENT_RUNNING_PROCESS->remCB += CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS- (p->STATE_TS + p->timeInPrevState);
					if(st == TRANS_TO_BLOCK)
						CURRENT_RUNNING_PROCESS->rem -= - CURRENT_RUNNING_PROCESS->STATE_TS + (p->STATE_TS + p->timeInPrevState);
					else
						CURRENT_RUNNING_PROCESS->rem +=  CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS - (p->STATE_TS + p->timeInPrevState);
					CURRENT_RUNNING_PROCESS->timeInPrevState -= CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS- (p->STATE_TS + p->timeInPrevState);
					evt->evtProcess = CURRENT_RUNNING_PROCESS;
					evt->evtTimeStamp = p->STATE_TS + p->timeInPrevState;
					evt->transition = TRANS_TO_RUN;
					put_event(evt);
				}
			}
		}	
		active[p->current_prio].push_back(p);
	}
	/* p comes from block */
	else if(p->remCB == 0){
		//cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "BLOCK -> READY" << endl;
		p->current_prio = p->prio - 1;
		/* test if this unblocking process would preempt the currently running process*/ 
		if(CURRENT_RUNNING_PROCESS != nullptr){
			if(CURRENT_RUNNING_PROCESS->current_prio < p->current_prio){
				/* if priority of this unblocking process is higher than that of the currently running process */
				if(CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS > p->STATE_TS + p->timeInPrevState){
					/* if the currently running process doesn't have a pending event for the same time stamp */
					/* remove the future event for the currently running process */
					list<EVENT*>::iterator iter = eventqueue.begin();
					list<EVENT*>::iterator next = eventqueue.end();
					process_state_t st;
					for(; iter != eventqueue.end(); iter = next){
						next = ++iter;
						--iter;
						if((*iter)->evtProcess->pid == CURRENT_RUNNING_PROCESS->pid){
							st = (*iter)->transition;
							eventqueue.erase(iter);
							break;
						}
					}
					/* create an event due to preemption */
					EVENT* evt = new EVENT();
					CURRENT_RUNNING_PROCESS->remCB += CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS- (p->STATE_TS + p->timeInPrevState);
					if(st == TRANS_TO_BLOCK)
						CURRENT_RUNNING_PROCESS->rem -= - CURRENT_RUNNING_PROCESS->STATE_TS + (p->STATE_TS + p->timeInPrevState);
					else
						CURRENT_RUNNING_PROCESS->rem +=  CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS - (p->STATE_TS + p->timeInPrevState);
					CURRENT_RUNNING_PROCESS->timeInPrevState -= CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS- (p->STATE_TS + p->timeInPrevState);
					evt->evtProcess = CURRENT_RUNNING_PROCESS;
					evt->evtTimeStamp = p->STATE_TS + p->timeInPrevState;
					evt->transition = TRANS_TO_RUN;
					put_event(evt);
				}
			}
		}		
		p->timeInPrevState = 0;
		active[p->current_prio].push_back(p);
	}
	/* p comes from preempt */
	else{
		//cout << p->timeInPrevState + p->STATE_TS << " " << p->pid << " " << p->timeInPrevState << ": " << "RUNNG -> READY" 
		//	<< " cb=" << p->remCB << " rem=" << p->rem << " prio=" << p->current_prio << endl;
		p->timeInPrevState = p->quantum;
		if(p->current_prio == 0){
			p->current_prio = p->prio - 1;
			expire[p->current_prio].push_back(p);
		}
		else{
			p->current_prio--;
			active[p->current_prio].push_back(p);
		}
	}
}

int get_next_event_time(){
	if(eventqueue.empty())
		return -1;
	return eventqueue.front()->evtTimeStamp;
}

void Simulation(SCHEDULER* THE_SCHEDULER){
	EVENT* evt = new EVENT();
	bool CALL_SCHEDULER = false;
	while(evt = get_event()){
		Process *proc = evt->evtProcess;	
		int flag = 0;
		int randCB = 0;
		int CURRENT_TIME = evt->evtTimeStamp;
		if((CURRENT_RUNNING_PROCESS != nullptr) && (CURRENT_RUNNING_PROCESS->timeInPrevState + CURRENT_RUNNING_PROCESS->STATE_TS) <= CURRENT_TIME){
			CURRENT_RUNNING_PROCESS = nullptr;
		}
		if(CURRENT_TIME < proc->STATE_TS)
			proc->timeInPrevState = 0;
		else
			proc->timeInPrevState = CURRENT_TIME - proc->STATE_TS;
		int randIO = 0;
		EVENT* newevt = new EVENT();
		switch(evt->transition){
		case TRANS_TO_READY:
			// must come from BLOCKED or from PREEMPTION
			// must add to run queue
			THE_SCHEDULER->add_to_queue(proc);
			CALL_SCHEDULER = true; // conditional on whether something is run
			break;
		case TRANS_TO_RUN:
			// create an event for either preemption or blocking
			newevt->evtProcess = evt->evtProcess;
			newevt->evtTimeStamp = CURRENT_TIME;
			newevt->transition = TRANS_TO_READY;
			put_event(newevt);
			break;
		case TRANS_TO_BLOCK:
			// create an event for when process becomes READY again
			/* process finishes */
			if(proc->timeInPrevState == proc->rem){
				//cout << CURRENT_TIME << " " << proc->pid << " " << proc->timeInPrevState << ": Done" << endl;
				proc->FT = CURRENT_TIME;
				proc->TT = proc->FT - proc->AT;
				proc->CW = proc->TT - proc->TC - proc->IT;
				
				if(finish.empty()){
					finish.push_back(proc);
					CALL_SCHEDULER = true;
					break;
				}
				else{
					list<Process*>::iterator iter = finish.begin(); 
					while(iter != finish.end()){
						if((*iter)->pid < proc->pid)
							iter++;
						else{
							finish.insert(iter, proc);
							CALL_SCHEDULER = true;
							break;
						}
					}
					if(iter == finish.end()){
						finish.push_back(proc);
						CALL_SCHEDULER = true;
						break;
					}
					else{
						CALL_SCHEDULER = true;
						break;
					}
				}
				
			}
			/* io burst */
			else{
				randIO = myrandom(proc->IO);
				proc->IT += randIO;
				proc->rem -= proc->timeInPrevState;
				//cout << CURRENT_TIME << " " << proc->pid << " " << proc->timeInPrevState << ": RUNNG -> BLOCK "
				//	<< "ib=" << randIO << " rem=" << proc->rem << endl;
				proc->STATE_TS = CURRENT_TIME;
				newevt = new EVENT(TRANS_TO_READY, proc, randIO + CURRENT_TIME);
				put_event(newevt);
				CALL_SCHEDULER = true;

				/* computer the time that at least one process is doing io */
				list<EVENT*>::iterator iter;
				if(eventqueue.size() != 1){
					iter = eventqueue.end();
					iter --;
					while((*iter)->evtProcess != proc){
						if((*iter)->transition == TRANS_TO_READY && (*iter)->evtTimeStamp != (*iter)->evtProcess->AT){
							proc->IT1 += 0;
							flag = 1;
							CALL_SCHEDULER = true;
							break;
						}
						else
							iter--;
					}
					if(flag == 0){
						if(iter == eventqueue.begin()){
							proc->IT1 += randIO;
							flag = 1;
						}
						else{
							while(iter != eventqueue.begin()){
								iter--;
								if((*iter)->transition == TRANS_TO_READY && (*iter)->evtTimeStamp != (*iter)->evtProcess->AT){
									if((*iter)->evtTimeStamp > CURRENT_TIME){
										proc->IT1 += randIO + CURRENT_TIME - (*iter)->evtTimeStamp;
										flag = 1;
										CALL_SCHEDULER = true;
										break;
									}
								}
							}
							if(flag == 0 && iter == eventqueue.begin()){
								if((*iter)->transition == TRANS_TO_READY && (*iter)->evtTimeStamp != (*iter)->evtProcess->AT){
									if((*iter)->evtTimeStamp > CURRENT_TIME){
										proc->IT1 += randIO + CURRENT_TIME - (*iter)->evtTimeStamp;
										CALL_SCHEDULER = true;
										break;
									}
									else{
										proc->IT1 += randIO;
										CALL_SCHEDULER = true;
										break;
									}
								}
								else{
									proc->IT1 += randIO;
									CALL_SCHEDULER = true;
									break;
								}
							}
						}
					}
				}
				else
					evt->evtProcess->IT1 += randIO;
				CALL_SCHEDULER = true;
				break;
			}
		case TRANS_TO_PREEMPT:
			// add to runqueue (no event is generated)
			THE_SCHEDULER->add_to_queue(proc);
			CALL_SCHEDULER = true;
			break;
		}
		// remove current event object from Memory
		delete evt;
		evt = nullptr;

		if(CALL_SCHEDULER){
			if(get_next_event_time() == CURRENT_TIME){
				continue; // process next event from Event queue
			}
			CALL_SCHEDULER = false; // reset global flag
			if(CURRENT_RUNNING_PROCESS == nullptr){
				CURRENT_RUNNING_PROCESS = THE_SCHEDULER->get_next_process();
				if(CURRENT_RUNNING_PROCESS == nullptr)
					continue;
				if(CURRENT_RUNNING_PROCESS->remCB == 0){
					// comes from BLOCK
					randCB = myrandom(CURRENT_RUNNING_PROCESS->CB);
					if(randCB > CURRENT_RUNNING_PROCESS->rem)
						randCB = CURRENT_RUNNING_PROCESS->rem;
					CURRENT_RUNNING_PROCESS->remCB = randCB;
				}
				if(CURRENT_RUNNING_PROCESS->quantum < CURRENT_RUNNING_PROCESS->remCB){
					/* RUN & TRANS_TO_PREEMPT */
					//cout << CURRENT_TIME << " " << CURRENT_RUNNING_PROCESS->pid << " " << CURRENT_RUNNING_PROCESS->timeInPrevState 
					//	<< ": READY -> RUNNG" << " cb=" << CURRENT_RUNNING_PROCESS->remCB << " rem=" << CURRENT_RUNNING_PROCESS->rem
					//	<< " prio=" << CURRENT_RUNNING_PROCESS->current_prio << endl;
					CURRENT_RUNNING_PROCESS->remCB -= CURRENT_RUNNING_PROCESS->quantum;
					CURRENT_RUNNING_PROCESS->rem -= CURRENT_RUNNING_PROCESS->quantum;
					CURRENT_RUNNING_PROCESS->STATE_TS = CURRENT_TIME;
					CURRENT_RUNNING_PROCESS->timeInPrevState = CURRENT_RUNNING_PROCESS->quantum;
					EVENT* evt = new EVENT(TRANS_TO_PREEMPT, CURRENT_RUNNING_PROCESS, CURRENT_TIME + CURRENT_RUNNING_PROCESS->quantum);
					put_event(evt);
					if(readyqueue.empty())
						active[CURRENT_RUNNING_PROCESS->current_prio].pop_front();
					else
						readyqueue.pop_front();
				}
				else{
					/* RUN & TRANS_TO_BLOCK */
					//create event to make this process runnable for same time					
					//cout << CURRENT_TIME << " " << CURRENT_RUNNING_PROCESS->pid << " " << CURRENT_RUNNING_PROCESS->timeInPrevState 
					//	<< ": READY -> RUNNG " << " cb=" << CURRENT_RUNNING_PROCESS->remCB << " rem=" << CURRENT_RUNNING_PROCESS->rem << " prio=" 
					//	<< CURRENT_RUNNING_PROCESS->current_prio << endl;
					CURRENT_RUNNING_PROCESS->STATE_TS = CURRENT_TIME;
					CURRENT_RUNNING_PROCESS->timeInPrevState = CURRENT_RUNNING_PROCESS->remCB;
					EVENT* evt = new EVENT(TRANS_TO_BLOCK, CURRENT_RUNNING_PROCESS, CURRENT_TIME + CURRENT_RUNNING_PROCESS->remCB);
					CURRENT_RUNNING_PROCESS->remCB = 0;
					put_event(evt);
					if(readyqueue.empty())
						active[CURRENT_RUNNING_PROCESS->current_prio].pop_front();
					else
						readyqueue.pop_front();
				}
			}
		}
	}
}

int main(int argc, char* argv[]){
	char* scheduler = NULL;
	char* inputname = NULL;
	char* rfilename = NULL;
	int QUANTUM = 1000000;
	int opt = 0;
	opt = getopt(argc, argv, optString);
	while(opt != -1){
		switch(opt){
			case 's':
			scheduler = optarg;
			break;

			case '?':
			cout << "Plese input an instruction for the name of scheduler.";
			return 0;
			break;

			default:
			abort();
		}
		opt = getopt(argc, argv, optString);
	}
	int index = optind;
	inputname = argv[index];
	index++;
	rfilename = argv[index];
	/* get the scheduler name and the quantum */
	SCHEDULER* sched;
	string s = scheduler;
	if(s == "F"){
		sched = new FCFS();
		cout << "FCFS" << endl;
	}
	else if(s == "L"){
		sched = new LCFS();
		cout << "LCFS" << endl;
	}
	else if(s == "S"){
		sched = new SRTF();
		cout << "SRTF" << endl;
	}
	else{
		QUANTUM = stoi(s.substr(1, s.length()-1));
		if(s.substr(0, 1) == "R"){
			sched = new RR();
			cout << "RR " << QUANTUM << endl;
		}
		else if(s.substr(0,1) == "P"){
			sched = new PRIO();
			cout << "PRIO " << QUANTUM << endl;
		}
		else if(s.substr(0,1) == "E"){
			sched = new PREPRIO();
			cout << "PREPRIO " << QUANTUM << endl;
		}

	}
	/* get the random number file and the input file */
	ifstream rfile(rfilename);
	char buf[20];
	rfile.getline(buf, 20);
	rnum = atoi(buf);
	randvals = new int[rnum];
	int i = 0;
	for(i = 0; i < rnum; i++){
		rfile.getline(buf, 20);
		randvals[i] = atoi(buf);
	}
	rfile.close();
	rfile.clear();
	ifstream input(inputname);
	int pid = 0;
	/* read the file */
	char buffer[50];
	while(input.getline(buffer,50)){
		/* create process object */
		int static_prio = myrandom(4);
		int AT = atoi(strtok(buffer, " "));
		int TC = atoi(strtok(NULL, " "));
		int CB = atoi(strtok(NULL, " "));
		int IO = atoi(strtok(NULL, " "));
		Process* proc = new Process(AT, TC, CB, IO, static_prio, pid);
		proc->current_prio = static_prio - 1;
		proc->rem = TC;
		proc->STATE_TS = AT;
		proc->timeInPrevState = 0;
		proc->IT = 0;
		proc->IT1 = 0;
		proc->remCB = 0;
		proc->quantum = QUANTUM;
		pid ++;
		/* put initial event to eventqueue */
		EVENT* evt = new EVENT(TRANS_TO_READY, proc, AT);		
		put_event(evt);
	}
	Simulation(sched);
	
	int finishtime = 0;
	int totalcpu = 0;
	int totalio = 0;
	int totaltt = 0; //total turnaround time
	int totalcw = 0;
	for(list<Process*>::iterator iter = finish.begin(); iter != finish.end(); iter ++){
		Process* proc = *iter;
		cout << setw(4) << setfill('0') << proc->pid << ": " ;
				cout << setw(4) << setfill (' ') << proc->AT << " " << setw(4) << proc->TC << " " << setw(4) << proc->CB << " "
					<< setw(4) << proc->IO << " " << setw(1) << proc->prio << " | " << setw(5) << proc->FT << " " << setw(5) 
					<< proc->TT	<< " " << setw(5) << proc->IT << " " << setw(5) << proc->CW << endl;
		if(finishtime < proc->FT)
			finishtime = proc->FT;
		totalcpu += proc->TC;
		totalio += proc->IT1;
		totaltt += proc->TT;
		totalcw += proc->CW;
	}
	cout << "SUM: " << finishtime << " " << fixed << setprecision(2) << (double)totalcpu / finishtime * 100 << " " 
		<< (double)totalio / finishtime * 100 << " " << (double)totaltt / pid << " " << (double)totalcw / pid 
		<< " " << fixed << setprecision(3) << (double)pid/finishtime*100 << endl;	
	return 0;
}
