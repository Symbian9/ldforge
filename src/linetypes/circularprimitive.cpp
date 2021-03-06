#include "../algorithms/geometry.h"
#include "../glShared.h"
#include "../model.h"
#include "../algorithms/invert.h"
#include "circularprimitive.h"
#include "quadrilateral.h"
#include "primitives.h"

QString LDCircularPrimitive::buildFilename() const
{
	int numerator = segments();
	int denominator = divisions();
	QString prefix;

	if (divisions() != MediumResolution)
		prefix = QString::number(divisions()) + '\\';

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
	m_section {segments, divisions} {}

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
	int endSegment = (segments() == divisions()) ? segments() : segments() + 1;

	for (int i = 0; i < endSegment; i += 1)
	{
		QPointF point2d = pointOnLDrawCircumference(i, divisions());

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

bool LDCircularPrimitive::isFlat() const
{
	switch (m_type)
	{
	case PrimitiveModel::Cylinder:
	case PrimitiveModel::CylinderClosed:
	case PrimitiveModel::CylinderOpen:
		return false;

	default:
		return true;
	}
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
	primitive.segments = segments();
	primitive.divisions = divisions();
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

	if (divisions() == HighResolution)
		prefix = "Hi-Res";
	else if (divisions() == LowResolution)
		prefix = "Lo-Res";
	else if (divisions() != MediumResolution)
		prefix = format("%1-resolution", divisions());

	QString result = format(
		"%1 %2 %3 %4, (",
		prefix,
		PrimitiveModel::typeName(m_type),
		double(segments()) / double(divisions()),
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
	return m_section.segments;
}

void LDCircularPrimitive::setSegments(int newSegments)
{
	changeProperty(&m_section.segments, newSegments);
}

int LDCircularPrimitive::divisions() const
{
	return m_section.divisions;
}

void LDCircularPrimitive::setDivisions(int newDivisions)
{
	changeProperty(&m_section.divisions, newDivisions);
}

const CircularSection&LDCircularPrimitive::section() const
{
	return m_section;
}

void LDCircularPrimitive::setSection(const CircularSection& newSection)
{
	changeProperty(&m_section, newSection);
}

int LDCircularPrimitive::triangleCount(DocumentManager*) const
{
	switch (m_type)
	{
	case PrimitiveModel::Ring:
	case PrimitiveModel::Cone:
		throw std::logic_error("Bad primitive type to LDCircularPrimitive");

	case PrimitiveModel::Cylinder:
	case PrimitiveModel::CylinderOpen:
		return 2 * segments();

	case PrimitiveModel::CylinderClosed:
		return 3 * segments();

	case PrimitiveModel::Disc:
	case PrimitiveModel::DiscNegative:
		return segments();

	case PrimitiveModel::Circle:
		return 0;

	case PrimitiveModel::Chord:
		return qBound(0, segments() - 1, divisions() - 2);
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
	case PrimitiveModel::CylinderOpen:
		return "cylinder";

	case PrimitiveModel::Disc:
		return "disc";

	case PrimitiveModel::DiscNegative:
		return "disc-negative";

	case PrimitiveModel::Circle:
		return "circle";

	case PrimitiveModel::CylinderClosed:
		return "closed-cylinder";

	case PrimitiveModel::Chord:
		return "chord";
	}

	return "";
}

void LDCircularPrimitive::serialize(class Serializer& serializer)
{
	LDMatrixObject::serialize(serializer);
	serializer << m_section << m_type;
}
