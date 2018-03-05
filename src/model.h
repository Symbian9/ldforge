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

#pragma once
#include <QAbstractListModel>
#include "main.h"
#include "serializer.h"
#include "linetypes/modelobject.h"

class IndexGenerator
{
	class Iterator
	{
	public:
		Iterator(const QAbstractListModel* model, int row) :
			model {model},
			row {row} {}

		Iterator& operator++()
		{
			this->row += 1;
			return *this;
		}

		QModelIndex operator*() const
		{
			return this->model->index(this->row);
		}

		bool operator!=(const Iterator& other)
		{
			return this->row != other.row or this->model != other.model;
		}

	private:
		const QAbstractListModel* model;
		int row;
	};

public:
	IndexGenerator(const QAbstractListModel* model) :
		model {model} {}

	Iterator begin() const
	{
		return {this->model, 0};
	}

	Iterator end() const
	{
		return {this->model, this->model->rowCount()};
	}

private:
	const QAbstractListModel* model;
};

/*
 * This class represents a LDraw model, consisting of a vector of objects. It manages LDObject ownership.
 */
class Model : public QAbstractListModel
{
	Q_OBJECT

public:
	enum
	{
		ObjectIdRole = Qt::UserRole,
	};

	Model(class DocumentManager* manager);
	Model(const Model& other) = delete;
	~Model();

	void insertCopy(int position, LDObject* object);
	void insertFromArchive(int row, Serializer::Archive& archive);
	bool swapObjects(LDObject* one, LDObject* other);
	bool setObjectAt(int idx, Serializer::Archive& archive);
	template<typename T, typename... Args> T* emplace(Args&& ...args);
	template<typename T, typename... Args> T* emplaceAt(int position, Args&& ...args);
	void removeAt(int position);
	void removeAt(const QModelIndex& index);
	void remove(LDObject* object);
	void replace(LDObject *object, Model& model);
	void clear();
	void merge(Model& other, int position = -1);
	int size() const;
	const QVector<LDObject*>& objects() const;
	LDObject* getObject(int position) const;
	Q_SLOT void recountTriangles();
	int triangleCount() const;
	QVector<LDObject*>::iterator begin();
	QVector<LDObject*>::iterator end();
	QModelIndex indexOf(LDObject* object) const;
	bool isEmpty() const;
	class DocumentManager* documentManager() const;
	LDObject* insertFromString(int position, QString line);
	LDObject* addFromString(QString line);
	LDObject* replaceWithFromString(LDObject* object, QString line);
	IndexGenerator indices() const;
	LDObject* lookup(const QModelIndex& index) const;
	QModelIndex indexFromId(qint32 id) const;

	int rowCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role) const override;

signals:
	void objectAdded(const QModelIndex& object);
	void aboutToRemoveObject(const QModelIndex& index);
	void objectModified(LDObject* object);
	void objectsSwapped(const QModelIndex& index_1, const QModelIndex& index_2);

protected:
	template<typename T, typename... Args> T* constructObject(Args&& ...args);

	QVector<LDObject*> _objects;
	class DocumentManager* _manager;
	mutable int _triangleCount = 0;
	mutable bool _needsTriangleRecount;

private:
	void installObject(int row, LDObject* object);
};

int countof(Model& model);

/*
 * Given an LDObject type as the template parameter, and any number of variadic parameters, constructs an LDObject derivative
 * and inserts it into this model. The variadic parameters and this model pointer are passed to the constructor. The constructed object
 * is added to the end of the model.
 *
 * For instance, the LDLine contains a constructor as such:
 *
 *     LDLine(Vertex v1, Vertex v2);
 *
 * This constructor can be invoked as such:
 *
 *     model->emplace<LDLine>(v1, v2);
 */
template<typename T, typename... Args>
T* Model::emplace(Args&& ...args)
{
	return emplaceAt<T>(size(), args...);
}

/*
 * Like emplace<>() but also takes a position as the first argument and emplaces the object at the given position instead of the
 * end of the model.
 */
template<typename T, typename... Args>
T* Model::emplaceAt(int position, Args&& ...args)
{
	T* object = constructObject<T>(args...);
	installObject(position, object);
	return object;
}

/*
 * Constructs an LDObject such that it gets this model as its model pointer.
 */
template<typename T, typename... Args>
T* Model::constructObject(Args&& ...args)
{
	static_assert (std::is_base_of<LDObject, T>::value, "Can only use this function with LDObject-derivatives");
	T* object = new T {args...};

	// Set default color. Relying on virtual functions, this cannot be done in the c-tor.
	// TODO: store -1 as the default color
	if (object->isColored())
		object->setColor(object->defaultColor());

	return object;
}
