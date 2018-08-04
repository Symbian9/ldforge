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
#include <set>
#include "main.h"
#include "hierarchyelement.h"

class Model;

class DocumentManager : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	using Documents = std::set<std::unique_ptr<LDDocument>>;
	using iterator = Documents::iterator;

	DocumentManager (QObject* parent = nullptr);
	~DocumentManager();

	void addRecentFile (QString path);
	const Documents& allDocuments() const;
	Documents::iterator begin();
	void clear();
	LDDocument* createNew(bool implicit);
	Documents::iterator end();
	QString findDocument(QString name) const;
	iterator findDocumentByName(const QString& name);
	LDDocument* getDocumentByName (QString filename);
	bool isSafeToCloseAll();
	void loadLogoedStuds();
	LDDocument* openDocument(QString path, bool search, bool implicit);
	void openMainModel (QString path);
	bool preInline (LDDocument* doc, Model& model, bool deep, bool renderinline);

signals:
	void documentCreated(LDDocument* document, bool cache);
	void documentClosed(LDDocument* document);

private:
	Q_SLOT void printParseErrorMessage(QString message);

	std::set<std::unique_ptr<LDDocument>> m_documents;
	bool m_loadingMainFile;
	bool m_isLoadingLogoedStuds;
	LDDocument* m_logoedStud;
	LDDocument* m_logoedStud2;
};
