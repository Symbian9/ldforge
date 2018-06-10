#include "../algorithms/geometry.h"
#include "../glShared.h"
#include "../model.h"
#include "../algorithms/invert.h"
#include "circularprimitive.h"
#include "quadrilateral.h"
#include "primitives.h"

QString LDCircularPrimitive::buildFilename() const
{
	int numerator = this->m_segments;
	int denominator = this->m_divisions;
	QString prefix;

	if (m_divisions != MediumResolution)
		prefix = QString::number(m_divisions) + '\\';

	simplify(numerator, denominator);

	// Ensure that the denominator is at least 4, expand if necessary
	if (denominator < 4)
	{
		numerator = static_cast<int>(round(numerator * 4.0 / denominator));
		denominator = 4;
	}

	return format("%1%2-%3%4.dat", prefix, numerator, denominator, stem());
}

LDCircularPrimitive::LDCircularPrimitive(
	PrimitiveModel::Type type,
	int segments,
	int divisions,
	const Matrix& transformationMatrix,
	const Vertex& position
) :
	LDMatrixObject {transformationMatrix, position},
	m_type {type},
	m_segments {segments},
	m_divisions {divisions} {}

LDObjectType LDCircularPrimitive::type() const
{
	return SubclassType;
}

QString LDCircularPrimitive::asText() const
{
	return LDSubfileReference(buildFilename(), transformationMatrix(), position()).asText();
}

void LDCircularPrimitive::getVertices(DocumentManager* /* context */, QSet<Vertex>& vertices) const
{
	int endSegment = (m_segments == m_divisions) ? m_segments : m_segments + 1;

	for (int i = 0; i < endSegment; i += 1)
	{
		QPointF point2d = pointOnLDrawCircumference(i, m_divisions);

		for (double y_value : {0.0, 1.0})
		{
			Vertex vertex {point2d.x(), y_value, point2d.y()};
			vertex.transform(transformationMatrix(), position());
			vertices.insert(vertex);
		}
	}
}

bool LDCircularPrimitive::isRasterizable() const
{
	return true;
}

void LDCircularPrimitive::rasterize(
	DocumentManager* context,
	Winding /* parentWinding */,
	Model& model,
	bool /* deep */,
	bool /* render */
) {
	Model cylinderBody {context};
	buildPrimitiveBody(cylinderBody);

	for (LDObject* object : cylinderBody.objects())
	{
		for (int i = 0; i < object->numVertices(); i += 1)
		{
			Vertex vertex = object->vertex(i);
			vertex.transform(transformationMatrix(), position());
			object->setVertex(i, vertex);
		}
	}

	model.merge(cylinderBody);
}

QVector<LDPolygon> LDCircularPrimitive::rasterizePolygons(DocumentManager* context, Winding winding)
{
	Model cylinderBody {context};
	buildPrimitiveBody(cylinderBody);
	QVector<LDPolygon> result;
	bool cachedShouldInvert = shouldInvert(winding, context);

	for (LDObject* object : cylinderBody.objects())
	{
		for (int i = 0; i < object->numVertices(); i += 1)
		{
			Vertex vertex = object->vertex(i);
			vertex.transform(transformationMatrix(), position());
			object->setVertex(i, vertex);
		}

		LDPolygon polygon = object->getPolygon();

		if (polygon.isValid())
		{
			if (cachedShouldInvert)
				invertPolygon(polygon);

			result.append(polygon);
		}
	}

	return result;
}

void LDCircularPrimitive::buildPrimitiveBody(Model& model) const
{
	PrimitiveModel primitive;
	primitive.type = m_type;
	primitive.segments = m_segments;
	primitive.divisions = m_divisions;
	primitive.ringNumber = 0;
	primitive.generateBody(model);
}

QString LDCircularPrimitive::stem() const
{
	switch (m_type)
	{
	case PrimitiveModel::Cylinder:
		return "cyli";

	case PrimitiveModel::Circle:
		return "edge";

	case PrimitiveModel::Disc:
		return "disc";

	case PrimitiveModel::DiscNegative:
		return "ndis";

	default:
		throw std::logic_error("Bad primitive type to LDCircularPrimitive");
	}
}

QString LDCircularPrimitive::objectListText() const
{
	QString result = format(
		"%1 %2 %3, (",
		PrimitiveModel::typeName(m_type),
		m_segments / m_divisions,
		position().toString(true)
	);

	for (int i = 0; i < 9; ++i)
		result += format("%1%2", transformationMatrix().value(i), (i != 8) ? " " : "");

	result += ')';
	return result;
}

int LDCircularPrimitive::triangleCount(DocumentManager*) const
{
	switch (m_type)
	{
	case PrimitiveModel::Ring:
	case PrimitiveModel::Cone:
		throw std::logic_error("Bad primitive type to LDCircularPrimitive");

	case PrimitiveModel::Cylinder:
		return 2 * m_segments;

	case PrimitiveModel::Disc:
	case PrimitiveModel::DiscNegative:
		return m_segments;

	case PrimitiveModel::Circle:
		return 0;
	}

	return 0;
}

QString LDCircularPrimitive::typeName() const
{
	return "circular-primitive";
}

void LDCircularPrimitive::serialize(class Serializer& serializer)
{
	LDMatrixObject::serialize(serializer);
	serializer << m_segments << m_divisions;
}
