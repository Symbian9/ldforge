/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
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
class DocumentManager;

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
	enum Flag
	{
		IsCache = (1 << 0),
		VerticesOutdated = (1 << 1),
		NeedsVertexMerge = (1 << 2),
		IsBeingDestroyed = (1 << 3),
		NeedsRecache = (1 << 4), // The next polygon inline of this document rebuilds stored polygon data.
		NeedsTriangleRecount = (1 << 5),
	};

	Q_DECLARE_FLAGS(Flags, Flag)

	LDDocument (DocumentManager* parent);
	~LDDocument();

	void addHistoryStep();
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
	const QSet<Vertex>& inlineVertices();
	void insertObject (int pos, LDObject* obj);
	bool isCache() const;
	bool isSafeToClose();
	QString name() const;
	void needVertexMerge();
	const LDObjectList& objects() const;
	void openForEditing();
	const QList<LDPolygon>& polygonData() const;
	void recountTriangles();
	void redo();
	void redoVertices();
	void reloadAllSubfiles();
	void removeFromSelection (LDObject* obj);
	bool save (QString path = "", int64* sizeptr = nullptr);
	long savePosition() const;
	void setDefaultName (QString value);
	void setFullPath (QString value);
	void setName (QString value);
	void setObjectAt (int idx, LDObject* obj);
	void setSavePosition (long value);
	void setTabIndex (int value);
	void swapObjects (LDObject* one, LDObject* other);
	int tabIndex() const;
	int triangleCount();
	void undo();
	void vertexChanged (const Vertex& a, const Vertex& b);

	static QString shortenName (QString a); // Turns a full path into a relative path

private:
	QString m_name;
	QString m_fullPath;
	QString m_defaultName;
	LDObjectList m_objects;
	EditHistory* m_history;
	Flags m_flags;
	long m_savePosition;
	int m_tabIndex;
	int m_triangleCount;
	QList<LDPolygon> m_polygonData;
	QMap<LDObject*, QSet<Vertex>> m_objectVertices;
	QSet<Vertex> m_vertices;
	LDObjectList m_sel;
	LDGLData* m_gldata;
	DocumentManager* m_manager;

	DEFINE_FLAG_ACCESS_METHODS(m_flags)
	void addKnownVertices (LDObject* obj);
	void mergeVertices();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LDDocument::Flags)

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* ParseLine (QString line);
