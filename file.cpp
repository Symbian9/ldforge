/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#include <vector>

#include "common.h"
#include "config.h"
#include "file.h"
#include "misc.h"
#include "bbox.h"
#include "gui.h"

cfg (str, io_ldpath, "");

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* findLoadedFile (str zName) {
	FOREACH (OpenFile, *, file, g_LoadedFiles)
		if (file->zFileName == zName)
			return file;
	
	return nullptr;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* openDATFile (str path) {
	logf ("Opening %s...\n", path.chars());
	
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	str zTruePath = -path;
#ifndef WIN32
	zTruePath.replace ("\\", "/");
#endif // WIN32
	
	FILE* fp = fopen (path.chars (), "r");
	
	if (!fp && ~io_ldpath.value) {
		char const* saSubdirectories[] = {
			"parts",
			"p",
		};
		
		for (ushort i = 0; i < sizeof saSubdirectories / sizeof *saSubdirectories; ++i) {
			str zFilePath = str::mkfmt ("%s" DIRSLASH "%s" DIRSLASH "%s",
				io_ldpath.value.chars(), saSubdirectories[i], zTruePath.chars());
			
			fp = fopen (zFilePath.chars (), "r");
			
			if (fp)
				break;
		}
	}
	
	if (!fp) {
		logf (LOG_Error, "Couldn't open %s: %s\n", path.chars (), strerror (errno));
		return nullptr;
	}
	
	OpenFile* load = new OpenFile;
	ulong numWarnings = 0;
	
	load->zFileName = path;
	
	vector<str> lines;
	
	{
		char line[1024];
		while (fgets (line, sizeof line, fp)) {
			// Trim the trailing newline
			str zLine = line;
			while (zLine[~zLine - 1] == '\n' || zLine[~zLine - 1] == '\r')
				zLine -= 1;
			
			lines.push_back (zLine);
		}
	}
	
	fclose (fp);
	
	for (ulong i = 0; i < lines.size(); ++i) {
		LDObject* obj = parseLine (lines[i]);
		load->objects.push_back (obj);
		
		// Check for parse errors and warn abotu tthem
		if (obj->getType() == OBJ_Gibberish) {
			logf (LOG_Warning, "Couldn't parse line #%lu: %s\n",
				i, static_cast<LDGibberish*> (obj)->zReason.chars());
			logf (LOG_Warning, "- Line was: %s\n", lines[i].chars());
			numWarnings++;
		}
	}
	
	g_LoadedFiles.push_back (load);
	
	logf (LOG_Success, "File %s parsed successfully (%lu warning%s).\n",
		path.chars(), numWarnings, PLURAL (numWarnings));
	
	return load;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// Clear everything from the model
void OpenFile::close () {
	for (ulong j = 0; j < objects.size(); ++j)
		delete objects[j];
	
	delete this;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void closeAll () {
	if (!g_LoadedFiles.size())
		return;
	
	// Remove all loaded files and the objects they contain
	for (ushort i = 0; i < g_LoadedFiles.size(); i++) {
		OpenFile* f = g_LoadedFiles[i];
		f->close ();
	}
	
	// Clear the array
	g_LoadedFiles.clear();
	g_CurrentFile = NULL;
	
	g_qWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void newFile () {
	// Create a new anonymous file and set it to our current
	closeAll ();
	
	OpenFile* f = new OpenFile;
	f->zFileName = "";
	g_LoadedFiles.push_back (f);
	g_CurrentFile = f;
	
	g_BBox.calculate();
	g_qWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void openMainFile (str zPath) {
	closeAll ();
	
	OpenFile* pFile = openDATFile (zPath);
	
	if (!pFile)
		return;
	
	g_CurrentFile = pFile;
	
	// Recalculate the bounding box
	g_BBox.calculate();
	
	// Rebuild the object tree view now.
	g_qWindow->refresh ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
bool OpenFile::save (str zPath) {
	if (!~zPath)
		zPath = zFileName;
	
	FILE* fp = fopen (zPath, "w");
	if (!fp)
		return false;
	
	// Write all entries now
	for (ulong i = 0; i < objects.size(); ++i) {
		LDObject* obj = objects[i];
		
		// LDraw requires lines to have DOS line endings
		str zLine = str::mkfmt ("%s\r\n",obj->getContents ().chars ());
		
		fwrite (zLine.chars(), 1, ~zLine, fp);
	}
	
	fclose (fp);
	return true;
}

#define CHECK_TOKEN_COUNT(N) \
	if (tokens.size() != N) \
		return new LDGibberish (zLine, "Bad amount of tokens");

#define CHECK_TOKEN_NUMBERS(MIN,MAX) \
	for (ushort i = MIN; i <= MAX; ++i) \
		if (!isNumber (tokens[i])) \
			return new LDGibberish (zLine, str::mkfmt ("Token #%u was `%s`, expected a number", \
				(i + 1), tokens[i].chars()));

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
static vertex parseVertex (vector<str>& s, const ushort n) {
	// Disable the locale while parsing the line or atof's behavior changes
	// between locales (i.e. fails to read decimals properly). That is
	// quite undesired...
	setlocale (LC_NUMERIC, "C");
	
	vertex v;
	v.x = atof (s[n]);
	v.y = atof (s[n + 1]);
	v.z = atof (s[n + 2]);
	
	return v;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
LDObject* parseLine (str zLine) {
	vector<str> tokens = zLine.split (" ", true);
	
	if (!tokens.size ()) {
		// Line was empty, or only consisted of whitespace
		return new LDEmpty;
	}
	
	if (~tokens[0] != 1)
		return new LDGibberish (zLine, "Illogical line code");
	
	char const c = tokens[0][0];
	switch (c - '0') {
	case 0:
		{
			// Comment
			str zComment = zLine.substr (2, -1);
			
			if (tokens.size() > 2 && tokens[1] == "!LDFORGE") {
				// Handle LDForge-specific types, they're embedded into comments
				
				if (tokens[2] == "VERTEX") {
					// Vertex (0 !LDFORGE VERTEX)
					CHECK_TOKEN_COUNT (7)
					CHECK_TOKEN_NUMBERS (3, 6)
					
					LDVertex* obj = new LDVertex;
					obj->dColor = atol (tokens[3]);
					obj->vPosition.x = atof (tokens[4]);
					obj->vPosition.y = atof (tokens[5]);
					obj->vPosition.z = atof (tokens[6]);
					
					return obj;
				}
			}
			
			LDComment* obj = new LDComment;
			obj->zText = zComment;
			return obj;
		}
	
	case 1:
		{
			// Subfile
			CHECK_TOKEN_COUNT (15)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Try open the file
			OpenFile* pFile = loadSubfile (tokens[14]);
			
			// If we cannot open the file, mark it an error
			if (!pFile)
				return new LDGibberish (zLine, "Could not open referred file");
			
			LDSubfile* obj = new LDSubfile;
			obj->dColor = atol (tokens[1]);
			obj->vPosition = parseVertex (tokens, 2); // 2 - 4
			
			for (short i = 0; i < 9; ++i)
				obj->mMatrix[i] = atof (tokens[i + 5]); // 5 - 13
			
			obj->zFileName = tokens[14];
			obj->pFile = pFile;
			return obj;
		}
	
	case 2:
		{
			CHECK_TOKEN_COUNT (8)
			CHECK_TOKEN_NUMBERS (1, 7)
			
			// Line
			LDLine* obj = new LDLine;
			obj->dColor = atol (tokens[1]);
			for (short i = 0; i < 2; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 7
			return obj;
		}
	
	case 3:
		{
			CHECK_TOKEN_COUNT (11)
			CHECK_TOKEN_NUMBERS (1, 10)
			
			// Triangle
			LDTriangle* obj = new LDTriangle;
			obj->dColor = atol (tokens[1]);
			
			for (short i = 0; i < 3; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 10
			
			return obj;
		}
	
	case 4:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Quadrilateral
			LDQuad* obj = new LDQuad;
			obj->dColor = atol (tokens[1]);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 13
			
			return obj;
		}
	
	case 5:
		{
			CHECK_TOKEN_COUNT (14)
			CHECK_TOKEN_NUMBERS (1, 13)
			
			// Conditional line
			LDCondLine* obj = new LDCondLine;
			obj->dColor = atol (tokens[1]);
			
			for (short i = 0; i < 4; ++i)
				obj->vaCoords[i] = parseVertex (tokens, 2 + (i * 3)); // 2 - 13
			return obj;
		}
		
	default: // Strange line we couldn't parse
		return new LDGibberish (zLine, "Unknown line code number");
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenFile* loadSubfile (str zFile) {
	// Try open the file
	OpenFile* pFile = findLoadedFile (zFile);
	if (!pFile)
		pFile = openDATFile (zFile);
	
	return pFile;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void reloadAllSubfiles () {
	if (!g_CurrentFile)
		return;
	
	// First, close all but the current open file.
	for (ushort i = 0; i < g_LoadedFiles.size(); ++i)
		if (g_LoadedFiles[i] != g_CurrentFile)
			g_LoadedFiles[i]->close();
	
	g_LoadedFiles.clear ();
	g_LoadedFiles.push_back (g_CurrentFile);
	
	// Go through all the current file and reload the subfiles
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		LDObject* obj = g_CurrentFile->objects[i];
		
		// Reload subfiles
		if (obj->getType() == OBJ_Subfile) {
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			OpenFile* pFile = loadSubfile (ref->zFileName);
			
			if (pFile)
				ref->pFile = pFile;
			else {
				// Couldn't load the file, mark it an error
				ref->replace (new LDGibberish (ref->getContents (), "Could not open referred file"));
			}
		}
		
		// Reparse gibberish files. It could be that they are invalid because
		// the file could not be opened. Circumstances may be different now.
		if (obj->getType() == OBJ_Gibberish)
			obj->replace (parseLine (static_cast<LDGibberish*> (obj)->zContents));
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void OpenFile::addObject (LDObject* obj) {
	if (this != g_CurrentFile) {
		objects.insert (objects.end (), obj);
		return;
	}
	
	const ulong ulSpot = g_qWindow->getInsertionPoint ();
	objects.insert (objects.begin() + ulSpot, obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void OpenFile::forgetObject (LDObject* obj) {
	// Find the index for the given object
	ulong ulIndex;
	for (ulIndex = 0; ulIndex < (ulong)objects.size(); ++ulIndex)
		if (objects[ulIndex] == obj)
			break; // found it
	
	if (ulIndex >= objects.size ())
		return; // was not found
	
	// Erase it from memory
	objects.erase (objects.begin() + ulIndex);
}