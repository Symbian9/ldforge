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
#include "lddocument.h"
#include "miscallenous.h"
#include "mainwindow.h"
#include "canvas.h"
#include "documentloader.h"
#include "dialogs/openprogressdialog.h"
#include "documentmanager.h"
#include "linetypes/comment.h"

LDDocument::LDDocument (DocumentManager* parent) :
    Model {parent},
	HierarchyElement (parent),
    m_history (new EditHistory (this)),
	m_savePosition(-1),
    m_tabIndex(-1),
	m_manager (parent)
{
	connect(
		this,
		SIGNAL(objectAdded(QModelIndex)),
		this,
		SLOT(handleNewObject(QModelIndex))
	);

	connect(
		this,
		&Model::rowsMoved,
		[&](const QModelIndex&, int start, int end, const QModelIndex&, int row)
		{
			history()->add<MoveHistoryEntry>(start, end, row);
		}
	);
}

LDDocument::~LDDocument()
{
	m_isBeingDestroyed = true;
	delete m_history;
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

void LDDocument::setFrozen(bool value)
{
	m_isFrozen = value;
}

bool LDDocument::isFrozen() const
{
	return m_isFrozen;
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

void LDDocument::close()
{
	if (not isFrozen())
	{
		setFrozen(true);
		m_manager->documentClosed(this);
	}
}

// =============================================================================
//
// Performs safety checks. Do this before closing any files!
//
bool LDDocument::isSafeToClose()
{
	setlocale (LC_ALL, "C");

	// If we have unsaved changes, warn and give the option of saving.
	if (hasUnsavedChanges())
	{
		QString message = format(tr("There are unsaved changes to %1. Should it be saved?"), getDisplayName());

		int button = QMessageBox::question (m_window, tr("Unsaved Changes"), message,
		    (QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel), QMessageBox::Cancel);

		switch (button)
		{
		    case QMessageBox::Yes:
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

					if (QMessageBox::critical (m_window, tr("Save Failure"), message,
					    (QMessageBox::Yes | QMessageBox::No), QMessageBox::No) == QMessageBox::No)
					{
						return false;
					}
				}
				break;
			}

		    case QMessageBox::Cancel:
				return false;

			default:
				break;
		}
	}

	return true;
}

// =============================================================================
//
bool LDDocument::save (QString path, qint64* sizeptr)
{
	if (isFrozen())
		return false;

	if (path.isEmpty())
		path = fullPath();

	// If the second object in the list holds the file name, update that now.
	LDObject* nameObject = getObject (1);

	if (nameObject and nameObject->type() == LDObjectType::Comment)
	{
		LDComment* nameComment = static_cast<LDComment*> (nameObject);

		if (nameComment->text().left (6) == "Name: ")
		{
			QString newname = shortenName (path);
			nameComment->setText (format ("Name: %1", newname));
		}
	}

	QByteArray data;

	// File is open, now save the model to it. Note that LDraw requires files to have DOS line endings.
	for (LDObject* obj : objects())
	{
		if (obj->isInverted())
			data += "0 BFC INVERTNEXT\r\n";

		data += (obj->asText() + "\r\n").toUtf8();
	}

	QFile f (path);

	if (not f.open (QIODevice::WriteOnly))
		return false;

	if (sizeptr)
		*sizeptr = data.size();

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

void LDDocument::handleNewObject(const QModelIndex& index)
{
	LDObject* object = lookup(index);
	history()->add<AddHistoryEntry>(index);
	connect(
		object,
		SIGNAL(codeChanged(LDObjectState, LDObjectState)),
		this,
		SLOT(objectChanged(LDObjectState, LDObjectState))
	);
	connect(
		this,
		SIGNAL(aboutToRemoveObject(QModelIndex)),
		this,
		SLOT(handleImminentObjectRemoval(QModelIndex)),
		Qt::DirectConnection
	);

#ifdef DEBUG
	if (not isFrozen())
		print("Inserted object #%1 (%2) at %3\n", object->id(), object->typeName(), index.row());
#endif
}

void LDDocument::objectChanged(const LDObjectState& before, const LDObjectState& after)
{
	LDObject* object = qobject_cast<LDObject*>(sender());

	if (object)
	{
		QModelIndex index = this->indexOf(object);
		history()->add<EditHistoryEntry>(index, before, after);
		redoVertices();
		emit objectModified(object);
		emit dataChanged(index, index);
	}
}

void LDDocument::handleImminentObjectRemoval(const QModelIndex& index)
{
	LDObject* object = lookup(index);

	if (not isFrozen() and not m_isBeingDestroyed)
	{
		history()->add<DelHistoryEntry>(index);
		m_objectVertices.remove(object);
	}
}

// =============================================================================
//
bool LDDocument::hasUnsavedChanges() const
{
	return not isFrozen() and history()->position() != savePosition();
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
	if (m_needsRecache)
	{
		m_vertices.clear();
		Model model {m_documents};
		inlineContents(model, true, true);

		for (LDObject* obj : model.objects())
		{
			if (obj->type() == LDObjectType::SubfileReference)
			{
				print ("Warning: unable to inline %1 into %2",
					static_cast<LDSubfileReference*> (obj)->referenceName(),
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

		m_needsRecache = false;
	}

	if (m_verticesOutdated)
	{
		m_objectVertices.clear();
		Model model {m_documents};
		inlineContents(model, true, false);

		for (LDObject* object : model)
		{
			auto iterator = m_objectVertices.find (object);

			if (iterator == m_objectVertices.end())
				iterator = m_objectVertices.insert (object, QSet<Vertex>());
			else
				iterator->clear();

			object->getVertices(documentManager(), *iterator);
		}

		m_vertices.clear();

		for (const QSet<Vertex>& vertices : m_objectVertices)
			m_vertices.unite(vertices);

		m_verticesOutdated = false;
	}
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
		if (deep and object->type() == LDObjectType::SubfileReference)
			static_cast<LDSubfileReference*>(object)->inlineContents(documentManager(), model, deep, renderinline);
		else
			model.addFromString(object->asText());
	}
}

// =============================================================================
//
QString LDDocument::shortenName (QString a) // [static]
{
	QString shortname = Basename (a);
	QString topdirname = Basename (Dirname (a));

	if (DocumentManager::specialSubdirectories.contains (topdirname))
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
	m_verticesOutdated = true;
}
