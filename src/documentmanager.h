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
#include "main.h"
#include "hierarchyelement.h"

class DocumentManager : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	using DocumentMap = QHash<QString, LDDocument*>;
	using DocumentMapIterator = QHashIterator<QString, LDDocument*>;

	DocumentManager (QObject* parent = nullptr);
	~DocumentManager();

	void addRecentFile (QString path);
	void clear();
	void closeAllDocuments();
	LDDocument* findDocumentByName (QString name);
	QString findDocumentPath (QString relpath, bool subdirs);
	LDDocument* getDocumentByName (QString filename);
	bool isSafeToCloseAll();
	void loadLogoedStuds();
	LDDocument* openDocument (QString path, bool search, bool implicit, LDDocument* fileToOverride, bool* aborted);
	QFile* openLDrawFile (QString relpath, bool subdirs, QString* pathpointer);
	void openMainModel (QString path);
	bool preInline (LDDocument* doc, LDObjectList& objs);

private:
	DocumentMap m_documents;
	bool g_loadingMainFile;
	bool g_loadingLogoedStuds;
	LDDocument* g_logoedStud;
	LDDocument* g_logoedStud2;

	LDObjectList loadFileContents (QFile* fp, int* numWarnings, bool* ok);
};

static const QStringList g_specialSubdirectories ({ "s", "48", "8" });