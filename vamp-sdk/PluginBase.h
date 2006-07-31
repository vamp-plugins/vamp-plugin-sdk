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

#ifndef _VAMP_PLUGIN_BASE_H_
#define _VAMP_PLUGIN_BASE_H_

#include <string>
#include <vector>

#define VAMP_SDK_VERSION "0.9.5"

namespace Vamp {

/**
 * A base class for plugins with optional configurable parameters,
 * programs, etc.
 *
 * This does not provide the necessary interfaces to instantiate or
 * run a plugin -- that depends on the plugin subclass, as different
 * plugin types may have quite different operating structures.  This
 * class just specifies the necessary interface to show editable
 * controls for the plugin to the user.
 */

class PluginBase 
{
public:
    virtual ~PluginBase() { }

    /**
     * Get the computer-usable name of the plugin.  This should be
     * reasonably short and contain no whitespace or punctuation
     * characters.  It may be shown to the user, but it won't be the
     * main method for a user to identify a plugin (that will be the
     * description, below).  This may only contain the characters
     * [a-zA-Z0-9_].
     */
    virtual std::string getName() const = 0;

    /**
     * Get a human-readable description of the plugin.  This should be
     * self-contained, as it may be shown to the user in isolation
     * without also showing the plugin's "name".
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * Get the name of the author or vendor of the plugin in
     * human-readable form.
     */
    virtual std::string getMaker() const = 0;

    /**
     * Get the version number of the plugin.
     */
    virtual int getPluginVersion() const = 0;

    /**
     * Get the copyright statement or licensing summary of the plugin.
     */
    virtual std::string getCopyright() const = 0;

    /**
     * Get the type of plugin (e.g. DSSI, etc).  This is likely to be
     * implemented by the immediate subclass, not by actual plugins.
     */
    virtual std::string getType() const = 0;


    struct ParameterDescriptor
    {
	/**
	 * The name of the parameter, in computer-usable form.  Should
	 * be reasonably short, and may only contain the characters
	 * [a-zA-Z0-9_].
	 */
	std::string name;

	/**
	 * The human-readable name of the parameter.
	 */
	std::string description;

	/**
	 * The unit of the parameter, in human-readable form.
	 */
	std::string unit;

	/**
	 * The minimum value of the parameter.
	 */
	float minValue;

	/**
	 * The maximum value of the parameter.
	 */
	float maxValue;

	/**
	 * The default value of the parameter.  The plugin should
	 * ensure that parameters have this value on initialisation
	 * (i.e. the host is not required to explicitly set parameters
	 * if it wants to use their default values).
	 */
	float defaultValue;
	
	/**
	 * True if the parameter values are quantized to a particular
	 * resolution.
	 */
	bool isQuantized;

	/**
	 * Quantization resolution of the parameter values (e.g. 1.0
	 * if they are all integers).  Undefined if isQuantized is
	 * false.
	 */
	float quantizeStep;

        /**
         * Names for the quantized values.  If isQuantized is true,
         * this may either be empty or contain one string for each of
         * the quantize steps from minValue up to maxValue inclusive.
         * Undefined if isQuantized is false.
         *
         * If these names are provided, they should be shown to the
         * user in preference to the values themselves.  The user may
         * never see the actual numeric values unless they are also
         * encoded in the names.
         */
        std::vector<std::string> valueNames;
    };

    typedef std::vector<ParameterDescriptor> ParameterList;

    /**
     * Get the controllable parameters of this plugin.
     */
    virtual ParameterList getParameterDescriptors() const {
	return ParameterList();
    }

    /**
     * Get the value of a named parameter.  The argument is the name
     * field from that parameter's descriptor.
     */
    virtual float getParameter(std::string) const { return 0.0; }

    /**
     * Set a named parameter.  The first argument is the name field
     * from that parameter's descriptor.
     */
    virtual void setParameter(std::string, float) { } 

    
    typedef std::vector<std::string> ProgramList;

    /**
     * Get the program settings available in this plugin.
     * The programs must have unique names.
     */
    virtual ProgramList getPrograms() const { return ProgramList(); }

    /**
     * Get the current program.
     */
    virtual std::string getCurrentProgram() const { return ""; }

    /**
     * Select a program.  (If the given program name is not one of the
     * available programs, do nothing.)
     */
    virtual void selectProgram(std::string) { }
};

}

#endif
