// -*- C++ -*-

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

// memory
//#include "xmemory.h"
//#include "internalheap.h"

// Grace utilities
//#include "atomic.h"

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
		//installSignalHandler();
//    InternalHeap::getInstance().initialize();

		fprintf(stderr, "xrun initialize before xthread initialize\n");
		xthread::getInstance().initialize();

//		fprintf(stderr, "xrun initialize before xmemory initialize\n");
    // Initialize the memory (install the memory handler)
//    _memory.initialize();
  }

  void finalize (void)
  {
//		xthread::getInstance().finalize();

    // If the tid was set, it means that this instance was
    // initialized: end the transaction (at the end of main()).
//   	_memory.finalize();
//		report();
  }

#if 0
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
      perror ("installing SIGINT failed\n");
      exit (-1);
		}
#endif
	}

	static void sigHandler(int signum) {
    if(signum == SIGINT) {
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
