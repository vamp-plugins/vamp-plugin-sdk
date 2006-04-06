/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006 Chris Cannam.
  
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

#ifndef _VAMP_PLUGIN_ADAPTER_H_
#define _VAMP_PLUGIN_ADAPTER_H_

#include <vamp/vamp.h>

#include "Plugin.h"

#include <map>

namespace Vamp {

class PluginAdapterBase
{
public:
    virtual ~PluginAdapterBase();
    const VampPluginDescriptor *getDescriptor();

protected:
    PluginAdapterBase();

    virtual Plugin *createPlugin(float inputSampleRate) = 0;

    static VampPluginHandle vampInstantiate(const VampPluginDescriptor *desc,
                                          float inputSampleRate);

    static void vampCleanup(VampPluginHandle handle);

    static int vampInitialise(VampPluginHandle handle, unsigned int channels,
                             unsigned int stepSize, unsigned int blockSize);

    static void vampReset(VampPluginHandle handle);

    static float vampGetParameter(VampPluginHandle handle, int param);
    static void vampSetParameter(VampPluginHandle handle, int param, float value);

    static unsigned int vampGetCurrentProgram(VampPluginHandle handle);
    static void vampSelectProgram(VampPluginHandle handle, unsigned int program);

    static unsigned int vampGetPreferredStepSize(VampPluginHandle handle);
    static unsigned int vampGetPreferredBlockSize(VampPluginHandle handle);
    static unsigned int vampGetMinChannelCount(VampPluginHandle handle);
    static unsigned int vampGetMaxChannelCount(VampPluginHandle handle);

    static unsigned int vampGetOutputCount(VampPluginHandle handle);

    static VampOutputDescriptor *vampGetOutputDescriptor(VampPluginHandle handle,
                                                       unsigned int i);

    static void vampReleaseOutputDescriptor(VampOutputDescriptor *desc);

    static VampFeatureList **vampProcess(VampPluginHandle handle,
                                       float **inputBuffers,
                                       int sec,
                                       int nsec);

    static VampFeatureList **vampGetRemainingFeatures(VampPluginHandle handle);

    static void vampReleaseFeatureSet(VampFeatureList **fs);

    void cleanup(Plugin *plugin);
    void checkOutputMap(Plugin *plugin);
    unsigned int getOutputCount(Plugin *plugin);
    VampOutputDescriptor *getOutputDescriptor(Plugin *plugin,
                                             unsigned int i);
    VampFeatureList **process(Plugin *plugin,
                              float **inputBuffers,
                              int sec, int nsec);
    VampFeatureList **getRemainingFeatures(Plugin *plugin);
    VampFeatureList **convertFeatures(const Plugin::FeatureSet &features);
    
    typedef std::map<const void *, PluginAdapterBase *> AdapterMap;
    static AdapterMap m_adapterMap;
    static PluginAdapterBase *lookupAdapter(VampPluginHandle);

    bool m_populated;
    VampPluginDescriptor m_descriptor;
    Plugin::ParameterList m_parameters;
    Plugin::ProgramList m_programs;
    
    typedef std::map<Plugin *, Plugin::OutputList *> OutputMap;
    OutputMap m_pluginOutputs;

    typedef std::map<Plugin *, VampFeature ***> FeatureBufferMap;
    FeatureBufferMap m_pluginFeatures;
};

template <typename P>
class PluginAdapter : public PluginAdapterBase
{
public:
    PluginAdapter() : PluginAdapterBase() { }
    ~PluginAdapter() { }

protected:
    Plugin *createPlugin(float inputSampleRate) {
        P *p = new P(inputSampleRate);
        Plugin *plugin = dynamic_cast<Plugin *>(p);
        if (!plugin) {
            std::cerr << "ERROR: PluginAdapter::createPlugin: "
                      << "Template type is not a plugin!"
                      << std::endl;
            delete p;
            return 0;
        }
        return plugin;
    }
};
    
}

#endif

