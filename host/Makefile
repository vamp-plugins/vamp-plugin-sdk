
SDKPATH := ../vamp-sdk
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

simplehost.o:	simplehost.cpp system.h $(APIPATH)/vamp.h $(SDKPATH)/PluginHostAdapter.h $(SDKPATH)/PluginHostAdapter.cpp $(SDKPATH)/RealTime.h $(SDKPATH)/RealTime.cpp $(SDKPATH)/Plugin.h $(SDKPATH)/PluginBase.h
