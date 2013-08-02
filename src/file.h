/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#ifndef FILE_H
#define FILE_H

#include "common.h"
#include "ldtypes.h"
#include "history.h"
#include <QObject>

#define curfile LDFile::current()

class History;
class OpenProgressDialog;

namespace LDPaths {
	void initPaths();
	bool tryConfigure (str path);
	
	str ldconfig();
	str prims();
	str parts();
	str getError();
}

// =============================================================================
// LDFile
//
// The LDFile class stores a file opened in LDForge either as a editable file
// for the user or for subfile caching. Its methods handle file input and output.
//
// A file is implicit when they are opened automatically for caching purposes
// and are hidden from the user. User-opened files are explicit (not implicit).
// =============================================================================
class LDFile : public QObject {
	Q_OBJECT
	READ_PROPERTY (List<LDObject*>, objs, setObjects)
	READ_PROPERTY (History, history, setHistory)
	READ_PROPERTY (List<LDObject*>, vertices, setVertices)
	PROPERTY (str, name, setName)
	PROPERTY (bool, implicit, setImplicit)
	PROPERTY (List<LDObject*>, cache, setCache)
	PROPERTY (long, savePos, setSavePos)
	DECLARE_PROPERTY (QListWidgetItem*, listItem, setListItem)
	
public:
	typedef List<LDObject*>::it it;
	typedef List<LDObject*>::c_it c_it;
	
	LDFile();
	~LDFile();
	
	ulong addObject (LDObject* obj);                 // Adds an object to this file at the end of the file.
	void forgetObject (LDObject* obj);               // Deletes the given object from the object chain.
	bool hasUnsavedChanges() const;                  // Does this file have unsaved changes?
	void insertObj (const ulong pos, LDObject* obj);
	ulong numObjs() const;
	LDObject* object (ulong pos) const;
	LDObject* obj (ulong pos) const;
	bool save (str path = "");                       // Saves this file to disk.
	bool safeToClose();                              // Perform safety checks. Do this before closing any files!
	void setObject (ulong idx, LDObject* obj);

	LDFile& operator<< (LDObject* obj) {
		addObject (obj);
		return *this;
	}
	
	LDFile& operator<< (List<LDObject*> objs);
	
	it begin() {
		return PROP_NAME (objs).begin();
	}
	
	c_it begin() const {
		return PROP_NAME (objs).begin();
	}
	
	it end() {
		return PROP_NAME (objs).end();
	}
	
	c_it end() const {
		return PROP_NAME (objs).end();
	}
	
	void openHistory() {
		m_history.open();
	}
	
	void closeHistory() {
		m_history.close();
	}
	
	void undo() {
		m_history.undo();
	}
	
	void redo() {
		m_history.redo();
	}
	
	void clearHistory() {
		m_history.clear();
	}
	
	void addToHistory (AbstractHistoryEntry* entry) {
		m_history << entry;
	}
	
	static void closeUnused();
	static LDFile* current();
	static void setCurrent (LDFile* f);
	static void closeInitialFile();
	static int countExplicitFiles();
	str getShortName();
	
private:
	static LDFile* m_curfile;
};

// Close all current loaded files and start off blank.
void newFile();

// Opens the given file as the main file. Everything is closed first.
void openMainFile (str path);

// Finds an OpenFile by name or null if not open
LDFile* findLoadedFile (str name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
LDFile* openDATFile (str path, bool search);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
File* openLDrawFile (str relpath, bool subdirs);

// Close all open files, whether user-opened or subfile caches.
void closeAll();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* parseLine (str line);

// Retrieves the pointer to - or loads - the given subfile.
LDFile* getFile (str filename);

// Re-caches all subfiles.
void reloadAllSubfiles();

// Is it safe to close all files?
bool safeToCloseAll();

List<LDObject*> loadFileContents (File* f, ulong* numWarnings, bool* ok = null);

extern List<LDFile*> g_loadedFiles;

void addRecentFile (str path);
str basename (str path);
str dirname (str path);

extern List<LDFile*> g_loadedFiles; // Vector of all currently opened files.

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// FileLoader
//
// Loads the given file and parses it to LDObjects using parseLine. It's a
// separate class so as to be able to do the work in a separate thread.
// =============================================================================
class FileLoader : public QObject {
	Q_OBJECT
	READ_PROPERTY (List<LDObject*>, objs, setObjects)
	READ_PROPERTY (bool, done, setDone)
	READ_PROPERTY (ulong, progress, setProgress)
	READ_PROPERTY (bool, aborted, setAborted)
	PROPERTY (List<str>, lines, setLines)
	PROPERTY (ulong*, warningsPointer, setWarningsPointer)
	PROPERTY (bool, concurrent, setConcurrent)
	
public slots:
	void start();
	void abort();
	
private:
	OpenProgressDialog* dlg;
	
private slots:
	void work (int i);
	
signals:
	void progressUpdate (int progress);
	void workDone();
};

#endif // FILE_H