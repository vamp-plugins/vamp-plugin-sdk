
TARGET	:= plugins.so
OBJECTS	:= ZeroCrossing.o SpectralCentroid.o plugins.o

SDKPATH := ../sdk
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

ZeroCrossing.o:	ZeroCrossing.cpp ../vamp/vamp.h ../sdk/PluginAdapter.h ../sdk/PluginAdapter.cpp ../sdk/RealTime.h ../sdk/RealTime.cpp ../sdk/Plugin.h ../sdk/PluginBase.h
SpectralCentroid.o:	SpectralCentroid.cpp ../vamp/vamp.h ../sdk/PluginAdapter.h ../sdk/PluginAdapter.cpp ../sdk/RealTime.h ../sdk/RealTime.cpp ../sdk/Plugin.h ../sdk/PluginBase.h
