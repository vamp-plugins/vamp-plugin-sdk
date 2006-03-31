/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _VAMP_PLUGIN_BASE_H_
#define _VAMP_PLUGIN_BASE_H_

#include <string>
#include <vector>

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
    /**
     * Get the computer-usable name of the plugin.  This should be
     * reasonably short and contain no whitespace or punctuation
     * characters.  It may be shown to the user, but it won't be the
     * main method for a user to identify a plugin (that will be the
     * description, below).
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
	 * The default value of the parameter.
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
