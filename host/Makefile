
SDKPATH := ../sdk
APIPATH := ../vamp

CXXFLAGS := $(CXXFLAGS) -O2 -Wall -I$(SDKPATH) -I$(APIPATH)
LIBS := $(LIBS) -ldl

SDKFILES := $(SDKPATH)/PluginHostAdapter.cpp $(SDKPATH)/RealTime.cpp

simplehost:	simplehost.o
	$(CXX) $(CXXFLAGS) -o simplehost $(SDKFILES) simplehost.o $(LIBS)

all: 	simplehost

test:
	simplehost ../examples/plugins.so

clean:
	rm -f simplehost.o

distclean:	clean
	rm -f simplehost *~ *.bak

simplehost.o:	simplehost.cpp system.h ../vamp/vamp.h ../sdk/PluginHostAdapter.h ../sdk/PluginHostAdapter.cpp ../sdk/RealTime.h ../sdk/RealTime.cpp ../sdk/Plugin.h ../sdk/PluginBase.h
