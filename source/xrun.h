/*
 * @file   xrun.h
 * @brief  Overall program execution control
 * @author Tongping Liu<tongping.liu@utsa.edu>, Mejbah ul Alam <mejbah.alam@gmail.com>
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

		xthread::getInstance().initialize();
		
  }

  void finalize (void)
  {

		xthread::getInstance().finalize();
  }
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
      exit(0);
    }
    else if (signum == SIGUSR2) {
      fprintf(stderr, "Recieving SIGUSR2, not handled\n");
    }
  }
private:

  int _tid;
};


#endif
