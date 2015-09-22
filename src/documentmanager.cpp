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

#include <QFileInfo>
#include <QFile>
#include "documentmanager.h"
#include "ldDocument.h"
#include "mainwindow.h"
#include "partdownloader.h"

enum
{
	MAX_RECENT_FILES = 10
};

DocumentManager::DocumentManager (QObject* parent) :
	QObject (parent),
	g_loadingMainFile (false),
	g_loadingLogoedStuds (false),
	g_logoedStud (nullptr),
	g_logoedStud2 (nullptr) {}

DocumentManager::~DocumentManager()
{
	clear();
}

void DocumentManager::clear()
{
	for (DocumentMapIterator it (m_documents); it.hasNext();)
	{
		LDDocument* document = it.next().value();
		document->close();
		delete document;
	}

	m_documents.clear();
}

LDDocument* DocumentManager::getDocumentByName (QString filename)
{
	// Try find the file in the list of loaded files
	LDDocument* doc = findDocumentByName (filename);

	// If it's not loaded, try open it
	if (not doc)
		doc = openDocument (filename, true, true);

	return doc;
}

void DocumentManager::openMainModel (QString path)
{
	// If there's already a file with the same name, this file must replace it.
	LDDocument* documentToReplace = nullptr;
	LDDocument* file = nullptr;
	QString shortName = LDDocument::shortenName (path);

	for (LDDocument* doc : m_window->allDocuments())
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

	g_loadingMainFile = true;

	// If we're replacing an existing document, clear the document and
	// make it ready for being loaded to.
	if (documentToReplace)
	{
		file = documentToReplace;
		file->clear();
	}

	bool aborted;
	file = openDocument (path, false, false, file, &aborted);

	if (file == nullptr)
	{
		if (not aborted)
		{
			// Tell the user loading failed.
			setlocale (LC_ALL, "C");
			Critical (format (tr ("Failed to open %1: %2"), path, strerror (errno)));
		}

		g_loadingMainFile = false;
		return;
	}

	file->openForEditing();
	m_window->closeInitialDocument();
	m_window->changeDocument (file);
	m_window->doFullRefresh();
	addRecentFile (path);
	g_loadingMainFile = false;

	// If there were problems loading subfile references, try see if we can find these
	// files on the parts tracker.
	QStringList unknowns;

	for (LDObject* obj : file->objects())
	{
		if (obj->type() != OBJ_Error or static_cast<LDError*> (obj)->fileReferenced().isEmpty())
			continue;

		unknowns << static_cast<LDError*> (obj)->fileReferenced();
	}

	if (m_window->configBag()->tryDownloadMissingFiles() and not unknowns.isEmpty())
	{
		PartDownloader dl (m_window);
		dl.setSourceType (PartDownloader::PartsTracker);
		dl.setPrimaryFile (file);

		for (QString const& unknown : unknowns)
			dl.downloadFromPartsTracker (unknown);

		dl.exec();
		dl.checkIfFinished();
		file->reloadAllSubfiles();
	}
}

LDDocument* DocumentManager::findDocumentByName (QString name)
{
	for (DocumentMapIterator it (m_documents); it.hasNext();)
	{
		LDDocument* document = it.next().value();

		if (isOneOf (name, document->name(), document->defaultName()))
			return document;
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

QString DocumentManager::findDocumentPath (QString relativePath, bool subdirs)
{
	QString fullPath;

	// LDraw models use backslashes as path separators. Replace those into forward slashes for Qt.
	relativePath.replace ("\\", "/");

	// Try find it relative to other currently open documents. We want a file in the immediate vicinity of a current
	// part model to override stock LDraw stuff.
	QString reltop = Basename (Dirname (relativePath));

	for (LDDocument* doc : m_window->allDocuments())
	{
		QString partpath = format ("%1/%2", Dirname (doc->fullPath()), relativePath);
		QFile f (partpath);

		if (f.exists())
		{
			// ensure we don't mix subfiles and 48-primitives with non-subfiles and non-48
			QString proptop = Basename (Dirname (partpath));

			bool bogus = false;

			for (QString s : g_specialSubdirectories)
			{
				if ((proptop == s and reltop != s) or (reltop == s and proptop != s))
				{
					bogus = true;
					break;
				}
			}

			if (not bogus)
				return partpath;
		}
	}

	if (QFile::exists (relativePath))
		return relativePath;

	// Try with just the LDraw path first
	fullPath = format ("%1" DIRSLASH "%2", m_window->configBag()->lDrawPath(), relativePath);

	if (QFile::exists (fullPath))
		return fullPath;

	if (subdirs)
	{
		// Look in sub-directories: parts and p. Also look in the download path, since that's where we download parts
		// from the PT to.
		QStringList dirs = { m_window->configBag()->lDrawPath(), m_window->configBag()->downloadFilePath() };
		for (const QString& topdir : dirs)
		{
			for (const QString& subdir : QStringList ({ "parts", "p" }))
			{
				fullPath = format ("%1" DIRSLASH "%2" DIRSLASH "%3", topdir, subdir, relativePath);

				if (QFile::exists (fullPath))
					return fullPath;
			}
		}
	}

	// Did not find the file.
	return "";
}

QFile* DocumentManager::openLDrawFile (QString relpath, bool subdirs, QString* pathpointer)
{
	print ("Opening %1...\n", relpath);
	QString path = findDocumentPath (relpath, subdirs);

	if (pathpointer)
		*pathpointer = path;

	if (path.isEmpty())
		return nullptr;

	QFile* fp = new QFile (path);

	if (fp->open (QIODevice::ReadOnly))
		return fp;

	fp->deleteLater();
	return nullptr;
}

LDObjectList DocumentManager::loadFileContents (QFile* fp, int* numWarnings, bool* ok)
{
	LDObjectList objs;

	if (numWarnings)
		*numWarnings = 0;

	DocumentLoader* loader = new DocumentLoader (g_loadingMainFile);
	loader->read (fp);
	loader->start();

	// After start() returns, if the loader isn't done yet, it's delaying
	// its next iteration through the event loop. We need to catch this here
	// by telling the event loop to tick, which will tick the file loader again.
	// We keep doing this until the file loader is ready.
	while (not loader->isDone())
		qApp->processEvents();

	// If we wanted the success value, supply that now
	if (ok)
		*ok = not loader->hasAborted();

	objs = loader->objects();
	delete loader;
	return objs;
}

LDDocument* DocumentManager::openDocument (QString path, bool search, bool implicit, LDDocument* fileToOverride, bool* aborted)
{
	// Convert the file name to lowercase when searching because some parts contain subfile
	// subfile references with uppercase file names. I'll assume here that the library will always
	// use lowercase file names for the part files.
	QFile* fp;
	QString fullpath;

	if (search)
	{
		fp = openLDrawFile (path.toLower(), true, &fullpath);
	}
	else
	{
		fp = new QFile (path);
		fullpath = path;

		if (not fp->open (QIODevice::ReadOnly))
		{
			delete fp;
			return nullptr;
		}
	}

	if (not fp)
		return nullptr;

	LDDocument* load = (fileToOverride ? fileToOverride : m_window->newDocument (implicit));
	load->setFullPath (fullpath);
	load->setName (LDDocument::shortenName (load->fullPath()));

	// Loading the file shouldn't count as actual edits to the document.
	load->history()->setIgnoring (true);

	int numWarnings;
	bool ok;
	LDObjectList objs = loadFileContents (fp, &numWarnings, &ok);
	fp->close();
	fp->deleteLater();

	if (aborted)
		*aborted = ok == false;

	if (not ok)
	{
		load->close();
		return nullptr;
	}

	load->addObjects (objs);

	if (g_loadingMainFile)
	{
		m_window->changeDocument (load);
		m_window->renderer()->setDocument (load);
		print (tr ("File %1 parsed successfully (%2 errors)."), path, numWarnings);
	}

	load->history()->setIgnoring (false);
	return load;
}

void DocumentManager::closeAllDocuments()
{
	for (LDDocument* file : m_window->allDocuments())
		file->close();
}

void DocumentManager::addRecentFile (QString path)
{
	QStringList recentFiles = m_window->configBag()->recentFiles();
	int idx = recentFiles.indexOf (path);

	// If this file already is in the list, pop it out.
	if (idx != -1)
	{
		if (idx == recentFiles.size() - 1)
			return; // first recent file - abort and do nothing

		recentFiles.removeAt (idx);
	}

	// If there's too many recent files, drop one out.
	while (recentFiles.size() > (MAX_RECENT_FILES - 1))
		recentFiles.removeAt (0);

	// Add the file
	recentFiles << path;
	m_window->configBag()->setRecentFiles (recentFiles);
	m_window->syncSettings();
	m_window->updateRecentFilesMenu();
}

bool DocumentManager::isSafeToCloseAll()
{
	for (LDDocument* f : m_window->allDocuments())
	{
		if (not f->isSafeToClose())
			return false;
	}

	return true;
}

void DocumentManager::loadLogoedStuds()
{
	if (g_loadingLogoedStuds or (g_logoedStud and g_logoedStud2))
		return;

	g_loadingLogoedStuds = true;
	g_logoedStud = openDocument ("stud-logo.dat", true, true);
	g_logoedStud2 = openDocument ("stud2-logo.dat", true, true);
	print (tr ("Logoed studs loaded.\n"));
	g_loadingLogoedStuds = false;
}

bool DocumentManager::preInline (LDDocument* doc, LDObjectList& objs)
{
	// Possibly substitute with logoed studs:
	// stud.dat -> stud-logo.dat
	// stud2.dat -> stud-logo2.dat
	if (m_config->useLogoStuds() and renderinline)
	{
		// Ensure logoed studs are loaded first
		loadLogoedStuds();

		if (doc->name() == "stud.dat" and g_logoedStud)
			return g_logoedStud->inlineContents (deep, renderinline);
		else if (doc->name() == "stud2.dat" and g_logoedStud2)
			return g_logoedStud2->inlineContents (deep, renderinline);
	}
}
