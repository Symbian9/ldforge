/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include "configuration.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainWindow.h"
#include "editHistory.h"
#include "dialogs.h"
#include "glRenderer.h"
#include "glCompiler.h"
#include "partDownloader.h"

CFGENTRY (String, LDrawPath, "")
CFGENTRY (List, RecentFiles, {})
EXTERN_CFGENTRY (String, DownloadFilePath)
EXTERN_CFGENTRY (Bool, UseLogoStuds)

static bool g_loadingMainFile = false;
static const int g_maxRecentFiles = 10;
static bool g_aborted = false;
static LDDocumentPtr g_logoedStud;
static LDDocumentPtr g_logoedStud2;
static QList<LDDocumentWeakPtr> g_allDocuments;
static QList<LDDocumentPtr> g_explicitDocuments;
static LDDocumentPtr g_currentDocument;

const QStringList g_specialSubdirectories ({ "s", "48", "8" });

// =============================================================================
//
namespace LDPaths
{
	static QString pathError;

	struct
	{
		QString LDConfigPath;
		QString partsPath, primsPath;
	} pathInfo;

	void initPaths()
	{
		if (not tryConfigure (cfg::LDrawPath))
		{
			LDrawPathDialog dlg (false);

			if (not dlg.exec())
				Exit();

			cfg::LDrawPath = dlg.filename();
		}
	}

	bool tryConfigure (QString path)
	{
		QDir dir;

		if (not dir.cd (path))
		{
			pathError = "Directory does not exist.";
			return false;
		}

		QStringList mustHave = { "LDConfig.ldr", "parts", "p" };
		QStringList contents = dir.entryList (mustHave);

		if (contents.size() != mustHave.size())
		{
			pathError = "Not an LDraw directory! Must<br />have LDConfig.ldr, parts/ and p/.";
			return false;
		}

		pathInfo.partsPath = format ("%1" DIRSLASH "parts", path);
		pathInfo.LDConfigPath = format ("%1" DIRSLASH "LDConfig.ldr", path);
		pathInfo.primsPath = format ("%1" DIRSLASH "p", path);

		return true;
	}

	// Accessors
	QString getError()
	{
		return pathError;
	}

	QString ldconfig()
	{
		return pathInfo.LDConfigPath;
	}

	QString prims()
	{
		return pathInfo.primsPath;
	}

	QString parts()
	{
		return pathInfo.partsPath;
	}
}

// =============================================================================
//
LDDocument::LDDocument (LDDocumentPtr* selfptr) :
	m_isImplicit (true),
	m_flags (0),
	m_verticesOutdated (true),
	m_needVertexMerge (true),
	m_gldata (new LDGLData)
{
	*selfptr = LDDocumentPtr (this);
	setSelf (*selfptr);
	setSavePosition (-1);
	setTabIndex (-1);
	setHistory (new History);
	history()->setDocument (*selfptr);
	m_needsReCache = true;
	g_allDocuments << *selfptr;
}

// =============================================================================
//
LDDocumentPtr LDDocument::createNew()
{
	LDDocumentPtr ptr;
	new LDDocument (&ptr);
	return ptr;
}

// =============================================================================
//
LDDocument::~LDDocument()
{
	// Don't bother during program termination
	if (IsExiting())
		return;

	g_allDocuments.removeOne (self());
	m_flags |= DOCF_IsBeingDestroyed;
	delete m_history;
	delete m_gldata;
}

// =============================================================================
//
extern QMap<long, LDObjectWeakPtr>	g_allObjects;
void LDDocument::setImplicit (bool const& a)
{
	if (m_isImplicit != a)
	{
		m_isImplicit = a;

		if (a == false)
		{
			g_explicitDocuments << self().toStrongRef();
			print ("Opened %1", name());

			// Implicit files are not compiled by the GL renderer. Now that this
			// part is no longer implicit, it needs to be compiled.
			if (g_win != null)
				g_win->R()->compiler()->compileDocument (self());
		}
		else
		{
			g_explicitDocuments.removeOne (self().toStrongRef());
			print ("Closed %1", name());
			int count = 0;

			for (LDObjectWeakPtr obj : g_allObjects)
			{
				if (obj == null)
					continue;

				LDSubfilePtr ref = obj.toStrongRef().dynamicCast<LDSubfile>();

				if (ref != null and ref->fileInfo() == self())
					count++;
			}
		}

		if (g_win != null)
			g_win->updateDocumentList();

		// If the current document just became implicit (e.g. it was 'closed'),
		// we need to get a new current document.
		if (current() == self() and isImplicit())
		{
			if (explicitDocuments().isEmpty())
				newFile();
			else
				setCurrent (explicitDocuments().first());
		}
	}
}

// =============================================================================
//
QList<LDDocumentPtr> const& LDDocument::explicitDocuments()
{
	return g_explicitDocuments;
}

// =============================================================================
//
LDDocumentPtr FindDocument (QString name)
{
	for (LDDocumentWeakPtr weakfile : g_allDocuments)
	{
		if (weakfile == null)
			continue;

		LDDocumentPtr file (weakfile.toStrongRef());

		if (Eq (name, file->name(), file->defaultName()))
			return file;
	}

	return LDDocumentPtr();
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

	// LDraw models use Windows-style path separators. If we're not on Windows,
	// replace the path separator now before opening any files. Qt expects
	// forward-slashes as directory separators.
#ifndef WIN32
	relpath.replace ("\\", "/");
#endif // WIN32

	// Try find it relative to other currently open documents. We want a file
	// in the immediate vicinity of a current model to override stock LDraw stuff.
	QString reltop = Basename (Dirname (relpath));

	for (LDDocumentWeakPtr doc : g_allDocuments)
	{
		if (doc == null)
			continue;

		QString partpath = format ("%1/%2", Dirname (doc.toStrongRef()->fullPath()), relpath);
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
	fullPath = format ("%1" DIRSLASH "%2", cfg::LDrawPath, relpath);

	if (QFile::exists (fullPath))
		return fullPath;

	if (subdirs)
	{
		// Look in sub-directories: parts and p. Also look in net_downloadpath, since that's
		// where we download parts from the PT to.
		for (const QString& topdir : QList<QString> ({ cfg::LDrawPath, cfg::DownloadFilePath }))
		{
			for (const QString& subdir : QList<QString> ({ "parts", "p" }))
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
		for (LDObjectPtr obj : m_objects)
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

		LDObjectPtr obj = ParseLine (line);

		// Check for parse errors and warn about tthem
		if (obj->type() == OBJ_Error)
		{
			print ("Couldn't parse line #%1: %2",
				progress() + 1, obj.staticCast<LDError>()->reason());

			if (warnings() != null)
				(*warnings())++;
		}

		m_objects << obj;
		setProgress (i);

		// If we have a dialog pointer, update the progress now
		if (isOnForeground())
			dlg->updateProgress (i);
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
LDDocumentPtr OpenDocument (QString path, bool search, bool implicit, LDDocumentPtr fileToOverride)
{
	// Convert the file name to lowercase since some parts contain uppercase
	// file names. I'll assume here that the library will always use lowercase
	// file names for the actual parts..
	QFile* fp;
	QString fullpath;

	if (search)
		fp = OpenLDrawFile (path.toLower(), true, &fullpath);
	else
	{
		fp = new QFile (path);
		fullpath = path;

		if (not fp->open (QIODevice::ReadOnly))
		{
			delete fp;
			return LDDocumentPtr();
		}
	}

	if (not fp)
		return LDDocumentPtr();

	LDDocumentPtr load = (fileToOverride != null ? fileToOverride : LDDocument::createNew());
	load->setImplicit (implicit);
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
		return LDDocumentPtr();
	}

	load->addObjects (objs);

	if (g_loadingMainFile)
	{
		LDDocument::setCurrent (load);
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

		int button = msgbox::question (g_win, tr ("Unsaved Changes"), message,
			(msgbox::Yes | msgbox::No | msgbox::Cancel), msgbox::Cancel);

		switch (button)
		{
			case msgbox::Yes:
			{
				// If we don't have a file path yet, we have to ask the user for one.
				if (name().length() == 0)
				{
					QString newpath = QFileDialog::getSaveFileName (g_win, tr ("Save As"),
						CurrentDocument()->name(), tr ("LDraw files (*.dat *.ldr)"));

					if (newpath.length() == 0)
						return false;

					setName (newpath);
				}

				if (not save())
				{
					message = format (tr ("Failed to save %1 (%2)\nDo you still want to close?"),
						name(), strerror (errno));

					if (msgbox::critical (g_win, tr ("Save Failure"), message,
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
	for (LDDocumentPtr file : g_explicitDocuments)
		file->dismiss();
}

// =============================================================================
//
void newFile()
{
	// Create a new anonymous file and set it to our current
	LDDocumentPtr f = LDDocument::createNew();
	f->setName ("");
	f->setImplicit (false);
	LDDocument::setCurrent (f);
	LDDocument::closeInitialFile();
	g_win->R()->setDocument (f);
	g_win->doFullRefresh();
	g_win->updateTitle();
	g_win->updateActions();
}

// =============================================================================
//
void AddRecentFile (QString path)
{
	auto& rfiles = cfg::RecentFiles;
	int idx = rfiles.indexOf (path);

	// If this file already is in the list, pop it out.
	if (idx != -1)
	{
		if (idx == rfiles.size() - 1)
			return; // first recent file - abort and do nothing

		rfiles.removeAt (idx);
	}

	// If there's too many recent files, drop one out.
	while (rfiles.size() > (g_maxRecentFiles - 1))
		rfiles.removeAt (0);

	// Add the file
	rfiles << path;

	Config::Save();
	g_win->updateRecentFilesMenu();
}

// =============================================================================
// Open an LDraw file and set it as the main model
// =============================================================================
void OpenMainModel (QString path)
{
	// If there's already a file with the same name, this file must replace it.
	LDDocumentPtr documentToReplace;
	LDDocumentPtr file;
	QString shortName = LDDocument::shortenName (path);

	for (LDDocumentWeakPtr doc : g_allDocuments)
	{
		if (doc.toStrongRef()->name() == shortName)
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
			CriticalError (format (QObject::tr ("Failed to open %1: %2"), path, strerror (errno)));
		}

		g_loadingMainFile = false;
		return;
	}

	file->setImplicit (false);

	// If we have an anonymous, unchanged file open as the only open file
	// (aside of the one we just opened), close it now.
	LDDocument::closeInitialFile();

	// Rebuild the object tree view now.
	LDDocument::setCurrent (file);
	g_win->doFullRefresh();

	// Add it to the recent files list.
	AddRecentFile (path);
	g_loadingMainFile = false;

	// If there were problems loading subfile references, try see if we can find these
	// files on the parts tracker.
	QStringList unknowns;

	for (LDObjectPtr obj : file->objects())
	{
		if (obj->type() != OBJ_Error or obj.staticCast<LDError>()->fileReferenced().isEmpty())
			continue;

		unknowns << obj.staticCast<LDError>()->fileReferenced();
	}

	if (not unknowns.isEmpty())
	{
		PartDownloader dl;
		dl.setSource (PartDownloader::PartsTracker);
		dl.setPrimaryFile (file);

		for (QString const& unknown : unknowns)
			dl.downloadFromPartsTracker (unknown);

		dl.exec();
		dl.checkIfFinished();
		file->reloadAllSubfiles();
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
	LDObjectPtr nameObject = getObject (1);

	if (nameObject != null and nameObject->type() == OBJ_Comment)
	{
		LDCommentPtr nameComment = nameObject.staticCast<LDComment>();

		if (nameComment->text().left (6) == "Name: ")
		{
			QString newname = shortenName (path);
			nameComment->setText (format ("Name: %1", newname));
			g_win->buildObjList();
		}
	}

	QByteArray data;

	if (sizeptr != null)
		*sizeptr = 0;

	// File is open, now save the model to it. Note that LDraw requires files to
	// have DOS line endings, so we terminate the lines with \r\n.
	for (LDObjectPtr obj : objects())
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

	g_win->updateDocumentListItem (self().toStrongRef());
	g_win->updateTitle();
	return true;
}

// =============================================================================
//
void LDDocument::clear()
{
	for (LDObjectPtr obj : objects())
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
LDObjectPtr ParseLine (QString line)
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
					elif (commentTextSimplified == "BFC CERTIFY CLIP")
						return LDSpawn<LDBFC> (BFCStatement::Clip);
					elif (commentTextSimplified == "BFC CERTIFY NOCLIP")
						return LDSpawn<LDBFC> (BFCStatement::NoClip);
				}

				if (tokens.size() > 2 and tokens[1] == "!LDFORGE")
				{
					// Handle LDForge-specific types, they're embedded into comments too
					if (tokens[2] == "VERTEX")
					{
						// Vertex (0 !LDFORGE VERTEX)
						CheckTokenCount (tokens, 7);
						CheckTokenNumbers (tokens, 3, 6);

						LDVertexPtr obj = LDSpawn<LDVertex>();
						obj->setColor (LDColor::fromIndex (StringToNumber (tokens[3])));
						obj->pos.apply ([&](Axis ax, double& value)
							{ value = tokens[4 + ax].toDouble(); });
						return obj;
					}
					elif (tokens[2] == "OVERLAY")
					{
						CheckTokenCount (tokens, 9);
						CheckTokenNumbers (tokens, 5, 8);

						LDOverlayPtr obj = LDSpawn<LDOverlay>();
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
				LDCommentPtr obj = LDSpawn<LDComment>();
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
				LDDocumentPtr load = GetDocument (tokens[14]);
				g_loadingMainFile = tmp;

				// If we cannot open the file, mark it an error. Note we cannot use LDParseError
				// here because the error object needs the document reference.
				if (not load)
				{
					LDErrorPtr obj = LDSpawn<LDError> (line, format ("Could not open %1", tokens[14]));
					obj->setFileReferenced (tokens[14]);
					return obj;
				}

				LDSubfilePtr obj = LDSpawn<LDSubfile>();
				obj->setColor (LDColor::fromIndex (StringToNumber (tokens[1])));
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
				LDLinePtr obj (LDSpawn<LDLine>());
				obj->setColor (LDColor::fromIndex (StringToNumber (tokens[1])));

				for (int i = 0; i < 2; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 7

				return obj;
			}

			case 3:
			{
				CheckTokenCount (tokens, 11);
				CheckTokenNumbers (tokens, 1, 10);

				// Triangle
				LDTrianglePtr obj (LDSpawn<LDTriangle>());
				obj->setColor (LDColor::fromIndex (StringToNumber (tokens[1])));

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
				LDObjectPtr obj;

				if (num == 4)
					obj = LDSpawn<LDQuad>();
				else
					obj = LDSpawn<LDCondLine>();

				obj->setColor (LDColor::fromIndex (StringToNumber (tokens[1])));

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
LDDocumentPtr GetDocument (QString filename)
{
	// Try find the file in the list of loaded files
	LDDocumentPtr doc = FindDocument (filename);

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
	for (LDObjectPtr obj : objects())
	{
		if (obj->type() == OBJ_Subfile)
		{
			LDSubfilePtr ref = obj.staticCast<LDSubfile>();
			LDDocumentPtr fileInfo = GetDocument (ref->fileInfo()->name());

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
			obj->replace (ParseLine (obj.staticCast<LDError>()->contents()));
	}

	m_needsReCache = true;

	if (self() == CurrentDocument())
		g_win->buildObjList();
}

// =============================================================================
//
int LDDocument::addObject (LDObjectPtr obj)
{
	history()->add (new AddHistory (objects().size(), obj));
	m_objects << obj;
	addKnownVertices (obj);
	obj->setDocument (self());
	g_win->R()->compileObject (obj);
	return getObjectCount() - 1;
}

// =============================================================================
//
void LDDocument::addObjects (const LDObjectList& objs)
{
	for (LDObjectPtr obj : objs)
	{
		if (obj != null)
			addObject (obj);
	}
}

// =============================================================================
//
void LDDocument::insertObj (int pos, LDObjectPtr obj)
{
	history()->add (new AddHistory (pos, obj));
	m_objects.insert (pos, obj);
	obj->setDocument (self());
	g_win->R()->compileObject (obj);
	

#ifdef DEBUG
	if (not isImplicit())
		dprint ("Inserted object #%1 (%2) at %3\n", obj->id(), obj->typeName(), pos);
#endif
}

// =============================================================================
//
void LDDocument::addKnownVertices (LDObjectPtr obj)
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
void LDDocument::forgetObject (LDObjectPtr obj)
{
	int idx = obj->lineNumber();
	obj->deselect();
	assert (m_objects[idx] == obj);

	if (not isImplicit() and not (flags() & DOCF_IsBeingDestroyed))
	{
		history()->add (new DelHistory (idx, obj));
		m_objectVertices.remove (obj);
	}

	m_objects.removeAt (idx);
	obj->setDocument (LDDocumentPtr());
}

// =============================================================================
//
bool IsSafeToCloseAll()
{
	for (LDDocumentPtr f : LDDocument::explicitDocuments())
	{
		if (not f->isSafeToClose())
			return false;
	}

	return true;
}

// =============================================================================
//
void LDDocument::setObject (int idx, LDObjectPtr obj)
{
	assert (idx >= 0 and idx < m_objects.size());

	// Mark this change to history
	if (not m_history->isIgnoring())
	{
		QString oldcode = getObject (idx)->asText();
		QString newcode = obj->asText();
		*m_history << new EditHistory (idx, oldcode, newcode);
	}

	m_objectVertices.remove (m_objects[idx]);
	m_objects[idx]->deselect();
	m_objects[idx]->setDocument (LDDocumentPtr());
	obj->setDocument (self());
	addKnownVertices (obj);
	g_win->R()->compileObject (obj);
	m_objects[idx] = obj;
	needVertexMerge();
}

// =============================================================================
//
LDObjectPtr LDDocument::getObject (int pos) const
{
	if (m_objects.size() <= pos)
		return LDObjectPtr();

	return m_objects[pos];
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

	return tr ("untitled");
}

// =============================================================================
//
void LDDocument::initializeCachedData()
{
	if (m_needsReCache)
	{
		m_vertices.clear();

		for (LDObjectPtr obj : inlineContents (true, true))
		{
			if (obj->type() == OBJ_Subfile)
			{
				print ("Warning: unable to inline %1 into %2",
					obj.staticCast<LDSubfile>()->fileInfo()->getDisplayName(),
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

		for (LDObjectPtr obj : inlineContents (true, false))
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

	RemoveDuplicates (m_vertices);
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
	if (cfg::UseLogoStuds and renderinline)
	{
		// Ensure logoed studs are loaded first
		LoadLogoStuds();

		if (name() == "stud.dat" and g_logoedStud != null)
			return g_logoedStud->inlineContents (deep, renderinline);
		elif (name() == "stud2.dat" and g_logoedStud2 != null)
			return g_logoedStud2->inlineContents (deep, renderinline);
	}

	LDObjectList objs, objcache;

	for (LDObjectPtr obj : objects())
	{
		// Skip those without scemantic meaning
		if (not obj->isScemantic())
			continue;

		// Got another sub-file reference, inline it if we're deep-inlining. If not,
		// just add it into the objects normally. Yay, recursion!
		if (deep == true and obj->type() == OBJ_Subfile)
		{
			for (LDObjectPtr otherobj : obj.staticCast<LDSubfile>()->inlineContents (deep, renderinline))
				objs << otherobj;
		}
		else
			objs << obj->createCopy();
	}

	return objs;
}

// =============================================================================
//
LDDocumentPtr LDDocument::current()
{
	return g_currentDocument;
}

// =============================================================================
// Sets the given file as the current one on display. At some point in time this
// was an operation completely unheard of. ;)
//
// TODO: f can be temporarily null. This probably should not be the case.
// =============================================================================
void LDDocument::setCurrent (LDDocumentPtr f)
{
	// Implicit files were loaded for caching purposes and must never be set
	// current.
	if (f != null and f->isImplicit())
		return;

	g_currentDocument = f;

	if (g_win and f)
	{
		// A ton of stuff needs to be updated
		g_win->updateDocumentListItem (f);
		g_win->buildObjList();
		g_win->updateTitle();
		g_win->R()->setDocument (f);
		g_win->R()->compiler()->needMerge();
		print ("Changed file to %1", f->getDisplayName());
	}
}

// =============================================================================
//
int LDDocument::countExplicitFiles()
{
	return g_explicitDocuments.size();
}

// =============================================================================
// This little beauty closes the initial file that was open at first when opening
// a new file over it.
// =============================================================================
void LDDocument::closeInitialFile()
{
	if (g_explicitDocuments.size() == 2 and
		g_explicitDocuments[0]->name().isEmpty() and
		not g_explicitDocuments[1]->name().isEmpty() and
		not g_explicitDocuments[0]->hasUnsavedChanges())
	{
		g_explicitDocuments[0]->dismiss();
	}
}

// =============================================================================
//
void LoadLogoStuds()
{
	if (g_logoedStud and g_logoedStud2)
		return;

	g_logoedStud = OpenDocument ("stud-logo.dat", true, true);
	g_logoedStud2 = OpenDocument ("stud2-logo.dat", true, true);

	print (LDDocument::tr ("Logoed studs loaded.\n"));
}

// =============================================================================
//
void LDDocument::addToSelection (LDObjectPtr obj) // [protected]
{
	if (obj->isSelected())
		return;

	assert (obj->document() == self());
	m_sel << obj;
	g_win->R()->compileObject (obj);
	obj->setSelected (true);
}

// =============================================================================
//
void LDDocument::removeFromSelection (LDObjectPtr obj) // [protected]
{
	if (not obj->isSelected())
		return;

	assert (obj->document() == self());
	m_sel.removeOne (obj);
	g_win->R()->compileObject (obj);
	obj->setSelected (false);
}

// =============================================================================
//
void LDDocument::clearSelection()
{
	for (LDObjectPtr obj : m_sel)
		removeFromSelection (obj);

	assert (m_sel.isEmpty());
}

// =============================================================================
//
const LDObjectList& LDDocument::getSelection() const
{
	return m_sel;
}

// =============================================================================
//
void LDDocument::swapObjects (LDObjectPtr one, LDObjectPtr other)
{
	int a = m_objects.indexOf (one);
	int b = m_objects.indexOf (other);
	assert (a != b and a != -1 and b != -1);
	m_objects[b] = one;
	m_objects[a] = other;
	addToHistory (new SwapHistory (one->id(), other->id()));
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
