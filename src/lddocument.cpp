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

#include <QMessageBox>
#include <QFileDialog>
#include "lddocument.h"
#include "documentmanager.h"
#include "parser.h"
#include "editHistory.h"
#include "glShared.h"

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
	connect(
		this,
		SIGNAL(aboutToRemoveObject(QModelIndex)),
		this,
		SLOT(handleImminentObjectRemoval(QModelIndex)),
		Qt::DirectConnection
	);
	connect(
		this,
		&Model::modelChanged,
		[&]()
		{
			this->m_needsRecache = true;
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
	return this->header.name;
}

void LDDocument::setName (QString value)
{
	this->header.name = value;
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

static QString headerToString(const Model& model, const LDHeader& header)
{
	QString result;

	if (header.type != LDHeader::NoHeader)
	{
		QString partTypeString;

		for (
			auto iterator = Parser::typeStrings.begin();
			iterator != Parser::typeStrings.end();
			++iterator
		) {
			if (iterator.value() == header.type)
			{
				partTypeString += "Unofficial_" + iterator.key();
				break;
			}
		}

		if (header.qualfiers & LDHeader::Physical_Color)
			partTypeString += " Physical_Colour";

		if (header.qualfiers & LDHeader::Flexible_Section)
			partTypeString += " Flexible_Section";

		if (header.qualfiers & LDHeader::Alias)
			partTypeString += " Alias";

		result += "0 " + header.description + "\r\n";
		result += "0 Name: " + header.name + "\r\n";
		result += "0 Author: " + header.author + "\r\n";
		result += "0 !LDRAW_ORG " + partTypeString + "\r\n";

		switch (header.license)
		{
		case LDHeader::CaLicense:
			result += "0 !LICENSE Redistributable under CCAL version 2.0 : see CAreadme.txt\r\n";
			break;
		case LDHeader::NonCaLicense:
			result += "0 !LICENSE Not redistributable : see NonCAreadme.txt\r\n";
			break;
		case LDHeader::UnspecifiedLicense:
			break;
		}

		if (not header.help.isEmpty())
		{
			result += "\r\n";
			for (QString line : header.help.split("\n"))
				result += "0 !HELP " + line + "\r\n";
		}

		result += "\r\n";

		switch (model.winding())
		{
		case CounterClockwise:
			result += "0 BFC CERTIFY CCW\r\n";
			break;

		case Clockwise:
			result += "0 BFC CERTIFY CW\r\n";
			break;

		case NoWinding:
			result += "0 BFC NOCERTIFY\r\n";
			break;
		}

		if (not header.category.isEmpty() or not header.keywords.isEmpty())
		{
			result += "\r\n";

			if (not header.category.isEmpty())
				result += "0 !CATEGORY " + header.category + "\r\n";

			if (not header.keywords.isEmpty())
			{
				for (QString line : header.keywords.split("\n"))
					result += "0 !KEYWORDS " + line + "\r\n";
			}
		}

		if (not header.cmdline.isEmpty())
		{
			result += "\r\n";
			result += "0 !CMDLINE " + header.cmdline + "\r\n";
		}

		if (not header.history.isEmpty())
		{
			result += "\r\n";

			for (const LDHeader::HistoryEntry& historyEntry : header.history)
			{
				QString author = historyEntry.author;

				if (not author.startsWith("{"))
					author = "[" + author + "]";

				result += "0 !HISTORY ";
				result += historyEntry.date.toString(Qt::ISODate) + " ";
				result += author + " ";
				result += historyEntry.description + "\r\n";
			}
		}

		result += "\r\n";
	}

	return result.toUtf8();
}

// =============================================================================
//
bool LDDocument::save (QString path, qint64* sizeptr)
{
	if (isFrozen())
		return false;

	if (path.isEmpty())
		path = fullPath();

	QByteArray data;

	if (this->header.type != LDHeader::NoHeader)
	{
		header.name = LDDocument::shortenName(path);
		data += headerToString(*this, this->header).toUtf8();
	}

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
		SIGNAL(modified(LDObjectState, LDObjectState)),
		this,
		SLOT(objectChanged(LDObjectState, LDObjectState))
	);
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
		Model model {m_documents};
		this->inlineContents(model, true, true);

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

/*
 * Inlines this document into the given model
 */
void LDDocument::inlineContents(Model& model, bool deep, bool renderinline)
{
	// Protect against circular references by not inling if called by recursion again.
	if (not m_isInlining)
	{
		m_isInlining = true;

		// First ask the manager to deal with this inline (this takes logoed studs into account)
		if (not m_manager->preInline(this, model, deep, renderinline))
		{
			for (LDObject* object : objects())
			{
				// Skip those without effect on the model meaning
				if (object->isScemantic())
				{
					// Got another sub-file reference, recurse and inline it too if we're deep-inlining.
					// If not, just add it into the objects normally.
					if (deep and object->type() == LDObjectType::SubfileReference)
					{
						LDSubfileReference* reference = static_cast<LDSubfileReference*>(object);
						reference->inlineContents(
							documentManager(),
							this->winding(),
							model,
							deep,
							renderinline
						);
					}
					else
					{
						model.insertCopy(model.size(), object);
					}
				}
			}
		}

		m_isInlining = false;
	}
}

// =============================================================================
//
QString LDDocument::shortenName(const QFileInfo& path) // [static]
{
	QString shortname = path.fileName();
	QString topdirname = QFileInfo {path.absoluteFilePath()}.dir().dirName();

	if (isOneOf(topdirname, "s", "48", "8"))
		return topdirname + "\\" + shortname;
	else
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

decltype(LDHeader::license) LDHeader::defaultLicense()
{
	if (config::useCaLicense())
		return LDHeader::CaLicense;
	else
		return LDHeader::UnspecifiedLicense;
}
