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
#include <QSet>
#include "main.h"
#include "hierarchyelement.h"

class Model;

class DocumentManager : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	using Documents = QSet<LDDocument*>;

	DocumentManager (QObject* parent = nullptr);
	~DocumentManager();

	void addRecentFile (QString path);
	const Documents& allDocuments() const { return m_documents; }
	void clear();
	LDDocument* createNew();
	LDDocument* findDocumentByName (QString name);
	QString findDocumentPath (QString relpath, bool subdirs);
	LDDocument* getDocumentByName (QString filename);
	bool isSafeToCloseAll();
	void loadFileContents(QIODevice* fp, Model& model, int* numWarnings, bool* ok);
	void loadLogoedStuds();
	LDDocument* openDocument (QString path, bool search, bool implicit, LDDocument* fileToOverride = nullptr,
							  bool* aborted = nullptr);
	QFile* openLDrawFile (QString relpath, bool subdirs, QString* pathpointer);
	void openMainModel (QString path);
	bool preInline (LDDocument* doc, Model& model, bool deep, bool renderinline);

private:
	Documents m_documents;
	bool m_loadingMainFile;
	bool m_isLoadingLogoedStuds;
	LDDocument* m_logoedStud;
	LDDocument* m_logoedStud2;
};

QString Basename (QString path);
QString Dirname (QString path);
static const QStringList g_specialSubdirectories ({ "s", "48", "8" });