
# Makefile for the Vamp plugin SDK.  This builds the SDK objects,
# libraries, example plugins, and the test host.  Please adjust to
# suit your operating system requirements.

SDKDIR		= vamp-sdk
APIDIR		= vamp
EXAMPLEDIR	= examples
HOSTDIR		= host


### Start of user-serviceable parts

# Locations for "make install".  This will need quite a bit of 
# editing for non-Linux platforms.  Of course you don't necessarily
# have to use "make install".
#
INSTALL_PREFIX	 	  := /usr/local
INSTALL_API_HEADERS	  := $(INSTALL_PREFIX)/include/vamp
INSTALL_SDK_HEADERS	  := $(INSTALL_PREFIX)/include/vamp-sdk
INSTALL_SDK_LIBS	  := $(INSTALL_PREFIX)/lib

INSTALL_SDK_LIBNAME	  := libvamp-sdk.so.1.0.0
INSTALL_SDK_LINK_ABI	  := libvamp-sdk.so.1
INSTALL_SDK_LINK_DEV	  := libvamp-sdk.so
INSTALL_SDK_STATIC        := libvamp-sdk.a
INSTALL_SDK_LA            := libvamp-sdk.la

INSTALL_HOSTSDK_LIBNAME   := libvamp-hostsdk.so.1.0.0
INSTALL_HOSTSDK_LINK_ABI  := libvamp-hostsdk.so.1
INSTALL_HOSTSDK_LINK_DEV  := libvamp-hostsdk.so
INSTALL_HOSTSDK_STATIC    := libvamp-hostsdk.a
INSTALL_HOSTSDK_LA        := libvamp-hostsdk.la

INSTALL_PKGCONFIG	  := $(INSTALL_PREFIX)/lib/pkgconfig

# Compile flags
#
CXXFLAGS	:= $(CXXFLAGS) -O2 -Wall -I$(SDKDIR) -I$(APIDIR) -I.

# Libraries required for the host at link time
#
HOST_LIBS	= vamp-sdk/libvamp-hostsdk.a -lsndfile -ldl

# Libraries required for the plugin.  Note that we can (and actively
# want to) statically link libstdc++, because our plugin exposes only
# a C API so there are no boundary compatibility problems.
#
PLUGIN_LIBS	= vamp-sdk/libvamp-sdk.a
#PLUGIN_LIBS	= vamp-sdk/libvamp-sdk.a $(shell g++ -print-file-name=libstdc++.a)

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
		$(SDKDIR)/RealTime.h

HOSTSDK_HEADERS	= \
		$(SDKDIR)/Plugin.h \
		$(SDKDIR)/PluginBase.h \
		$(SDKDIR)/PluginHostAdapter.h \
		$(SDKDIR)/RealTime.h

SDK_OBJECTS	= \
		$(SDKDIR)/PluginAdapter.o \
		$(SDKDIR)/RealTime.o

HOSTSDK_OBJECTS	= \
		$(SDKDIR)/PluginHostAdapter.o \
		$(SDKDIR)/RealTime.o

SDK_STATIC	= \
		$(SDKDIR)/libvamp-sdk.a

HOSTSDK_STATIC	= \
		$(SDKDIR)/libvamp-hostsdk.a

SDK_DYNAMIC	= \
		$(SDKDIR)/libvamp-sdk.so

HOSTSDK_DYNAMIC	= \
		$(SDKDIR)/libvamp-hostsdk.so

SDK_LA		= \
		$(SDKDIR)/libvamp-sdk.la

PLUGIN_HEADERS	= \
		$(EXAMPLEDIR)/SpectralCentroid.h \
		$(EXAMPLEDIR)/PercussionOnsetDetector.h \
		$(EXAMPLEDIR)/AmplitudeFollower.h \
		$(EXAMPLEDIR)/ZeroCrossing.h

PLUGIN_OBJECTS	= \
		$(EXAMPLEDIR)/SpectralCentroid.o \
		$(EXAMPLEDIR)/PercussionOnsetDetector.o \
		$(EXAMPLEDIR)/AmplitudeFollower.o \
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

all:		$(SDK_STATIC) $(SDK_DYNAMIC) $(HOSTSDK_STATIC) $(HOSTSDK_DYNAMIC) $(PLUGIN_TARGET) $(HOST_TARGET) test

$(SDK_STATIC):	$(SDK_OBJECTS) $(API_HEADERS) $(SDK_HEADERS)
		$(AR) r $@ $(SDK_OBJECTS)

$(HOSTSDK_STATIC):	$(HOSTSDK_OBJECTS) $(API_HEADERS) $(HOSTSDK_HEADERS)
		$(AR) r $@ $(HOSTSDK_OBJECTS)

$(SDK_DYNAMIC):	$(SDK_OBJECTS) $(API_HEADERS) $(SDK_HEADERS)
		$(CXX) $(LDFLAGS) $(PLUGIN_LDFLAGS) -o $@ $(SDK_OBJECTS)

$(HOSTSDK_DYNAMIC):	$(HOSTSDK_OBJECTS) $(API_HEADERS) $(HOSTSDK_HEADERS)
		$(CXX) $(LDFLAGS) $(PLUGIN_LDFLAGS) -o $@ $(HOSTSDK_OBJECTS)

$(PLUGIN_TARGET):	$(PLUGIN_OBJECTS) $(SDK_TARGET) $(PLUGIN_HEADERS)
		$(CXX) $(LDFLAGS) $(PLUGIN_LDFLAGS) -o $@ $(PLUGIN_OBJECTS) $(PLUGIN_LIBS)

$(HOST_TARGET):	$(HOST_OBJECTS) $(HOSTSDK_TARGET) $(HOST_HEADERS)
		$(CXX) $(LDFLAGS) $(HOST_LDFLAGS) -o $@ $(HOST_OBJECTS) $(HOST_LIBS)

test:		$(HOST_TARGET) $(PLUGIN_TARGET)
		$(HOST_TARGET) $(PLUGIN_TARGET)

clean:		
		rm -f $(SDK_OBJECTS) $(HOSTSDK_OBJECTS) $(PLUGIN_OBJECTS) $(HOST_OBJECTS)

distclean:	clean
		rm -f $(SDK_STATIC) $(SDK_DYNAMIC) $(HOSTSDK_STATIC) $(HOSTSDK_DYNAMIC) $(PLUGIN_TARGET) $(HOST_TARGET) *~ */*~

install:	$(SDK_STATIC) $(SDK_DYNAMIC) $(HOSTSDK_STATIC) $(HOSTSDK_DYNAMIC) $(PLUGIN_TARGET) $(HOST_TARGET)
		mkdir -p $(INSTALL_API_HEADERS)
		mkdir -p $(INSTALL_SDK_HEADERS)
		mkdir -p $(INSTALL_SDK_LIBS)
		mkdir -p $(INSTALL_PKGCONFIG)
		cp $(API_HEADERS) $(INSTALL_API_HEADERS)
		cp $(SDK_HEADERS) $(INSTALL_SDK_HEADERS)
		cp $(HOSTSDK_HEADERS) $(INSTALL_SDK_HEADERS)
		cp $(SDK_STATIC) $(INSTALL_SDK_LIBS)
		cp $(HOSTSDK_STATIC) $(INSTALL_SDK_LIBS)
		cp $(SDK_DYNAMIC) $(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LIBNAME)
		cp $(HOSTSDK_DYNAMIC) $(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LIBNAME)
		rm -f $(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_ABI)
		ln -s $(INSTALL_SDK_LIBNAME) $(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_ABI)
		rm -f $(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_ABI)
		ln -s $(INSTALL_HOSTSDK_LIBNAME) $(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_ABI)
		rm -f $(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_DEV)
		ln -s $(INSTALL_SDK_LIBNAME) $(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_DEV)
		rm -f $(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_DEV)
		ln -s $(INSTALL_HOSTSDK_LIBNAME) $(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_DEV)
		sed "s,%PREFIX%,$(INSTALL_PREFIX)," $(APIDIR)/vamp.pc.in \
		> $(INSTALL_PKGCONFIG)/vamp.pc
		sed "s,%PREFIX%,$(INSTALL_PREFIX)," $(SDKDIR)/vamp-sdk.pc.in \
		> $(INSTALL_PKGCONFIG)/vamp-sdk.pc
		sed "s,%PREFIX%,$(INSTALL_PREFIX)," $(SDKDIR)/vamp-hostsdk.pc.in \
		> $(INSTALL_PKGCONFIG)/vamp-hostsdk.pc
		sed -e "s,%LIBNAME%,$(INSTALL_SDK_LIBNAME),g" \
		    -e "s,%LINK_ABI%,$(INSTALL_SDK_LINK_ABI),g" \
		    -e "s,%LINK_DEV%,$(INSTALL_SDK_LINK_DEV),g" \
		    -e "s,%STATIC%,$(INSTALL_SDK_STATIC),g" \
		    -e "s,%LIBS%,$(INSTALL_SDK_LIBS),g" $(SDK_LA).in \
		> $(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LA)
		sed -e "s,%LIBNAME%,$(INSTALL_HOSTSDK_LIBNAME),g" \
		    -e "s,%LINK_ABI%,$(INSTALL_HOSTSDK_LINK_ABI),g" \
		    -e "s,%LINK_DEV%,$(INSTALL_HOSTSDK_LINK_DEV),g" \
		    -e "s,%STATIC%,$(INSTALL_HOSTSDK_STATIC),g" \
		    -e "s,%LIBS%,$(INSTALL_SDK_LIBS),g" $(SDK_LA).in \
		> $(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LA)

