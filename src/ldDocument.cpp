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
	m_flags(IsCache | VerticesOutdated | NeedsVertexMerge | NeedsRecache),
	m_savePosition(-1),
    m_tabIndex(-1),
	m_gldata (new LDGLData),
	m_manager (parent) {}

LDDocument::~LDDocument()
{
	m_flags |= IsBeingDestroyed;
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
	if (isCache())
	{
		m_flags &= ~IsCache;
		print ("Opened %1", name());

		// Cache files are not compiled by the GL renderer. Now that this file is open for editing, it needs to be
		// compiled.
		m_window->renderer()->compiler()->compileDocument (this);
		m_window->updateDocumentList();
	}
}

bool LDDocument::isCache() const
{
	return !!(m_flags & IsCache);
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
		m_flags |= IsCache;
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
				if (name().isEmpty())
				{
					QString newpath = QFileDialog::getSaveFileName (m_window, QObject::tr ("Save As"),
						name(), QObject::tr ("LDraw files (*.dat *.ldr)"));

					if (newpath.isEmpty())
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

	if (path.isEmpty())
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
			*sizeptr += countof(subdata);
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
static void CheckTokenCount (const QStringList& tokens, int num)
{
	if (countof(tokens) != num)
		throw QString (format ("Bad amount of tokens, expected %1, got %2", num, countof(tokens)));
}

// =============================================================================
//
static void CheckTokenNumbers (const QStringList& tokens, int min, int max)
{
	bool ok;
	QRegExp scientificRegex ("\\-?[0-9]+\\.[0-9]+e\\-[0-9]+");

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
		if (scientificRegex.exactMatch (tokens[i]))
			return;

		throw QString (format ("Token #%1 was `%2`, expected a number (matched length: %3)",
			(i + 1), tokens[i], scientificRegex.matchedLength()));
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

		if (countof(tokens) <= 0)
		{
			// Line was empty, or only consisted of whitespace
			return LDSpawn<LDEmpty>();
		}

		if (countof(tokens[0]) != 1 or not tokens[0][0].isDigit())
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
				if (countof(tokens) > 2 and tokens[1] == "BFC")
				{
					for (BfcStatement statement : iterateEnum<BfcStatement>())
					{
						if (commentTextSimplified == format("BFC %1", LDBfc::statementToString (statement)))
							return LDSpawn<LDBfc>(statement);
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

				if (countof(tokens) > 2 and tokens[1] == "!LDFORGE")
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
					else if (tokens[2] == "BEZIER_CURVE")
					{
						CheckTokenCount (tokens, 16);
						CheckTokenNumbers (tokens, 3, 15);
						LDBezierCurve* obj = LDSpawn<LDBezierCurve>();
						obj->setColor (StringToNumber (tokens[3]));

						for (int i = 0; i < 4; ++i)
							obj->setVertex (i, ParseVertex (tokens, 4 + (i * 3)));

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

				LDSubfileReference* obj = LDSpawn<LDSubfileReference>();
				obj->setColor (StringToNumber (tokens[1]));
				obj->setPosition (ParseVertex (tokens, 2));  // 2 - 4

				Matrix transform;

				for (int i = 0; i < 9; ++i)
					transform.value(i) = tokens[i + 5].toDouble(); // 5 - 13

				obj->setTransformationMatrix (transform);
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
		if (obj->type() == OBJ_SubfileReference)
		{
			LDSubfileReference* reference = static_cast<LDSubfileReference*> (obj);
			LDDocument* fileInfo = m_documents->getDocumentByName (reference->fileInfo()->name());

			if (fileInfo)
				reference->setFileInfo (fileInfo);
			else
				emplaceReplacement<LDError>(reference, reference->asText(), format("Could not open %1", reference->fileInfo()->name()));
		}

		// Reparse gibberish files. It could be that they are invalid because
		// of loading errors. Circumstances may be different now.
		if (obj->type() == OBJ_Error)
			obj->replace (ParseLine (static_cast<LDError*> (obj)->contents()));
	}

	m_flags |= NeedsRecache;

	if (this == m_window->currentDocument())
		m_window->buildObjectList();
}

// =============================================================================
//
void LDDocument::addObjects (const LDObjectList& objects)
{
	for (LDObject* object : objects)
	{
		if (object)
			addObject (object);
	}
}

// =============================================================================
//
void LDDocument::insertObject (int pos, LDObject* obj)
{
	Model::insertObject(pos, obj);
	history()->add(new AddHistoryEntry {pos, obj});
	m_window->renderer()->compileObject(obj);
	connect(obj, SIGNAL(codeChanged(int,QString,QString)), this, SLOT(objectChanged(int,QString,QString)));

#ifdef DEBUG
	if (not isCache())
		dprint ("Inserted object #%1 (%2) at %3\n", obj->id(), obj->typeName(), pos);
#endif
}

void LDDocument::objectChanged(int position, QString before, QString after)
{
	LDObject* object = static_cast<LDObject*>(sender());
	addToHistory(new EditHistoryEntry {position, before, after});
	m_window->renderer()->compileObject(object);
	m_window->currentDocument()->redoVertices();
}

// =============================================================================
//
void LDDocument::addKnownVertices (LDObject* obj)
{
	auto it = m_objectVertices.find (obj);

	if (it == m_objectVertices.end())
		it = m_objectVertices.insert (obj, QSet<Vertex>());
	else
		it->clear();

	obj->getVertices (*it);
	needVertexMerge();
}

LDObject* LDDocument::withdrawAt(int position)
{
	LDObject* object = getObject(position);

	if (not isCache() and not checkFlag(IsBeingDestroyed))
	{
		history()->add(new DelHistoryEntry {position, object});
		m_objectVertices.remove(object);
	}

	m_selection.remove(object);
	return Model::withdrawAt(position);
}

// =============================================================================
//
int LDDocument::getObjectCount() const
{
	return countof(objects());
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
	if (checkFlag(NeedsRecache))
	{
		m_vertices.clear();
		Model model;
		inlineContents(model, true, true);

		for (LDObject* obj : model.objects())
		{
			if (obj->type() == OBJ_SubfileReference)
			{
				print ("Warning: unable to inline %1 into %2",
					static_cast<LDSubfileReference*> (obj)->fileInfo()->getDisplayName(),
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

		unsetFlag(NeedsRecache);
	}

	if (checkFlag(VerticesOutdated))
	{
		m_objectVertices.clear();
		Model model;
		inlineContents(model, true, false);

		for (LDObject* obj : model)
			addKnownVertices (obj);

		mergeVertices();
		unsetFlag(VerticesOutdated);
	}

	if (checkFlag(NeedsVertexMerge))
		mergeVertices();
}

// =============================================================================
//
void LDDocument::mergeVertices()
{
	m_vertices.clear();

	for (const QSet<Vertex>& vertices : m_objectVertices)
		m_vertices.unite(vertices);

	unsetFlag(NeedsVertexMerge);
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
void LDDocument::inlineContents(Model& model, bool deep, bool renderinline)
{
	if (m_manager->preInline(this, model, deep, renderinline))
		return; // Manager dealt with this inline

	for (LDObject* object : objects())
	{
		// Skip those without scemantic meaning
		if (not object->isScemantic())
			continue;

		// Got another sub-file reference, inline it if we're deep-inlining. If not,
		// just add it into the objects normally. Yay, recursion!
		if (deep and object->type() == OBJ_SubfileReference)
			static_cast<LDSubfileReference*>(object)->inlineContents(model, deep, renderinline);
		else
			model.addObject(object->createCopy());
	}
}

// =============================================================================
//
void LDDocument::addToSelection (LDObject* obj) // [protected]
{
	if (not m_selection.contains(obj) and obj->model() == this)
	{
		m_selection.insert(obj);
		m_window->renderer()->compileObject (obj);

		// If this object is inverted with INVERTNEXT, select the INVERTNEXT as well.
		LDBfc* invertnext;

		if (obj->previousIsInvertnext(invertnext))
			addToSelection(invertnext);
	}
}

// =============================================================================
//
void LDDocument::removeFromSelection (LDObject* obj) // [protected]
{
	if (m_selection.contains(obj))
	{
		m_selection.remove(obj);
		m_window->renderer()->compileObject (obj);

		// If this object is inverted with INVERTNEXT, deselect the INVERTNEXT as well.
		LDBfc* invertnext;

		if (obj->previousIsInvertnext(invertnext))
			removeFromSelection(invertnext);
	}
}

// =============================================================================
//
void LDDocument::clearSelection()
{
	for (LDObject* object : m_selection)
		m_window->renderer()->compileObject(object);

	m_selection.clear();
}

// =============================================================================
//
const QSet<LDObject*>& LDDocument::getSelection() const
{
	return m_selection;
}

// =============================================================================
//
bool LDDocument::swapObjects (LDObject* one, LDObject* other)
{
	if (Model::swapObjects(one, other))
	{
		addToHistory(new SwapHistoryEntry {one->id(), other->id()});
		return true;
	}
	else
	{
		return false;
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
const QSet<Vertex>& LDDocument::inlineVertices()
{
	initializeCachedData();
	return m_vertices;
}

void LDDocument::redoVertices()
{
	setFlag(VerticesOutdated);
}

void LDDocument::needVertexMerge()
{
	setFlag(NeedsVertexMerge);
}

