#include "pin.H"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <assert.h>
#include "sync_data.h"





/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
volatile INT16 active_threads = 1;
volatile INT16 start_profile = 0;

fstream out_file;
THREADID total_threads;
bool *record;  //per thread flag for record start
vector<long>call_site; //TODO: use map
SyncData *data; //per thread data for reporting


typedef struct {
	THREADID thread_id; // if it is equal to total_threads then it is a shared mem
	unsigned int access_count;
}mem_track_t;


//map<ADDRINT,THREADID>memAccessMap;
map<ADDRINT,mem_track_t*>memAccessMap;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool","o", "main.out", "specify the output file name" );
KNOB<string> KnobThreadCount(KNOB_MODE_WRITEONCE, "pintool","threads", "1", "specify number of threads in the program" );
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool","i", "main.in", "specify the input file name" );


/* ===================================================================== */
/* Analysis routines                                                     */
/* ===================================================================== */
VOID ThreadBegin(THREADID thread_id, CONTEXT *ctxt, INT32 flags, VOID *v)
{
  INT16 val = 1;
  __asm__ __volatile__("lock xaddw %3, %0"
                         : "=m"(active_threads), "=r"(val)
                         :  "m"(active_threads), "1"(val)
                         );
  ASSERTX(thread_id < total_threads);
  //ASSERTX(thread_id < MAX_THREADS);
  if (start_profile == 0 && active_threads > 1) start_profile = 1;
  std::cout << "Thread start " << thread_id << endl;
}


VOID ThreadEnd(THREADID thread_id, const CONTEXT *ctxt, INT32 code, VOID *v)
{
  INT16 val = -1;
  __asm__ __volatile__("lock xaddw %3, %0"
                    : "=m"(active_threads), "=r"(val)
                    :  "m"(active_threads), "1"(val)
                     );
  cout << "Thread end " << thread_id << endl;
}

VOID ProcessLock( ADDRINT ret_address, THREADID thread_id ){
  if(start_profile){
    //std::cout <<std::hex << ret_address << std::endl;
    //if ret address == one of the interesting call stack addresses turn on record flag
		vector<long>::iterator it = find( call_site.begin(), call_site.end(), ret_address );
		if(it != call_site.end() ){
			data[thread_id].critical_sections++;
			record[thread_id] = true;
		}
  }
}

VOID ProcessUnlock( THREADID thread_id ){
  if(start_profile){
    //turn off record  
		record[thread_id]  = false;
  }
}


bool doMemAccess( ADDRINT mem_addr, THREADID thread_id ){
	map<ADDRINT,mem_track_t*>::iterator it;
	bool shared_access = false;
	it = memAccessMap.find(mem_addr);
	if(it != memAccessMap.end()) {
		if(it->second->thread_id != total_threads){ // it->second == total_threads means it is shared data access
			if(it->second->thread_id != thread_id) {
				it->second->thread_id = total_threads; //at least 2 threads touched the mem address, so shared data	
				shared_access = true;
			}	
		}
		else {
			shared_access = true;
		}	
		//it->second->access_count++; //TODO: atomic
		unsigned int val = 1;
		__asm__ __volatile__("lock xaddl %3, %0"
                       : "=m"(it->second->access_count), "=r"(val)
                       :  "m"(it->second->access_count), "1"(val)
                      );
	}
	else { //new entry
		mem_track_t *memEntry = (mem_track_t*)malloc(sizeof(mem_track_t));
	 	memEntry->thread_id = thread_id;
		memEntry->access_count = 1;
		memAccessMap[mem_addr] = memEntry;
	}

	return shared_access;

}

VOID ProcessInst( THREADID thread_id , bool is_mem_access, bool is_stack_access, 
                  ADDRINT _waddr, ADDRINT _raddr, ADDRINT _raddr2 ){

	ASSERTX(thread_id < total_threads);
	if(record[thread_id]){
		data[thread_id].instcount++;	
		bool shared_data = false;	
		if(is_mem_access && !is_stack_access){
			assert(_waddr || _raddr || _raddr2);
			data[thread_id].inst_mem_access++;
			if(_waddr){ shared_data =  doMemAccess(_waddr, thread_id);} //TODO: no function call in analysis functions
			if(_raddr & !shared_data ) { shared_data = doMemAccess(_raddr, thread_id);}
			if(_raddr2 & !shared_data ) doMemAccess(_raddr2, thread_id);					
		}		
		
	}

}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
VOID Image(IMG img, VOID *v){

  RTN mutex_lock_rtn = RTN_FindByName( img, "pthread_mutex_lock");
  if(RTN_Valid(mutex_lock_rtn)){
    RTN_Open(mutex_lock_rtn);
    RTN_InsertCall(mutex_lock_rtn, IPOINT_AFTER, (AFUNPTR)ProcessLock, IARG_RETURN_IP, IARG_THREAD_ID, IARG_END);
    RTN_Close(mutex_lock_rtn);
  }

  RTN mutex_unlock_rtn = RTN_FindByName( img, "pthread_mutex_unlock");
  if(RTN_Valid(mutex_unlock_rtn)){
    RTN_Open(mutex_unlock_rtn);
    RTN_InsertCall(mutex_unlock_rtn, IPOINT_BEFORE, (AFUNPTR)ProcessUnlock, IARG_THREAD_ID, IARG_END );
    RTN_Close(mutex_unlock_rtn);
  }

	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)){
    for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
      RTN_Open(rtn);
      for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        // Avoid instrumenting the instrumentation
        if (!INS_IsOriginal(ins))
          continue;
				

				if ((INS_IsMemoryWrite(ins) || INS_IsMemoryRead(ins)) && INS_HasFallThrough(ins)) {
					bool isStackAccess = false;
					if (INS_IsStackRead(ins) || INS_IsStackWrite(ins)) {
          	isStackAccess = true;	
        	}
					if (INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) &&
              INS_HasMemoryRead2(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessInst, IARG_THREAD_ID,
                          IARG_BOOL, true,
                          IARG_BOOL, isStackAccess, 
                          IARG_MEMORYWRITE_EA,
                          IARG_MEMORYREAD_EA,
                          IARG_MEMORYREAD2_EA,
                          IARG_END); 
          }					
					else if (INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) &&
        	          !INS_HasMemoryRead2(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessInst, IARG_THREAD_ID,
                          IARG_BOOL, true,
                          IARG_BOOL, isStackAccess, 
                          IARG_MEMORYWRITE_EA,
                          IARG_MEMORYREAD_EA,
                          IARG_ADDRINT, 0,
                          IARG_END);		
						
					}
					else if (INS_IsMemoryWrite(ins) && !INS_IsMemoryRead(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessInst, IARG_THREAD_ID,
                          IARG_BOOL, true,
                          IARG_BOOL, isStackAccess, 
                          IARG_MEMORYWRITE_EA,
                          IARG_ADDRINT, 0,
                          IARG_ADDRINT, 0,
                          IARG_END);		
					}
					else if (!INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) &&
                  INS_HasMemoryRead2(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessInst, IARG_THREAD_ID,
                          IARG_BOOL, true,
                          IARG_BOOL, isStackAccess, 
               						IARG_ADDRINT, 0,
                          IARG_MEMORYREAD_EA,
                          IARG_MEMORYREAD2_EA,
                          IARG_END);
					}
					else if (!INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) &&
                  !INS_HasMemoryRead2(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessInst, IARG_THREAD_ID,
                          IARG_BOOL, true,
                          IARG_BOOL, isStackAccess, 
               						IARG_ADDRINT, 0,
                          IARG_MEMORYREAD_EA,
                          IARG_ADDRINT,0,
                          IARG_END);	
					}
					else { //not a memory opeartion
						INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessInst, IARG_THREAD_ID,
														IARG_BOOL,  false,
														IARG_BOOL,  false, 
														IARG_ADDRINT, 0,
														IARG_ADDRINT, 0,
														IARG_ADDRINT, 0, IARG_END);
					}
				}
			}
			RTN_Close(rtn);
		}
	}

  
}


VOID Fini(INT32 code, VOID *v){
	unsigned int instcount = 0;
	unsigned int inst_mem_access = 0;
	unsigned int cs_count = 0;

	cout << "In fini\n";	
	for(THREADID i=0; i<total_threads; i++){
		instcount += data[i].instcount;
		inst_mem_access += data[i].inst_mem_access;	
		cs_count += data[i].critical_sections++;
	}
	out_file << "Total instruction: " << instcount << endl;
	out_file << "Total memory access inst: " << inst_mem_access << endl;
	out_file << "No of critical sections: " << 	cs_count << endl;


	map<ADDRINT,mem_track_t*>::iterator it;
  unsigned int mem_access_count = 0;
	unsigned int shared_access_count = 0;
	for(it=memAccessMap.begin(); it!=memAccessMap.end(); ++it){
		mem_access_count += it->second->access_count;
		if(it->second->thread_id == total_threads)
			shared_access_count += it->second->access_count;
	}

	out_file << "Total mem acces: " << mem_access_count << endl;	
	
	out_file << "Total shared acess: " << shared_access_count << endl;

	out_file.close();
}

INT32 Usage(){
  return 0;
}

void ReadInput(){
	fstream in_file;
	
	in_file.open(KnobInputFile.Value().c_str(), fstream::in);
	string str;
	while(getline(in_file, str)){
		stringstream ss(str);
		long ret_address;
		ss >> hex >> ret_address;
		cout <<"input : 0x" << hex << ret_address << endl;
	 	call_site.push_back(ret_address);
	}
	
}

int main( int argc, char **argv ){
  
  PIN_InitSymbols();

  if( PIN_Init(argc,argv) ) {
    return Usage();
  }

	out_file.open(KnobOutputFile.Value().c_str(), fstream::out);

		
	
	
	stringstream ss(KnobThreadCount.Value());
	ss >> total_threads;

	out_file << "Thread count: " << total_threads << endl;
	record = new bool[total_threads];
	for(THREADID i=0; i<total_threads; i++) record[i] = false;

	data = new SyncData[total_threads];
	//read input file
	ReadInput();

	
	
  PIN_AddThreadStartFunction(ThreadBegin,0);
  PIN_AddThreadFiniFunction(ThreadEnd,0);

  IMG_AddInstrumentFunction(Image, 0);

  PIN_AddFiniFunction(Fini, 0);
  
  PIN_StartProgram();
}

