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
#include "partdownloader.h"
#include "ldpaths.h"
#include "documentloader.h"
#include "dialogs/openprogressdialog.h"
#include "documentmanager.h"

LDDocument::LDDocument (DocumentManager* parent) :
	QObject (parent),
	HierarchyElement (parent),
	m_history (new EditHistory (this)),
	m_isCache (true),
	m_verticesOutdated (true),
	m_needVertexMerge (true),
	m_beingDestroyed (false),
	m_gldata (new LDGLData),
	m_manager (parent)
{
	setSavePosition (-1);
	setTabIndex (-1);
	m_needsReCache = true;
}

LDDocument::~LDDocument()
{
	m_beingDestroyed = true;

	for (int i = 0; i < m_objects.size(); ++i)
		m_objects[i]->destroy();

	delete m_history;
	delete m_gldata;
}

QString LDDocument::name() const
{
	return m_name;
}

void LDDocument::setName (QString value)
{
	m_name = value;
}

const LDObjectList& LDDocument::objects() const
{
	return m_objects;
}

EditHistory* LDDocument::history() const
{
	return m_history;
}

QString LDDocument::fullPath()
{
	return m_fullPath;
}

void LDDocument::setFullPath (QString value)
{
	m_fullPath = value;
}

int LDDocument::tabIndex() const
{
	return m_tabIndex;
}

void LDDocument::setTabIndex (int value)
{
	m_tabIndex = value;
}

const QList<LDPolygon>& LDDocument::polygonData() const
{
	return m_polygonData;
}

long LDDocument::savePosition() const
{
	return m_savePosition;
}

void LDDocument::setSavePosition (long value)
{
	m_savePosition = value;
}

QString LDDocument::defaultName() const
{
	return m_defaultName;
}

void LDDocument::setDefaultName (QString value)
{
	m_defaultName = value;
}

void LDDocument::openForEditing()
{
	if (m_isCache)
	{
		m_isCache = false;
		print ("Opened %1", name());

		// Cache files are not compiled by the GL renderer. Now that this file is open for editing, it needs to be
		// compiled.
		m_window->renderer()->compiler()->compileDocument (this);
		m_window->updateDocumentList();
	}
}

bool LDDocument::isCache() const
{
	return m_isCache;
}

void LDDocument::addHistoryStep()
{
	history()->addStep();
}

void LDDocument::undo()
{
	history()->undo();
}

void LDDocument::redo()
{
	history()->redo();
}

void LDDocument::clearHistory()
{
	history()->clear();
}

void LDDocument::addToHistory (AbstractHistoryEntry* entry)
{
	history()->add (entry);
}

void LDDocument::close()
{
	if (not isCache())
	{
		m_isCache = true;
		print ("Closed %1", name());
		m_window->updateDocumentList();

		// If the current document just became implicit (i.e. user closed it), we need to get a new one to show.
		if (currentDocument() == this)
			m_window->currentDocumentClosed();
	}
}

LDGLData* LDDocument::glData()
{
	return m_gldata;
}

// =============================================================================
//
// Performs safety checks. Do this before closing any files!
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
bool LDDocument::save (QString path, int64* sizeptr)
{
	if (isCache())
		return false;

	if (not path.length())
		path = fullPath();

	// If the second object in the list holds the file name, update that now.
	LDObject* nameObject = getObject (1);

	if (nameObject and nameObject->type() == OBJ_Comment)
	{
		LDComment* nameComment = static_cast<LDComment*> (nameObject);

		if (nameComment->text().left (6) == "Name: ")
		{
			QString newname = shortenName (path);
			nameComment->setText (format ("Name: %1", newname));
			m_window->buildObjectList();
		}
	}

	QByteArray data;

	if (sizeptr)
		*sizeptr = 0;

	// File is open, now save the model to it. Note that LDraw requires files to have DOS line endings.
	for (LDObject* obj : objects())
	{
		QByteArray subdata ((obj->asText() + "\r\n").toUtf8());
		data.append (subdata);

		if (sizeptr)
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

static int32 StringToNumber (QString a, bool* ok = nullptr)
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
					for_enum (BfcStatement, i)
					{
						if (commentTextSimplified == format ("BFC %1", LDBfc::statementToString (i)))
							return LDSpawn<LDBfc> (i);
					}

					// MLCAD is notorious for stuffing these statements in parts it
					// creates. The above block only handles valid statements, so we
					// need to handle MLCAD-style invertnext, clip and noclip separately.
					if (commentTextSimplified == "BFC CERTIFY INVERTNEXT")
						return LDSpawn<LDBfc> (BfcStatement::InvertNext);
					else if (commentTextSimplified == "BFC CERTIFY CLIP")
						return LDSpawn<LDBfc> (BfcStatement::Clip);
					else if (commentTextSimplified == "BFC CERTIFY NOCLIP")
						return LDSpawn<LDBfc> (BfcStatement::NoClip);
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
				LDDocument* document = g_win->documents()->getDocumentByName (tokens[14]);

				// If we cannot open the file, mark it an error. Note we cannot use LDParseError
				// here because the error object needs the document reference.
				if (not document)
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
				obj->setFileInfo (document);
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
void LDDocument::reloadAllSubfiles()
{
	print ("Reloading subfiles of %1", getDisplayName());

	// Go through all objects in the current file and reload the subfiles
	for (LDObject* obj : objects())
	{
		if (obj->type() == OBJ_Subfile)
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			LDDocument* fileInfo = m_documents->getDocumentByName (ref->fileInfo()->name());

			if (fileInfo)
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
		m_window->buildObjectList();
}

// =============================================================================
//
int LDDocument::addObject (LDObject* obj)
{
	history()->add (new AddHistoryEntry (objects().size(), obj));
	m_objects << obj;
	addKnownVertices (obj);
	obj->setDocument (this);
	m_window->renderer()->compileObject (obj);
	return getObjectCount() - 1;
}

// =============================================================================
//
void LDDocument::addObjects (const LDObjectList& objs)
{
	for (LDObject* obj : objs)
	{
		if (obj)
			addObject (obj);
	}
}

// =============================================================================
//
void LDDocument::insertObj (int pos, LDObject* obj)
{
	history()->add (new AddHistoryEntry (pos, obj));
	m_objects.insert (pos, obj);
	obj->setDocument (this);
	m_window->renderer()->compileObject (obj);
	

#ifdef DEBUG
	if (not isCache())
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

		if (not isCache() and not m_beingDestroyed)
		{
			history()->add (new DelHistoryEntry (idx, obj));
			m_objectVertices.remove (obj);
		}

		m_objects.removeAt (idx);
		obj->setDocument (nullptr);
	}
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
		m_history->add (new EditHistoryEntry (idx, oldcode, newcode));
	}

	m_objectVertices.remove (m_objects[idx]);
	m_objects[idx]->deselect();
	m_objects[idx]->setDocument (nullptr);
	obj->setDocument (this);
	addKnownVertices (obj);
	m_window->renderer()->compileObject (obj);
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
	return not isCache() and history()->position() != savePosition();
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

			if (data)
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
	LDObjectList objs;

	if (m_manager->preInline (this, objs, deep, renderinline))
		return objs; // Manager dealt with this inline

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
void LDDocument::addToSelection (LDObject* obj) // [protected]
{
	if (obj->isSelected() and obj->document() == this)
	{
		m_sel << obj;
		m_window->renderer()->compileObject (obj);
	}
}

// =============================================================================
//
void LDDocument::removeFromSelection (LDObject* obj) // [protected]
{
	if (not obj->isSelected() and obj->document() == this)
	{
		m_sel.removeOne (obj);
		m_window->renderer()->compileObject (obj);
	}
}

// =============================================================================
//
void LDDocument::clearSelection()
{
	for (LDObject* obj : m_sel)
	{
		obj->deselect();
		m_window->renderer()->compileObject (obj);
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
		addToHistory (new SwapHistoryEntry (one->id(), other->id()));
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