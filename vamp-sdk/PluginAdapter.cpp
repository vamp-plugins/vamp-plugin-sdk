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

#include "PluginAdapter.h"

#define DEBUG_PLUGIN_ADAPTER 1


namespace Vamp {

PluginAdapterBase::PluginAdapterBase() :
    m_populated(false)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase[" << this << "]::PluginAdapterBase" << std::endl;
#endif
}

const VampPluginDescriptor *
PluginAdapterBase::getDescriptor()
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase[" << this << "]::getDescriptor" << std::endl;
#endif

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

    unsigned int i;
    
    for (i = 0; i < m_parameters.size(); ++i) {
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
        desc->valueNames = 0;
        if (desc->isQuantized && !m_parameters[i].valueNames.empty()) {
            desc->valueNames = (const char **)
                malloc((m_parameters[i].valueNames.size()+1) * sizeof(char *));
            for (unsigned int j = 0; j < m_parameters[i].valueNames.size(); ++j) {
                desc->valueNames[j] = strdup(m_parameters[i].valueNames[j].c_str());
            }
            desc->valueNames[m_parameters[i].valueNames.size()] = 0;
        }
        m_descriptor.parameters[i] = desc;
    }
    
    m_descriptor.programCount = m_programs.size();
    m_descriptor.programs = (const char **)
        malloc(m_programs.size() * sizeof(const char *));
    
    for (i = 0; i < m_programs.size(); ++i) {
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
    
    if (!m_adapterMap) {
        m_adapterMap = new AdapterMap;
    }
    (*m_adapterMap)[&m_descriptor] = this;

    delete plugin;

    m_populated = true;
    return &m_descriptor;
}

PluginAdapterBase::~PluginAdapterBase()
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase[" << this << "]::~PluginAdapterBase" << std::endl;
#endif

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
        if (desc->valueNames) {
            for (unsigned int j = 0; desc->valueNames[j]; ++j) {
                free((void *)desc->valueNames[j]);
            }
            free((void *)desc->valueNames);
        }
    }
    free((void *)m_descriptor.parameters);

    for (unsigned int i = 0; i < m_descriptor.programCount; ++i) {
        free((void *)m_descriptor.programs[i]);
    }
    free((void *)m_descriptor.programs);

    if (m_adapterMap) {
        
        m_adapterMap->erase(&m_descriptor);

        if (m_adapterMap->empty()) {
            delete m_adapterMap;
            m_adapterMap = 0;
        }
    }
}

PluginAdapterBase *
PluginAdapterBase::lookupAdapter(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::lookupAdapter(" << handle << ")" << std::endl;
#endif

    if (!m_adapterMap) return 0;
    AdapterMap::const_iterator i = m_adapterMap->find(handle);
    if (i == m_adapterMap->end()) return 0;
    return i->second;
}

VampPluginHandle
PluginAdapterBase::vampInstantiate(const VampPluginDescriptor *desc,
                                   float inputSampleRate)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampInstantiate(" << desc << ")" << std::endl;
#endif

    if (!m_adapterMap) {
        m_adapterMap = new AdapterMap();
    }

    if (m_adapterMap->find(desc) == m_adapterMap->end()) {
        std::cerr << "WARNING: PluginAdapterBase::vampInstantiate: Descriptor " << desc << " not in adapter map" << std::endl;
        return 0;
    }

    PluginAdapterBase *adapter = (*m_adapterMap)[desc];
    if (desc != &adapter->m_descriptor) return 0;

    Plugin *plugin = adapter->createPlugin(inputSampleRate);
    if (plugin) {
        (*m_adapterMap)[plugin] = adapter;
    }

#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampInstantiate(" << desc << "): returning handle " << plugin << std::endl;
#endif

    return plugin;
}

void
PluginAdapterBase::vampCleanup(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampCleanup(" << handle << ")" << std::endl;
#endif

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
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampInitialise(" << handle << ", " << channels << ", " << stepSize << ", " << blockSize << ")" << std::endl;
#endif

    bool result = ((Plugin *)handle)->initialise
        (channels, stepSize, blockSize);
    return result ? 1 : 0;
}

void
PluginAdapterBase::vampReset(VampPluginHandle handle) 
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampReset(" << handle << ")" << std::endl;
#endif

    ((Plugin *)handle)->reset();
}

float
PluginAdapterBase::vampGetParameter(VampPluginHandle handle,
                                    int param) 
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetParameter(" << handle << ", " << param << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0.0;
    Plugin::ParameterList &list = adapter->m_parameters;
    return ((Plugin *)handle)->getParameter(list[param].name);
}

void
PluginAdapterBase::vampSetParameter(VampPluginHandle handle,
                                    int param, float value)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampSetParameter(" << handle << ", " << param << ", " << value << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return;
    Plugin::ParameterList &list = adapter->m_parameters;
    ((Plugin *)handle)->setParameter(list[param].name, value);
}

unsigned int
PluginAdapterBase::vampGetCurrentProgram(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetCurrentProgram(" << handle << ")" << std::endl;
#endif

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
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampSelectProgram(" << handle << ", " << program << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return;
    Plugin::ProgramList &list = adapter->m_programs;
    ((Plugin *)handle)->selectProgram(list[program]);
}

unsigned int
PluginAdapterBase::vampGetPreferredStepSize(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetPreferredStepSize(" << handle << ")" << std::endl;
#endif

    return ((Plugin *)handle)->getPreferredStepSize();
}

unsigned int
PluginAdapterBase::vampGetPreferredBlockSize(VampPluginHandle handle) 
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetPreferredBlockSize(" << handle << ")" << std::endl;
#endif

    return ((Plugin *)handle)->getPreferredBlockSize();
}

unsigned int
PluginAdapterBase::vampGetMinChannelCount(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetMinChannelCount(" << handle << ")" << std::endl;
#endif

    return ((Plugin *)handle)->getMinChannelCount();
}

unsigned int
PluginAdapterBase::vampGetMaxChannelCount(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetMaxChannelCount(" << handle << ")" << std::endl;
#endif

    return ((Plugin *)handle)->getMaxChannelCount();
}

unsigned int
PluginAdapterBase::vampGetOutputCount(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetOutputCount(" << handle << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);

//    std::cerr << "vampGetOutputCount: handle " << handle << " -> adapter "<< adapter << std::endl;

    if (!adapter) return 0;
    return adapter->getOutputCount((Plugin *)handle);
}

VampOutputDescriptor *
PluginAdapterBase::vampGetOutputDescriptor(VampPluginHandle handle,
                                           unsigned int i)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetOutputDescriptor(" << handle << ", " << i << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);

//    std::cerr << "vampGetOutputDescriptor: handle " << handle << " -> adapter "<< adapter << std::endl;

    if (!adapter) return 0;
    return adapter->getOutputDescriptor((Plugin *)handle, i);
}

void
PluginAdapterBase::vampReleaseOutputDescriptor(VampOutputDescriptor *desc)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampReleaseOutputDescriptor(" << desc << ")" << std::endl;
#endif

    if (desc->name) free((void *)desc->name);
    if (desc->description) free((void *)desc->description);
    if (desc->unit) free((void *)desc->unit);
    for (unsigned int i = 0; i < desc->binCount; ++i) {
        free((void *)desc->binNames[i]);
    }
    if (desc->binNames) free((void *)desc->binNames);
    free((void *)desc);
}

VampFeatureList *
PluginAdapterBase::vampProcess(VampPluginHandle handle,
                               float **inputBuffers,
                               int sec,
                               int nsec)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampProcess(" << handle << ", " << sec << ", " << nsec << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->process((Plugin *)handle,
                            inputBuffers, sec, nsec);
}

VampFeatureList *
PluginAdapterBase::vampGetRemainingFeatures(VampPluginHandle handle)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampGetRemainingFeatures(" << handle << ")" << std::endl;
#endif

    PluginAdapterBase *adapter = lookupAdapter(handle);
    if (!adapter) return 0;
    return adapter->getRemainingFeatures((Plugin *)handle);
}

void
PluginAdapterBase::vampReleaseFeatureSet(VampFeatureList *fs)
{
#ifdef DEBUG_PLUGIN_ADAPTER
    std::cerr << "PluginAdapterBase::vampReleaseFeatureSet" << std::endl;
#endif
}

void 
PluginAdapterBase::cleanup(Plugin *plugin)
{
    if (m_fs.find(plugin) != m_fs.end()) {
        size_t outputCount = 0;
        if (m_pluginOutputs[plugin]) {
            outputCount = m_pluginOutputs[plugin]->size();
        }
        VampFeatureList *list = m_fs[plugin];
        for (unsigned int i = 0; i < outputCount; ++i) {
            for (unsigned int j = 0; j < m_fsizes[plugin][i]; ++j) {
                if (list[i].features[j].label) {
                    free(list[i].features[j].label);
                }
                if (list[i].features[j].values) {
                    free(list[i].features[j].values);
                }
            }
            if (list[i].features) free(list[i].features);
        }
        m_fs.erase(plugin);
        m_fsizes.erase(plugin);
        m_fvsizes.erase(plugin);
    }

    if (m_pluginOutputs.find(plugin) != m_pluginOutputs.end()) {
        delete m_pluginOutputs[plugin];
        m_pluginOutputs.erase(plugin);
    }

    if (m_adapterMap) {
        m_adapterMap->erase(plugin);

        if (m_adapterMap->empty()) {
            delete m_adapterMap;
            m_adapterMap = 0;
        }
    }

    delete ((Plugin *)plugin);
}

void 
PluginAdapterBase::checkOutputMap(Plugin *plugin)
{
    if (m_pluginOutputs.find(plugin) == m_pluginOutputs.end() ||
        !m_pluginOutputs[plugin]) {
        m_pluginOutputs[plugin] = new Plugin::OutputList
            (plugin->getOutputDescriptors());
//        std::cerr << "PluginAdapterBase::checkOutputMap: Have " << m_pluginOutputs[plugin]->size() << " outputs for plugin label " << plugin->getName() << std::endl;
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
    desc->hasFixedBinCount = od.hasFixedBinCount;
    desc->binCount = od.binCount;

    if (od.hasFixedBinCount && od.binCount > 0) {
        desc->binNames = (const char **)
            malloc(od.binCount * sizeof(const char *));
        
        for (unsigned int i = 0; i < od.binCount; ++i) {
            if (i < od.binNames.size()) {
                desc->binNames[i] = strdup(od.binNames[i].c_str());
            } else {
                desc->binNames[i] = 0;
            }
        }
    } else {
        desc->binNames = 0;
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
    
VampFeatureList *
PluginAdapterBase::process(Plugin *plugin,
                           float **inputBuffers,
                           int sec, int nsec)
{
//    std::cerr << "PluginAdapterBase::process" << std::endl;
    RealTime rt(sec, nsec);
    checkOutputMap(plugin);
    return convertFeatures(plugin, plugin->process(inputBuffers, rt));
}
    
VampFeatureList *
PluginAdapterBase::getRemainingFeatures(Plugin *plugin)
{
//    std::cerr << "PluginAdapterBase::getRemainingFeatures" << std::endl;
    checkOutputMap(plugin);
    return convertFeatures(plugin, plugin->getRemainingFeatures());
}

VampFeatureList *
PluginAdapterBase::convertFeatures(Plugin *plugin,
                                   const Plugin::FeatureSet &features)
{
    int lastN = -1;

    int outputCount = 0;
    if (m_pluginOutputs[plugin]) outputCount = m_pluginOutputs[plugin]->size();
    
    resizeFS(plugin, outputCount);
    VampFeatureList *fs = m_fs[plugin];

    for (Plugin::FeatureSet::const_iterator fi = features.begin();
         fi != features.end(); ++fi) {

        int n = fi->first;
        
//        std::cerr << "PluginAdapterBase::convertFeatures: n = " << n << std::endl;

        if (n >= int(outputCount)) {
            std::cerr << "WARNING: PluginAdapterBase::convertFeatures: Too many outputs from plugin (" << n+1 << ", only should be " << outputCount << ")" << std::endl;
            continue;
        }

        if (n > lastN + 1) {
            for (int i = lastN + 1; i < n; ++i) {
                fs[i].featureCount = 0;
            }
        }

        const Plugin::FeatureList &fl = fi->second;

        size_t sz = fl.size();
        if (sz > m_fsizes[plugin][n]) resizeFL(plugin, n, sz);
        fs[n].featureCount = sz;
        
        for (size_t j = 0; j < sz; ++j) {

//            std::cerr << "PluginAdapterBase::convertFeatures: j = " << j << std::endl;

            VampFeature *feature = &fs[n].features[j];

            feature->hasTimestamp = fl[j].hasTimestamp;
            feature->sec = fl[j].timestamp.sec;
            feature->nsec = fl[j].timestamp.nsec;
            feature->valueCount = fl[j].values.size();

            if (feature->label) free(feature->label);

            if (fl[j].label.empty()) {
                feature->label = 0;
            } else {
                feature->label = strdup(fl[j].label.c_str());
            }

            if (feature->valueCount > m_fvsizes[plugin][n][j]) {
                resizeFV(plugin, n, j, feature->valueCount);
            }

            for (unsigned int k = 0; k < feature->valueCount; ++k) {
//                std::cerr << "PluginAdapterBase::convertFeatures: k = " << k << std::endl;
                feature->values[k] = fl[j].values[k];
            }
        }

        lastN = n;
    }

    if (lastN == -1) return 0;

    if (int(outputCount) > lastN + 1) {
        for (int i = lastN + 1; i < int(outputCount); ++i) {
            fs[i].featureCount = 0;
        }
    }

    return fs;
}

void
PluginAdapterBase::resizeFS(Plugin *plugin, int n)
{
//    std::cerr << "PluginAdapterBase::resizeFS(" << plugin << ", " << n << ")" << std::endl;

    int i = m_fsizes[plugin].size();
    if (i >= n) return;

//    std::cerr << "resizing from " << i << std::endl;

    m_fs[plugin] = (VampFeatureList *)realloc
        (m_fs[plugin], n * sizeof(VampFeatureList));

    while (i < n) {
        m_fs[plugin][i].featureCount = 0;
        m_fs[plugin][i].features = 0;
        m_fsizes[plugin].push_back(0);
        m_fvsizes[plugin].push_back(std::vector<size_t>());
        i++;
    }
}

void
PluginAdapterBase::resizeFL(Plugin *plugin, int n, size_t sz)
{
//    std::cerr << "PluginAdapterBase::resizeFL(" << plugin << ", " << n << ", "
//              << sz << ")" << std::endl;

    size_t i = m_fsizes[plugin][n];
    if (i >= sz) return;

//    std::cerr << "resizing from " << i << std::endl;

    m_fs[plugin][n].features = (VampFeature *)realloc
        (m_fs[plugin][n].features, sz * sizeof(VampFeature));

    while (m_fsizes[plugin][n] < sz) {
        m_fs[plugin][n].features[m_fsizes[plugin][n]].valueCount = 0;
        m_fs[plugin][n].features[m_fsizes[plugin][n]].values = 0;
        m_fs[plugin][n].features[m_fsizes[plugin][n]].label = 0;
        m_fvsizes[plugin][n].push_back(0);
        m_fsizes[plugin][n]++;
    }
}

void
PluginAdapterBase::resizeFV(Plugin *plugin, int n, int j, size_t sz)
{
//    std::cerr << "PluginAdapterBase::resizeFV(" << plugin << ", " << n << ", "
//              << j << ", " << sz << ")" << std::endl;

    size_t i = m_fvsizes[plugin][n][j];
    if (i >= sz) return;

//    std::cerr << "resizing from " << i << std::endl;

    m_fs[plugin][n].features[j].values = (float *)realloc
        (m_fs[plugin][n].features[j].values, sz * sizeof(float));

    m_fvsizes[plugin][n][j] = sz;
}
  
PluginAdapterBase::AdapterMap *
PluginAdapterBase::m_adapterMap = 0;

}

