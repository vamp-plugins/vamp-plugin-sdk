
Vamp
====

### An API for audio analysis and feature extraction plugins.

   https://www.vamp-plugins.org/

Vamp is an API for C and C++ plugins that process sampled audio data
to produce descriptive output (measurements or semantic observations).

This is version 2.10 of the Vamp plugin Software Development Kit.

 * Plugins and hosts built with this version of the SDK are binary
   compatible with those built using any other 2.x version of the SDK.
 * Plugins and hosts built with this version of the SDK are binary
   compatible with those built using version 1.0 of the SDK, with
   certain restrictions.  See the [compatibility
   README](README_on_compatibility.md) for more details.
 * See the file [CHANGELOG](CHANGELOG) for a list of the changes in
   this release.

A documentation guide to writing plugins using the Vamp SDK can be
found at https://www.vamp-plugins.org/guide.pdf .


Compiling and Installing the SDK and Examples
---------------------------------------------

More than one build system is currently supported:

### CMake

The SDK can be compiled on most platforms using CMake, and this is
what we suggest when building new plugins or vendoring the SDK into
host code.

For example:

```
$ mkdir build && cd build
$ cmake .. -DVAMPSDK_BUILD_EXAMPLE_PLUGINS=ON
$ cmake --build .
```

The following custom CMake defines are supported:

<table>
<tr><td><code>VAMPSDK_BUILD_EXAMPLE_PLUGINS</code></td><td>Build the example library of Vamp plugins.</td></tr>
<tr><td><code>VAMPSDK_BUILD_SIMPLE_HOST</code></td><td>Build the simple host executable. This requires that <a href="https://github.com/libsndfile/libsndfile">libsndfile</a> be installed in a way that CMake can detect.</td></tr>
<tr><td><code>VAMPSDK_BUILD_RDFGEN</code></td><td>Build the RDF template generator utility, which can help produce RDF description files for plugins.</td></tr>
</table>

By default all of these options are `OFF` and only the plugin and host
SDK libraries are built.

### Autoconf

An older autoconf-based build system is also provided. This is still
the recommended system for building installable shared libraries on
Linux, but is not really advised elsewhere.

```
$ ./configure && make && make install
```

### Other build systems

Miscellaneous older build files (including Visual Studio projects) can
be found in the `otherbuilds` directory. These are provided in case
they are still useful to anyone, but nothing in here should be
considered supported.

### Continuous integration

Builds are tested via CI on Linux, macOS, and Windows.

 * Linux CI build: [![Build Status](https://github.com/vamp-plugins/vamp-plugin-sdk/workflows/Linux%20CI/badge.svg)](https://github.com/vamp-plugins/vamp-plugin-sdk/actions?query=workflow%3A%22Linux+CI%22)
 * macOS CI build: [![Build Status](https://github.com/vamp-plugins/vamp-plugin-sdk/workflows/macOS%20CI/badge.svg)](https://github.com/vamp-plugins/vamp-plugin-sdk/actions?query=workflow%3A%22macOS+CI%22)
 * Windows CI build: [![Build Status](https://github.com/vamp-plugins/vamp-plugin-sdk/workflows/Windows%20CI/badge.svg)](https://github.com/vamp-plugins/vamp-plugin-sdk/actions?query=workflow%3A%22Windows+CI%22)


What's In This SDK
------------------

This SDK contains the following:


### vamp/vamp.h

The formal C language plugin API for Vamp plugins.

A Vamp plugin is a dynamic library (`.so`, `.dll` or `.dylib`
depending on platform) exposing one C-linkage entry point
(`vampGetPluginDescriptor`) which returns data defined in the rest of
this C header.

Although the C API is the official API for Vamp, we don't recommend
that you program directly to it.  The C++ abstractions found in the
`vamp-sdk` and `vamp-hostsdk` directories (below) are preferable for
most purposes and are more thoroughly documented.


### vamp-sdk

C++ classes for implementing Vamp plugins.

Plugins should subclass `Vamp::Plugin` and then use
`Vamp::PluginAdapter` to expose the correct C API for the plugin.
Plugin authors should read `vamp-sdk/PluginBase.h` and `Plugin.h` for
code documentation.

See "examples" below for details of the example plugins in the SDK,
from which you are welcome to take code and inspiration.

Plugins should link with `-lvamp-sdk`.


### vamp-hostsdk

C++ classes for implementing Vamp hosts.

Hosts will normally use a `Vamp::PluginHostAdapter` to convert each
plugin's exposed C API back into a useful `Vamp::Plugin` C++ object.

The `Vamp::HostExt` namespace contains several additional C++ classes
to do this work for them, and make the host's life easier:

 - `Vamp::HostExt::PluginLoader` provides a very easy interface for a
 host to discover, load, and find out category information about the
 available plugins.  Most Vamp hosts will probably want to use this
 class.

 - `Vamp::HostExt::PluginInputDomainAdapter` provides a simple means
 for hosts to handle plugins that want frequency-domain input, without
 having to convert the input themselves.

 - `Vamp::HostExt::PluginChannelAdapter` provides a simple means for
 hosts to use plugins that do not necessarily support the same number
 of audio channels as they have available, without having to apply a
 channel management / mixdown policy themselves.

 - `Vamp::HostExt::PluginBufferingAdapter` provides a means for hosts
 to avoid having to negotiate the input step and block size, instead
 permitting the host to use any block size they desire (and a step
 size equal to it).  This is particularly useful for "streaming" hosts
 that cannot seek backwards in the input audio stream and so would
 otherwise need to implement an additional buffer to support step
 sizes smaller than the block size.

 - `Vamp::HostExt::PluginSummarisingAdapter` provides summarisation
 methods such as mean and median averages of output features, for use
 in any context where an available plugin produces individual values
 but the result that is actually needed is some sort of aggregate.

The `PluginLoader` class can also use the input domain, channel, and
buffering adapters automatically to make these conversions transparent
to the host if required.

Host authors should also refer to the example host code in the host
directory of the SDK.

Hosts should link with `-lvamp-hostsdk`.


### vamp-hostsdk/host-c.h

A C-linkage header wrapping the part of the C++ SDK code that handles
plugin discovery and library loading. Host programs written in C or in
a language with a C-linkage foreign function interface may choose to
use this header to discover and load plugin libraries, together with
the `vamp/vamp.h` formal API to interact with plugins themselves. See
the header for more documentation.


### examples

Example plugins implemented using the C++ classes.

These plugins are intended to be useful examples you can draw code
from in order to provide the basic shape and structure of a Vamp
plugin.  They are also intended to be correct and useful, if simple.

 - [ZeroCrossing](examples/ZeroCrossing.cpp) calculates the positions
 and density of zero-crossing points in an audio waveform.

 - [SpectralCentroid](examples/SpectralCentroid.cpp) calculates the
 centre of gravity of the frequency domain representation of each
 block of audio.

 - [PowerSpectrum](examples/PowerSpectrum.cpp) calculates a power
 spectrum from the input audio.  Actually, it doesn't do any work
 except calculating power from a cartesian complex FFT output.  The
 work of calculating this frequency domain output is done for it by
 the host or host SDK; the plugin just needs to declare that it wants
 frequency domain input.  This is the simplest of the example plugins.

 - [AmplitudeFollower](examples/AmplitudeFollower.cpp) is a simple
 implementation of SuperCollider's amplitude-follower algorithm.

 - [PercussionOnsetDetector](examples/PercussionOnsetDetector.cpp)
 estimates the locations of percussive onsets using a simple method
 described in "Drum Source Separation using Percussive Feature
 Detection and Spectral Modulation" by Dan Barry, Derry Fitzgerald,
 Eugene Coyle and Bob Lawlor, ISSC 2005.

 - [FixedTempoEstimator](examples/FixedTempoEstimator.cpp) calculates
 a single beats-per-minute value which is an estimate of the tempo of
 a piece of music that is assumed to be of fixed tempo, using
 autocorrelation of a frequency domain energy rise metric.  It has
 several outputs that return intermediate results used in the
 calculation, and may be a useful example of a plugin having several
 outputs with varying feature structures.


### skeleton

Skeleton code that could be used as a template for your new plugin
implementation.


### host

A simple command-line Vamp host, capable of loading a plugin and using
it to process a complete audio file, with its default parameters.

This host also contains a number of options for listing the installed
plugins and their properties in various formats.  For that reason, it
isn't really as simple as one might hope.  The core of the code is
still reasonably straightforward, however.


Plugin Lookup and Categorisation
--------------------------------

The Vamp API does not officially specify how to load plugin libraries
or where to find them.  However, the SDK does include a function
(`Vamp::PluginHostAdapter::getPluginPath()`) that returns a
recommended directory search path that hosts may use for plugin
libraries, and a class (`Vamp::HostExt::PluginLoader`) that implements
a sensible cross-platform lookup policy using this path.  We recommend
using this class in your host unless you have a good reason not to
want to.  This implementation also permits the user to set the
environment variable `VAMP_PATH` to override the default path if
desired.

The policy used by `Vamp::HostExt::PluginLoader` -- and our
recommendation for any host -- is to search each directory in the path
returned by `getPluginPath` for `.DLL` (on Windows), `.so` (on Linux,
Solaris, BSD etc) or `.dylib` (on OS/X) files, then to load each one
and perform a dynamic name lookup on the vampGetPluginDescriptor
function to enumerate the plugins in the library.  This operation will
necessarily be system-dependent.

Vamp also has an informal convention for sorting plugins into
functional categories.  In addition to the library file itself, a
plugin library may install a category file with the same name as the
library but `.cat` extension.  The existence and format of this file
are not specified by the Vamp API, but by convention the file may
contain lines of the format

`vamp:pluginlibrary:pluginname::General Category > Specific Category`

which a host may read and use to assign plugins a location within a
category tree for display to the user.  The expectation is that
advanced users may also choose to set up their own preferred category
trees, which is why this information is not queried as part of the
Vamp plugin's API itself.  The `Vamp::HostExt::PluginLoader` class
also provides support for plugin category lookup using this scheme.


Licensing
---------

This plugin SDK is freely redistributable under a "new-style BSD"
licence.  See the file `COPYING` for more details.  In short, you may
modify and redistribute the SDK and example plugins within any
commercial or non-commercial, proprietary or open-source plugin or
application under almost any conditions, with no obligation to provide
source code, provided you retain the original copyright note.


See Also
--------

[Sonic Visualiser](https://www.sonicvisualiser.org/), an interactive
open-source graphical audio inspection, analysis and visualisation
tool supporting Vamp plugins.



Authors
-------

Vamp and the Vamp SDK were designed and made at the Centre for Digital
Music at Queen Mary, University of London.

The SDK was written by Chris Cannam, copyright (c) 2005-2024
Chris Cannam and QMUL.

The SDK incorporates KissFFT code, copyright (c) 2003-2010 Mark
Borgerding.

The CMake support was provided by Lukas Berbuer.

Mark Sandler and Christian Landone provided ideas and direction, and
Mark Levy, Dan Stowell, Martin Gasser and Craig Sapp provided testing
and other input for the 1.0 API and SDK.  The API also uses some ideas
from prior plugin systems, notably DSSI (http://dssi.sourceforge.net)
and FEAPI (http://feapi.sourceforge.net).

