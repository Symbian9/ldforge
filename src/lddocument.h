/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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
#include "model.h"
#include "hierarchyelement.h"

struct LDGLData;
class DocumentManager;
class EditHistory;

struct LDHeader
{
	struct HistoryEntry
	{
		QDate date;
		QString author;
		QString description;
	};
	enum FileType
	{
		NoHeader,
		Part,
		Subpart,
		Shortcut,
		Primitive,
		Primitive_8,
		Primitive_48,
		Configuration,
	} type = NoHeader;
	enum Qualifier
	{
		Alias = 1 << 0,
		Physical_Color = 1 << 1,
		Flexible_Section = 1 << 2,
	};
	QFlags<Qualifier> qualfiers;
	QString description;
	QString name;
	QString author;
	QString category;
	QString cmdline;
	QString help;
	QString keywords;
	QVector<HistoryEntry> history;
	enum
	{
		UnspecifiedLicense,
		CaLicense,
		NonCaLicense
	} license = UnspecifiedLicense;
	static decltype(license) defaultLicense();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFlags<LDHeader::Qualifier>)

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
	LDHeader header;

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
	QVector<LDPolygon> inlinePolygons();
	const QSet<Vertex>& inlineVertices();
	bool isFrozen() const;
	bool isSafeToClose();
	QString name() const;
	void objectRemoved(LDObject* object, int index);
	const QVector<LDPolygon>& polygonData() const;
	void recountTriangles();
	void redo();
	void redoVertices();
	bool save (QString path = "", qint64* sizeptr = nullptr);
	long savePosition() const;
	void setDefaultName (QString value);
	void setFrozen(bool value);
	void setFullPath (QString value);
	void setName (QString value);
	void setSavePosition (long value);
	void setTabIndex (int value);
	int tabIndex() const;
	void undo();
	void vertexChanged (const Vertex& a, const Vertex& b);

	static QString shortenName(const class QFileInfo& path); // Turns a full path into a relative path

protected:
	LDObject* withdrawAt(int position);

private:
	QString m_fullPath;
	QString m_defaultName;
	EditHistory* m_history;
	bool m_isFrozen = true; // Document may not be modified
	bool m_verticesOutdated = true;
	bool m_isBeingDestroyed = false;
	bool m_needsRecache = true; // The next polygon inline of this document rebuilds stored polygon data.
	bool m_isInlining = false;
	long m_savePosition;
	int m_tabIndex;
	int m_triangleCount;
	QVector<LDPolygon> m_polygonData;
	QMap<LDObject*, QSet<Vertex>> m_objectVertices;
	QSet<Vertex> m_vertices;
	DocumentManager* m_manager;

private slots:
	void objectChanged(const LDObjectState &before, const LDObjectState &after);
	void handleNewObject(const QModelIndex& index);
	void handleImminentObjectRemoval(const QModelIndex& index);
};

// Parses a string line containing an LDraw object and returns the object parsed.
LDObject* ParseLine (QString line);
