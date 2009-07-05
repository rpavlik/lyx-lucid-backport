// -*- C++ -*-
/**
 * \file ModuleList.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Richard Heck
 *
 * Full author contact details are available in file CREDITS.
 */

#ifndef MODULELIST_H
#define MODULELIST_H

#include <map>
#include <string>
#include <vector>

namespace lyx {
	
/**
 *  This struct represents a particular LyX "module", which is a like a layout
 *  file, except that it does not stand alone. In that sense, it is more like 
 *  a LaTeX package, where a layout file corresponds to a LaTeX class. Or, in 
 *  LyX's own terms, a module is more like an included file that can be used
 *  with various document classes. The difference is that using a module only
 *  means selecting it in the Document>Settings dialog, whereas including a 
 *  layout file means layout file editing.
 *
 *  In general, a given module can be used with any document class. That said,
 *  one module may `require' another, or it may `exclude' some other module.
 *  The requires and excludes are given in comments within the module file,
 *  which must begin roughly so:
 *   #\DeclareLyXModule{Theorems (By Section)}
 *   #DescriptionBegin
 *   #Numbers theorems and the like by section.
 *   #DescriptionEnd
 *   #Requires: theorems-std | theorems-ams
 *   #Excludes: theorems-chap
 *  The description is used in the gui to give information to the user. The
 *  Requires and Excludes lines are read by the configuration script and
 *  written to a file lyxmodules.lst in the user configuration directory.
 *
 *  Modules can also be "provided" or "excluded" by document classes, using
 *  the ProvidesModule and ExcludesModule tags.
 */

class LyXModule {
public:
	///
	LyXModule(std::string const & n, std::string const & i, 
	          std::string const & d, std::vector<std::string> const & p,
	          std::vector<std::string> const & r, 
	          std::vector<std::string> const & e);
	/// whether the required packages are available
	bool isAvailable();
	///
	std::string const & getName() const { return name; }
	///
	std::string const & getID() const { return id; }
	///
	std::string const & getFilename() const { return filename; }
	///
	std::string const & getDescription() const { return description; }
	///
	std::vector<std::string> const & getPackageList() const
		{ return packageList; }
	///
	std::vector<std::string> const & getRequiredModules() const 
		{ return requiredModules; }
	/// Modules this one excludes: the list should be treated disjunctively
	std::vector<std::string> const & getExcludedModules() const 
		{ return excludedModules; }
	/// \return true if the module is compatible with this one, i.e.,
	/// it does not exclude us and we do not exclude it.
	/// this will also return true if modName is unknown and we do not
	/// exclude it, since in that case we cannot check its exclusions.
	bool isCompatible(std::string const & modName) const;
	///
	static bool areCompatible(std::string const & mod1, std::string const & mod2);
private:
	/// what appears in the ui
	std::string name;
	/// the module's unique identifier
	/// at present, this is the filename, without the extension
	std::string id;
	/// the filename
	std::string filename;
	/// a short description for use in the ui
	std::string description;
	/// the LaTeX packages on which this depends, if any
	std::vector<std::string> packageList;
	/// Modules this one requires: at least one
	std::vector<std::string> requiredModules;
	/// Modules this one excludes: none of these
	std::vector<std::string> excludedModules;
	///
	bool checked;
	///
	bool available;
};

typedef std::vector<LyXModule> LyXModuleList;

/**
 *  The ModuleList represents the various LyXModule's that are available at
 *  present.
 */
class ModuleList {
public:
	///
	ModuleList() {}
	/// reads the modules from a file generated by configure.py
	bool load();
	///
	LyXModuleList::const_iterator begin() const;
	///
	LyXModuleList::iterator begin();
	///
	LyXModuleList::const_iterator end() const;
	///
	LyXModuleList::iterator end();
	///
	bool empty() const { return modlist_.empty(); }
	/// Returns a pointer to the LyXModule with filename str.
	/// Returns a null pointer if no such module is found.
	LyXModule * operator[](std::string const & str);
private:
	/// noncopyable
	ModuleList(ModuleList const &);
	///
	void operator=(ModuleList const &);
	/// add a module to the list
	void addLayoutModule(std::string const &, std::string const &, 
		std::string const &, std::vector<std::string> const &,
		std::vector<std::string> const &, std::vector<std::string> const &);
	///
	std::vector<LyXModule> modlist_;
};

extern ModuleList moduleList;
}
#endif