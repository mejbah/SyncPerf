#ifndef _REPORT_H_
#define _REPORT_H_

#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include "xdefines.h"
#include "recordentries.hh"
#include "mutex_manager.h"
#include<map>
#include<vector>
//#include<string.h>

#define MAXBUFSIZE 4096
#define COMBINED_REPORT 1

typedef struct {
	char line_info[MAX_NUM_STACKS][MAX_CALL_STACK_DEPTH * 30];
	double conflict_rate;
	double frequency;
	int count; //number of line info
}sync_perf_t;


class Report {

private:
	char _curFilename[MAXBUFSIZE];
	Report(){}
 	

public:
	static Report& getInstance() {
		static Report instance;
    return instance;		
	}	

	enum { THRESHOLD_CONFLICT = 10 };
	enum { THRESHOLD_FREQUENCY = 10000 };

	std::string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
  }

	void setFileName(){
		int count = readlink("/proc/self/exe", _curFilename, MAXBUFSIZE);
    if (count <= 0 || count >= MAXBUFSIZE)
    {
      fprintf(stderr, "Failed to get current executable file name\n" );
      exit(1);
    }
    _curFilename[count] = '\0';
	
	}

  std::string get_call_stack_string( long *call_stack ){

    //char _curFilename[MAXBUFSIZE];
    //int count = readlink("/proc/self/exe", _curFilename, MAXBUFSIZE);
    //if (count <= 0 || count >= MAXBUFSIZE)
    //{
    //  fprintf(stderr, "Failed to get current executable file name\n" );
    //  exit(1);
    //}
    //_curFilename[count] = '\0';


		
    char buf[MAXBUFSIZE];
    std::string stack_str="";

    int j=0;
    while(call_stack[j] != 0 ) {
      //printf("%#lx\n", m->stacks[i][j]);  
      sprintf(buf, "addr2line -e %s  -a 0x%lx  | tail -1", _curFilename, call_stack[j] );
      std::string source_line =  exec(buf);

      if(source_line[0] != '?') { // not found
        //get the file name only
        std::size_t found = source_line.find_last_of("/\\");
        source_line = source_line.substr(found+1);
        stack_str += source_line.erase(source_line.size()-1); // remove the newline at the end
        stack_str += " ";
      }
      j++;
    }
    return stack_str;
  }


	void print( RecordEntries<mutex_t>&sync_vars ){

#ifdef REPORT_LINE_INFO
		//set the file exe for lineinfo
		setFileName();
#endif

		std::cout<< "\n\nEND OF PROGRAM";
		std::cout<< "\nSYNCPERF REPORTING in file: syncperf.report " << std::endl;
#ifdef COMBINED_REPORT
		std::vector<sync_perf_t>high_conflict_low_freq;
		std::vector<sync_perf_t>high_conflict_high_freq;
		std::vector<sync_perf_t>low_conflict_high_freq;
#else

		std::fstream qhh_fs;
		std::fstream qhl_fs;
		std::fstream qlh_fs;
		qhh_fs.open("synperf_1.report", std::fstream::out);
		qhl_fs.open("synperf_2.report", std::fstream::out);
		qlh_fs.open("synperf_3.report", std::fstream::out);

		//high conflict, high frequency
		qhh_fs << "\n\n==============================" << std::endl;
		qhh_fs << "HIGH CONFLICT , HIGH FREQUENCY" << std::endl;
		qhh_fs << "==============================" << std::endl;

		//high conflict, low frequency
		qhl_fs << "\n\n==============================" << std::endl;
		qhl_fs << "HIGH CONFLICT , LOW FREQUENCY" << std::endl;
		qhl_fs << "==============================" << std::endl;	

	//low conflict, high frequency
		qlh_fs << "\n\n==============================" << std::endl;
		qlh_fs << "LOW CONFLICT , HIGH FREQUENCY" << std::endl;
		qlh_fs << "==============================" << std::endl;

		unsigned long qhh_count = 0;
		unsigned long qlh_count = 0;
		unsigned long qhl_count  = 0;
#endif
	  int total_threads = xthread::getInstance().getMaxThreadIndex();

		WAIT_TIME_TYPE *thread_waits = malloc(sizeof(WAIT_TIME_TYPE)*total_threads);
		for(int idx=0; idx<total_threads; idx++) thread_waits[idx] = 0;	

		int total_sync_vars = sync_vars.getEntriesNumb();

		

		int id = 0; //for debugging puporse, shoud match with total locks	

		for(int i=0; i<total_sync_vars; i++) {

			mutex_t *m = sync_vars.getEntry(i);
			assert(m->entry_index == i);
			
			unsigned int total_access_count = 0;
			unsigned int total_fail_count = 0;
			unsigned int total_cond_wait = 0;
			unsigned int total_trylock_fails = 0;
			WAIT_TIME_TYPE total_wait_time = 0;
			WAIT_TIME_TYPE total_lock_wait = 0;			

			// sum all thread local data
			for(int idx=0; idx<total_threads; idx++ ){
				//count
			  thread_mutex_t *per_thd_data =get_thread_mutex_data( m->entry_index, idx);
				total_access_count += per_thd_data->access_count;
				total_fail_count += per_thd_data->fail_count;
				total_cond_wait += per_thd_data->cond_waits;
				total_trylock_fails += per_thd_data->trylock_fail_count;
				//times
				total_wait_time += per_thd_data->cond_futex_wait;
				total_lock_wait += per_thd_data->futex_wait;
			
				thread_waits[idx] += (per_thd_data->cond_futex_wait + per_thd_data->futex_wait);
			}

			sync_perf_t sync_perf_entry;
			sync_perf_entry.count = 0;
			if( total_access_count > 0 ) { //TODO: access_count = 0 is poosible as fix setSyncEntry ignores new mutex ,index already increased in recordentries

				id++; //for debug only

				sync_perf_entry.conflict_rate = (100*total_fail_count)/double(total_access_count);
				sync_perf_entry.frequency  = double(total_access_count); //TODO: fix freqeuncey using max thd->actualRuntim
				
				if( sync_perf_entry.conflict_rate > THRESHOLD_CONFLICT ){

					//print call stacks
					for(int con=0; con<m->stack_count; con++){
#ifdef	 REPORT_LINE_INFO
						std::string call_contexts = get_call_stack_string(m->stacks[con]);		
#else
						int depth = 0;
					  std::string call_contexts = "";
						while(m->stacks[con][depth]){	
								call_contexts += "0x";
								//call_contexts += std::to_string(m->stacks[con][depth]);
								std::stringstream ss;
								ss << m->stacks[con][depth];
								call_contexts += ss.str();
								call_contexts += ",";
								depth++; 
						}									
#endif	 
						assert(call_contexts.size() <= MAX_CALL_STACK_DEPTH * 30);
						strcpy(sync_perf_entry.line_info[con], call_contexts.c_str());
						sync_perf_entry.count++;

					}

					if( sync_perf_entry.frequency > THRESHOLD_FREQUENCY ){
#ifdef COMBINED_REPORT
						high_conflict_high_freq.push_back(sync_perf_entry);
#else
						qhh_count++;
						report_quadrant(qhh_fs, sync_perf_entry, qhh_count);
#endif
					}
					else{
#ifdef COMBINED_REPORT
						high_conflict_low_freq.push_back(sync_perf_entry);
#else
						qhl_count++;
						report_quadrant(qhl_fs, sync_perf_entry, qhl_count);
#endif
					}
				}
				else{
					if( sync_perf_entry.frequency > THRESHOLD_FREQUENCY ){
						//print call stacks
						for(int con=0; con<m->stack_count; con++){
#ifdef	 REPORT_LINE_INFO
							std::string call_contexts = get_call_stack_string(m->stacks[con]);		
#else
							int depth = 0;
						  std::string call_contexts = "";
							while(m->stacks[con][depth]){	
									call_contexts += "0x";
									//call_contexts += std::to_string(m->stacks[con][depth]);
									std::stringstream ss;
									ss << m->stacks[con][depth];
									call_contexts += ss.str();

									call_contexts += ",";
									depth++; 
							}									
#endif	 
							assert(call_contexts.size() <= MAX_CALL_STACK_DEPTH * 30);
							strcpy(sync_perf_entry.line_info[con], call_contexts.c_str());
							sync_perf_entry.count++;

						}		
#ifdef COMBINED_REPORT
						low_conflict_high_freq.push_back(sync_perf_entry);
#else
						qlh_count++;
						report_quadrant(qlh_fs, sync_perf_entry, qlh_count);	
#endif
					}
				}
			}
		}	
#ifdef COMBINED_REPORT
		std::fstream fs;
		fs.open("syncperf.report", std::fstream::out);
		
		//high conflict, high frequency
		fs << "\n\n==============================" << std::endl;
		fs << "HIGH CONFLICT , HIGH FREQUENCY" << std::endl;
		fs << "==============================" << std::endl;
		write_report(fs, high_conflict_high_freq);

		//high conflict, low frequency
		fs << "\n\n==============================" << std::endl;
		fs << "HIGH CONFLICT , LOW FREQUENCY" << std::endl;
		fs << "==============================" << std::endl;
		write_report(fs, high_conflict_low_freq);

		//low conflict, high frequency
		fs << "\n\n==============================" << std::endl;
		fs << "HIGH CONFLICT , LOW FREQUENCY" << std::endl;
		fs << "==============================" << std::endl;
		write_report(fs, low_conflict_high_freq);
	
		fs.close();
#endif
		std::fstream thd_fs;

		thd_fs.open("thread_waits.csv", std::fstream::out);
		thd_fs << "tid, type, runtime" << std::endl; 

		for(int idx=0; idx< total_threads; idx++){
			thread_t *thd = xthread::getInstance().getThreadInfoByIndex(idx);
			thd_fs << idx << ", " << std::hex <<(void*)( thd->startRoutine)<< ", " <<std::dec<< (thd->actualRuntime - thread_waits[idx]) << std::endl;
		}

		thd_fs.close();	
		std::cout << total_threads << " threads, " << id <<  " mutexes\n";

	}

	void write_report( std::fstream& fs, std::vector<sync_perf_t>& results){
		fs << "Total found : " << results.size() << std::endl;
		std::vector<sync_perf_t>::iterator it;
		int id = 0;
		for( it=results.begin(); it != results.end(); it++ ){
			id++;
			fs << "No."<< id << std::endl;
			fs << "-------" << std::endl;
			fs << "\t\tConflict Rate: " << it->conflict_rate << std::endl;
			fs << "\t\tAcquisition Frequency: " << it->frequency << std::endl;
			fs << "\t\tLine Numbers: " << it->count << std::endl;
			for( int i=0; i<it->count; i++ ){
				fs << "\t\t\t" << it->line_info[i] << std::endl;
			}	
		}

	}

	void report_quadrant( std::fstream& fs, sync_perf_t sync_perf_entry,int count){
		fs << "# " << count << std::endl;
		fs << "-------" << std::endl;
		fs << "\t\tConflict Rate: " << sync_perf_entry.conflict_rate << std::endl;
		fs << "\t\tAcquisition Frequency: " << sync_perf_entry.frequency << std::endl;
		fs << "\t\tLine Numbers: " << sync_perf_entry.count << std::endl;
		for( int i=0; i<sync_perf_entry.count; i++ ){
			fs << "\t\t\t" << sync_perf_entry.line_info[i] << std::endl;
		}	
	}	

};

#endif
