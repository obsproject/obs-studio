//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fcommandline.h
// Created by  : Steinberg, 2007
// Description : Very simple command-line parser. @see Steinberg::CommandLine 
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <map>
#include <sstream>
#include <vector>

namespace Steinberg {
//------------------------------------------------------------------------
/** Very simple command-line parser.

Parses the command-line into a CommandLine::VariablesMap.\n
The command-line parser uses CommandLine::Descriptions to define the available options.

@b Example:
\code
#include "base/source/fcommandline.h"
#include <iostream>

int main (int argc, char* argv[])
{
    using namespace std;

    CommandLine::Descriptions desc;
    CommandLine::VariablesMap valueMap;

    desc.addOptions ("myTool")
        ("help", "produce help message")
        ("opt1", string(), "option 1")
        ("opt2", string(), "option 2")
    ;
    CommandLine::parse (argc, argv, desc, valueMap);

    if (valueMap.hasError () || valueMap.count ("help"))
    {
        cout << desc << "\n";
        return 1;
    }
    if (valueMap.count ("opt1"))
    {
        cout << "Value of option 1 " << valueMap["opt1"] << "\n";
    }
    if (valueMap.count ("opt2"))
    {
        cout << "Value of option 2 " << valueMap["opt2"] << "\n";
    }
    return 0;
}
\endcode
@note
This is a "header only" implementation.\n
If you need the declarations in more than one cpp file, you have to define
@c SMTG_NO_IMPLEMENTATION in all but one file.

*/
//------------------------------------------------------------------------
namespace CommandLine {

//------------------------------------------------------------------------
/**	Command-line parsing result.
    This is the result of the parser.\n
    - Use hasError() to check for errors.\n
    - To test if a option was specified on the command-line use: count()\n
    - To retrieve the value of an options, use operator [](const VariablesMapContainer::key_type
   k)\n
*/
//------------------------------------------------------------------------
class VariablesMap
{
	bool mParaError;
	using VariablesMapContainer = std::map<std::string, std::string>;
	VariablesMapContainer mVariablesMapContainer;

public:
	VariablesMap () : mParaError (false) {} ///< Constructor. Creates a empty VariablesMap.
	bool hasError () const { return mParaError; } ///< Returns @c true when an error has occurred.
	void setError () { mParaError = true; } ///< Sets the error state to @c true.
	
	/** Retrieve the value of option @c k.*/
	std::string& operator[] (const VariablesMapContainer::key_type k);
	
	/** Retrieve the value of option @c k. */
	const std::string& operator[] (const VariablesMapContainer::key_type k) const;
	
	/** Returns @c != @c 0 if command-line contains option @c k. */
	VariablesMapContainer::size_type count (const VariablesMapContainer::key_type k) const;
};

//! type of the list of elements on the command line that are not handled by options parsing
using FilesVector = std::vector<std::string>;

//------------------------------------------------------------------------
/** The description of one single command-line option.
    Normally you rarely use a Description directly.\n
    In most cases you will use the Descriptions::addOptions (const std::string&) method to create
   and add descriptions.
*/
//------------------------------------------------------------------------
class Description : public std::string
{
public:
	/** Construct a Description */
	Description (const std::string& name, const std::string& help,
	             const std::string& valueType);
	std::string mHelp; ///< The help string for this option.
	std::string mType; ///< The type of this option (kBool, kString).
	static const std::string kBool;
	static const std::string kString;
};
//------------------------------------------------------------------------
/** List of command-line option descriptions.

    Use addOptions (const std::string&) to add Descriptions.
*/
//------------------------------------------------------------------------
class Descriptions
{
	using DescriptionsList = std::deque<Description>;
	DescriptionsList mDescriptions;
	std::string mCaption;

public:
	/** Sets the command-line tool caption and starts adding Descriptions. */
	Descriptions& addOptions (const std::string& caption = "",
	                          std::initializer_list<Description>&& options = {});
	
	/** Parse the command-line. */
	bool parse (int ac, char* av[], VariablesMap& result, FilesVector* files = nullptr) const;
	
	/** Print a brief description for the command-line tool into the stream @c os. */
	void print (std::ostream& os) const;

	/** Add a new switch. Only */
	Descriptions& operator () (const std::string& name, const std::string& help);
	
	/** Add a new option of type @c inType. Currently only std::string is supported. */
	template <typename Type>
	Descriptions& operator () (const std::string& name, const Type& inType, std::string help);
};

//------------------------------------------------------------------------
// If you need the declarations in more than one cpp file you have to define
// SMTG_NO_IMPLEMENTATION in all but one file.
//------------------------------------------------------------------------
#ifndef SMTG_NO_IMPLEMENTATION

//------------------------------------------------------------------------
/*! If command-line contains option @c k more than once, only the last value will survive. */
//------------------------------------------------------------------------
std::string& VariablesMap::operator[] (const VariablesMapContainer::key_type k)
{
	return mVariablesMapContainer[k];
}

//------------------------------------------------------------------------
/*! If command-line contains option @c k more than once, only the last value will survive. */
//------------------------------------------------------------------------
const std::string& VariablesMap::operator[] (const VariablesMapContainer::key_type k) const
{
	return (*const_cast<VariablesMap*> (this))[k];
}

//------------------------------------------------------------------------
VariablesMap::VariablesMapContainer::size_type VariablesMap::count (
    const VariablesMapContainer::key_type k) const
{
	return mVariablesMapContainer.count (k);
}

//------------------------------------------------------------------------
/** Add a new option with a string as parameter. */
//------------------------------------------------------------------------
template <>
Descriptions& Descriptions::operator () (const std::string& name, const std::string& inType,
                                         std::string help)
{
	mDescriptions.emplace_back (name, help, inType);
	return *this;
}

/** Parse the command - line. */
bool parse (int ac, char* av[], const Descriptions& desc, VariablesMap& result,
            FilesVector* files = nullptr);

/** Make Descriptions stream able. */
std::ostream& operator<< (std::ostream& os, const Descriptions& desc);

const std::string Description::kBool = "bool";
const std::string Description::kString = "string";

//------------------------------------------------------------------------
/*! In most cases you will use the Descriptions::addOptions (const std::string&) method to create
 and add descriptions.
 
    @param[in] name of the option.
    @param[in] help a help description for this option.
    @param[out] valueType Description::kBool or Description::kString.
*/
Description::Description (const std::string& name, const std::string& help,
                          const std::string& valueType)
: std::string (name), mHelp (help), mType (valueType)
{
}

//------------------------------------------------------------------------
/*! Returning a reference to *this, enables chaining of calls to operator()(const std::string&,
 const std::string&).

    @param[in] name of the added option.
    @param[in] help a help description for this option.
    @return a reference to *this.
*/
Descriptions& Descriptions::operator () (const std::string& name, const std::string& help)
{
	mDescriptions.emplace_back (name, help, Description::kBool);
	return *this;
}

//------------------------------------------------------------------------
/*!	<b>Usage example:</b>
                                                            @code
CommandLine::Descriptions desc;
desc.addOptions ("myTool")           // Set caption to "myTool"
    ("help", "produce help message") // add switch -help
    ("opt1", string(), "option 1")   // add string option -opt1
    ("opt2", string(), "option 2")   // add string option -opt2
;
                                                            @endcode
@note
    The operator() is used for every additional option.

Or with initializer list :
                                                            @code
CommandLine::Descriptions desc;
desc.addOptions ("myTool",                                 // Set caption to "myTool"
    {{"help", "produce help message", Description::kBool}, // add switch -help
     {"opt1", "option 1", Description::kString},           // add string option -opt1
     {"opt2", "option 2", Description::kString}}           // add string option -opt2
);
                                                            @endcode
@param[in] caption the caption of the command-line tool.
@param[in] options initializer list with options
@return a reverense to *this.
*/
Descriptions& Descriptions::addOptions (const std::string& caption,
                                        std::initializer_list<Description>&& options)
{
	mCaption = caption;
	std::move (options.begin (), options.end (), std::back_inserter (mDescriptions));
	return *this;
}

//------------------------------------------------------------------------
/*!	@param[in] ac count of command-line parameters
    @param[in] av command-line as array of strings
    @param[out] result the parsing result
    @param[out] files optional list of elements on the command line that are not handled by options
   parsing
*/
bool Descriptions::parse (int ac, char* av[], VariablesMap& result, FilesVector* files) const
{
	using namespace std;

	for (int i = 1; i < ac; i++)
	{
		string current = av[i];
		if (current[0] == '-')
		{
			int pos = current[1] == '-' ? 2 : 1;
			current = current.substr (pos, string::npos);

			DescriptionsList::const_iterator found =
			    find (mDescriptions.begin (), mDescriptions.end (), current);
			if (found != mDescriptions.end ())
			{
				result[*found] = "true";
				if (found->mType != Description::kBool)
				{
					if (((i + 1) < ac) && *av[i + 1] != '-')
					{
						result[*found] = av[++i];
					}
					else
					{
						result[*found] = "error!";
						result.setError ();
						return false;
					}
				}
			}
			else
			{
				result.setError ();
				return false;
			}
		}
		else if (files)
			files->push_back (av[i]);
	}
	return true;
}

//------------------------------------------------------------------------
/*!	The description includes the help strings for all options. */
//------------------------------------------------------------------------
void Descriptions::print (std::ostream& os) const
{
	if (!mCaption.empty ())
		os << mCaption << ":\n";

	size_t maxLength = 0u;
	std::for_each (mDescriptions.begin (), mDescriptions.end (),
	               [&] (const Description& d) { maxLength = std::max (maxLength, d.size ()); });

	for (const Description& opt : mDescriptions)
	{
		os << "-" << opt;
		for (auto s = opt.size (); s < maxLength; ++s)
			os << " ";
		os << " | " << opt.mHelp << "\n";
	}
}

//------------------------------------------------------------------------
std::ostream& operator<< (std::ostream& os, const Descriptions& desc)
{
	desc.print (os);
	return os;
}

//------------------------------------------------------------------------
/*! @param[in] ac count of command-line parameters
    @param[in] av command-line as array of strings
    @param[in] desc Descriptions including all allowed options
    @param[out] result the parsing result
    @param[out] files optional list of elements on the command line that are not handled by options
   parsing
*/
bool parse (int ac, char* av[], const Descriptions& desc, VariablesMap& result, FilesVector* files)
{
	return desc.parse (ac, av, result, files);
}
#endif

//------------------------------------------------------------------------
} // namespace CommandLine
} // namespace Steinberg
