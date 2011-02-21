#
# OMNeT++/OMNEST Makefile for copy-inet-amended
#
# This file was generated with the command:
#  opp_makemake -f
#

# Name of target to be created (-o option)
TARGET = copy-inet-amended$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = -I.

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS =

# Output directory
PROJECT_OUTPUT_DIR = ../../../out
PROJECTRELATIVE_PATH = /src/networklayer/ipv4
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = \
    $O/ErrorHandling.o \
    $O/ICMP.o \
    $O/IP.o \
    $O/IPFragBuf.o \
    $O/IPRoute.o \
    $O/IPv4InterfaceData.o \
    $O/RoutingTable.o \
    $O/RoutingTableParser.o \
    $O/ControlManetRouting_m.o \
    $O/ICMPMessage_m.o \
    $O/IPDatagram_m.o

# Message files
MSGFILES = \
    ControlManetRouting.msg \
    ICMPMessage.msg \
    IPDatagram.msg

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET)

$(TARGET) : $O/$(TARGET)
	ln -s -f $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@mkdir -p $O
	$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY:

.SUFFIXES: .cc

$O/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -c $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	-rm -rf $O
	-rm -f copy-inet-amended copy-inet-amended.exe libcopy-inet-amended.so libcopy-inet-amended.a libcopy-inet-amended.dll libcopy-inet-amended.dylib
	-rm -f ./*_m.cc ./*_m.h

cleanall: clean
	-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/ControlManetRouting_m.o: ControlManetRouting_m.cc \
  ./ControlManetRouting_m.h
$O/ErrorHandling.o: ErrorHandling.cc \
  ./ICMPMessage_m.h \
  ./IPDatagram_m.h \
  ./ICMPMessage.h \
  ./IPDatagram.h \
  ./ErrorHandling.h
$O/ICMP.o: ICMP.cc \
  ./IPRoute.h \
  ./RoutingTableAccess.h \
  ./ICMPMessage_m.h \
  ./IPDatagram_m.h \
  ./ICMPMessage.h \
  ./IRoutingTable.h \
  ./IPDatagram.h \
  ./ICMP.h
$O/ICMPMessage_m.o: ICMPMessage_m.cc \
  ./ICMPMessage_m.h
$O/IP.o: IP.cc \
  ./IPFragBuf.h \
  ./IPRoute.h \
  ./ICMPAccess.h \
  ./IPDatagram_m.h \
  ./RoutingTableAccess.h \
  ./IPv4InterfaceData.h \
  ./ICMPMessage_m.h \
  ./ICMPMessage.h \
  ./IRoutingTable.h \
  ./IPDatagram.h \
  ./IP.h \
  ./ICMP.h
$O/IPDatagram_m.o: IPDatagram_m.cc \
  ./IPDatagram_m.h
$O/IPFragBuf.o: IPFragBuf.cc \
  ./IPRoute.h \
  ./IPFragBuf.h \
  ./RoutingTableAccess.h \
  ./ICMPMessage_m.h \
  ./IPDatagram_m.h \
  ./ICMPMessage.h \
  ./IRoutingTable.h \
  ./IPDatagram.h \
  ./ICMP.h
$O/IPRoute.o: IPRoute.cc \
  ./IPRoute.h
$O/IPv4InterfaceData.o: IPv4InterfaceData.cc \
  ./IPv4InterfaceData.h
$O/RoutingTable.o: RoutingTable.cc \
  ./IPRoute.h \
  ./IPv4InterfaceData.h \
  ./RoutingTableParser.h \
  ./IRoutingTable.h \
  ./RoutingTable.h
$O/RoutingTableParser.o: RoutingTableParser.cc \
  ./IPRoute.h \
  ./IPv4InterfaceData.h \
  ./RoutingTableParser.h \
  ./IRoutingTable.h \
  ./RoutingTable.h
