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
#include "linetypes/modelobject.h"
#include "editHistory.h"
#include "glShared.h"
#include "model.h"
#include "hierarchyelement.h"

struct LDGLData;
class DocumentManager;

struct LDHeader
{
	struct HistoryEntry
	{
		QDate date;
		QString author;
		QString description;
		enum { UserName, RealName } authorType = UserName;
	};
	enum FileType
	{
		Part,
		Subpart,
		Shortcut,
		Primitive,
		Primitive_8,
		Primitive_48,
		Configuration,
	} type = Part;
	enum Qualifier
	{
		Alias = 1 << 0,
		Physical_Color = 1 << 1,
		Flexible_Section = 1 << 2,
	};
	QFlags<Qualifier> qualfiers;
	QString description;
	QString name;
	struct
	{
		QString realName;
		QString userName;
	} author;
	QString category;
	QString cmdline;
	QStringList help;
	QStringList keywords;
	QVector<HistoryEntry> history;
	enum
	{
		NoWinding,
		CounterClockwise,
		Clockwise,
	} winding = NoWinding;
	enum
	{
		CaLicense,
		NonCaLicense
	} license = CaLicense;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFlags<LDHeader::Qualifier>)

decltype(LDHeader::winding) operator^(
	decltype(LDHeader::winding) one,
	decltype(LDHeader::winding) other
);

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
class LDDocument : public Model, public HierarchyElement
{
	Q_OBJECT

public:
	LDDocument (DocumentManager* parent);
	~LDDocument();

	void addHistoryStep();
	void clearHistory();
	void close();
	QString defaultName() const;
	QString fullPath();
	QString getDisplayName();
	bool hasUnsavedChanges() const;
	EditHistory* history() const;
	void initializeCachedData();
	void inlineContents(Model& model, bool deep, bool renderinline);
	QList<LDPolygon> inlinePolygons();
	const QSet<Vertex>& inlineVertices();
	bool isFrozen() const;
	bool isSafeToClose();
	QString name() const;
	void objectRemoved(LDObject* object, int index);
	const QList<LDPolygon>& polygonData() const;
	void recountTriangles();
	void redo();
	void redoVertices();
	bool save (QString path = "", qint64* sizeptr = nullptr);
	long savePosition() const;
	void setDefaultName (QString value);
	void setFrozen(bool value);
	void setFullPath (QString value);
	void setHeader(LDHeader&& header);
	void setName (QString value);
	void setSavePosition (long value);
	void setTabIndex (int value);
	int tabIndex() const;
	void undo();
	void vertexChanged (const Vertex& a, const Vertex& b);

	static QString shortenName (QString a); // Turns a full path into a relative path

protected:
	LDObject* withdrawAt(int position);

private:
	LDHeader m_header;
	QString m_name;
	QString m_fullPath;
	QString m_defaultName;
	EditHistory* m_history;
	bool m_isFrozen = true; // Document may not be modified
	bool m_verticesOutdated = true;
	bool m_isBeingDestroyed = false;
	bool m_needsRecache = true; // The next polygon inline of this document rebuilds stored polygon data.
	long m_savePosition;
	int m_tabIndex;
	int m_triangleCount;
	QList<LDPolygon> m_polygonData;
	QMap<LDObject*, QSet<Vertex>> m_objectVertices;
	QSet<Vertex> m_vertices;
	DocumentManager* m_manager;

	template<typename T, typename... Args>
	void addToHistory(Args&&... args)
	{
		m_history->add<T>(args...);
	}

private slots:
	void objectChanged(const LDObjectState &before, const LDObjectState &after);
	void handleNewObject(const QModelIndex& index);
	void handleImminentObjectRemoval(const QModelIndex& index);
};

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* ParseLine (QString line);
