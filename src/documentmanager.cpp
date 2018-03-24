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

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include "documentmanager.h"
#include "lddocument.h"
#include "partdownloader.h"
#include "parser.h"
#include "editHistory.h"

const QStringList DocumentManager::specialSubdirectories {"s", "48", "8"};

enum
{
	MaxRecentFiles = 10
};

DocumentManager::DocumentManager (QObject* parent) :
	QObject (parent),
	HierarchyElement (parent),
	m_loadingMainFile (false),
	m_isLoadingLogoedStuds (false),
	m_logoedStud (nullptr),
	m_logoedStud2 (nullptr) {}

DocumentManager::~DocumentManager()
{
	clear();
}

void DocumentManager::clear()
{
	for (LDDocument* document : m_documents)
	{
		document->close();
		delete document;
	}

	m_documents.clear();
}

LDDocument* DocumentManager::getDocumentByName (QString filename)
{
	if (not filename.isEmpty())
	{
		LDDocument* doc = findDocumentByName (filename);

		if (doc == nullptr)
		{
			bool tmp = m_loadingMainFile;
			m_loadingMainFile = false;
			doc = openDocument (filename, true, true);
			m_loadingMainFile = tmp;
		}

		return doc;
	}
	else
	{
		return nullptr;
	}
}

void DocumentManager::openMainModel (QString path)
{
	// If there's already a file with the same name, this file must replace it.
	LDDocument* documentToReplace = nullptr;
	LDDocument* file = nullptr;
	QString shortName = LDDocument::shortenName (path);

	for (LDDocument* doc : m_documents)
	{
		if (doc->name() == shortName)
		{
			documentToReplace = doc;
			break;
		}
	}

	// We cannot open this file if the document this would replace is not
	// safe to close.
	if (documentToReplace and not documentToReplace->isSafeToClose())
		return;

	m_loadingMainFile = true;

	// If we're replacing an existing document, clear the document and
	// make it ready for being loaded to.
	if (documentToReplace)
	{
		file = documentToReplace;
		file->clear();
	}

	file = openDocument (path, false, false, file);

	if (file == nullptr)
	{
		// Tell the user loading failed.
		setlocale (LC_ALL, "C");
		QMessageBox::critical(m_window, tr("Error"), format(tr("Failed to open %1: %2"), path, strerror (errno)));
		m_loadingMainFile = false;
		return;
	}

	m_window->openDocumentForEditing(file);
	m_window->closeInitialDocument();
	m_window->changeDocument (file);
	m_window->doFullRefresh();
	addRecentFile (path);
	m_loadingMainFile = false;

	// If there were problems loading subfile references, try see if we can find these
	// files on the parts tracker.
	QStringList unknowns;

	for (LDObject* obj : file->objects())
	{
		if (obj->type() == LDObjectType::SubfileReference)
		{
			LDSubfileReference* reference = static_cast<LDSubfileReference*>(obj);
			LDDocument* document = reference->fileInfo(this);

			if (document == nullptr)
				unknowns << reference->referenceName();
		}
	}

	if (config::tryDownloadMissingFiles() and not unknowns.isEmpty())
	{
		PartDownloader dl (m_window);
		dl.setSourceType (PartDownloader::PartsTracker);
		dl.setPrimaryFile (file);

		for (QString const& unknown : unknowns)
			dl.downloadFromPartsTracker (unknown);

		dl.exec();
		dl.checkIfFinished();
	}
}

LDDocument* DocumentManager::findDocumentByName (QString name)
{
	if (not name.isEmpty())
	{
		for (LDDocument* document : m_documents)
		{
			if (isOneOf (name, document->name(), document->defaultName()))
				return document;
		}
	}

	return nullptr;
}

QString Dirname (QString path)
{
	int lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos > 0)
		return path.left (lastpos);

#ifndef _WIN32
	if (path[0] == DIRSLASH_CHAR)
		return DIRSLASH;
#endif // _WIN32

	return "";
}

QString Basename (QString path)
{
	int lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos != -1)
		return path.mid (lastpos + 1);

	return path;
}

QString DocumentManager::findDocument(QString name) const
{
	name = name.replace("\\", "/");

	for (const Library& library : config::libraries())
	{
		for (const QString& subdirectory : {"parts", "p"})
		{
			QDir dir {library.path + "/" + subdirectory};

			if (dir.exists(name))
				return QDir::cleanPath(dir.filePath(name));
		}
	}

	return {};
}

void DocumentManager::printParseErrorMessage(QString message)
{
	print(message);
}

LDDocument* DocumentManager::openDocument(
	QString path,
	bool search,
	bool implicit,
	LDDocument* fileToOverride
) {
	if (search and not QFileInfo {path}.exists())
	{
		// Convert the file name to lowercase when searching because some parts contain subfile
		// subfile references with uppercase file names. I'll assume here that the library will
		// always use lowercase file names for the part files.
		path = this->findDocument(path.toLower());
	}

	QFile file {path};

	if (file.open(QIODevice::ReadOnly))
	{
		LDDocument* load = fileToOverride;

		if (fileToOverride == nullptr)
			load = m_window->newDocument(implicit);

		load->setFullPath(path);
		load->setName(LDDocument::shortenName(path));

		// Loading the file shouldn't count as actual edits to the document.
		load->history()->setIgnoring (true);

		Parser parser {file};
		Winding winding = NoWinding;
		load->header = parser.parseHeader(winding);
		load->setWinding(winding);
		parser.parseBody(*load);
		file.close();

		if (m_loadingMainFile)
		{
			int numWarnings = 0;

			for (LDObject* object : load->objects())
			{
				if (object->type() == LDObjectType::Error)
					numWarnings += 1;
			}

			m_window->changeDocument(load);
			print(tr("File %1 opened successfully (%2 errors)."), load->name(), numWarnings);
		}

		load->history()->setIgnoring (false);
		return load;
	}
	else
	{
		return nullptr;
	}
}

void DocumentManager::addRecentFile (QString path)
{
	QStringList recentFiles = config::recentFiles();
	int idx = recentFiles.indexOf (path);

	// If this file already is in the list, pop it out.
	if (idx != -1)
	{
		if (idx == countof(recentFiles) - 1)
			return; // first recent file - abort and do nothing

		recentFiles.removeAt (idx);
	}

	// If there's too many recent files, drop one out.
	while (countof(recentFiles) > (MaxRecentFiles - 1))
		recentFiles.removeAt (0);

	// Add the file
	recentFiles << path;
	config::setRecentFiles (recentFiles);
	settingsObject().sync();
	m_window->updateRecentFilesMenu();
}

bool DocumentManager::isSafeToCloseAll()
{
	for (LDDocument* document : m_documents)
	{
		if (not document->isSafeToClose())
			return false;
	}

	return true;
}

void DocumentManager::loadLogoedStuds()
{
	if (m_isLoadingLogoedStuds or (m_logoedStud and m_logoedStud2))
		return;

	m_isLoadingLogoedStuds = true;
	m_logoedStud = openDocument ("stud-logo.dat", true, true);
	m_logoedStud2 = openDocument ("stud2-logo.dat", true, true);
	m_isLoadingLogoedStuds = false;

	if (m_logoedStud and m_logoedStud2)
		print (tr ("Logoed studs loaded.\n"));
}

bool DocumentManager::preInline (LDDocument* doc, Model& model, bool deep, bool renderinline)
{
	// Possibly substitute with logoed studs:
	// stud.dat -> stud-logo.dat
	// stud2.dat -> stud-logo2.dat
	if (config::useLogoStuds() and renderinline)
	{
		// Ensure logoed studs are loaded first
		loadLogoedStuds();

		if (doc->name() == "stud.dat" and m_logoedStud)
		{
			m_logoedStud->inlineContents(model, deep, renderinline);
			return true;
		}
		else if (doc->name() == "stud2.dat" and m_logoedStud2)
		{
			m_logoedStud2->inlineContents(model, deep, renderinline);
			return true;
		}
	}
	return false;
}

LDDocument* DocumentManager::createNew()
{
	LDDocument* document = new LDDocument (this);
	m_documents.insert (document);
	return document;
}
