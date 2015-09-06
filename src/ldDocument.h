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

class EditHistory;
class OpenProgressDialog;
struct LDGLData;
class GLCompiler;

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
class LDDocument : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	LDDocument (QObject* parent);
	~LDDocument();

	void addHistoryStep();
	void addKnownVertices (LDObject* obj);
	int addObject (LDObject* obj);
	void addObjects (const LDObjectList& objs);
	void addToHistory (AbstractHistoryEntry* entry);
	void addToSelection (LDObject* obj);
	void clear();
	void clearHistory();
	void clearSelection();
	void close();
	QString defaultName() const;
	void forgetObject (LDObject* obj);
	QString fullPath();
	QString getDisplayName();
	LDObject* getObject (int pos) const;
	int getObjectCount() const;
	const LDObjectList& getSelection() const;
	LDGLData* glData();
	bool hasUnsavedChanges() const;
	EditHistory* history() const;
	void initializeCachedData();
	LDObjectList inlineContents (bool deep, bool renderinline);
	QList<LDPolygon> inlinePolygons();
	const QVector<Vertex>& inlineVertices();
	void insertObj (int pos, LDObject* obj);
	bool isCache() const;
	bool isSafeToClose();
	void mergeVertices();
	QString name() const;
	void needVertexMerge();
	const LDObjectList& objects() const;
	void openForEditing();
	const QList<LDPolygon>& polygonData() const;
	void redo();
	void redoVertices();
	void reloadAllSubfiles();
	void removeFromSelection (LDObject* obj);
	bool save (QString path = "", int64* sizeptr = nullptr);
	long savePosition() const;
	void setDefaultName (QString value);
	void setFullPath (QString value);
	void setImplicit (bool value);
	void setName (QString value);
	void setObject (int idx, LDObject* obj);
	void setSavePosition (long value);
	void setTabIndex (int value);
	void swapObjects (LDObject* one, LDObject* other);
	int tabIndex() const;
	void undo();
	void vertexChanged (const Vertex& a, const Vertex& b);

	static QString shortenName (QString a); // Turns a full path into a relative path

private:
	QString m_name;
	QString m_fullPath;
	QString m_defaultName;
	LDObjectList m_objects;
	EditHistory* m_history;
	bool m_isCache;
	bool m_verticesOutdated;
	bool m_needVertexMerge;
	bool m_needsReCache; // If true, next polygon inline of this document rebuilds stored polygon data.
	bool m_beingDestroyed;
	long m_savePosition;
	int m_tabIndex;
	QList<LDPolygon> m_polygonData;
	QMap<LDObject*, QVector<Vertex>> m_objectVertices;
	QVector<Vertex> m_vertices;
	LDObjectList m_sel;
	LDGLData* m_gldata;
};

// Opens the given file as the main file. Everything is closed first.
void OpenMainModel (QString path);

// Finds an OpenFile by name or null if not open
LDDocument* FindDocument (QString name);

// Opens the given file and parses the LDraw code within. Returns a pointer
// to the opened file or null on error.
LDDocument* OpenDocument (QString path, bool search, bool implicit, LDDocument* fileToOverride = nullptr, bool* aborted = nullptr);

// Opens the given file and returns a pointer to it, potentially looking in /parts and /p
QFile* OpenLDrawFile (QString relpath, bool subdirs, QString* pathpointer = nullptr);

// Close all open files, whether user-opened or subfile caches.
void CloseAllDocuments();

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* ParseLine (QString line);

// Retrieves the pointer to the given document by file name. Document is loaded
// from file if necessary. Can return null if neither succeeds.
LDDocument* GetDocument (QString filename);

// Is it safe to close all files?
bool IsSafeToCloseAll();

LDObjectList LoadFileContents (QFile* f, int* numWarnings, bool* ok = nullptr);

const LDObjectList& selectedObjects();
void AddRecentFile (QString path);
void LoadLogoStuds();
QString Basename (QString path);
QString Dirname (QString path);
