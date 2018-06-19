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

LDCircularPrimitive::LDCircularPrimitive(PrimitiveModel::Type type,
	int segments,
	int divisions,
	const QMatrix4x4& matrix) :
	LDMatrixObject {matrix},
	m_type {type},
	m_segments {segments},
	m_divisions {divisions} {}

LDObjectType LDCircularPrimitive::type() const
{
	return SubclassType;
}

QString LDCircularPrimitive::asText() const
{
	return LDSubfileReference(buildFilename(), transformationMatrix()).asText();
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
			vertex.transform(transformationMatrix());
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
	buildPrimitiveBody(cylinderBody, false);

	for (LDObject* object : cylinderBody.objects())
	{
		for (int i = 0; i < object->numVertices(); i += 1)
		{
			Vertex vertex = object->vertex(i);
			vertex.transform(transformationMatrix());
			object->setVertex(i, vertex);
		}
	}

	model.merge(cylinderBody);
}

QVector<LDPolygon> LDCircularPrimitive::rasterizePolygons(DocumentManager* context, Winding winding)
{
	Model cylinderBody {context};
	buildPrimitiveBody(cylinderBody, true);
	QVector<LDPolygon> result;
	bool cachedShouldInvert = shouldInvert(winding, context);

	for (LDObject* object : cylinderBody.objects())
	{
		for (int i = 0; i < object->numVertices(); i += 1)
		{
			Vertex vertex = object->vertex(i);
			vertex.transform(transformationMatrix());
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

void LDCircularPrimitive::buildPrimitiveBody(Model& model, bool deep) const
{
	PrimitiveModel primitive;
	primitive.type = m_type;
	primitive.segments = m_segments;
	primitive.divisions = m_divisions;
	primitive.ringNumber = 0;
	primitive.generateBody(model, deep);
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

	case PrimitiveModel::CylinderClosed:
		return "cylc";

	case PrimitiveModel::CylinderOpen:
		return "cylo";

	case PrimitiveModel::Chord:
		return "chrd";

	case PrimitiveModel::Ring:
	case PrimitiveModel::Cone:
		break;
	}

	throw std::logic_error("Bad primitive type to LDCircularPrimitive");
}

QString LDCircularPrimitive::objectListText() const
{
	QString prefix;

	if (m_divisions == HighResolution)
		prefix = "Hi-Res";
	else if (m_divisions == LowResolution)
		prefix = "Lo-Res";
	else if (m_divisions != MediumResolution)
		prefix = format("%1-resolution", m_divisions);

	QString result = format(
		"%1 %2 %3 %4, (",
		prefix,
		PrimitiveModel::typeName(m_type),
		m_segments / m_divisions,
		position().toString(true)
	).simplified();

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
		result += format("%1%2", transformationMatrix()(i, j), (i != 2 or j != 2) ? " " : "");

	result += ')';
	return result;
}

PrimitiveModel::Type LDCircularPrimitive::primitiveType() const
{
	return m_type;
}

void LDCircularPrimitive::setPrimitiveType(PrimitiveModel::Type newType)
{
	changeProperty(&m_type, newType);
}

int LDCircularPrimitive::segments() const
{
	return m_segments;
}

void LDCircularPrimitive::setSegments(int newSegments)
{
	changeProperty(&m_segments, newSegments);
}

int LDCircularPrimitive::divisions() const
{
	return m_divisions;
}

void LDCircularPrimitive::setDivisions(int newDivisions)
{
	changeProperty(&m_divisions, newDivisions);
}

int LDCircularPrimitive::triangleCount(DocumentManager*) const
{
	switch (m_type)
	{
	case PrimitiveModel::Ring:
	case PrimitiveModel::Cone:
		throw std::logic_error("Bad primitive type to LDCircularPrimitive");

	case PrimitiveModel::Cylinder:
	case PrimitiveModel::CylinderClosed:
	case PrimitiveModel::CylinderOpen:
		return 2 * m_segments;

	case PrimitiveModel::Disc:
	case PrimitiveModel::DiscNegative:
		return m_segments;

	case PrimitiveModel::Circle:
		return 0;

	case PrimitiveModel::Chord:
		if (m_segments >= m_divisions -1)
			return m_divisions - 2;
		else
			return max(0, m_segments - 1);
	}

	return 0;
}

QString LDCircularPrimitive::iconName() const
{
	switch (m_type)
	{
	case PrimitiveModel::Ring:
	case PrimitiveModel::Cone:
		break;

	case PrimitiveModel::Cylinder:
		return "cylinder";

	case PrimitiveModel::Disc:
		return "disc";

	case PrimitiveModel::DiscNegative:
		return "disc-negative";

	case PrimitiveModel::Circle:
		return "circle";

	case PrimitiveModel::CylinderClosed:
		return "cylinder-closed";

	case PrimitiveModel::CylinderOpen:
		return "cylinder-open";

	case PrimitiveModel::Chord:
		return "chord";
	}

	return "";
}

void LDCircularPrimitive::serialize(class Serializer& serializer)
{
	LDMatrixObject::serialize(serializer);
	serializer << m_segments << m_divisions << m_type;
}
