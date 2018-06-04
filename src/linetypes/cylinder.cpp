#include "../algorithms/geometry.h"
#include "../glShared.h"
#include "../model.h"
#include "../algorithms/invert.h"
#include "cylinder.h"
#include "quadrilateral.h"
#include "primitives.h"

QString LDCylinder::buildFilename() const
{
	int numerator = this->m_segments;
	int denominator = this->m_divisions;
	QString prefix;

	if (m_divisions != MediumResolution)
		prefix = QString::number(m_divisions) + '\\';

	simplify(numerator, denominator);
	return format("%1%2-%3cyli.dat", prefix, numerator, denominator);
}

LDCylinder::LDCylinder(
	int segments,
	int divisions,
	const Matrix& transformationMatrix,
	const Vertex& position
) :
	LDMatrixObject {transformationMatrix, position},
	m_segments {segments},
	m_divisions {divisions} {}

QString LDCylinder::asText() const
{
	return LDSubfileReference(buildFilename(), transformationMatrix(), position()).asText();
}

void LDCylinder::getVertices(DocumentManager* /* context */, QSet<Vertex>& vertices) const
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

void LDCylinder::rasterize(
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

QVector<LDPolygon> LDCylinder::rasterizePolygons(DocumentManager* context, Winding winding)
{
	Model cylinderBody {context};
	buildPrimitiveBody(cylinderBody, winding);
	QVector<LDPolygon> result;

	for (LDObject* object : cylinderBody.objects())
	{
		for (int i = 0; i < object->numVertices(); i += 1)
		{
			Vertex vertex = object->vertex(i);
			vertex.transform(transformationMatrix(), position());
			object->setVertex(i, vertex);
		}

		LDPolygon* polygon = object->getPolygon();

		if (polygon)
		{
			if (shouldInvert(winding, context))
				invertPolygon(*polygon);

			result.append(*polygon);
		}

		delete polygon;
	}

	return result;
}

void LDCylinder::buildPrimitiveBody(Model& model, Winding winding) const
{
	PrimitiveModel primitive;
	primitive.type = PrimitiveModel::Cylinder;
	primitive.segments = m_segments;
	primitive.divisions = m_divisions;
	primitive.ringNumber = 0;
	primitive.generateCylinder(model, winding);
}

QString LDCylinder::objectListText() const
{
	QString result = format("Cylinder %1 %2, (", m_segments / m_divisions, position().toString(true));

	for (int i = 0; i < 9; ++i)
		result += format("%1%2", transformationMatrix().value(i), (i != 8) ? " " : "");

	result += ')';
	return result;
}

void LDCylinder::serialize(class Serializer& serializer)
{
	LDMatrixObject::serialize(serializer);
	serializer << m_segments << m_divisions;
}
