#pragma once
#include "main.h"
#include "ldObject.h"

class Model
{
public:
	Model();
	Model(const Model& other) = delete;
	~Model();

	virtual void addObject(LDObject* object);
	virtual void insertObject(int position, LDObject* object);
	virtual bool swapObjects(LDObject* one, LDObject* other);
	virtual bool setObjectAt(int idx, LDObject* obj);
	void removeAt(int position);
	void remove(LDObject* object);
	void replace(LDObject *object, Model& model);
	void clear();
	void merge(Model& other, int position = -1);
	void replace(LDObject* object, LDObject* newObject);
	int size() const;
	const QVector<LDObject*>& objects() const;
	LDObject* getObject(int position) const;
	void recountTriangles();
	int triangleCount() const;
	QVector<LDObject*>::iterator begin();
	QVector<LDObject*>::iterator end();
	bool isEmpty() const;

	template<typename T, typename... Args>
	T* emplace(Args&& ...args)
	{
		T* object = constructObject<T>(args...);
		addObject(object);
		return object;
	}

	template<typename T, typename... Args>
	T* emplaceAt(int position, Args&& ...args)
	{
		T* object = constructObject<T>(args...);
		insertObject(position, object);
		return object;
	}

	template<typename T, typename... Args>
	T* emplaceReplacement(LDObject* object, Args&& ...args)
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

	template<typename T, typename... Args>
	T* emplaceReplacementAt(int position, Args&& ...args)
	{
		T* replacement = constructObject<T>(args...);
		setObjectAt(position, replacement);
		return replacement;
	}

protected:
	template<typename T, typename... Args>
	T* constructObject(Args&& ...args)
	{
		static_assert (std::is_base_of<LDObject, T>::value, "Can only use this function with LDObject-derivatives");
		T* object = new T {args..., this};

		// Set default color. Relying on virtual functions, this cannot be done in the c-tor.
		// TODO: store -1 as the default color
		if (object->isColored())
			object->setColor(object->defaultColor());

		return object;
	}

	void withdraw(LDObject* object);
	virtual LDObject* withdrawAt(int position);

	QVector<LDObject*> _objects;
	mutable int _triangleCount = 0;
	mutable bool _needsTriangleRecount;
};
