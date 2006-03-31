
TARGET	:= plugins.so
OBJECTS	:= ZeroCrossing.o SpectralCentroid.o plugins.o

SDKPATH := ../sdk
APIPATH := ../vamp

CXXFLAGS := $(CXXFLAGS) -O2 -Wall -I$(SDKPATH) -I$(APIPATH)

LDFLAGS := -shared -Wl,-Bsymbolic

APIFILES := $(SDKPATH)/PluginAdapter.cpp $(SDKPATH)/RealTime.cpp

plugins.so:	$(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(APIFILES) $(OBJECTS)

all:	$(TARGET)

clean:
	rm -f $(OBJECTS)

distclean:	clean
	rm -f *~ *.bak $(TARGET)

