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

#include "model.h"
#include "linetypes/modelobject.h"
#include "documentmanager.h"
#include "generics/migrate.h"
#include "editHistory.h"
#include "lddocument.h"

Model::Model(DocumentManager* manager) :
	QAbstractListModel {manager},
    _manager {manager} {}

Model::~Model()
{
	for (int i = 0; i < countof(_objects); ++i)
		delete _objects[i];
}

void Model::installObject(int row, LDObject* object)
{
	connect(
		object,
		&LDObject::modified,
		[&]()
		{
			this->recountTriangles();
			emit objectModified(static_cast<LDObject*>(sender()));
			emit modelChanged();
		}
	);

	beginInsertRows({}, row, row);
	_objects.insert(row, object);

	if (this->pickingColorCursor <= 0xffffff)
	{
		this->pickingColors[object] = this->pickingColorCursor;
		this->pickingColorCursor += 1;
	}

	_triangleCount += object->triangleCount(documentManager());
	emit objectAdded(index(row));
	emit modelChanged();
	endInsertRows();
}

/*
 * Returns the amount of objects in this model.
 */
int Model::size() const
{
	return _objects.size();
}

/*
 * Returns the vector of objects in this model.
 */
const QVector<LDObject*>& Model::objects() const
{
	return _objects;
}

/*
 * Takes an existing object and copies it to this model, at the specified position.
 */
void Model::insertCopy(int position, LDObject* object)
{
	installObject(position, Serializer::clone(object));
}

void Model::insertFromArchive(int row, Serializer::Archive& archive)
{
	LDObject* object = Serializer::restore(archive);

	if (object)
		installObject(row, object);
}

bool Model::moveRows(
	const QModelIndex&,
	int sourceRow,
	int count,
	const QModelIndex&,
	int destinationRow
) {
	int sourceRowLast = sourceRow + count - 1;

	if (range(0, 1, size() - count).contains(sourceRow)
		and range(0, 1, size()).contains(destinationRow)
	) {
		beginMoveRows({}, sourceRow, sourceRowLast, {}, destinationRow);
		migrate(_objects, sourceRow, sourceRowLast, destinationRow);
		endMoveRows();
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Assigns a new object to the specified position in the model. The object that already is in the position is deleted in the process.
 */
bool Model::setObjectAt(int row, Serializer::Archive& archive)
{
	LDObject* object = Serializer::restore(archive);

	if (object and row >= 0 and row < countof(_objects))
	{
		removeAt(row);
		insertCopy(row, object);
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Returns the object at the specified position, or null if not found.
 */
LDObject* Model::getObject(int position) const
{
	if (position >= 0 and position < countof(_objects))
		return _objects[position];
	else
		return nullptr;
}

/*
 * Removes the given object from the model.
 */
void Model::remove(LDObject* object)
{
	int position = indexOf(object).row();

	if (_objects[position] == object)
		removeAt(position);
}

/*
 * Removes the object at the provided position.
 */
void Model::removeAt(int position)
{
	beginRemoveRows({}, position, position);
	LDObject* object = _objects[position];
	emit aboutToRemoveObject(this->index(position));
	_objects.removeAt(position);
	_needsTriangleRecount = true;
	endRemoveRows();
	emit modelChanged();
	delete object;
}

void Model::removeAt(const QModelIndex& index)
{
	removeAt(index.row());
}

/*
 * Replaces the given object with the contents of a model.
 */
void Model::replace(LDObject* object, Model& model)
{
	QModelIndex index = this->indexOf(object);

	if (index.isValid())
	{
		removeAt(index.row());
		int position = index.row();

		for (signed int i = 0; i < countof(model); i += 1)
			insertCopy(position + i, model.objects()[i]);
	}
}

/*
 * Signals the model to recount its triangles.
 */
void Model::recountTriangles()
{
	_needsTriangleRecount = true;
}

/*
 * Returns the triangle count in the model.
 */
int Model::triangleCount() const
{
	if (_needsTriangleRecount)
	{
		_triangleCount = 0;

		for (LDObject* object : objects())
			_triangleCount += object->triangleCount(documentManager());

		_needsTriangleRecount = false;
	}

	return _triangleCount;
}

/*
 * Merges the given model into this model, starting at the given position. The other model is emptied in the process.
 */
void Model::merge(Model& other, int position)
{
	if (position < 0)
		position = countof(_objects);

	// Copy the vector of objects to copy, so that we don't change the vector while iterating over it.
	QVector<LDObject*> objectsCopy = other._objects;

	// Inform the contents of their new owner
	for (LDObject* object : objectsCopy)
	{
		insertCopy(position, object);
		position += 1;
	}

	other.clear();
}

/*
 * Returns the begin-iterator into this model, so that models can be used in foreach-loops.
 */
QVector<LDObject*>::iterator Model::begin()
{
	return _objects.begin();
}

/*
 * Returns the end-iterator into this mode, so that models can be used in foreach-loops.
 */
QVector<LDObject*>::iterator Model::end()
{
	return _objects.end();
}

/*
 * Removes all objects in this model.
 */
void Model::clear()
{
	for (int i = _objects.size() - 1; i >= 0; i -= 1)
		removeAt(i);

	_triangleCount = 0;
	_needsTriangleRecount = false;
}

/*
 * Returns whether or not this model is empty.
 */
bool Model::isEmpty() const
{
	return _objects.isEmpty();
}

/*
 * Returns an index that corresponds to the given object
 */
QModelIndex Model::indexOf(LDObject* object) const
{
	for (int i = 0; i < this->size(); ++i)
	{
		if (this->getObject(i) == object)
			return index(i);
	}

	return {};
}

/*
 * Returns the model's associated document manager. This pointer is used to resolve subfile references.
 */
DocumentManager* Model::documentManager() const
{
	return _manager;
}

IndexGenerator Model::indices() const
{
	return {this};
}

int Model::rowCount(const QModelIndex&) const
{
	return this->objects().size();
}

QVariant Model::data(const QModelIndex& index, int role) const
{
	static QMap<QString, QPixmap> scaledIcons;

	if (index.row() < 0 or index.row() >= size())
		return {};

	LDObject* object = this->objects()[index.row()];

	switch (role)
	{
	case Qt::DisplayRole:
		{
			QString result = object->objectListText();

			if (object->isInverted())
				result.prepend("↺ ");

			if (object->type() == LDObjectType::SubfileReference)
			{
				LDSubfileReference* reference = static_cast<LDSubfileReference*>(object);
				LDDocument* subfile = reference->fileInfo(this->documentManager());
				if (subfile != nullptr)
					result.prepend(subfile->header.description + "\n");
			}

			return result;
		}

	case Qt::DecorationRole:
		{
			QString iconName = object->iconName();
			auto it = scaledIcons.find(iconName);
			if (it == scaledIcons.end())
			{
				QPixmap pixmap = MainWindow::getIcon(object->iconName())
					.scaled({24, 24}, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				it = scaledIcons.insert(iconName, pixmap);
			}
			return *it;
		}

	case Qt::BackgroundColorRole:
		if (object->type() == LDObjectType::Error)
			return QColor {"#AA0000"};
		else
			return {};

	case Qt::ForegroundRole:
		if (object->type() == LDObjectType::Error)
		{
			return QColor {"#FFAA00"};
		}
		else if (
			object->isColored()
			and object->color().isValid()
			and object->color() != MainColor
			and object->color() != EdgeColor
		) {
			// If the object isn't in the main or edge color, draw this list
			// entry in that color.
			return object->color().faceColor();
		}
		else
		{
			return {};
		}

	case Qt::FontRole:
		if (object->isHidden())
		{
			QFont font;
			font.setItalic(true);
			return font;
		}
		else
		{
			return {};
		}

	default:
		return {};
	}
}

/*
bool Model::removeRows(int row, int count, const QModelIndex& parent)
{
	if (row >= 0 and row < size() and count <= size() - row)
	{
		beginRemoveRows(parent, row, row + count - 1);

		for (signed int i = row + count - 1; i >= row; i -= 1)
			this->removeAt(i);

		endRemoveRows();
		return true;
	}
	else
	{
		return false;
	}
}
*/

/*
 * Looks up an object by the given index.
 */
LDObject* Model::lookup(const QModelIndex &index) const
{
	if (index.row() >= 0 and index.row() < size())
		return this->objects()[index.row()];
	else
		return nullptr;
}

QColor Model::pickingColorForObject(const QModelIndex& objectIndex) const
{
	return QColor::fromRgba(this->pickingColors.value(this->lookup(objectIndex)) | 0xff000000);
}

QModelIndex Model::objectByPickingColor(const QColor& color) const
{
	if (color != qRgb(0, 0, 0))
	{
		for (int row = 0; row < this->size(); row += 1)
		{
			if (this->pickingColorForObject(this->index(row)) == color)
				return this->index(row);
		}

		return {};
	}
	else
	{
		return {};
	}
}

Winding Model::winding() const
{
	return this->_winding;
}

void Model::setWinding(Winding winding)
{
	this->_winding = winding;
	emit windingChanged(this->_winding);
}

int countof(Model& model)
{
	return model.size();
}
