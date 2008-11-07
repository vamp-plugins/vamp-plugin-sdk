/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006 Chris Cannam.
    FFT code from Don Cross's public domain FFT implementation.
  
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

#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>

#include <iostream>
#include <fstream>
#include <set>
#include <sndfile.h>

#include <cstring>
#include <cstdlib>

#include "system.h"

#include <cmath>

using namespace std;

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

#define HOST_VERSION "1.3"

enum Verbosity {
    PluginIds,
    PluginOutputIds,
    PluginInformation
};

void printFeatures(int, int, int, Plugin::FeatureSet, ofstream *, bool frames);
void transformInput(float *, size_t);
void fft(unsigned int, bool, double *, double *, double *, double *);
void printPluginPath(bool verbose);
void printPluginCategoryList();
void enumeratePlugins(Verbosity);
void listPluginsInLibrary(string soname);
int runPlugin(string myname, string soname, string id, string output,
              int outputNo, string inputFile, string outfilename, bool frames);

void usage(const char *name)
{
    cerr << "\n"
         << name << ": A simple Vamp plugin host.\n\n"
        "Centre for Digital Music, Queen Mary, University of London.\n"
        "Copyright 2006-2007 Chris Cannam and QMUL.\n"
        "Freely redistributable; published under a BSD-style license.\n\n"
        "Usage:\n\n"
        "  " << name << " [-s] pluginlibrary[." << PLUGIN_SUFFIX << "]:plugin[:output] file.wav [-o out.txt]\n"
        "  " << name << " [-s] pluginlibrary[." << PLUGIN_SUFFIX << "]:plugin file.wav [outputno] [-o out.txt]\n\n"
        "    -- Load plugin id \"plugin\" from \"pluginlibrary\" and run it on the\n"
        "       audio data in \"file.wav\", retrieving the named \"output\", or output\n"
        "       number \"outputno\" (the first output by default) and dumping it to\n"
        "       standard output, or to \"out.txt\" if the -o option is given.\n\n"
        "       \"pluginlibrary\" should be a library name, not a file path; the\n"
        "       standard Vamp library search path will be used to locate it.  If\n"
        "       a file path is supplied, the directory part(s) will be ignored.\n\n"
        "       If the -s option is given, results will be labelled with the audio\n"
        "       sample frame at which they occur. Otherwise, they will be labelled\n"
        "       with time in seconds.\n\n"
        "  " << name << " -l\n\n"
        "    -- List the plugin libraries and Vamp plugins in the library search path\n"
        "       in a verbose human-readable format.\n\n"
        "  " << name << " --list-ids\n\n"
        "    -- List the plugins in the search path in a terse machine-readable format,\n"
        "       in the form vamp:soname:identifier.\n\n"
        "  " << name << " --list-outputs\n\n"
        "    -- List the outputs for plugins in the search path in a machine-readable\n"
        "       format, in the form vamp:soname:identifier:output.\n\n"
        "  " << name << " --list-by-category\n\n"
        "    -- List the plugins as a plugin index by category, in a machine-readable\n"
        "       format.  The format may change in future releases.\n\n"
        "  " << name << " -p\n\n"
        "    -- Print out the Vamp library search path.\n\n"
        "  " << name << " -v\n\n"
        "    -- Display version information only.\n"
         << endl;
    exit(2);
}

int main(int argc, char **argv)
{
    char *scooter = argv[0];
    char *name = 0;
    while (scooter && *scooter) {
        if (*scooter == '/' || *scooter == '\\') name = ++scooter;
        else ++scooter;
    }
    if (!name || !*name) name = argv[0];
    
    if (argc < 2) usage(name);

    if (argc == 2) {

        if (!strcmp(argv[1], "-v")) {

            cout << "Simple Vamp plugin host version: " << HOST_VERSION << endl
                 << "Vamp API version: " << VAMP_API_VERSION << endl
                 << "Vamp SDK version: " << VAMP_SDK_VERSION << endl;
            return 0;

        } else if (!strcmp(argv[1], "-l")) {

            printPluginPath(true);
            enumeratePlugins(PluginInformation);
            return 0;

        } else if (!strcmp(argv[1], "-p")) {

            printPluginPath(false);
            return 0;

        } else if (!strcmp(argv[1], "--list-ids")) {

            enumeratePlugins(PluginIds);
            return 0;

        } else if (!strcmp(argv[1], "--list-outputs")) {

            enumeratePlugins(PluginOutputIds);
            return 0;

        } else if (!strcmp(argv[1], "--list-by-category")) {

            printPluginCategoryList();
            return 0;

        } else usage(name);
    }

    if (argc < 3) usage(name);

    bool useFrames = false;
    
    int base = 1;
    if (!strcmp(argv[1], "-s")) {
        useFrames = true;
        base = 2;
    }

    string soname = argv[base];
    string wavname = argv[base+1];
    string plugid = "";
    string output = "";
    int outputNo = -1;
    string outfilename;

    if (argc >= base+3) {

        int idx = base+2;

        if (isdigit(*argv[idx])) {
            outputNo = atoi(argv[idx++]);
        }

        if (argc == idx + 2) {
            if (!strcmp(argv[idx], "-o")) {
                outfilename = argv[idx+1];
            } else usage(name);
        } else if (argc != idx) {
            (usage(name));
        }
    }

    cerr << endl << name << ": Running..." << endl;

    cerr << "Reading file: \"" << wavname << "\", writing to ";
    if (outfilename == "") {
        cerr << "standard output" << endl;
    } else {
        cerr << "\"" << outfilename << "\"" << endl;
    }

    string::size_type sep = soname.find(':');

    if (sep != string::npos) {
        plugid = soname.substr(sep + 1);
        soname = soname.substr(0, sep);

        sep = plugid.find(':');
        if (sep != string::npos) {
            output = plugid.substr(sep + 1);
            plugid = plugid.substr(0, sep);
        }
    }

    if (plugid == "") {
        usage(name);
    }

    if (output != "" && outputNo != -1) {
        usage(name);
    }

    if (output == "" && outputNo == -1) {
        outputNo = 0;
    }

    return runPlugin(name, soname, plugid, output, outputNo,
                     wavname, outfilename, useFrames);
}


int runPlugin(string myname, string soname, string id,
              string output, int outputNo, string wavname,
              string outfilename, bool useFrames)
{
    PluginLoader *loader = PluginLoader::getInstance();

    PluginLoader::PluginKey key = loader->composePluginKey(soname, id);
    
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));

    sndfile = sf_open(wavname.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
	cerr << myname << ": ERROR: Failed to open input file \""
             << wavname << "\": " << sf_strerror(sndfile) << endl;
	return 1;
    }

    ofstream *out = 0;
    if (outfilename != "") {
        out = new ofstream(outfilename.c_str(), ios::out);
        if (!*out) {
            cerr << myname << ": ERROR: Failed to open output file \""
                 << outfilename << "\" for writing" << endl;
            delete out;
            return 1;
        }
    }

    Plugin *plugin = loader->loadPlugin
        (key, sfinfo.samplerate, PluginLoader::ADAPT_ALL_SAFE);
    if (!plugin) {
        cerr << myname << ": ERROR: Failed to load plugin \"" << id
             << "\" from library \"" << soname << "\"" << endl;
        sf_close(sndfile);
        if (out) {
            out->close();
            delete out;
        }
        return 1;
    }

    cerr << "Running plugin: \"" << plugin->getIdentifier() << "\"..." << endl;

    int blockSize = plugin->getPreferredBlockSize();
    int stepSize = plugin->getPreferredStepSize();

    if (blockSize == 0) {
        blockSize = 1024;
    }
    if (stepSize == 0) {
        if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
            stepSize = blockSize/2;
        } else {
            stepSize = blockSize;
        }
    } else if (stepSize > blockSize) {
        cerr << "WARNING: stepSize " << stepSize << " > blockSize " << blockSize << ", resetting blockSize to ";
        if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
            blockSize = stepSize * 2;
        } else {
            blockSize = stepSize;
        }
        cerr << blockSize << endl;
    }

    int channels = sfinfo.channels;

    float *filebuf = new float[blockSize * channels];
    float **plugbuf = new float*[channels];
    for (int c = 0; c < channels; ++c) plugbuf[c] = new float[blockSize + 2];

    cerr << "Using block size = " << blockSize << ", step size = "
              << stepSize << endl;

    int minch = plugin->getMinChannelCount();
    int maxch = plugin->getMaxChannelCount();
    cerr << "Plugin accepts " << minch << " -> " << maxch << " channel(s)" << endl;
    cerr << "Sound file has " << channels << " (will mix/augment if necessary)" << endl;

    Plugin::OutputList outputs = plugin->getOutputDescriptors();
    Plugin::OutputDescriptor od;

    int returnValue = 1;
    int progress = 0;

    RealTime rt;
    PluginWrapper *wrapper = 0;
    RealTime adjustment = RealTime::zeroTime;

    if (outputs.empty()) {
	cerr << "ERROR: Plugin has no outputs!" << endl;
        goto done;
    }

    if (outputNo < 0) {

        for (size_t oi = 0; oi < outputs.size(); ++oi) {
            if (outputs[oi].identifier == output) {
                outputNo = oi;
                break;
            }
        }

        if (outputNo < 0) {
            cerr << "ERROR: Non-existent output \"" << output << "\" requested" << endl;
            goto done;
        }

    } else {

        if (int(outputs.size()) <= outputNo) {
            cerr << "ERROR: Output " << outputNo << " requested, but plugin has only " << outputs.size() << " output(s)" << endl;
            goto done;
        }        
    }

    od = outputs[outputNo];
    cerr << "Output is: \"" << od.identifier << "\"" << endl;

    if (!plugin->initialise(channels, stepSize, blockSize)) {
        cerr << "ERROR: Plugin initialise (channels = " << channels
             << ", stepSize = " << stepSize << ", blockSize = "
             << blockSize << ") failed." << endl;
        goto done;
    }

    wrapper = dynamic_cast<PluginWrapper *>(plugin);
    if (wrapper) {
        PluginInputDomainAdapter *ida =
            wrapper->getWrapper<PluginInputDomainAdapter>();
        if (ida) adjustment = ida->getTimestampAdjustment();
    }

    for (size_t i = 0; i < sfinfo.frames; i += stepSize) {

        int count;

        if (sf_seek(sndfile, i, SEEK_SET) < 0) {
            cerr << "ERROR: sf_seek failed: " << sf_strerror(sndfile) << endl;
            break;
        }
        
        if ((count = sf_readf_float(sndfile, filebuf, blockSize)) < 0) {
            cerr << "ERROR: sf_readf_float failed: " << sf_strerror(sndfile) << endl;
            break;
        }

        for (int c = 0; c < channels; ++c) {
            int j = 0;
            while (j < count) {
                plugbuf[c][j] = filebuf[j * sfinfo.channels + c];
                ++j;
            }
            while (j < blockSize) {
                plugbuf[c][j] = 0.0f;
                ++j;
            }
        }

        rt = RealTime::frame2RealTime(i, sfinfo.samplerate);

        printFeatures
            (RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
             sfinfo.samplerate, outputNo, plugin->process(plugbuf, rt),
             out, useFrames);

        int pp = progress;
        progress = lrintf((float(i) / sfinfo.frames) * 100.f);
        if (progress != pp && out) {
            cerr << "\r" << progress << "%";
        }
    }
    if (out) cerr << "\rDone" << endl;

    rt = RealTime::frame2RealTime(sfinfo.frames, sfinfo.samplerate);

    printFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                  sfinfo.samplerate, outputNo,
                  plugin->getRemainingFeatures(), out, useFrames);

    returnValue = 0;

done:
    delete plugin;
    if (out) {
        out->close();
        delete out;
    }
    sf_close(sndfile);
    return returnValue;
}

void
printFeatures(int frame, int sr, int output,
              Plugin::FeatureSet features, ofstream *out, bool useFrames)
{
    for (unsigned int i = 0; i < features[output].size(); ++i) {

        if (useFrames) {

            int displayFrame = frame;

            if (features[output][i].hasTimestamp) {
                displayFrame = RealTime::realTime2Frame
                    (features[output][i].timestamp, sr);
            }

            (out ? *out : cout) << displayFrame;

            if (features[output][i].hasDuration) {
                displayFrame = RealTime::realTime2Frame
                    (features[output][i].duration, sr);
                (out ? *out : cout) << "," << displayFrame;
            }

            (out ? *out : cout)  << ":";

        } else {

            RealTime rt = RealTime::frame2RealTime(frame, sr);

            if (features[output][i].hasTimestamp) {
                rt = features[output][i].timestamp;
            }

            (out ? *out : cout) << rt.toString();

            if (features[output][i].hasDuration) {
                rt = features[output][i].duration;
                (out ? *out : cout) << "," << rt.toString();
            }

            (out ? *out : cout) << ":";
        }

        for (unsigned int j = 0; j < features[output][i].values.size(); ++j) {
            (out ? *out : cout) << " " << features[output][i].values[j];
        }

        (out ? *out : cout) << endl;
    }
}

void
printPluginPath(bool verbose)
{
    if (verbose) {
        cout << "\nVamp plugin search path: ";
    }

    vector<string> path = PluginHostAdapter::getPluginPath();
    for (size_t i = 0; i < path.size(); ++i) {
        if (verbose) {
            cout << "[" << path[i] << "]";
        } else {
            cout << path[i] << endl;
        }
    }

    if (verbose) cout << endl;
}

void
enumeratePlugins(Verbosity verbosity)
{
    PluginLoader *loader = PluginLoader::getInstance();

    if (verbosity == PluginInformation) {
        cout << "\nVamp plugin libraries found in search path:" << endl;
    }

    vector<PluginLoader::PluginKey> plugins = loader->listPlugins();
    typedef multimap<string, PluginLoader::PluginKey>
        LibraryMap;
    LibraryMap libraryMap;

    for (size_t i = 0; i < plugins.size(); ++i) {
        string path = loader->getLibraryPathForPlugin(plugins[i]);
        libraryMap.insert(LibraryMap::value_type(path, plugins[i]));
    }

    string prevPath = "";
    int index = 0;

    for (LibraryMap::iterator i = libraryMap.begin();
         i != libraryMap.end(); ++i) {
        
        string path = i->first;
        PluginLoader::PluginKey key = i->second;

        if (path != prevPath) {
            prevPath = path;
            index = 0;
            if (verbosity == PluginInformation) {
                cout << "\n  " << path << ":" << endl;
            }
        }

        Plugin *plugin = loader->loadPlugin(key, 48000);
        if (plugin) {

            char c = char('A' + index);
            if (c > 'Z') c = char('a' + (index - 26));

            if (verbosity == PluginInformation) {

                cout << "    [" << c << "] [v"
                     << plugin->getVampApiVersion() << "] "
                     << plugin->getName() << ", \""
                     << plugin->getIdentifier() << "\"" << " ["
                     << plugin->getMaker() << "]" << endl;

                PluginLoader::PluginCategoryHierarchy category =
                    loader->getPluginCategory(key);

                if (!category.empty()) {
                    cout << "       ";
                    for (size_t ci = 0; ci < category.size(); ++ci) {
                        cout << " > " << category[ci];
                    }
                    cout << endl;
                }

                if (plugin->getDescription() != "") {
                    cout << "        - " << plugin->getDescription() << endl;
                }

            } else if (verbosity == PluginIds) {
                cout << "vamp:" << key << endl;
            }
            
            Plugin::OutputList outputs =
                plugin->getOutputDescriptors();

            if (outputs.size() > 1 || verbosity == PluginOutputIds) {
                for (size_t j = 0; j < outputs.size(); ++j) {
                    if (verbosity == PluginInformation) {
                        cout << "         (" << j << ") "
                             << outputs[j].name << ", \""
                             << outputs[j].identifier << "\"" << endl;
                        if (outputs[j].description != "") {
                            cout << "             - " 
                                 << outputs[j].description << endl;
                        }
                    } else if (verbosity == PluginOutputIds) {
                        cout << "vamp:" << key << ":" << outputs[j].identifier << endl;
                    }
                }
            }

            ++index;

            delete plugin;
        }
    }

    if (verbosity == PluginInformation) {
        cout << endl;
    }
}

void
printPluginCategoryList()
{
    PluginLoader *loader = PluginLoader::getInstance();

    vector<PluginLoader::PluginKey> plugins = loader->listPlugins();

    set<string> printedcats;

    for (size_t i = 0; i < plugins.size(); ++i) {

        PluginLoader::PluginKey key = plugins[i];
        
        PluginLoader::PluginCategoryHierarchy category =
            loader->getPluginCategory(key);

        Plugin *plugin = loader->loadPlugin(key, 48000);
        if (!plugin) continue;

        string catstr = "";

        if (category.empty()) catstr = '|';
        else {
            for (size_t j = 0; j < category.size(); ++j) {
                catstr += category[j];
                catstr += '|';
                if (printedcats.find(catstr) == printedcats.end()) {
                    std::cout << catstr << std::endl;
                    printedcats.insert(catstr);
                }
            }
        }

        std::cout << catstr << key << ":::" << plugin->getName() << ":::" << plugin->getMaker() << ":::" << plugin->getDescription() << std::endl;
    }
}

