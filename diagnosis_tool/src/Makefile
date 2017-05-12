PIN_ROOT=/home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux
CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config
#include $(TOOLS_ROOT)/Config/makefile.default.rules


LINKER =${CXX}
OPT =${COPT}

PINTOOL = sync_track.so
PINTOOL_OBJS = sync_track.o

all : $(PINTOOL)

sync_track.o : sync_track.cpp
	$(CXX) $(TOOL_CXXFLAGS) $(COMP_OBJ)$@ $<
#	$(CC) $(TOOL_CXXFLAGS) $(COMP_OBJ)$@ $<
sync_track.so : $(PINTOOL_OBJS)
	$(LINKER) $(TOOL_LDFLAGS) $(LINK_EXE)$@ $< $(TOOL_LPATHS) $(TOOL_LIBS)


clean:
	  rm -rf *.d *.so *.o
