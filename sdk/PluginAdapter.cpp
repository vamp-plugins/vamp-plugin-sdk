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
    NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of the Centre for
    Digital Music; Queen Mary, University of London; and Chris Cannam
    shall not be used in advertising or otherwise to promote the sale,
    use or other dealings in this Software without prior written
    authorization.
*/

#include "PluginAdapter.h"

namespace Vamp {

PluginAdapterBase::PluginAdapterBase() :
    m_populated(false)
{
}

const VampPluginDescriptor *
PluginAdapterBase::getDescriptor()
{
    if (m_populated) return &m_descriptor;

    Plugin *plugin = createPlugin(48000);

    m_parameters = plugin->getParameterDescriptors();
    m_programs = plugin->getPrograms();
    
    m_descriptor.name = strdup(plugin->getName().c_str());
    m_descriptor.description = strdup(plugin->getDescription().c_str());
    m_descriptor.maker = strdup(plugin->getMaker().c_str());
    m_descriptor.pluginVersion = plugin->getPluginVersion();
    m_descriptor.copyright = strdup(plugin->getCopyright().c_str());
    
    m_descriptor.parameterCount = m_parameters.size();
    m_descriptor.parameters = (const VampParameterDescriptor **)
        malloc(m_parameters.size() * sizeof(VampParameterDescriptor));
    
    for (unsigned int i = 0; i < m_parameters.size(); ++i) {
        VampParameterDescriptor *desc = (VampParameterDescriptor *)
            malloc(sizeof(VampParameterDescriptor));
        desc->name = strdup(m_parameters[i].name.c_str());
        desc->description = strdup(m_parameters[i].description.c_str());
        desc->unit = strdup(m_parameters[i].unit.c_str());
        desc->minValue = m_parameters[i].minValue;
        desc->maxValue = m_parameters[i].maxValue;
        desc->defaultValue = m_parameters[i].defaultValue;
        desc->isQuantized = m_parameters[i].isQuantized;
        desc->quantizeStep = m_parameters[i].quantizeStep;
        m_descriptor.parameters[i] = desc;
    }
    
    m_descriptor.programCount = m_programs.size();
    m_descriptor.programs = (const char **)
        malloc(m_programs.size() * sizeof(const char *));
    
    for (unsigned int i = 0; i < m_programs.size(); ++i) {
        m_descriptor.programs[i] = strdup(m_programs[i].c_str());
    }
    
    if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
        m_descriptor.inputDomain = vampFrequencyDomain;
    } else {
        m_descriptor.inputDomain = vampTimeDomain;
    }

    m_descriptor.instantiate = vampInstantiate;
    m_descriptor.cleanup = vampCleanup;
    m_descriptor.initialise = vampInitialise;
    m_descriptor.reset = vampReset;
    m_descriptor.getParameter = vampGetParameter;
    m_descriptor.setParameter = vampSetParameter;
    m_descriptor.getCurrentProgram = vampGetCurrentProgram;
    m_descriptor.selectProgram = vampSelectProgram;
    m_descriptor.getPreferredStepSize = vampGetPreferredStepSize;
    m_descriptor.getPreferredBlockSize = vampGetPreferredBlockSize;
    m_descriptor.getMinChannelCount = vampGetMinChannelCount;
    m_descriptor.getMaxChannelCount = vampGetMaxChannelCount;
    m_descriptor.getOutputCount = vampGetOutputCount;
    m_descriptor.getOutputDescriptor = vampGetOutputDescriptor;
    m_descriptor.releaseOutputDescriptor = vampReleaseOutputDescriptor;
    m_descriptor.process = vampProcess;
    m_descriptor.getRemainingFeatures = vampGetRemainingFeatures;
    m_descriptor.releaseFeatureSet = vampReleaseFeatureSet;
    
    m_adapterMap[&m_descriptor] = this;

    delete plugin;

    m_populated = true;
    return &m_descriptor;
}

PluginAdapterBase::~PluginAdapterBase()
{
    if (!m_populated) return;

    free((void *)m_descriptor.name);
    free((void *)m_descriptor.description);
    free((void *)m_descriptor.maker);
    free((void *)m_descriptor.copyright);
        
    for (unsigned int i = 0; i < m_descriptor.parameterCount; ++i) {
        const VampParameterDescriptor *desc = m_descriptor.parameters[i];
        free((void *)desc->name);
        free((void *)desc->description);
        free((void *)desc->unit);
    }
    free((void *)m_descriptor.parameters);

    for (unsigned int i = 0; i < m_descriptor.programCount; ++i) {
        free((void *)m_descriptor.programs[i]);
    }
    free((void *)m_descriptor.programs);

    m_adapterMap.erase(&m_descriptor);
}

PluginAdapterBase *
PluginAdapterBase::lookupAdapter(VampPluginHandle handle)
{
    AdapterMap::const_iterator i = m_adapterMap.find(handle);
    if (i == m_adapterMap.end()) return 0;
    return i->second;
}

VampPluginHandle
PluginAdapterBase::vampInstantiate(const VampPluginDescriptor *desc,
                                   float inputSampleRate)
{
    if (m_adapterMap.find(desc) == m_adapterMap.end()) return 0;
    PluginAdapterBase *adapter = m_adapterMap[desc];
    if (desc != &adapter->m_descriptor) return 0;

    Plugin *plugin = adapter->createPlugin(inputSampleRate);
    if (plugin) {
        m_adapterMap[plugin] = adapter;
    }

    return plugin;
}

void
PluginAdapterBase::vampCleanup(VampPluginHandle handle)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) {
        delete ((Plugin *)handle);
        return;
    }
    adapter->cleanup(((Plugin *)handle));
}

int
PluginAdapterBase::vampInitialise(VampPluginHandle handle,
                                  unsigned int channels,
                                  unsigned int stepSize,
                                  unsigned int blockSize)
{
    bool result = ((Plugin *)handle)->initialise
        (channels, stepSize, blockSize);
    return result ? 1 : 0;
}

void
PluginAdapterBase::vampReset(VampPluginHandle handle) 
{
    ((Plugin *)handle)->reset();
}

float
PluginAdapterBase::vampGetParameter(VampPluginHandle handle,
                                    int param) 
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0.0;
    Plugin::ParameterList &list = adapter->m_parameters;
    return ((Plugin *)handle)->getParameter(list[param].name);
}

void
PluginAdapterBase::vampSetParameter(VampPluginHandle handle,
                                    int param, float value)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return;
    Plugin::ParameterList &list = adapter->m_parameters;
    ((Plugin *)handle)->setParameter(list[param].name, value);
}

unsigned int
PluginAdapterBase::vampGetCurrentProgram(VampPluginHandle handle)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    Plugin::ProgramList &list = adapter->m_programs;
    std::string program = ((Plugin *)handle)->getCurrentProgram();
    for (unsigned int i = 0; i < list.size(); ++i) {
        if (list[i] == program) return i;
    }
    return 0;
}

void
PluginAdapterBase::vampSelectProgram(VampPluginHandle handle,
                                     unsigned int program)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return;
    Plugin::ProgramList &list = adapter->m_programs;
    ((Plugin *)handle)->selectProgram(list[program]);
}

unsigned int
PluginAdapterBase::vampGetPreferredStepSize(VampPluginHandle handle)
{
    return ((Plugin *)handle)->getPreferredStepSize();
}

unsigned int
PluginAdapterBase::vampGetPreferredBlockSize(VampPluginHandle handle) 
{
    return ((Plugin *)handle)->getPreferredBlockSize();
}

unsigned int
PluginAdapterBase::vampGetMinChannelCount(VampPluginHandle handle)
{
    return ((Plugin *)handle)->getMinChannelCount();
}

unsigned int
PluginAdapterBase::vampGetMaxChannelCount(VampPluginHandle handle)
{
    return ((Plugin *)handle)->getMaxChannelCount();
}

unsigned int
PluginAdapterBase::vampGetOutputCount(VampPluginHandle handle)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getOutputCount((Plugin *)handle);
}

VampOutputDescriptor *
PluginAdapterBase::vampGetOutputDescriptor(VampPluginHandle handle,
                                           unsigned int i)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getOutputDescriptor((Plugin *)handle, i);
}

void
PluginAdapterBase::vampReleaseOutputDescriptor(VampOutputDescriptor *desc)
{
    if (desc->name) free((void *)desc->name);
    if (desc->description) free((void *)desc->description);
    if (desc->unit) free((void *)desc->unit);
    for (unsigned int i = 0; i < desc->valueCount; ++i) {
        free((void *)desc->valueNames[i]);
    }
    if (desc->valueNames) free((void *)desc->valueNames);
    free((void *)desc);
}

VampFeatureList **
PluginAdapterBase::vampProcess(VampPluginHandle handle,
                               float **inputBuffers,
                               int sec,
                               int nsec)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->process((Plugin *)handle,
                            inputBuffers, sec, nsec);
}

VampFeatureList **
PluginAdapterBase::vampGetRemainingFeatures(VampPluginHandle handle)
{
    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getRemainingFeatures((Plugin *)handle);
}

void
PluginAdapterBase::vampReleaseFeatureSet(VampFeatureList **fs)
{
    if (!fs) return;
    for (unsigned int i = 0; fs[i]; ++i) {
        for (unsigned int j = 0; j < fs[i]->featureCount; ++j) {
            VampFeature *feature = &fs[i]->features[j];
            if (feature->values) free((void *)feature->values);
            if (feature->label) free((void *)feature->label);
            free((void *)feature);
        }
        if (fs[i]->features) free((void *)fs[i]->features);
        free((void *)fs[i]);
    }
    free((void *)fs);
}

void 
PluginAdapterBase::cleanup(Plugin *plugin)
{
    if (m_pluginOutputs.find(plugin) != m_pluginOutputs.end()) {
        delete m_pluginOutputs[plugin];
        m_pluginOutputs.erase(plugin);
    }
    m_adapterMap.erase(plugin);
    delete ((Plugin *)plugin);
}

void 
PluginAdapterBase::checkOutputMap(Plugin *plugin)
{
    if (!m_pluginOutputs[plugin]) {
        m_pluginOutputs[plugin] = new Plugin::OutputList
            (plugin->getOutputDescriptors());
    }
}

unsigned int 
PluginAdapterBase::getOutputCount(Plugin *plugin)
{
    checkOutputMap(plugin);
    return m_pluginOutputs[plugin]->size();
}

VampOutputDescriptor *
PluginAdapterBase::getOutputDescriptor(Plugin *plugin,
                                       unsigned int i)
{
    checkOutputMap(plugin);
    Plugin::OutputDescriptor &od =
        (*m_pluginOutputs[plugin])[i];

    VampOutputDescriptor *desc = (VampOutputDescriptor *)
        malloc(sizeof(VampOutputDescriptor));

    desc->name = strdup(od.name.c_str());
    desc->description = strdup(od.description.c_str());
    desc->unit = strdup(od.unit.c_str());
    desc->hasFixedValueCount = od.hasFixedValueCount;
    desc->valueCount = od.valueCount;

    desc->valueNames = (const char **)
        malloc(od.valueCount * sizeof(const char *));
        
    for (unsigned int i = 0; i < od.valueCount; ++i) {
        if (i < od.valueNames.size()) {
            desc->valueNames[i] = strdup(od.valueNames[i].c_str());
        } else {
            desc->valueNames[i] = 0;
        }
    }

    desc->hasKnownExtents = od.hasKnownExtents;
    desc->minValue = od.minValue;
    desc->maxValue = od.maxValue;
    desc->isQuantized = od.isQuantized;
    desc->quantizeStep = od.quantizeStep;

    switch (od.sampleType) {
    case Plugin::OutputDescriptor::OneSamplePerStep:
        desc->sampleType = vampOneSamplePerStep; break;
    case Plugin::OutputDescriptor::FixedSampleRate:
        desc->sampleType = vampFixedSampleRate; break;
    case Plugin::OutputDescriptor::VariableSampleRate:
        desc->sampleType = vampVariableSampleRate; break;
    }

    desc->sampleRate = od.sampleRate;

    return desc;
}
    
VampFeatureList **
PluginAdapterBase::process(Plugin *plugin,
                           float **inputBuffers,
                           int sec, int nsec)
{
    RealTime rt(sec, nsec);
    return convertFeatures(plugin->process(inputBuffers, rt));
}
    
VampFeatureList **
PluginAdapterBase::getRemainingFeatures(Plugin *plugin)
{
    return convertFeatures(plugin->getRemainingFeatures());
}

VampFeatureList **
PluginAdapterBase::convertFeatures(const Plugin::FeatureSet &features)
{
    unsigned int n = 0;
    if (features.begin() != features.end()) {
        Plugin::FeatureSet::const_iterator i = features.end();
        --i;
        n = i->first + 1;
    }

    if (!n) return 0;

    VampFeatureList **fs = (VampFeatureList **)
        malloc((n + 1) * sizeof(VampFeatureList *));

    for (unsigned int i = 0; i < n; ++i) {
        fs[i] = (VampFeatureList *)malloc(sizeof(VampFeatureList));
        if (features.find(i) == features.end()) {
            fs[i]->featureCount = 0;
            fs[i]->features = 0;
        } else {
            Plugin::FeatureSet::const_iterator fi =
                features.find(i);
            const Plugin::FeatureList &fl = fi->second;
            fs[i]->featureCount = fl.size();
            fs[i]->features = (VampFeature *)malloc(fl.size() *
                                                   sizeof(VampFeature));
            for (unsigned int j = 0; j < fl.size(); ++j) {
                fs[i]->features[j].hasTimestamp = fl[j].hasTimestamp;
                fs[i]->features[j].sec = fl[j].timestamp.sec;
                fs[i]->features[j].nsec = fl[j].timestamp.nsec;
                fs[i]->features[j].valueCount = fl[j].values.size();
                fs[i]->features[j].values = (float *)malloc
                    (fs[i]->features[j].valueCount * sizeof(float));
                for (unsigned int k = 0; k < fs[i]->features[j].valueCount; ++k) {
                    fs[i]->features[j].values[k] = fl[j].values[k];
                }
                fs[i]->features[j].label = strdup(fl[j].label.c_str());
            }
        }
    }

    fs[n] = 0;

    return fs;
}

PluginAdapterBase::AdapterMap 
PluginAdapterBase::m_adapterMap;

}

