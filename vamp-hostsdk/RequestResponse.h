/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006-2016 Chris Cannam and QMUL.
  
    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of the Centre for
    Digital Music; Queen Mary, University of London; and Chris Cannam
    shall not be used in advertising or otherwise to promote the sale,
    use or other dealings in this Software without prior written
    authorization.
*/

#ifndef VAMP_REQUEST_RESPONSE_H
#define VAMP_REQUEST_RESPONSE_H

#include "PluginStaticData.h"
#include "PluginConfiguration.h"

#include "hostguard.h"

#include <map>
#include <string>

_VAMP_SDK_HOSTSPACE_BEGIN(RequestResponse.h)

namespace Vamp {

class Plugin;

namespace HostExt {

/**
 * \class LoadRequest RequestResponse.h <vamp-hostsdk/RequestResponse.h>
 * 
 * Vamp::HostExt::LoadRequest is a structure containing the
 * information necessary to load a plugin. When a request is made to
 * load a plugin using a LoadRequest, the response is typically
 * returned in a LoadResponse structure.
 *
 * \see LoadResponse
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK, along with the PluginLoader method that accepts this structure
 * rather than accepting its elements individually.
 */
struct LoadRequest
{
    LoadRequest() : // invalid request by default
	inputSampleRate(0.f),
	adapterFlags(0) { }

    /**
     * PluginKey is a string type that is used to identify a plugin
     * uniquely within the scope of "the current system". For further
     * details \see PluginLoader::PluginKey.
     */
    typedef std::string PluginKey;

    /**
     * The identifying key for the plugin to be loaded.
     */
    PluginKey pluginKey;

    /**
     * Sample rate to be passed to the plugin's constructor.
     */
    float inputSampleRate;

    /** 
     * A bitwise OR of the values in the PluginLoader::AdapterFlags
     * enumeration, indicating under which circumstances an adapter
     * should be used to wrap the original plugin.  If adapterFlags is
     * 0, no optional adapters will be used.
     *
     * \see PluginLoader::AdapterFlags, PluginLoader::loadPlugin
     */
    int adapterFlags;
};

/**
 * \class LoadResponse RequestResponse.h <vamp-hostsdk/RequestResponse.h>
 * 
 * Vamp::HostExt::LoadResponse is a structure containing the
 * information returned by PluginLoader when asked to load a plugin
 * using a LoadRequest.
 *
 * If the plugin could not be loaded, the plugin field will be 0.
 *
 * The caller takes ownership of the plugin contained here, which
 * should be deleted (using the standard C++ delete keyword) after
 * use.
 *
 * \see LoadRequest
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK, along with the PluginLoader method that returns this structure.
 */
struct LoadResponse
{
    LoadResponse() : // invalid (failed) response by default
	plugin(0) { }

    /**
     * A pointer to the loaded plugin, or 0 if loading failed. Caller
     * takes ownership of the plugin and must delete it after use.
     */
    Plugin *plugin;

    /**
     * The static data associated with the loaded plugin, that is, all
     * information about it that does not depend on its configuration
     * (parameters, programs, initialisation parameters). The contents
     * of this structure are only valid if plugin is non-0.
     *
     * Much of the data in here is duplicated with the plugin itself.
     */
    PluginStaticData staticData;

    /**
     * The default configuration for this plugin, that is, default
     * values for parameters etc. The contents of this structure are
     * only valid if plugin is non-0.
     */
    PluginConfiguration defaultConfiguration;
};

/**
 * \class ConfigurationRequest RequestResponse.h <vamp-hostsdk/RequestResponse.h>
 * 
 * A wrapper for a plugin pointer and PluginConfiguration, bundling up
 * the data needed to configure a plugin after it has been loaded.
 *
 * \see PluginConfiguration, ConfigurationResponse, LoadRequest, LoadResponse
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK, along with the PluginLoader method that returns this structure.
 */
struct ConfigurationRequest
{
public:
    ConfigurationRequest() : // invalid request by default
	plugin(0) { }

    Plugin *plugin;
    PluginConfiguration configuration;
};

/**
 * \class ConfigurationResponse RequestResponse.h <vamp-hostsdk/RequestResponse.h>
 *
 * The return value from a configuration request (i.e. setting the
 * parameters and initialising the plugin). If the configuration was
 * successful, the output list will contain the final
 * post-initialisation output descriptors. If configuration failed,
 * the output list will be empty.
 *
 * \see PluginConfiguration, ConfigurationRequest, LoadRequest, LoadResponse
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK, along with the PluginLoader method that returns this structure.
 */
struct ConfigurationResponse
{
public:
    ConfigurationResponse() // failed by default
    { }

    Plugin::OutputList outputs;
};

/**
 * \class ProcessRequest RequestResponse.h <vamp-hostsdk/RequestResponse.h>
 *
 * A structure that bundles the necessary data for making a process
 * call: plugin, input buffers, and timestamp. Caller retains
 * ownership of the plugin, but the buffers are passed "by value" to
 * avoid ownership concerns. 
 *
 * \see Plugin::process()
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK, but it is not currently used by the SDK. It is supplied as a
 * convenience for code using the SDK, and for symmetry with the load
 * and configuration request structs.
 */
struct ProcessRequest
{
public:
    ProcessRequest() : // invalid by default
        plugin(0) { }

    Plugin *plugin;
    RealTime timestamp;
    std::vector<std::vector<float> > input;
};

/**
 * \class ProcessResponse RequestResponse.h <vamp-hostsdk/RequestResponse.h>
 *
 * A structure that bundles the data returned by a process call and by
 * Plugin::getRemainingFeatures(). This is simply a FeatureSet
 * wrapper, named for symmetry with the other request-response pairs.
 *
 * \see Plugin::process(), Plugin::getRemainingFeatures()
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK, but it is not currently used by the SDK. It is supplied as a
 * convenience for code using the SDK, and for symmetry with the load
 * and configuration request structs.
 */
struct ProcessResponse
{
public:
    ProcessResponse() // empty by default
    { }

    Plugin::FeatureSet features;
};

}

}

_VAMP_SDK_HOSTSPACE_END(RequestResponse.h)

#endif
