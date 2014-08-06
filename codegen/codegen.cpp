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

struct EntryType
{
	string name;
	string type;
	string defvalue;

	inline bool operator< (EntryType const& other) const
	{
		return name < other.name;
	}

	inline bool operator== (EntryType const& other) const
	{
		return name == other.name and type == other.type;
	}

	inline bool operator!= (EntryType const& other) const
	{
		return not operator== (other);
	}
};

char AdvancePointer (char const*& ptr)
{
	char a = *ptr++;

	if (*ptr == '\0')
		throw false;

	return a;
}

void ReadConfigEntries (string const& filename, vector<EntryType>& entries, const char* macroname)
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
			EntryType entry;

			// Skip to paren
			while (*ptr != '(')
				AdvancePointer (ptr);

			// Skip whitespace
			while (isspace (*ptr))
				AdvancePointer (ptr);

			// Skip paren
			AdvancePointer (ptr);

			// Read type
			while (*ptr != ',')
				entry.type += AdvancePointer (ptr);

			// Skip comma and whitespace
			for (AdvancePointer (ptr); isspace (*ptr); AdvancePointer (ptr))
				;

			// Read name
			while (*ptr != ',')
				entry.name += AdvancePointer (ptr);

			// Skip comma and whitespace
			for (AdvancePointer (ptr); isspace (*ptr); AdvancePointer (ptr))
				;

			// Read default
			while (*ptr != ')')
				entry.defvalue += AdvancePointer (ptr);

			entries.push_back (entry);
		}
		catch (bool) {}
	}
}

bool CheckEquality (vector<EntryType> a, vector<EntryType> b)
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
	vector<EntryType> entries;
	vector<EntryType> oldentries;
	ReadConfigEntries (argv[argc - 1], oldentries, "CODEGEN_CACHE");

	for (int arg = 1; arg < argc - 1; ++arg)
		ReadConfigEntries (argv[arg], entries, "CFGENTRY");

	if (CheckEquality (entries, oldentries))
	{
		cout << "Configuration options unchanged" << endl;
		return 0;
	}

	std::ofstream os (argv[argc - 1]);
	os << "#pragma once" << endl;
	os << "#define CODEGEN_CACHE(A,B,C)" << endl;

	for (vector<EntryType>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
		os << "CODEGEN_CACHE (" << it->type << ", " << it->name << ", " <<
			  it->defvalue << ")" << endl;
	}

	os << endl;
	for (vector<EntryType>::const_iterator it = entries.begin(); it != entries.end(); ++it)
		os << "EXTERN_CFGENTRY (" << it->type << ", " << it->name << ")" << endl;

	os << endl;
	os << "static void InitConfigurationEntry (AbstractConfigEntry* entry);" << endl;
	os << "static void SetupConfigurationLists()" << endl;
	os << "{" << endl;

	for (vector<EntryType>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
		os << "\tInitConfigurationEntry (new " << it->type << "ConfigEntry (&cfg::" <<
			  it->name << ", \"" << it->name << "\", " << it->defvalue << "));" << endl;
	}

	os << "}" << endl;
	cout << "Wrote configuration options list to " << argv[argc - 1] << "." << endl;
	return 0;
}
