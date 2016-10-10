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

#ifndef VAMP_PLUGIN_STATIC_DATA_H
#define VAMP_PLUGIN_STATIC_DATA_H

#include "hostguard.h"
#include "Plugin.h"

_VAMP_SDK_HOSTSPACE_BEGIN(PluginStaticData.h)

namespace Vamp {

namespace HostExt {

/**
 * \class PluginStaticData PluginStaticData.h <vamp-hostsdk/PluginStaticData.h>
 * 
 * Vamp::HostExt::PluginStaticData is a structure bundling together
 * all the information about a plugin that cannot be changed by the
 * plugin after it is loaded. That is, everything that does not depend
 * on a parameter or initialisation setting.
 *
 * All of the information in here can be queried from other sources
 * directly (notably the Plugin class itself); this structure just
 * pulls it together in one place and provides something that can be
 * stored and recalled without having a Plugin object to hand.
 *
 * \note This class was introduced in version 2.7 of the Vamp plugin
 * SDK and is used only by host SDK functions that were also
 * introduced in that release (or newer).
 */
struct PluginStaticData
{
public:
    struct Basic {
	std::string identifier;
	std::string name;
	std::string description;
    };
    typedef std::vector<Basic> BasicList;

    PluginStaticData() : // invalid static data by default
	pluginVersion(0), minChannelCount(0), maxChannelCount(0),
	inputDomain(Plugin::TimeDomain) { }

    std::string pluginKey;
    Basic basic;
    std::string maker;
    std::string copyright;
    int pluginVersion;
    std::vector<std::string> category;
    int minChannelCount;
    int maxChannelCount;
    PluginBase::ParameterList parameters;
    PluginBase::ProgramList programs;
    Plugin::InputDomain inputDomain;
    BasicList basicOutputInfo;

    static PluginStaticData
    fromPlugin(std::string pluginKey,
	       std::vector<std::string> category,
	       Plugin *p) {

	PluginStaticData d;
	d.pluginKey = pluginKey;
	d.basic.identifier = p->getIdentifier();
	d.basic.name = p->getName();
	d.basic.description = p->getDescription();
	d.maker = p->getMaker();
	d.copyright = p->getCopyright();
	d.pluginVersion = p->getPluginVersion();
	d.category = category;
	d.minChannelCount = p->getMinChannelCount();
	d.maxChannelCount = p->getMaxChannelCount();
	d.parameters = p->getParameterDescriptors();
	d.programs = p->getPrograms();
	d.inputDomain = p->getInputDomain();

	Plugin::OutputList outputs = p->getOutputDescriptors();
	for (Plugin::OutputList::const_iterator i = outputs.begin();
	     i != outputs.end(); ++i) {
	    Basic b;
	    b.identifier = i->identifier;
	    b.name = i->name;
	    b.description = i->description;
	    d.basicOutputInfo.push_back(b);
	}

	return d;
    }
};

}

}

_VAMP_SDK_HOSTSPACE_END(PluginStaticData.h)

#endif
