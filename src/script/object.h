#pragma once
#include "../main.h"
#include "objtype.h"

namespace Script
{
	using ObjectPointer = QSharedPointer<class Object>;

	class Object
	{
	public:
		Object();

	private:
		ObjectType m_type;
	};

	class IntObject : public Object
	{
	private:
		qint32 m_value;
	};

	class RealObject : public Object
	{
	private:
		qreal m_value;
	};

	class StringObject : public Object
	{
	private:
		QString m_value;
	};

	class ContainerObject : public Object
	{
	private:
		QList<ObjectPointer> m_elements;
	}
}
