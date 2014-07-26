/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

using std::string;
using std::vector;
using std::ifstream;
using std::size_t;
using std::strncmp;
using std::getline;
using std::cout;
using std::endl;

struct entry_type
{
	string name;
	string type;
	string defvalue;

	inline bool operator< (entry_type const& other) const
	{
		return name < other.name;
	}

	inline bool operator== (entry_type const& other) const
	{
		return name == other.name and type == other.type;
	}

	inline bool operator!= (entry_type const& other) const
	{
		return not operator== (other);
	}
};

char advance_pointer (char const*& ptr)
{
	char a = *ptr++;
	if (*ptr == '\0')
		throw false;

	return a;
}

void read_cfgentries (string const& filename, vector<entry_type>& entries, const char* macroname)
{
	ifstream is (filename.c_str());
	string line;
	size_t const macrolen = strlen (macroname);

	while (getline (is, line))
	{
		try
		{
			if (strncmp (line.c_str(), macroname, macrolen) != 0)
				continue;

			char const* ptr = &line[macrolen];
			entry_type entry;

			// Skip to paren
			while (*ptr != '(')
				advance_pointer (ptr);

			// Skip whitespace
			while (isspace (*ptr))
				advance_pointer (ptr);

			// Skip paren
			advance_pointer (ptr);

			// Read type
			while (*ptr != ',')
				entry.type += advance_pointer (ptr);

			// Skip comma and whitespace
			for (advance_pointer (ptr); isspace (*ptr); advance_pointer (ptr))
				;

			// Read name
			while (*ptr != ',')
				entry.name += advance_pointer (ptr);

			// Skip comma and whitespace
			for (advance_pointer (ptr); isspace (*ptr); advance_pointer (ptr))
				;

			// Read default
			while (*ptr != ')')
				entry.defvalue += advance_pointer (ptr);

			entries.push_back (entry);
		}
		catch (bool) {}
	}
}

bool check_equality (vector<entry_type> a, vector<entry_type> b)
{
	if (a.size() != b.size())
		return false;

	std::sort (a.begin(), a.end());
	std::sort (b.begin(), b.end());

	for (size_t i = 0; i < a.size(); ++i)
	{
		if (a[i] != b[i])
			return false;
	}

	return true;
}

int main (int argc, char* argv[])
{
	vector<entry_type> entries;
	vector<entry_type> oldentries;
	read_cfgentries (argv[argc - 1], oldentries, "CODEGEN_CACHE");

	for (int arg = 1; arg < argc - 1; ++arg)
		read_cfgentries (argv[arg], entries, "CFGENTRY");

	if (check_equality (entries, oldentries))
	{
		cout << "Configuration options unchanged" << endl;
	}
	else
	{
		std::ofstream os (argv[argc - 1]);
		os << "#pragma once" << endl;
		os << "#define CODEGEN_CACHE(A,B,C)" << endl;

		for (vector<entry_type>::const_iterator it = entries.begin(); it != entries.end(); ++it)
			os << "CODEGEN_CACHE (" << it->type << ", " << it->name << ", " << it->defvalue << ")" << endl;

		os << endl;
		for (vector<entry_type>::const_iterator it = entries.begin(); it != entries.end(); ++it)
			os << "EXTERN_CFGENTRY (" << it->type << ", " << it->name << ")" << endl;

		os << endl;
		os << "static void InitConfigurationEntry (AbstractConfigEntry* entry);" << endl;
		os << "static void SetupConfigurationLists()" << endl;
		os << "{" << endl;

		for (vector<entry_type>::const_iterator it = entries.begin(); it != entries.end(); ++it)
		{
			os << "\tInitConfigurationEntry (new " << it->type << "ConfigEntry (&cfg::" <<
				it->name << ", \"" << it->name << "\", " << it->defvalue << "));" << endl;
		}

		os << "}" << endl;

		cout << "Wrote configuration options list to " << argv[argc - 1] << "." << endl;
	}

	return 0;
}
