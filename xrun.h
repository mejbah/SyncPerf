/*
  Author: Emery Berger, http://www.cs.umass.edu/~emery
 
  Copyright (c) 2007-12 Emery Berger, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   xrun.h
 * @brief  The main engine for consistency management, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XRUN_H
#define _XRUN_H

#include "xdefines.h"

#include "xthread.h"

#include "mutex_manager.h"



class xrun {

private:

  xrun()
  {
  }

public:

  static xrun& getInstance() {
    //static char buf[sizeof(xrun)];
    static xrun * theOneTrueObject = new xrun();

    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize()
  {
#ifdef GET_STATISTICS
		totalLocks = 0;
#endif
		installSignalHandler();

		fprintf(stderr, "xrun initialize before xthread initialize. mutex size %ld. mytex_t %ld\n", sizeof(pthread_mutex_t), sizeof(mutex_t));
		xthread::getInstance().initialize();
		
//		fprintf(stderr, "xrun initialize before xmemory initialize\n");
//    _memory.initialize();
  }

  void finalize (void)
  {
#ifdef GET_STATISTICS
		fprintf(stderr, "total locks in this program is %ld\n", totalLocks);
#endif

		xthread::getInstance().finalize();
		//report();
  }
#if 1
  /// @brief Install a handler for KILL signals.
  void installSignalHandler() {
    struct sigaction siga;

    // Point to the handler function.
    siga.sa_flags = SA_RESTART | SA_NODEFER;

    siga.sa_handler = sigHandler;
    if (sigaction(SIGINT, &siga, NULL) == -1) {
      perror ("installing SIGINT failed\n");
      exit (-1);
		}
#ifdef USING_SIGUSR2
    if (sigaction(SIGUSR2, &siga, NULL) == -1) {
      perror ("installing SIGUSR2 failed\n");
      exit (-1);
		}
#endif
	}

	static void sigHandler(int signum) {
    if(signum == SIGINT) {
			fprintf(stderr, "Recieved SIGINT, Genearting Report\n");
			//report_mutex_conflicts();	
			//report_thread_waits();
      exit(0);
    }
    else if (signum == SIGUSR2) {
      fprintf(stderr, "Recieving SIGUSR2, check false sharing now:\n");
      //xmemory::getInstance().reportFalseSharing();
    }
  }
#endif
private:

  int _tid;

  /// The memory manager (for both heap and globals).
//  xmemory&     _memory;
};


#endif
