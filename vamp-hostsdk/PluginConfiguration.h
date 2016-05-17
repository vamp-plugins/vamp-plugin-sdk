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

#ifndef VAMP_PLUGIN_CONFIGURATION_H
#define VAMP_PLUGIN_CONFIGURATION_H

#include "hostguard.h"

#include "Plugin.h"

#include <map>
#include <string>

_VAMP_SDK_HOSTSPACE_BEGIN(PluginConfiguration.h)

namespace Vamp {

namespace HostExt {

/**
 * \class PluginConfiguration PluginConfiguration.h <vamp-hostsdk/PluginConfiguration.h>
 * 
 * Vamp::HostExt::PluginConfiguration is a structure bundling together
 * data that affect the configuration of a plugin: parameter values,
 * programs, and initialisation settings. Although an interactive Vamp
 * plugin host may configure a plugin in stages, for example to take
 * into account that a plugin's preferred step and block size may
 * change when its parameters are changed, a batch host or a host
 * supporting store and recall of configurations may wish to keep all
 * configuration settings together.
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK.
 */
struct PluginConfiguration
{
    PluginConfiguration() : // invalid configuration by default
	channelCount(0), stepSize(0), blockSize(0) { }
	
    int channelCount;
    int stepSize;
    int blockSize;
    typedef std::map<std::string, float> ParameterMap;
    ParameterMap parameterValues;
    std::string currentProgram;

    static PluginConfiguration
    fromPlugin(Plugin *p,
	       int channelCount,
	       int stepSize,
	       int blockSize) {
	
	PluginConfiguration c;
	
	c.channelCount = channelCount;
	c.stepSize = stepSize;
	c.blockSize = blockSize;

	PluginBase::ParameterList params = p->getParameterDescriptors();
	for (PluginBase::ParameterList::const_iterator i = params.begin();
	     i != params.end(); ++i) {
	    std::string pid = i->identifier;
	    c.parameterValues[pid] = p->getParameter(pid);
	}
	
	if (!p->getPrograms().empty()) {
	    c.currentProgram = p->getCurrentProgram();
	}

	return c;
    }
};

/**
 * \class ConfigurationRequest PluginConfiguration.h <vamp-hostsdk/PluginConfiguration.h>
 * 
 * A wrapper for a plugin pointer and PluginConfiguration, bundling up
 * the data needed to configure a plugin after it has been loaded.
 *
 * \see PluginConfiguration, ConfigurationResponse, LoadRequest, LoadResponse
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
 * \class ConfigurationResponse PluginConfiguration.h <vamp-hostsdk/PluginConfiguration.h>
 *
 * The return value from a configuration request (i.e. setting the
 * parameters and initialising the plugin). If the configuration was
 * successful, the output list will contain the final
 * post-initialisation output descriptors. If configuration failed,
 * the output list will be empty.
 *
 * \see PluginConfiguration, ConfigurationRequest, LoadRequest, LoadResponse
 */
struct ConfigurationResponse
{
public:
    ConfigurationResponse() // failed by default
    { }

    Plugin::OutputList outputs;
};

}

}

_VAMP_SDK_HOSTSPACE_END(PluginConfiguration.h)

#endif
