
TARGET	:= plugins.so
OBJECTS	:= ZeroCrossing.o SpectralCentroid.o plugins.o

SDKPATH := ../vamp-sdk
APIPATH := ../vamp

CXXFLAGS := $(CXXFLAGS) -O2 -Wall -I$(SDKPATH) -I$(APIPATH)

LDFLAGS := -shared -Wl,-Bsymbolic

SDKFILES := $(SDKPATH)/PluginAdapter.cpp $(SDKPATH)/RealTime.cpp

plugins.so:	$(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(SDKFILES) $(OBJECTS)

all:	$(TARGET)

clean:
	rm -f $(OBJECTS)

distclean:	clean
	rm -f *~ *.bak $(TARGET)

ZeroCrossing.o:	ZeroCrossing.cpp $(APIPATH)/vamp.h $(SDKPATH)/PluginAdapter.h $(SDKPATH)/PluginAdapter.cpp $(SDKPATH)/RealTime.h $(SDKPATH)/RealTime.cpp $(SDKPATH)/Plugin.h $(SDKPATH)/PluginBase.h
SpectralCentroid.o:	SpectralCentroid.cpp $(APIPATH)/vamp.h $(SDKPATH)/PluginAdapter.h $(SDKPATH)/PluginAdapter.cpp $(SDKPATH)/RealTime.h $(SDKPATH)/RealTime.cpp $(SDKPATH)/Plugin.h $(SDKPATH)/PluginBase.h
