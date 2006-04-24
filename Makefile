
# Makefile for the Vamp plugin SDK.  This builds the SDK objects,
# example plugins, and the test host.  Please adjust to suit your
# operating system requirements.

SDKDIR		= vamp-sdk
APIDIR		= vamp
EXAMPLEDIR	= examples
HOSTDIR		= host


### Start of user-serviceable parts

# Compile flags
#
CXXFLAGS	:= $(CXXFLAGS) -g -Wall -I$(SDKDIR) -I$(APIDIR) -I.

# Libraries required for the host at link time
#
HOST_LIBS	= -Lvamp-sdk -lvamp-sdk -lsndfile -ldl

# Libraries required for the plugin.  Note that we can (and actively
# want to) statically link libstdc++, because our plugin exposes only
# a C API so there are no boundary compatibility problems.
#
PLUGIN_LIBS	= -Lvamp-sdk -lvamp-sdk
#PLUGIN_LIBS	= -Lvamp-sdk -lvamp-sdk $(shell g++ -print-file-name=libstdc++.a)

# Flags required to tell the compiler to link to a dynamically loadable object
#
PLUGIN_LDFLAGS	= -shared -Wl,-Bsymbolic -static-libgcc

# File extension for a dynamically loadable object
#
PLUGIN_EXT	= .so

## For OS/X with g++:
#PLUGIN_LDFLAGS	= -dynamiclib
#PLUGIN_EXT	= .dylib

### End of user-serviceable parts

API_HEADERS	= \
		$(APIDIR)/vamp.h

SDK_HEADERS	= \
		$(SDKDIR)/Plugin.h \
		$(SDKDIR)/PluginAdapter.h \
		$(SDKDIR)/PluginBase.h \
		$(SDKDIR)/PluginHostAdapter.h \
		$(SDKDIR)/RealTime.h

SDK_OBJECTS	= \
		$(SDKDIR)/PluginAdapter.o \
		$(SDKDIR)/PluginHostAdapter.o \
		$(SDKDIR)/RealTime.o

SDK_TARGET	= \
		$(SDKDIR)/libvamp-sdk.a

PLUGIN_HEADERS	= \
		$(EXAMPLEDIR)/SpectralCentroid.h \
		$(EXAMPLEDIR)/ZeroCrossing.h

PLUGIN_OBJECTS	= \
		$(EXAMPLEDIR)/SpectralCentroid.o \
		$(EXAMPLEDIR)/ZeroCrossing.o \
		$(EXAMPLEDIR)/plugins.o

PLUGIN_TARGET	= \
		$(EXAMPLEDIR)/vamp-example-plugins$(PLUGIN_EXT)

HOST_HEADERS	= \
		$(HOSTDIR)/system.h

HOST_OBJECTS	= \
		$(HOSTDIR)/vamp-simple-host.o

HOST_TARGET	= \
		$(HOSTDIR)/vamp-simple-host

all:		$(SDK_TARGET) $(PLUGIN_TARGET) $(HOST_TARGET) test

$(SDK_TARGET):	$(SDK_OBJECTS) $(API_HEADERS) $(SDK_HEADERS)
		$(AR) r $@ $(SDK_OBJECTS)

$(PLUGIN_TARGET):	$(PLUGIN_OBJECTS) $(SDK_TARGET) $(PLUGIN_HEADERS)
		$(CXX) $(LDFLAGS) $(PLUGIN_LDFLAGS) -o $@ $(PLUGIN_OBJECTS) $(PLUGIN_LIBS)

$(HOST_TARGET):	$(HOST_OBJECTS) $(SDK_TARGET) $(HOST_HEADERS)
		$(CXX) $(LDFLAGS) $(HOST_LDFLAGS) -o $@ $(HOST_OBJECTS) $(HOST_LIBS)

test:		$(HOST_TARGET) $(PLUGIN_TARGET)
		$(HOST_TARGET) $(PLUGIN_TARGET)

clean:		
		rm -f $(SDK_OBJECTS) $(PLUGIN_OBJECTS) $(HOST_OBJECTS)

distclean:	clean
		rm -f $(SDK_TARGET) $(PLUGIN_TARGET) $(HOST_TARGET) *~ */*~


