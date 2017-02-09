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
#include "main.h"
#include "ldObject.h"

/*
 * This class represents a LDraw model, consisting of a vector of objects. It manages LDObject ownership.
 */
class Model : public QObject
{
	Q_OBJECT

public:
	Model(DocumentManager* manager);
	Model(const Model& other) = delete;
	~Model();

	void addObject(LDObject* object);
	virtual void insertObject(int position, LDObject* object);
	virtual bool swapObjects(LDObject* one, LDObject* other);
	virtual bool setObjectAt(int idx, LDObject* obj);
	template<typename T, typename... Args> T* emplace(Args&& ...args);
	template<typename T, typename... Args> T* emplaceAt(int position, Args&& ...args);
	template<typename T, typename... Args> T* emplaceReplacement(LDObject* object, Args&& ...args);
	template<typename T, typename... Args> T* emplaceReplacementAt(int position, Args&& ...args);
	void removeAt(int position);
	void remove(LDObject* object);
	void replace(LDObject *object, Model& model);
	void clear();
	void merge(Model& other, int position = -1);
	int size() const;
	const QVector<LDObject*>& objects() const;
	LDObject* getObject(int position) const;
	void recountTriangles();
	int triangleCount() const;
	QVector<LDObject*>::iterator begin();
	QVector<LDObject*>::iterator end();
	bool isEmpty() const;
	DocumentManager* documentManager() const;
	LDObject* insertFromString(int position, QString line);
	LDObject* addFromString(QString line);
	LDObject* replaceWithFromString(LDObject* object, QString line);

signals:
	void objectAdded(LDObject* object);
	void aboutToRemoveObject(LDObject* object);
	void objectModified(LDObject* object);

protected:
	template<typename T, typename... Args> T* constructObject(Args&& ...args);
	void withdraw(LDObject* object);
	virtual LDObject* withdrawAt(int position);

	QVector<LDObject*> _objects;
	DocumentManager* _manager;
	mutable int _triangleCount = 0;
	mutable bool _needsTriangleRecount;
};

int countof(Model& model);

/*
 * Given an LDObject type as the template parameter, and any number of variadic parameters, constructs an LDObject derivative
 * and inserts it into this model. The variadic parameters and this model pointer are passed to the constructor. The constructed object
 * is added to the end of the model.
 *
 * For instance, the LDLine contains a constructor as such:
 *
 *     LDLine(Vertex v1, Vertex v2, Model* model);
 *
 * This constructor can be invoked as such:
 *
 *     model->emplace<LDLine>(v1, v2);
 */
template<typename T, typename... Args>
T* Model::emplace(Args&& ...args)
{
	T* object = constructObject<T>(args...);
	addObject(object);
	return object;
}

/*
 * Like emplace<>() but also takes a position as the first argument and emplaces the object at the given position instead of the
 * end of the model.
 */
template<typename T, typename... Args>
T* Model::emplaceAt(int position, Args&& ...args)
{
	T* object = constructObject<T>(args...);
	insertObject(position, object);
	return object;
}

/*
 * Like emplace<>() but instead of inserting the constructed object, the new object replaces the object given in the first parameter.
 * If the old object cannot be replaced, the new object will not be constructed at all.
 */
template<typename T, typename... Args>
T* Model::emplaceReplacement(LDObject* object, Args&& ...args)
{
	if (object->model() == this)
	{
		int position = object->lineNumber();
		T* replacement = constructObject<T>(args...);
		setObjectAt(position, replacement);
		return replacement;
	}
	else
		return nullptr;
}

/*
 * Like emplaceAt<>() but instead of inserting the constructed object, it replaces the document at the given spot instead.
 * The replaced object is deleted in the process.
 */
template<typename T, typename... Args>
T* Model::emplaceReplacementAt(int position, Args&& ...args)
{
	T* replacement = constructObject<T>(args...);
	setObjectAt(position, replacement);
	return replacement;
}

/*
 * Constructs an LDObject such that it gets this model as its model pointer.
 */
template<typename T, typename... Args>
T* Model::constructObject(Args&& ...args)
{
	static_assert (std::is_base_of<LDObject, T>::value, "Can only use this function with LDObject-derivatives");
	T* object = new T {args..., this};

	// Set default color. Relying on virtual functions, this cannot be done in the c-tor.
	// TODO: store -1 as the default color
	if (object->isColored())
		object->setColor(object->defaultColor());

	return object;
}