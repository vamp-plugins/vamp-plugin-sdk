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

#include "vamp-sdk/PluginHostAdapter.h"
#include "vamp-sdk/hostext/PluginChannelAdapter.h"
#include "vamp-sdk/hostext/PluginInputDomainAdapter.h"
#include "vamp-sdk/hostext/PluginLoader.h"
#include "vamp/vamp.h"

#include <iostream>
#include <sndfile.h>

#include "system.h"

#include <cmath>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

using Vamp::HostExt::PluginLoader;

#define HOST_VERSION "1.1"

void printFeatures(int, int, int, Vamp::Plugin::FeatureSet);
void transformInput(float *, size_t);
void fft(unsigned int, bool, double *, double *, double *, double *);
void printPluginPath(bool verbose);
void enumeratePlugins();
void listPluginsInLibrary(string soname);
int runPlugin(string myname, string soname, string id, string output,
              int outputNo, string inputFile);

void usage(const char *name)
{
    cerr << "\n"
         << name << ": A simple Vamp plugin host.\n\n"
        "Centre for Digital Music, Queen Mary, University of London.\n"
        "Copyright 2006-2007 Chris Cannam and QMUL.\n"
        "Freely redistributable; published under a BSD-style license.\n\n"
        "Usage:\n\n"
        "  " << name << " pluginlibrary[." << PLUGIN_SUFFIX << "]:plugin[:output] file.wav\n"
        "  " << name << " pluginlibrary[." << PLUGIN_SUFFIX << "]:plugin file.wav [outputno]\n\n"
        "    -- Load plugin id \"plugin\" from \"pluginlibrary\" and run it on the\n"
        "       audio data in \"file.wav\", retrieving the named \"output\", or output\n"
        "       number \"outputno\" (the first output by default) and dumping it to\n"
        "       standard output.\n\n"
        "       \"pluginlibrary\" should be a library name, not a file path; the\n"
        "       standard Vamp library search path will be used to locate it.  If\n"
        "       a file path is supplied, the directory part(s) will be ignored.\n\n"
        "  " << name << " -l\n\n"
        "    -- List the plugin libraries and Vamp plugins in the library search path.\n\n"
        "  " << name << " -p\n\n"
        "    -- Print out the Vamp library search path.\n\n"
        "  " << name << " -v\n\n"
        "    -- Display version information only.\n\n"
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
    
    if (argc < 2 || argc > 4 ||
        (argc == 2 &&
         (!strcmp(argv[1], "-?") ||
          !strcmp(argv[1], "-h") ||
          !strcmp(argv[1], "--help")))) {

        usage(name); // does not return
    }

    if (argc == 2 && !strcmp(argv[1], "-v")) {
	cout << "Simple Vamp plugin host version: " << HOST_VERSION << endl
	     << "Vamp API version: " << VAMP_API_VERSION << endl
	     << "Vamp SDK version: " << VAMP_SDK_VERSION << endl;
	return 0;
    }
    
    if (argc == 2 && !strcmp(argv[1], "-l")) {
        printPluginPath(true);
        enumeratePlugins();
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "-p")) {
        printPluginPath(false);
        return 0;
    }

    cerr << endl << name << ": Running..." << endl;

    string soname = argv[1];
    string plugid = "";
    string output = "";
    int outputNo = -1;
    string wavname;
    if (argc >= 3) wavname = argv[2];

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

    if (argc == 4) outputNo = atoi(argv[3]);

    if (output != "" && outputNo != -1) {
        usage(name);
    }

    return runPlugin(name, soname, plugid, output, outputNo, wavname);
}


int runPlugin(string myname, string soname, string id,
              string output, int outputNo, string wavname)
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

    Vamp::Plugin *plugin = loader->loadPlugin
        (key, sfinfo.samplerate, PluginLoader::ADAPT_ALL);
    if (!plugin) {
        cerr << myname << ": ERROR: Failed to load plugin \"" << id
             << "\" from library \"" << soname << "\"" << endl;
        sf_close(sndfile);
        return 1;
    }

    cerr << "Running plugin: \"" << plugin->getIdentifier() << "\"..." << endl;

    int blockSize = plugin->getPreferredBlockSize();
    int stepSize = plugin->getPreferredStepSize();

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

    Vamp::Plugin::OutputList outputs = plugin->getOutputDescriptors();
    Vamp::Plugin::OutputDescriptor od;

    int returnValue = 1;

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

        printFeatures
            (i, sfinfo.samplerate, outputNo, plugin->process
             (plugbuf, Vamp::RealTime::frame2RealTime(i, sfinfo.samplerate)));
    }

    printFeatures(sfinfo.frames, sfinfo.samplerate, outputNo,
                  plugin->getRemainingFeatures());

    returnValue = 0;

done:
    delete plugin;
    sf_close(sndfile);
    return returnValue;
}

void
printPluginPath(bool verbose)
{
    if (verbose) {
        cout << "\nVamp plugin search path: ";
    }

    vector<string> path = Vamp::PluginHostAdapter::getPluginPath();
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
enumeratePlugins()
{
    PluginLoader *loader = PluginLoader::getInstance();

    cout << "\nVamp plugin libraries found in search path:" << endl;

    std::vector<PluginLoader::PluginKey> plugins = loader->listPlugins();
    typedef std::multimap<std::string, PluginLoader::PluginKey>
        LibraryMap;
    LibraryMap libraryMap;

    for (size_t i = 0; i < plugins.size(); ++i) {
        std::string path = loader->getLibraryPathForPlugin(plugins[i]);
        libraryMap.insert(LibraryMap::value_type(path, plugins[i]));
    }

    std::string prevPath = "";
    int index = 0;

    for (LibraryMap::iterator i = libraryMap.begin();
         i != libraryMap.end(); ++i) {
        
        std::string path = i->first;
        PluginLoader::PluginKey key = i->second;

        if (path != prevPath) {
            prevPath = path;
            index = 0;
            cout << "\n  " << path << ":" << endl;
        }

        Vamp::Plugin *plugin = loader->loadPlugin(key, 48000);
        if (plugin) {

            char c = char('A' + index);
            if (c > 'Z') c = char('a' + (index - 26));

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

            Vamp::Plugin::OutputList outputs =
                plugin->getOutputDescriptors();

            if (outputs.size() > 1) {
                for (size_t j = 0; j < outputs.size(); ++j) {
                    cout << "         (" << j << ") "
                         << outputs[j].name << ", \""
                         << outputs[j].identifier << "\"" << endl;
                    if (outputs[j].description != "") {
                        cout << "             - " 
                             << outputs[j].description << endl;
                    }
                }
            }

            ++index;

            delete plugin;
        }
    }

    cout << endl;
}

void
printFeatures(int frame, int sr, int output, Vamp::Plugin::FeatureSet features)
{
    for (unsigned int i = 0; i < features[output].size(); ++i) {
        Vamp::RealTime rt = Vamp::RealTime::frame2RealTime(frame, sr);
        if (features[output][i].hasTimestamp) {
            rt = features[output][i].timestamp;
        }
        cout << rt.toString() << ":";
        for (unsigned int j = 0; j < features[output][i].values.size(); ++j) {
            cout << " " << features[output][i].values[j];
        }
        cout << endl;
    }
}


        
