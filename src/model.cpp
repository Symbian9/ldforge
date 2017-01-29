#include "model.h"
#include "ldObject.h"

Model::Model() {}

Model::~Model()
{
	for (int i = 0; i < countof(_objects); ++i)
		delete _objects[i];
}

void Model::addObject(LDObject *object)
{
	insertObject(size(), object);
}

int Model::size() const
{
	return _objects.size();
}

const QVector<LDObject*>& Model::objects() const
{
	return _objects;
}

void Model::insertObject(int position, LDObject* object)
{
	if (object->model() and object->model() != this)
		object->model()->withdraw(object);

	if (not object->model())
	{
		_objects.insert(position, object);
		_needsTriangleRecount = true;
		object->setDocument(this);
		print("Object %1 added to position %2", object->id(), position);
	}
}

bool Model::swapObjects(LDObject* one, LDObject* other)
{
	int a = _objects.indexOf(one);
	int b = _objects.indexOf(other);

	if (a != b and a != -1 and b != -1)
	{
		_objects[b] = one;
		_objects[a] = other;
		return true;
	}
	else
	{
		return false;
	}
}

bool Model::setObjectAt(int idx, LDObject* obj)
{
	if (idx < 0 or idx >= countof(_objects))
	{
		return false;
	}
	else
	{
		removeAt(idx);
		insertObject(idx, obj);
		return true;
	}
}

LDObject* Model::getObject(int position) const
{
	if (position >= 0 and position < countof(_objects))
		return _objects[position];
	else
		return nullptr;
}

void Model::remove(LDObject* object)
{
	int position = object->lineNumber();
	printf("Going to remove %d from %p at %d (there are %d objects)\n", object->id(), this, position, countof(objects()));
	if (_objects[position] == object)
		removeAt(position);
}

void Model::removeAt(int position)
{
	LDObject* object = withdrawAt(position);
	delete object;
}

void Model::replace(LDObject* object, LDObject* newObject)
{
	if (object->model() == this)
		setObjectAt(object->lineNumber(), newObject);
}

void Model::replace(LDObject *object, Model &model)
{
	if (object->model() == this)
	{
		int position = object->lineNumber();

		for (int i = countof(model.objects()) - 1; i >= 1; i -= 1)
			insertObject(position + i, model.objects()[i]);

		setObjectAt(position, model.objects()[0]);
	}
}

void Model::recountTriangles()
{
	_needsTriangleRecount = true;
}

int Model::triangleCount() const
{
	if (_needsTriangleRecount)
	{
		_triangleCount = 0;

		for (LDObject* object : objects())
			_triangleCount += object->triangleCount();

		_needsTriangleRecount = false;
	}

	return _triangleCount;
}

void Model::merge(Model& other, int position)
{
	if (position < 0)
		position = countof(_objects);

	// Copy the vector of objects to copy, so that we don't change the vector while iterating over it.
	QVector<LDObject*> objectsCopy = other._objects;

	// Inform the contents of their new owner
	for (LDObject* object : objectsCopy)
	{
		insertObject(position, object);
		position += 1;
	}

	other.clear();
}

QVector<LDObject*>::iterator Model::begin()
{
	return _objects.begin();
}

QVector<LDObject*>::iterator Model::end()
{
	return _objects.end();
}

void Model::clear()
{
	for (int i = _objects.size() - 1; i >= 0; i -= 1)
		removeAt(i);

	_needsTriangleRecount = true;
}

/*
 * Drops the object from the model. The object becomes a free object as a result (thus violating the invariant that every object
 * has a model!). The caller must immediately add the withdrawn object to another model.
 */
void Model::withdraw(LDObject* object)
{
	if (object->model() == this)
	{
		int position = object->lineNumber();
		print("Withdrawing %1 from %2 at %3\n", object->id(), this, position);

		if (_objects[position] == object)
			withdrawAt(position);
	}
}

/*
 * Drops an object from the model at the provided position. The caller must immediately put the result value object into a new model.
 */
LDObject* Model::withdrawAt(int position)
{
	LDObject* object = _objects[position];
	_objects.removeAt(position);
	_needsTriangleRecount = true;
	return object;
}

bool Model::isEmpty() const
{
	return _objects.isEmpty();
}
