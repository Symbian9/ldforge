/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

#pragma once
#include <QObject>
#include "main.h"
#include "ldObject.h"
#include "editHistory.h"
#include "glShared.h"

class History;
class OpenProgressDialog;
struct LDGLData;
class GLCompiler;

//
// Flags for LDDocument
//
enum LDDocumentFlag
{
	DOCF_IsBeingDestroyed = (1 << 0),
};

Q_DECLARE_FLAGS (LDDocumentFlags, LDDocumentFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS (LDDocumentFlags)

//
// This class stores a document either as a editable file for the user or for
// subfile caching.
//
// A document is implicit when they are opened automatically for caching purposes
// and are hidden from the user. User-opened files are explicit (not implicit).
//
// The default name is a placeholder, initially suggested name for a file. The
// primitive generator uses this to give initial names to primitives.
//
class LDDocument
{
public:
	PROPERTY (public,	QString,				name,			setName,			STOCK_WRITE)
	PROPERTY (private,	LDObjectList,		objects, 		setObjects,			STOCK_WRITE)
	PROPERTY (private,	LDObjectList,		cache, 			setCache,			STOCK_WRITE)
	PROPERTY (private,	History*,			history,		setHistory,			STOCK_WRITE)
	PROPERTY (public,	QString,				fullPath,		setFullPath,		STOCK_WRITE)
	PROPERTY (public,	QString,				defaultName,	setDefaultName,		STOCK_WRITE)
	PROPERTY (public,	bool,				isImplicit,		setImplicit,		CUSTOM_WRITE)
	PROPERTY (public,	long,				savePosition,	setSavePosition,	STOCK_WRITE)
	PROPERTY (public,	int,				tabIndex,		setTabIndex,		STOCK_WRITE)
	PROPERTY (public,	QList<LDPolygon>,	polygonData,	setPolygonData,		STOCK_WRITE)
	PROPERTY (private,	LDDocumentFlags,	flags,			setFlags,			STOCK_WRITE)

	QMap<LDObject*, QVector<Vertex>> m_objectVertices;
	QVector<Vertex> m_vertices;
	bool m_verticesOutdated;
	bool m_needVertexMerge;

public:
	LDDocument();
	~LDDocument();

	int addObject (LDObject* obj); // Adds an object to this file at the end of the file.
	void addObjects (const LDObjectList& objs);
	void clearSelection();
	void forgetObject (LDObject* obj); // Deletes the given object from the object chain.
	QString getDisplayName();
	const LDObjectList& getSelection() const;
	bool hasUnsavedChanges() const; // Does this document have unsaved changes?
	void initializeCachedData();
	LDObjectList inlineContents (bool deep, bool renderinline);
	void insertObj (int pos, LDObject* obj);
	int getObjectCount() const;
	LDObject* getObject (int pos) const;
	bool save (QString path = "", int64* sizeptr = null); // Saves this file to disk.
	void swapObjects (LDObject* one, LDObject* other);
	bool isSafeToClose(); // Perform safety checks. Do this before closing any files!
	void setObject (int idx, LDObject* obj);
	QList<LDPolygon> inlinePolygons();
	void vertexChanged (const Vertex& a, const Vertex& b);
	const QVector<Vertex>& inlineVertices();
	void clear();
	void addKnownVertices (LDObject* obj);
	void redoVertices();
	void needVertexMerge();
	void reloadAllSubfiles();

	inline LDDocument& operator<< (LDObject* obj)
	{
		addObject (obj);
		return *this;
	}

	inline void addHistoryStep()
	{
		history()->addStep();
	}

	inline void undo()
	{
		history()->undo();
	}

	inline void redo()
	{
		history()->redo();
	}

	inline void clearHistory()
	{
		history()->clear();
	}

	inline void addToHistory (AbstractHistoryEntry* entry)
	{
		*history() << entry;
	}

	inline void dismiss()
	{
		setImplicit (true);
	}

	static LDDocument* current();
	static void setCurrent (LDDocument* f);
	static void closeInitialFile();
	static int countExplicitFiles();
	static LDDocument* createNew();

	// Turns a full path into a relative path
	static QString shortenName (QString a);
	static QList<LDDocument*> const& explicitDocuments();
	void mergeVertices();

protected:
	void addToSelection (LDObject* obj);
	void removeFromSelection (LDObject* obj);

	LDGLData* getGLData()
	{
		return m_gldata;
	}

	friend class LDObject;
	friend class GLRenderer;

private:
	LDObjectList			m_sel;
	LDGLData*				m_gldata;

	// If set to true, next polygon inline of this document discards the
	// stored polygon data and re-builds it.
	bool					m_needsReCache;
};

inline LDDocument* CurrentDocument()
{
	return LDDocument::current();
}

// Close all current loaded files and start off blank.
void newFile();

// Opens the given file as the main file. Everything is closed first.
void OpenMainModel (QString path);

// Finds an OpenFile by name or null if not open
LDDocument* FindDocument (QString name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
LDDocument* OpenDocument (QString path, bool search, bool implicit, LDDocument* fileToOverride = nullptr);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
QFile* OpenLDrawFile (QString relpath, bool subdirs, QString* pathpointer = null);

// Close all open files, whether user-opened or subfile caches.
void CloseAllDocuments();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* ParseLine (QString line);

// Retrieves the pointer to the given document by file name. Document is loaded
// from file if necessary. Can return null if neither succeeds.
LDDocument* GetDocument (QString filename);

// Is it safe to close all files?
bool IsSafeToCloseAll();

LDObjectList LoadFileContents (QFile* f, int* numWarnings, bool* ok = null);

inline const LDObjectList& Selection()
{
	return CurrentDocument()->getSelection();
}

void AddRecentFile (QString path);
void LoadLogoStuds();
QString Basename (QString path);
QString Dirname (QString path);

// =============================================================================
//
// LDFileLoader
//
// Loads the given file and parses it to LDObjects using parseLine. It's a
// separate class so as to be able to do the work progressively through the
// event loop, allowing the program to maintain responsivity during loading.
//
class LDFileLoader : public QObject
{
	Q_OBJECT
	PROPERTY (private,	LDObjectList,	objects,		setObjects,			STOCK_WRITE)
	PROPERTY (private,	bool,			isDone,			setDone,			STOCK_WRITE)
	PROPERTY (private,	int,			progress,		setProgress,		STOCK_WRITE)
	PROPERTY (private,	bool,			isAborted,		setAborted,			STOCK_WRITE)
	PROPERTY (public,	QStringList,	lines,			setLines,			STOCK_WRITE)
	PROPERTY (public,	int*,			warnings,		setWarnings,		STOCK_WRITE)
	PROPERTY (public,	bool,			isOnForeground,	setOnForeground,	STOCK_WRITE)

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
