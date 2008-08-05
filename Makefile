
# Makefile for the Vamp plugin SDK.  This builds the SDK objects,
# libraries, example plugins, and the test host.  Please adjust to
# suit your operating system requirements.

APIDIR		= vamp
SDKDIR		= vamp-sdk
HOSTEXTDIR      = vamp-sdk/hostext
EXAMPLEDIR	= examples
HOSTDIR		= host

###
### Start of user-serviceable parts
###

# Default build target (or use "make <target>" to select one).
# Targets are:
#   all       -- build everything
#   sdk       -- build all the Vamp SDK libraries for plugins and hosts
#   sdkstatic -- build only the static versions of the SDK libraries
#   plugins   -- build the example plugins (and the SDK if required)
#   host      -- build the simple Vamp plugin host (and the SDK if required)
#   test      -- build the host and example plugins, and run a quick test
#   clean     -- remove binary targets
#   distclean -- remove all targets
#
default:	all

# Compile flags
#
CXXFLAGS	:= $(CXXFLAGS) -g -Wall -I. -fPIC

# ar, ranlib
#
AR		:= ar
RANLIB		:= ranlib

# Libraries required for the plugins.
#
PLUGIN_LIBS	= $(SDKDIR)/libvamp-sdk.a

# File extension for a dynamically loadable object
#
PLUGIN_EXT	= .so
#PLUGIN_EXT	= .dll
#PLUGIN_EXT	= .dylib

# Libraries required for the host.
#
HOST_LIBS	= $(SDKDIR)/libvamp-hostsdk.a -lsndfile -ldl

# Locations for "make install".  This will need quite a bit of 
# editing for non-Linux platforms.  Of course you don't necessarily
# have to use "make install".
#
INSTALL_PREFIX	 	  := /usr/local
INSTALL_API_HEADERS	  := $(INSTALL_PREFIX)/include/vamp
INSTALL_SDK_HEADERS	  := $(INSTALL_PREFIX)/include/vamp-sdk
INSTALL_HOSTEXT_HEADERS	  := $(INSTALL_PREFIX)/include/vamp-sdk/hostext
INSTALL_SDK_LIBS	  := $(INSTALL_PREFIX)/lib

INSTALL_SDK_LIBNAME	  := libvamp-sdk.so.2.0.0
INSTALL_SDK_LINK_ABI	  := libvamp-sdk.so.2
INSTALL_SDK_LINK_DEV	  := libvamp-sdk.so
INSTALL_SDK_STATIC        := libvamp-sdk.a
INSTALL_SDK_LA            := libvamp-sdk.la

INSTALL_HOSTSDK_LIBNAME   := libvamp-hostsdk.so.3.0.0
INSTALL_HOSTSDK_LINK_ABI  := libvamp-hostsdk.so.3
INSTALL_HOSTSDK_LINK_DEV  := libvamp-hostsdk.so
INSTALL_HOSTSDK_STATIC    := libvamp-hostsdk.a
INSTALL_HOSTSDK_LA        := libvamp-hostsdk.la

INSTALL_PKGCONFIG	  := $(INSTALL_PREFIX)/lib/pkgconfig

# Flags required to tell the compiler to create a dynamically loadable object
#
DYNAMIC_LDFLAGS		= -static-libgcc -shared -Wl,-Bsymbolic
SDK_DYNAMIC_LDFLAGS	= $(DYNAMIC_LDFLAGS) -Wl,-soname=$(INSTALL_SDK_LINK_ABI)
HOSTSDK_DYNAMIC_LDFLAGS	= $(DYNAMIC_LDFLAGS) -Wl,-soname=$(INSTALL_HOSTSDK_LINK_ABI)

# Additional flags for making a plugin.  This version script tells the
# GNU linker to make all symbols in the library hidden except for the
# public entry point.  It's not essential, but makes a tidier library.
PLUGIN_LDFLAGS		= $(DYNAMIC_LDFLAGS) -Wl,--version-script=vamp-plugin.map


## For OS/X with g++:
#DYNAMIC_LDFLAGS		= -dynamiclib
#PLUGIN_LDFLAGS			= $(DYNAMIC_LDFLAGS)
#SDK_DYNAMIC_LDFLAGS		= $(DYNAMIC_LDFLAGS)
#HOSTSDK_DYNAMIC_LDFLAGS	= $(DYNAMIC_LDFLAGS)


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

HOSTEXT_HEADERS = \
		$(HOSTEXTDIR)/PluginBufferingAdapter.h \
		$(HOSTEXTDIR)/PluginChannelAdapter.h \
		$(HOSTEXTDIR)/PluginInputDomainAdapter.h \
		$(HOSTEXTDIR)/PluginLoader.h \
		$(HOSTEXTDIR)/PluginSummarisingAdapter.h \
		$(HOSTEXTDIR)/PluginWrapper.h

SDK_OBJECTS	= \
		$(SDKDIR)/PluginAdapter.o \
		$(SDKDIR)/RealTime.o

HOSTSDK_OBJECTS	= \
		$(SDKDIR)/PluginHostAdapter.o \
		$(HOSTEXTDIR)/PluginBufferingAdapter.o \
		$(HOSTEXTDIR)/PluginChannelAdapter.o \
		$(HOSTEXTDIR)/PluginInputDomainAdapter.o \
		$(HOSTEXTDIR)/PluginLoader.o \
		$(HOSTEXTDIR)/PluginSummarisingAdapter.o \
		$(HOSTEXTDIR)/PluginWrapper.o \
		$(SDKDIR)/RealTime.o

SDK_STATIC	= \
		$(SDKDIR)/libvamp-sdk.a

HOSTSDK_STATIC	= \
		$(SDKDIR)/libvamp-hostsdk.a

SDK_DYNAMIC	= \
		$(SDKDIR)/libvamp-sdk$(PLUGIN_EXT)

HOSTSDK_DYNAMIC	= \
		$(SDKDIR)/libvamp-hostsdk$(PLUGIN_EXT)

SDK_LA		= \
		$(SDKDIR)/libvamp-sdk.la

HOSTSDK_LA	= \
		$(SDKDIR)/libvamp-hostsdk.la

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

sdk:		sdkstatic $(SDK_DYNAMIC) $(HOSTSDK_DYNAMIC)

sdkstatic:	$(SDK_STATIC) $(HOSTSDK_STATIC)
		$(RANLIB) $(SDK_STATIC)
		$(RANLIB) $(HOSTSDK_STATIC)

plugins:	$(PLUGIN_TARGET)

host:		$(HOST_TARGET)

all:		sdk plugins host test

$(SDK_STATIC):	$(SDK_OBJECTS) $(API_HEADERS) $(SDK_HEADERS)
		$(AR) r $@ $(SDK_OBJECTS)

$(HOSTSDK_STATIC):	$(HOSTSDK_OBJECTS) $(API_HEADERS) $(HOSTSDK_HEADERS) $(HOSTEXT_HEADERS)
		$(AR) r $@ $(HOSTSDK_OBJECTS)

$(SDK_DYNAMIC):	$(SDK_OBJECTS) $(API_HEADERS) $(SDK_HEADERS)
		$(CXX) $(LDFLAGS) $(SDK_DYNAMIC_LDFLAGS) -o $@ $(SDK_OBJECTS)

$(HOSTSDK_DYNAMIC):	$(HOSTSDK_OBJECTS) $(API_HEADERS) $(HOSTSDK_HEADERS) $(HOSTEXT_HEADERS)
		$(CXX) $(LDFLAGS) $(HOSTSDK_DYNAMIC_LDFLAGS) -o $@ $(HOSTSDK_OBJECTS)

$(PLUGIN_TARGET):	$(PLUGIN_OBJECTS) $(SDK_STATIC) $(PLUGIN_HEADERS)
		$(CXX) $(LDFLAGS) $(PLUGIN_LDFLAGS) -o $@ $(PLUGIN_OBJECTS) $(PLUGIN_LIBS)

$(HOST_TARGET):	$(HOST_OBJECTS) $(HOSTSDK_STATIC) $(HOST_HEADERS)
		$(CXX) $(LDFLAGS) $(HOST_LDFLAGS) -o $@ $(HOST_OBJECTS) $(HOST_LIBS)

test:		plugins host
		VAMP_PATH=$(EXAMPLEDIR) $(HOST_TARGET) -l

clean:		
		rm -f $(SDK_OBJECTS) $(HOSTSDK_OBJECTS) $(PLUGIN_OBJECTS) $(HOST_OBJECTS)

distclean:	clean
		rm -f $(SDK_STATIC) $(SDK_DYNAMIC) $(HOSTSDK_STATIC) $(HOSTSDK_DYNAMIC) $(PLUGIN_TARGET) $(HOST_TARGET) *~ */*~

install:	$(SDK_STATIC) $(SDK_DYNAMIC) $(HOSTSDK_STATIC) $(HOSTSDK_DYNAMIC) $(PLUGIN_TARGET) $(HOST_TARGET)
		mkdir -p $(DESTDIR)$(INSTALL_API_HEADERS)
		mkdir -p $(DESTDIR)$(INSTALL_SDK_HEADERS)
		mkdir -p $(DESTDIR)$(INSTALL_HOSTEXT_HEADERS)
		mkdir -p $(DESTDIR)$(INSTALL_SDK_LIBS)
		mkdir -p $(DESTDIR)$(INSTALL_PKGCONFIG)
		cp $(API_HEADERS) $(DESTDIR)$(INSTALL_API_HEADERS)
		cp $(SDK_HEADERS) $(DESTDIR)$(INSTALL_SDK_HEADERS)
		cp $(HOSTSDK_HEADERS) $(DESTDIR)$(INSTALL_SDK_HEADERS)
		cp $(HOSTEXT_HEADERS) $(DESTDIR)$(INSTALL_HOSTEXT_HEADERS)
		cp $(SDK_STATIC) $(DESTDIR)$(INSTALL_SDK_LIBS)
		cp $(HOSTSDK_STATIC) $(DESTDIR)$(INSTALL_SDK_LIBS)
		cp $(SDK_DYNAMIC) $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LIBNAME)
		cp $(HOSTSDK_DYNAMIC) $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LIBNAME)
		rm -f $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_ABI)
		ln -s $(INSTALL_SDK_LIBNAME) $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_ABI)
		rm -f $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_ABI)
		ln -s $(INSTALL_HOSTSDK_LIBNAME) $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_ABI)
		rm -f $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_DEV)
		ln -s $(INSTALL_SDK_LIBNAME) $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LINK_DEV)
		rm -f $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_DEV)
		ln -s $(INSTALL_HOSTSDK_LIBNAME) $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LINK_DEV)
		sed "s,%PREFIX%,$(INSTALL_PREFIX)," $(APIDIR)/vamp.pc.in \
		> $(DESTDIR)$(INSTALL_PKGCONFIG)/vamp.pc
		sed "s,%PREFIX%,$(INSTALL_PREFIX)," $(SDKDIR)/vamp-sdk.pc.in \
		> $(DESTDIR)$(INSTALL_PKGCONFIG)/vamp-sdk.pc
		sed "s,%PREFIX%,$(INSTALL_PREFIX)," $(SDKDIR)/vamp-hostsdk.pc.in \
		> $(DESTDIR)$(INSTALL_PKGCONFIG)/vamp-hostsdk.pc
		sed -e "s,%LIBNAME%,$(INSTALL_SDK_LIBNAME),g" \
		    -e "s,%LINK_ABI%,$(INSTALL_SDK_LINK_ABI),g" \
		    -e "s,%LINK_DEV%,$(INSTALL_SDK_LINK_DEV),g" \
		    -e "s,%STATIC%,$(INSTALL_SDK_STATIC),g" \
		    -e "s,%LIBS%,$(INSTALL_SDK_LIBS),g" $(SDK_LA).in \
		> $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_SDK_LA)
		sed -e "s,%LIBNAME%,$(INSTALL_HOSTSDK_LIBNAME),g" \
		    -e "s,%LINK_ABI%,$(INSTALL_HOSTSDK_LINK_ABI),g" \
		    -e "s,%LINK_DEV%,$(INSTALL_HOSTSDK_LINK_DEV),g" \
		    -e "s,%STATIC%,$(INSTALL_HOSTSDK_STATIC),g" \
		    -e "s,%LIBS%,$(INSTALL_SDK_LIBS),g" $(HOSTSDK_LA).in \
		> $(DESTDIR)$(INSTALL_SDK_LIBS)/$(INSTALL_HOSTSDK_LA)

# DO NOT DELETE

examples/AmplitudeFollower.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
examples/AmplitudeFollower.o: vamp-sdk/RealTime.h
examples/PercussionOnsetDetector.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
examples/PercussionOnsetDetector.o: vamp-sdk/RealTime.h
examples/SpectralCentroid.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
examples/SpectralCentroid.o: vamp-sdk/RealTime.h
examples/ZeroCrossing.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
examples/ZeroCrossing.o: vamp-sdk/RealTime.h
vamp-sdk/PluginAdapter.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/PluginAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/Plugin.o: vamp-sdk/PluginBase.h vamp-sdk/RealTime.h
vamp-sdk/PluginHostAdapter.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/PluginHostAdapter.o: vamp-sdk/RealTime.h
examples/AmplitudeFollower.o: examples/AmplitudeFollower.h vamp-sdk/Plugin.h
examples/AmplitudeFollower.o: vamp-sdk/PluginBase.h vamp-sdk/RealTime.h
examples/PercussionOnsetDetector.o: examples/PercussionOnsetDetector.h
examples/PercussionOnsetDetector.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
examples/PercussionOnsetDetector.o: vamp-sdk/RealTime.h
examples/plugins.o: vamp/vamp.h vamp-sdk/PluginAdapter.h vamp-sdk/Plugin.h
examples/plugins.o: vamp-sdk/PluginBase.h vamp-sdk/RealTime.h
examples/plugins.o: examples/ZeroCrossing.h vamp-sdk/Plugin.h
examples/plugins.o: examples/SpectralCentroid.h
examples/plugins.o: examples/PercussionOnsetDetector.h
examples/plugins.o: examples/AmplitudeFollower.h
examples/SpectralCentroid.o: examples/SpectralCentroid.h vamp-sdk/Plugin.h
examples/SpectralCentroid.o: vamp-sdk/PluginBase.h vamp-sdk/RealTime.h
examples/ZeroCrossing.o: examples/ZeroCrossing.h vamp-sdk/Plugin.h
examples/ZeroCrossing.o: vamp-sdk/PluginBase.h vamp-sdk/RealTime.h
host/vamp-simple-host.o: vamp-sdk/PluginHostAdapter.h vamp-sdk/Plugin.h
host/vamp-simple-host.o: vamp-sdk/PluginBase.h vamp-sdk/RealTime.h
host/vamp-simple-host.o: vamp-sdk/hostext/PluginChannelAdapter.h
host/vamp-simple-host.o: vamp-sdk/hostext/PluginWrapper.h
host/vamp-simple-host.o: vamp-sdk/hostext/PluginInputDomainAdapter.h
host/vamp-simple-host.o: vamp-sdk/hostext/PluginLoader.h vamp/vamp.h
host/vamp-simple-host.o: host/system.h
vamp-sdk/PluginAdapter.o: vamp-sdk/PluginAdapter.h vamp/vamp.h
vamp-sdk/PluginAdapter.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/PluginAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/PluginHostAdapter.o: vamp-sdk/PluginHostAdapter.h vamp/vamp.h
vamp-sdk/PluginHostAdapter.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/PluginHostAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/RealTime.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/Plugin.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/Plugin.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/Plugin.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginWrapper.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginWrapper.o: vamp-sdk/RealTime.h
rdf/generator/template-generator.o: vamp-sdk/PluginHostAdapter.h
rdf/generator/template-generator.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
rdf/generator/template-generator.o: vamp-sdk/RealTime.h
rdf/generator/template-generator.o: vamp-sdk/hostext/PluginChannelAdapter.h
rdf/generator/template-generator.o: vamp-sdk/hostext/PluginWrapper.h
rdf/generator/template-generator.o: vamp-sdk/hostext/PluginInputDomainAdapter.h
rdf/generator/template-generator.o: vamp-sdk/hostext/PluginLoader.h
rdf/generator/template-generator.o: vamp/vamp.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/hostext/PluginBufferingAdapter.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/Plugin.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginBufferingAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/hostext/PluginChannelAdapter.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/Plugin.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginChannelAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/hostext/PluginInputDomainAdapter.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/Plugin.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginInputDomainAdapter.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/PluginHostAdapter.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/RealTime.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/hostext/PluginLoader.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/hostext/PluginInputDomainAdapter.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/hostext/PluginChannelAdapter.h
vamp-sdk/hostext/PluginLoader.o: vamp-sdk/hostext/PluginBufferingAdapter.h
vamp-sdk/hostext/PluginWrapper.o: vamp-sdk/hostext/PluginWrapper.h
vamp-sdk/hostext/PluginWrapper.o: vamp-sdk/Plugin.h vamp-sdk/PluginBase.h
vamp-sdk/hostext/PluginWrapper.o: vamp-sdk/RealTime.h
