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

#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QTime>
#include <QApplication>
#include "main.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainwindow.h"
#include "editHistory.h"
#include "glRenderer.h"
#include "glCompiler.h"
#include "partDownloader.h"
#include "ldpaths.h"
#include "dialogs/openprogressdialog.h"

ConfigOption (QStringList RecentFiles)
ConfigOption (bool TryDownloadMissingFiles = false)

static bool g_loadingMainFile = false;
enum { MAX_RECENT_FILES = 10 };
static bool g_aborted = false;
static LDDocument* g_logoedStud;
static LDDocument* g_logoedStud2;
static bool g_loadingLogoedStuds = false;

const QStringList g_specialSubdirectories ({ "s", "48", "8" });

// =============================================================================
//
LDDocument::LDDocument (QObject* parent) :
	QObject (parent),
	HierarchyElement (parent),
	m_history (new History),
	m_isImplicit (true),
	m_flags (0),
	m_verticesOutdated (true),
	m_needVertexMerge (true),
	m_gldata (new LDGLData)
{
	setSavePosition (-1);
	setTabIndex (-1);
	m_history->setDocument (this);
	m_needsReCache = true;
}

// =============================================================================
//
LDDocument::~LDDocument()
{
	m_flags |= DOCF_IsBeingDestroyed;
	delete m_history;
	delete m_gldata;
}

// =============================================================================
//
void LDDocument::setImplicit (bool const& a)
{
	if (m_isImplicit != a)
	{
		m_isImplicit = a;

		if (a == false)
		{
			print ("Opened %1", name());

			// Implicit files are not compiled by the GL renderer. Now that this
			// part is no longer implicit, it needs to be compiled.
			m_window->R()->compiler()->compileDocument (this);
		}
		else
		{
			print ("Closed %1", name());
		}

		m_window->updateDocumentList();

		// If the current document just became implicit (i.e. user closed it), we need to get a new one to show.
		if (currentDocument() == this and a)
			m_window->currentDocumentClosed();
	}
}

// =============================================================================
//
LDDocument* FindDocument (QString name)
{
	for (LDDocument* document : g_win->allDocuments())
	{
		if (isOneOf (name, document->name(), document->defaultName()))
			return document;
	}

	return nullptr;
}

// =============================================================================
//
QString Dirname (QString path)
{
	long lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos > 0)
		return path.left (lastpos);

#ifndef _WIN32
	if (path[0] == DIRSLASH_CHAR)
		return DIRSLASH;
#endif // _WIN32

	return "";
}

// =============================================================================
//
QString Basename (QString path)
{
	long lastpos = path.lastIndexOf (DIRSLASH);

	if (lastpos != -1)
		return path.mid (lastpos + 1);

	return path;
}

// =============================================================================
//
static QString FindDocumentPath (QString relpath, bool subdirs)
{
	QString fullPath;

	// LDraw models use backslashes as path separators. Replace those into forward slashes for Qt.
	relpath.replace ("\\", "/");

	// Try find it relative to other currently open documents. We want a file in the immediate vicinity of a current
	// part model to override stock LDraw stuff.
	QString reltop = Basename (Dirname (relpath));

	for (LDDocument* doc : g_win->allDocuments())
	{
		QString partpath = format ("%1/%2", Dirname (doc->fullPath()), relpath);
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

	if (QFile::exists (relpath))
		return relpath;

	// Try with just the LDraw path first
	fullPath = format ("%1" DIRSLASH "%2", g_win->configBag()->lDrawPath(), relpath);

	if (QFile::exists (fullPath))
		return fullPath;

	if (subdirs)
	{
		// Look in sub-directories: parts and p. Also look in the download path, since that's where we download parts
		// from the PT to.
		QStringList dirs = { g_win->configBag()->lDrawPath(), g_win->configBag()->downloadFilePath() };
		for (const QString& topdir : dirs)
		{
			for (const QString& subdir : QStringList ({ "parts", "p" }))
			{
				fullPath = format ("%1" DIRSLASH "%2" DIRSLASH "%3", topdir, subdir, relpath);

				if (QFile::exists (fullPath))
					return fullPath;
			}
		}
	}

	// Did not find the file.
	return "";
}

// =============================================================================
//
QFile* OpenLDrawFile (QString relpath, bool subdirs, QString* pathpointer)
{
	print ("Opening %1...\n", relpath);
	QString path = FindDocumentPath (relpath, subdirs);

	if (pathpointer != null)
		*pathpointer = path;

	if (path.isEmpty())
		return null;

	QFile* fp = new QFile (path);

	if (fp->open (QIODevice::ReadOnly))
		return fp;

	fp->deleteLater();
	return null;
}

// =============================================================================
//
void LDFileLoader::start()
{
	setDone (false);
	setProgress (0);
	setAborted (false);

	if (isOnForeground())
	{
		g_aborted = false;

		// Show a progress dialog if we're loading the main ldDocument.here so we can
		// show progress updates and keep the WM posted that we're still here.
		// Of course we cannot exec() the dialog because then the dialog would
		// block.
		dlg = new OpenProgressDialog (g_win);
		dlg->setNumLines (lines().size());
		dlg->setModal (true);
		dlg->show();

		// Connect the loader in so we can show updates
		connect (this, SIGNAL (workDone()), dlg, SLOT (accept()));
		connect (dlg, SIGNAL (rejected()), this, SLOT (abort()));
	}
	else
		dlg = null;

	// Begin working
	work (0);
}

// =============================================================================
//
void LDFileLoader::work (int i)
{
	// User wishes to abort, so stop here now.
	if (isAborted())
	{
		for (LDObject* obj : m_objects)
			obj->destroy();

		m_objects.clear();
		setDone (true);
		return;
	}

	// Parse up to 300 lines per iteration
	int max = i + 300;

	for (; i < max and i < (int) lines().size(); ++i)
	{
		QString line = lines()[i];

		// Trim the trailing newline
		QChar c;

		while (line.endsWith ("\n") or line.endsWith ("\r"))
			line.chop (1);

		LDObject* obj = ParseLine (line);

		// Check for parse errors and warn about tthem
		if (obj->type() == OBJ_Error)
		{
			print ("Couldn't parse line #%1: %2",
				progress() + 1, static_cast<LDError*> (obj)->reason());

			if (warnings() != null)
				(*warnings())++;
		}

		m_objects << obj;
		setProgress (i);

		// If we have a dialog pointer, update the progress now
		if (dlg)
			dlg->setProgress (i);
	}

	// If we're done now, tell the environment we're done and stop.
	if (i >= ((int) lines().size()) - 1)
	{
		emit workDone();
		setDone (true);
		return;
	}

	// Otherwise, continue, by recursing back.
	if (not isDone())
	{
		// If we have a dialog to show progress output to, we cannot just call
		// work() again immediately as the dialog needs some processor cycles as
		// well. Thus, take a detour through the event loop by using the
		// meta-object system.
		//
		// This terminates the loop here and control goes back to the function
		// which called the file loader. It will keep processing the event loop
		// until we're ready (see loadFileContents), thus the event loop will
		// eventually catch the invokation we throw here and send us back. Though
		// it's not technically recursion anymore, more like a for loop. :P
		if (isOnForeground())
			QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection, Q_ARG (int, i));
		else
			work (i);
	}
}

// =============================================================================
//
void LDFileLoader::abort()
{
	setAborted (true);

	if (isOnForeground())
		g_aborted = true;
}

// =============================================================================
//
LDObjectList LoadFileContents (QFile* fp, int* numWarnings, bool* ok)
{
	QStringList lines;
	LDObjectList objs;

	if (numWarnings)
		*numWarnings = 0;

	// Read in the lines
	while (not fp->atEnd())
		lines << QString::fromUtf8 (fp->readLine());

	LDFileLoader* loader = new LDFileLoader;
	loader->setWarnings (numWarnings);
	loader->setLines (lines);
	loader->setOnForeground (g_loadingMainFile);
	loader->start();

	// After start() returns, if the loader isn't done yet, it's delaying
	// its next iteration through the event loop. We need to catch this here
	// by telling the event loop to tick, which will tick the file loader again.
	// We keep doing this until the file loader is ready.
	while (not loader->isDone())
		qApp->processEvents();

	// If we wanted the success value, supply that now
	if (ok)
		*ok = not loader->isAborted();

	objs = loader->objects();
	delete loader;
	return objs;
}

// =============================================================================
//
LDDocument* OpenDocument (QString path, bool search, bool implicit, LDDocument* fileToOverride)
{
	// Convert the file name to lowercase when searching because some parts contain subfile
	// subfile references with uppercase file names. I'll assume here that the library will always
	// use lowercase file names for the part files.
	QFile* fp;
	QString fullpath;

	if (search)
	{
		fp = OpenLDrawFile (path.toLower(), true, &fullpath);
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

	LDDocument* load = (fileToOverride != null ? fileToOverride : g_win->newDocument (implicit));
	load->setFullPath (fullpath);
	load->setName (LDDocument::shortenName (load->fullPath()));

	// Loading the file shouldn't count as actual edits to the document.
	load->history()->setIgnoring (true);

	int numWarnings;
	bool ok;
	LDObjectList objs = LoadFileContents (fp, &numWarnings, &ok);
	fp->close();
	fp->deleteLater();

	if (not ok)
	{
		load->dismiss();
		return nullptr;
	}

	load->addObjects (objs);

	if (g_loadingMainFile)
	{
		g_win->changeDocument (load);
		g_win->R()->setDocument (load);
		print (QObject::tr ("File %1 parsed successfully (%2 errors)."), path, numWarnings);
	}

	load->history()->setIgnoring (false);
	return load;
}

// =============================================================================
//
bool LDDocument::isSafeToClose()
{
	using msgbox = QMessageBox;
	setlocale (LC_ALL, "C");

	// If we have unsaved changes, warn and give the option of saving.
	if (hasUnsavedChanges())
	{
		QString message = format (tr ("There are unsaved changes to %1. Should it be saved?"), getDisplayName());

		int button = msgbox::question (m_window, QObject::tr ("Unsaved Changes"), message,
			(msgbox::Yes | msgbox::No | msgbox::Cancel), msgbox::Cancel);

		switch (button)
		{
			case msgbox::Yes:
			{
				// If we don't have a file path yet, we have to ask the user for one.
				if (name().length() == 0)
				{
					QString newpath = QFileDialog::getSaveFileName (m_window, QObject::tr ("Save As"),
						name(), QObject::tr ("LDraw files (*.dat *.ldr)"));

					if (newpath.length() == 0)
						return false;

					setName (newpath);
				}

				if (not save())
				{
					message = format (QObject::tr ("Failed to save %1 (%2)\nDo you still want to close?"),
						name(), strerror (errno));

					if (msgbox::critical (m_window, QObject::tr ("Save Failure"), message,
						(msgbox::Yes | msgbox::No), msgbox::No) == msgbox::No)
					{
						return false;
					}
				}
				break;
			}

			case msgbox::Cancel:
				return false;

			default:
				break;
		}
	}

	return true;
}

// =============================================================================
//
void CloseAllDocuments()
{
	for (LDDocument* file : g_win->allDocuments())
		file->dismiss();
}

// =============================================================================
//
void AddRecentFile (QString path)
{
	QStringList recentFiles = g_win->configBag()->recentFiles();
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
	g_win->configBag()->setRecentFiles (recentFiles);
	g_win->syncSettings();
	g_win->updateRecentFilesMenu();
}

// =============================================================================
// Open an LDraw file and set it as the main model
// =============================================================================
void OpenMainModel (QString path)
{
	// If there's already a file with the same name, this file must replace it.
	LDDocument* documentToReplace = nullptr;
	LDDocument* file = nullptr;
	QString shortName = LDDocument::shortenName (path);

	for (LDDocument* doc : g_win->allDocuments())
	{
		if (doc->name() == shortName)
		{
			documentToReplace = doc;
			break;
		}
	}

	// We cannot open this file if the document this would replace is not
	// safe to close.
	if (documentToReplace != null and not documentToReplace->isSafeToClose())
		return;

	g_loadingMainFile = true;

	// If we're replacing an existing document, clear the document and
	// make it ready for being loaded to.
	if (documentToReplace != null)
	{
		file = documentToReplace;
		file->clear();
	}

	file = OpenDocument (path, false, false, file);

	if (file == null)
	{
		if (not g_aborted)
		{
			// Tell the user loading failed.
			setlocale (LC_ALL, "C");
			Critical (format (QObject::tr ("Failed to open %1: %2"), path, strerror (errno)));
		}

		g_loadingMainFile = false;
		return;
	}

	file->setImplicit (false);
	g_win->closeInitialDocument();
	g_win->changeDocument (file);
	g_win->doFullRefresh();
	AddRecentFile (path);
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

	if (g_win->configBag()->tryDownloadMissingFiles() and not unknowns.isEmpty())
	{
		PartDownloader dl;

		if (dl.checkValidPath())
		{
			dl.setSource (PartDownloader::PartsTracker);
			dl.setPrimaryFile (file);

			for (QString const& unknown : unknowns)
				dl.downloadFromPartsTracker (unknown);

			dl.exec();
			dl.checkIfFinished();
			file->reloadAllSubfiles();
		}
	}
}

// =============================================================================
//
bool LDDocument::save (QString path, int64* sizeptr)
{
	if (isImplicit())
		return false;

	if (not path.length())
		path = fullPath();

	// If the second object in the list holds the file name, update that now.
	LDObject* nameObject = getObject (1);

	if (nameObject != null and nameObject->type() == OBJ_Comment)
	{
		LDComment* nameComment = static_cast<LDComment*> (nameObject);

		if (nameComment->text().left (6) == "Name: ")
		{
			QString newname = shortenName (path);
			nameComment->setText (format ("Name: %1", newname));
			m_window->buildObjList();
		}
	}

	QByteArray data;

	if (sizeptr != null)
		*sizeptr = 0;

	// File is open, now save the model to it. Note that LDraw requires files to have DOS line endings.
	for (LDObject* obj : objects())
	{
		QByteArray subdata ((obj->asText() + "\r\n").toUtf8());
		data.append (subdata);

		if (sizeptr != null)
			*sizeptr += subdata.size();
	}

	QFile f (path);

	if (not f.open (QIODevice::WriteOnly))
		return false;

	f.write (data);
	f.close();

	// We have successfully saved, update the save position now.
	setSavePosition (history()->position());
	setFullPath (path);
	setName (shortenName (path));
	m_window->updateDocumentListItem (this);
	m_window->updateTitle();
	return true;
}

// =============================================================================
//
void LDDocument::clear()
{
	for (LDObject* obj : objects())
		forgetObject (obj);
}

// =============================================================================
//
static void CheckTokenCount (const QStringList& tokens, int num)
{
	if (tokens.size() != num)
		throw QString (format ("Bad amount of tokens, expected %1, got %2", num, tokens.size()));
}

// =============================================================================
//
static void CheckTokenNumbers (const QStringList& tokens, int min, int max)
{
	bool ok;

	QRegExp scient ("\\-?[0-9]+\\.[0-9]+e\\-[0-9]+");

	for (int i = min; i <= max; ++i)
	{
		// Check for floating point
		tokens[i].toDouble (&ok);
		if (ok)
			return;

		// Check hex
		if (tokens[i].startsWith ("0x"))
		{
			tokens[i].mid (2).toInt (&ok, 16);

			if (ok)
				return;
		}

		// Check scientific notation, e.g. 7.99361e-15
		if (scient.exactMatch (tokens[i]))
			return;

		throw QString (format ("Token #%1 was `%2`, expected a number (matched length: %3)",
			(i + 1), tokens[i], scient.matchedLength()));
	}
}

// =============================================================================
//
static Vertex ParseVertex (QStringList& s, const int n)
{
	Vertex v;
	v.apply ([&] (Axis ax, double& a) { a = s[n + ax].toDouble(); });
	return v;
}

static int32 StringToNumber (QString a, bool* ok = null)
{
	int base = 10;

	if (a.startsWith ("0x"))
	{
		a.remove (0, 2);
		base = 16;
	}

	return a.toLong (ok, base);
}

// =============================================================================
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDError if it could not be parsed properly.
// =============================================================================
LDObject* ParseLine (QString line)
{
	try
	{
		QStringList tokens = line.split (" ", QString::SkipEmptyParts);

		if (tokens.size() <= 0)
		{
			// Line was empty, or only consisted of whitespace
			return LDSpawn<LDEmpty>();
		}

		if (tokens[0].length() != 1 or not tokens[0][0].isDigit())
			throw QString ("Illogical line code");

		int num = tokens[0][0].digitValue();

		switch (num)
		{
			case 0:
			{
				// Comment
				QString commentText (line.mid (line.indexOf ("0") + 2));
				QString commentTextSimplified (commentText.simplified());

				// Handle BFC statements
				if (tokens.size() > 2 and tokens[1] == "BFC")
				{
					for_enum (BFCStatement, i)
					{
						if (commentTextSimplified == format ("BFC %1",
							LDBFC::StatementStrings[int (i)]))
						{
							return LDSpawn<LDBFC> (i);
						}
					}

					// MLCAD is notorious for stuffing these statements in parts it
					// creates. The above block only handles valid statements, so we
					// need to handle MLCAD-style invertnext, clip and noclip separately.
					if (commentTextSimplified == "BFC CERTIFY INVERTNEXT")
						return LDSpawn<LDBFC> (BFCStatement::InvertNext);
					else if (commentTextSimplified == "BFC CERTIFY CLIP")
						return LDSpawn<LDBFC> (BFCStatement::Clip);
					else if (commentTextSimplified == "BFC CERTIFY NOCLIP")
						return LDSpawn<LDBFC> (BFCStatement::NoClip);
				}

				if (tokens.size() > 2 and tokens[1] == "!LDFORGE")
				{
					// Handle LDForge-specific types, they're embedded into comments too
					if (tokens[2] == "OVERLAY")
					{
						CheckTokenCount (tokens, 9);
						CheckTokenNumbers (tokens, 5, 8);

						LDOverlay* obj = LDSpawn<LDOverlay>();
						obj->setFileName (tokens[3]);
						obj->setCamera (tokens[4].toLong());
						obj->setX (tokens[5].toLong());
						obj->setY (tokens[6].toLong());
						obj->setWidth (tokens[7].toLong());
						obj->setHeight (tokens[8].toLong());
						return obj;
					}
				}

				// Just a regular comment:
				LDComment* obj = LDSpawn<LDComment>();
				obj->setText (commentText);
				return obj;
			}

			case 1:
			{
				// Subfile
				CheckTokenCount (tokens, 15);
				CheckTokenNumbers (tokens, 1, 13);

				// Try open the file. Disable g_loadingMainFile temporarily since we're
				// not loading the main file now, but the subfile in question.
				bool tmp = g_loadingMainFile;
				g_loadingMainFile = false;
				LDDocument* load = GetDocument (tokens[14]);
				g_loadingMainFile = tmp;

				// If we cannot open the file, mark it an error. Note we cannot use LDParseError
				// here because the error object needs the document reference.
				if (not load)
				{
					LDError* obj = LDSpawn<LDError> (line, format ("Could not open %1", tokens[14]));
					obj->setFileReferenced (tokens[14]);
					return obj;
				}

				LDSubfile* obj = LDSpawn<LDSubfile>();
				obj->setColor (StringToNumber (tokens[1]));
				obj->setPosition (ParseVertex (tokens, 2));  // 2 - 4

				Matrix transform;

				for (int i = 0; i < 9; ++i)
					transform[i] = tokens[i + 5].toDouble(); // 5 - 13

				obj->setTransform (transform);
				obj->setFileInfo (load);
				return obj;
			}

			case 2:
			{
				CheckTokenCount (tokens, 8);
				CheckTokenNumbers (tokens, 1, 7);

				// Line
				LDLine* obj (LDSpawn<LDLine>());
				obj->setColor (StringToNumber (tokens[1]));

				for (int i = 0; i < 2; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 7

				return obj;
			}

			case 3:
			{
				CheckTokenCount (tokens, 11);
				CheckTokenNumbers (tokens, 1, 10);

				// Triangle
				LDTriangle* obj (LDSpawn<LDTriangle>());
				obj->setColor (StringToNumber (tokens[1]));

				for (int i = 0; i < 3; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 10

				return obj;
			}

			case 4:
			case 5:
			{
				CheckTokenCount (tokens, 14);
				CheckTokenNumbers (tokens, 1, 13);

				// Quadrilateral / Conditional line
				LDObject* obj;

				if (num == 4)
					obj = LDSpawn<LDQuad>();
				else
					obj = LDSpawn<LDCondLine>();

				obj->setColor (StringToNumber (tokens[1]));

				for (int i = 0; i < 4; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 13

				return obj;
			}

			default:
				throw QString ("Unknown line code number");
		}
	}
	catch (QString& e)
	{
		// Strange line we couldn't parse
		return LDSpawn<LDError> (line, e);
	}
}

// =============================================================================
//
LDDocument* GetDocument (QString filename)
{
	// Try find the file in the list of loaded files
	LDDocument* doc = FindDocument (filename);

	// If it's not loaded, try open it
	if (not doc)
		doc = OpenDocument (filename, true, true);

	return doc;
}

// =============================================================================
//
void LDDocument::reloadAllSubfiles()
{
	print ("Reloading subfiles of %1", getDisplayName());

	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : objects())
	{
		if (obj->type() == OBJ_Subfile)
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDDocument* fileInfo = GetDocument (ref->fileInfo()->name());

			if (fileInfo != null)
			{
				ref->setFileInfo (fileInfo);
			}
			else
			{
				ref->replace (LDSpawn<LDError> (ref->asText(),
					format ("Could not open %1", ref->fileInfo()->name())));
			}
		}

		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if (obj->type() == OBJ_Error)
			obj->replace (ParseLine (static_cast<LDError*> (obj)->contents()));
	}

	m_needsReCache = true;

	if (this == m_window->currentDocument())
		m_window->buildObjList();
}

// =============================================================================
//
int LDDocument::addObject (LDObject* obj)
{
	history()->add (new AddHistory (objects().size(), obj));
	m_objects << obj;
	addKnownVertices (obj);
	obj->setDocument (this);
	m_window->R()->compileObject (obj);
	return getObjectCount() - 1;
}

// =============================================================================
//
void LDDocument::addObjects (const LDObjectList& objs)
{
	for (LDObject* obj : objs)
	{
		if (obj != null)
			addObject (obj);
	}
}

// =============================================================================
//
void LDDocument::insertObj (int pos, LDObject* obj)
{
	history()->add (new AddHistory (pos, obj));
	m_objects.insert (pos, obj);
	obj->setDocument (this);
	m_window->R()->compileObject (obj);
	

#ifdef DEBUG
	if (not isImplicit())
		dprint ("Inserted object #%1 (%2) at %3\n", obj->id(), obj->typeName(), pos);
#endif
}

// =============================================================================
//
void LDDocument::addKnownVertices (LDObject* obj)
{
	auto it = m_objectVertices.find (obj);

	if (it == m_objectVertices.end())
		it = m_objectVertices.insert (obj, QVector<Vertex>());
	else
		it->clear();

	obj->getVertices (*it);
	needVertexMerge();
}

// =============================================================================
//
void LDDocument::forgetObject (LDObject* obj)
{
	int idx = obj->lineNumber();

	if (m_objects[idx] == obj)
	{
		obj->deselect();

		if (not isImplicit() and not (flags() & DOCF_IsBeingDestroyed))
		{
			history()->add (new DelHistory (idx, obj));
			m_objectVertices.remove (obj);
		}

		m_objects.removeAt (idx);
		obj->setDocument (nullptr);
	}
}

// =============================================================================
//
bool IsSafeToCloseAll()
{
	for (LDDocument* f : g_win->allDocuments())
	{
		if (not f->isSafeToClose())
			return false;
	}

	return true;
}

// =============================================================================
//
void LDDocument::setObject (int idx, LDObject* obj)
{
	if (idx < 0 or idx >= m_objects.size())
		return;

	// Mark this change to history
	if (not m_history->isIgnoring())
	{
		QString oldcode = getObject (idx)->asText();
		QString newcode = obj->asText();
		*m_history << new EditHistory (idx, oldcode, newcode);
	}

	m_objectVertices.remove (m_objects[idx]);
	m_objects[idx]->deselect();
	m_objects[idx]->setDocument (nullptr);
	obj->setDocument (this);
	addKnownVertices (obj);
	m_window->R()->compileObject (obj);
	m_objects[idx] = obj;
	needVertexMerge();
}

// =============================================================================
//
LDObject* LDDocument::getObject (int pos) const
{
	if (pos < m_objects.size())
		return m_objects[pos];
	else
		return nullptr;
}

// =============================================================================
//
int LDDocument::getObjectCount() const
{
	return objects().size();
}

// =============================================================================
//
bool LDDocument::hasUnsavedChanges() const
{
	return not isImplicit() and history()->position() != savePosition();
}

// =============================================================================
//
QString LDDocument::getDisplayName()
{
	if (not name().isEmpty())
		return name();

	if (not defaultName().isEmpty())
		return "[" + defaultName() + "]";

	return QObject::tr ("untitled");
}

// =============================================================================
//
void LDDocument::initializeCachedData()
{
	if (m_needsReCache)
	{
		m_vertices.clear();

		for (LDObject* obj : inlineContents (true, true))
		{
			if (obj->type() == OBJ_Subfile)
			{
				print ("Warning: unable to inline %1 into %2",
					static_cast<LDSubfile*> (obj)->fileInfo()->getDisplayName(),
					getDisplayName());
				continue;
			}

			LDPolygon* data = obj->getPolygon();

			if (data != null)
			{
				m_polygonData << *data;
				delete data;
			}
		}

		m_needsReCache = false;
	}

	if (m_verticesOutdated)
	{
		m_objectVertices.clear();

		for (LDObject* obj : inlineContents (true, false))
			addKnownVertices (obj);

		mergeVertices();
		m_verticesOutdated = false;
	}

	if (m_needVertexMerge)
		mergeVertices();
}

// =============================================================================
//
void LDDocument::mergeVertices()
{
	m_vertices.clear();

	for (QVector<Vertex> const& verts : m_objectVertices)
		m_vertices << verts;

	removeDuplicates (m_vertices);
	m_needVertexMerge = false;
}

// =============================================================================
//
QList<LDPolygon> LDDocument::inlinePolygons()
{
	initializeCachedData();
	return polygonData();
}

// =============================================================================
// -----------------------------------------------------------------------------
LDObjectList LDDocument::inlineContents (bool deep, bool renderinline)
{
	// Possibly substitute with logoed studs:
	// stud.dat -> stud-logo.dat
	// stud2.dat -> stud-logo2.dat
	if (m_config->useLogoStuds() and renderinline)
	{
		// Ensure logoed studs are loaded first
		LoadLogoStuds();

		if (name() == "stud.dat" and g_logoedStud != null)
			return g_logoedStud->inlineContents (deep, renderinline);
		else if (name() == "stud2.dat" and g_logoedStud2 != null)
			return g_logoedStud2->inlineContents (deep, renderinline);
	}

	LDObjectList objs, objcache;

	for (LDObject* obj : objects())
	{
		// Skip those without scemantic meaning
		if (not obj->isScemantic())
			continue;

		// Got another sub-file reference, inline it if we're deep-inlining. If not,
		// just add it into the objects normally. Yay, recursion!
		if (deep == true and obj->type() == OBJ_Subfile)
		{
			for (LDObject* otherobj : static_cast<LDSubfile*> (obj)->inlineContents (deep, renderinline))
				objs << otherobj;
		}
		else
			objs << obj->createCopy();
	}

	return objs;
}

// =============================================================================
//
void LoadLogoStuds()
{
	if (g_loadingLogoedStuds or (g_logoedStud and g_logoedStud2))
		return;

	g_loadingLogoedStuds = true;
	g_logoedStud = OpenDocument ("stud-logo.dat", true, true);
	g_logoedStud2 = OpenDocument ("stud2-logo.dat", true, true);
	print (QObject::tr ("Logoed studs loaded.\n"));
	g_loadingLogoedStuds = false;
}

// =============================================================================
//
void LDDocument::addToSelection (LDObject* obj) // [protected]
{
	if (not obj->isSelected() and obj->document() == this)
	{
		m_sel << obj;
		m_window->R()->compileObject (obj);
		obj->setSelected (true);
	}
}

// =============================================================================
//
void LDDocument::removeFromSelection (LDObject* obj) // [protected]
{
	if (obj->isSelected() and obj->document() == this)
	{
		m_sel.removeOne (obj);
		m_window->R()->compileObject (obj);
		obj->setSelected (false);
	}
}

// =============================================================================
//
void LDDocument::clearSelection()
{
	for (LDObject* obj : m_sel)
	{
		m_window->R()->compileObject (obj);
		obj->setSelected (false);
	}

	m_sel.clear();
}

// =============================================================================
//
const LDObjectList& LDDocument::getSelection() const
{
	return m_sel;
}

// =============================================================================
//
void LDDocument::swapObjects (LDObject* one, LDObject* other)
{
	int a = m_objects.indexOf (one);
	int b = m_objects.indexOf (other);

	if (a != b and a != -1 and b != -1)
	{
		m_objects[b] = one;
		m_objects[a] = other;
		addToHistory (new SwapHistory (one->id(), other->id()));
	}
}

// =============================================================================
//
QString LDDocument::shortenName (QString a) // [static]
{
	QString shortname = Basename (a);
	QString topdirname = Basename (Dirname (a));

	if (g_specialSubdirectories.contains (topdirname))
		shortname.prepend (topdirname + "\\");

	return shortname;
}

// =============================================================================
//
QVector<Vertex> const& LDDocument::inlineVertices()
{
	initializeCachedData();
	return m_vertices;
}

void LDDocument::redoVertices()
{
	m_verticesOutdated = true;
}

void LDDocument::needVertexMerge()
{
	m_needVertexMerge = true;
}