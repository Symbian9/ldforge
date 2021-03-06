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

#include "../documentmanager.h"
#include "../linetypes/modelobject.h"
#include "../lddocument.h"
#include "../mainwindow.h"
#include "../editHistory.h"
#include "../canvas.h"
#include "../colors.h"
#include "../glcompiler.h"
#include "../algorithms/invert.h"
#include "edgeline.h"
#include "triangle.h"
#include "quadrilateral.h"
#include "conditionaledge.h"
#include "comment.h"
#include "empty.h"
#include "circularprimitive.h"

// List of all LDObjects
QMap<qint32, LDObject*> g_allObjects;

enum { MAX_LDOBJECT_IDS = (1 << 24) };

// =============================================================================
// LDObject constructors
//
LDObject::LDObject() :
	m_isHidden {false}
{
	m_randomColor = QColor::fromHsv (rand() % 360, rand() % 256, rand() % 96 + 128);
}

// =============================================================================
//
QString LDSubfileReference::asText() const
{
	auto cellString = [&](int row, int column) { return QString::number(transformationMatrix()(row, column)); };

	return QStringList {
		"1",
		color().indexString(),
		cellString(0, 3), // x
		cellString(1, 3), // y
		cellString(2, 3), // z
		cellString(0, 0), // matrix start
		cellString(0, 1), // ...
		cellString(0, 2), // ...
		cellString(1, 0), // ...
		cellString(1, 1), // ...
		cellString(1, 2), // ...
		cellString(2, 0), // ...
		cellString(2, 1), // ...
		cellString(2, 2), // matrix end
		referenceName()
	}.join(" ");
}

QString LDBezierCurve::asText() const
{
	QString result = format ("0 !LDFORGE BEZIER_CURVE %1", color());

	// Add the coordinates
	for (int i = 0; i < 4; ++i)
		result += format (" %1", vertex (i));

	return result;
}

// =============================================================================
//
QString LDError::asText() const
{
	return contents();
}

int LDObject::triangleCount(DocumentManager*) const
{
	return 0;
}

int LDSubfileReference::triangleCount(DocumentManager* context) const
{
	LDDocument* file = fileInfo(context);
	if (file)
		return file->triangleCount();
	else
		return 0;
}

int LDObject::numVertices() const
{
	return 0;
}

int LDObject::numPolygonVertices() const
{
	return this->numVertices();
}

// =============================================================================
//
LDBezierCurve::LDBezierCurve(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
	setVertex(0, v0);
	setVertex(1, v1);
	setVertex(2, v2);
	setVertex(3, v3);
}

// =============================================================================
//
static void TransformObject (LDObject* object, const QMatrix4x4& matrix, LDColor parentcolor)
{
	if (object->hasMatrix()) {
		LDMatrixObject* reference = static_cast<LDMatrixObject*>(object);
		QMatrix4x4 newMatrix = matrix * reference->transformationMatrix();
		reference->setTransformationMatrix(newMatrix);
	}
	else
	{
		for (int i = 0; i < object->numVertices(); ++i)
		{
			Vertex vertex = object->vertex(i);
			vertex.transform(matrix);
			object->setVertex(i, vertex);
		}
	}

	if (object->color() == MainColor)
		object->setColor(parentcolor);
}

/*
 * Returns whether or not a compound object should be inverted.
 */
bool LDMatrixObject::shouldInvert(Winding winding, DocumentManager* context)
{
	bool result = false;
	result ^= (isInverted());
	result ^= (transformationMatrix().determinant() < 0);
	result ^= (nativeWinding(context) != winding);
	return result;
}

/*
 * The winding used by the object's geometry. By default it's CCW but some documents referenced by
 * a subfile reference may use CW geometry.
 *
 * Since the native winding of a subfile reference depends on the actual document it references,
 * determining the winding requires the libraries for reference.
 */
Winding LDObject::nativeWinding(DocumentManager* /*context*/) const
{
	return CounterClockwise;
}

/*
 * Reimplementation of LDObject::nativeWinding for subfile references
 */
Winding LDSubfileReference::nativeWinding(DocumentManager* context) const
{
	return fileInfo(context)->winding();
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDSubfileReference::rasterize(
	DocumentManager* context,
	Winding parentWinding,
	Model& model,
	bool deep,
	bool render
) {
	LDDocument* subfile = this->fileInfo(context);

	if (subfile != nullptr)
	{
		Model inlined {context};
		subfile->inlineContents(inlined, deep, render);

		// Transform the objects
		for (LDObject* object : inlined)
		{
			if (shouldInvert(parentWinding, context))
				::invert(object, context);

			TransformObject(object, transformationMatrix(), color());
		}

		model.merge(inlined);
	}
}

// =============================================================================
//
LDPolygon LDObject::getPolygon()
{
	LDObjectType ot = type();
	LDPolygon::Type polygonType;

	switch (ot)
	{
	case LDObjectType::EdgeLine:
		polygonType = LDPolygon::Type::EdgeLine;
		break;

	case LDObjectType::Triangle:
		polygonType = LDPolygon::Type::Triangle;
		break;

	case LDObjectType::Quadrilateral:
		polygonType = LDPolygon::Type::Quadrilateral;
		break;

	case LDObjectType::ConditionalEdge:
		polygonType = LDPolygon::Type::ConditionalEdge;
		break;

	default:
		return {};
	}

	LDPolygon data;
	data.type = polygonType;
	data.color = color();

	for (int i = 0; i < data.numVertices(); ++i)
		data.vertices[i] = vertex(i);

	return data;
}

LDColor LDObject::defaultColor() const
{
	return MainColor;
}

bool LDObject::isColored() const
{
	return true;
}

bool LDObject::isScemantic() const
{
	return true;
}

bool LDObject::hasMatrix() const
{
	return false;
}

// =============================================================================
//
QVector<LDPolygon> LDSubfileReference::rasterizePolygons(DocumentManager* context, Winding parentWinding)
{
	LDDocument* file = fileInfo(context);

	if (file)
	{
		QVector<LDPolygon> data = fileInfo(context)->inlinePolygons();

		for (LDPolygon& entry : data)
		{
			for (int i = 0; i < entry.numVertices(); ++i)
				entry.vertices[i].transform(transformationMatrix());

			if (shouldInvert(parentWinding, context))
				::invertPolygon(entry);
		}

		return data;
	}
	else
	{
		return {};
	}
}

void LDSubfileReference::setReferenceName(const QString& newReferenceName)
{
	changeProperty(&this->m_referenceName, newReferenceName);
}

// =============================================================================
//
// Moves this object using the given vector
//
void LDObject::move(const QVector3D& vector)
{
	if (hasMatrix())
	{
		LDMatrixObject* mo = static_cast<LDMatrixObject*> (this);
		QMatrix4x4 matrix = mo->transformationMatrix();
		matrix(0, 3) += vector.x();
		matrix(1, 3) += vector.y();
		matrix(2, 3) += vector.z();
		mo->setTransformationMatrix(matrix);
	}
	else
	{
		for (int i = 0; i < numVertices(); ++i)
			setVertex (i, vertex (i) + vector);
	}
}

bool LDObject::isHidden() const
{
	return m_isHidden;
}

void LDObject::setHidden (bool value)
{
	m_isHidden = value;
}

LDColor LDObject::color() const
{
	if (this->m_color.isValid())
		return this->m_color;
	else
		return this->defaultColor();
}

QColor LDObject::randomColor() const
{
	return m_randomColor;
}

LDObject* LDObject::newFromType(LDObjectType type)
{
	switch (type)
	{
	case LDObjectType::SubfileReference:
		return new LDSubfileReference {};

	case LDObjectType::Quadrilateral:
		return new LDQuadrilateral {};

	case LDObjectType::Triangle:
		return new LDTriangle {};

	case LDObjectType::EdgeLine:
		return new LDEdgeLine {};

	case LDObjectType::ConditionalEdge:
		return new LDConditionalEdge {};

	case LDObjectType::Comment:
		return new LDComment {};

	case LDObjectType::Error:
		return new LDError {};

	case LDObjectType::Empty:
		return new LDEmpty {};

	case LDObjectType::BezierCurve:
		return new LDBezierCurve {};

	case LDObjectType::CircularPrimitive:
		return new LDCircularPrimitive {};

	case LDObjectType::_End:
		break;
	}

	return nullptr;
}

// =============================================================================
//
void LDObject::setColor (LDColor color)
{
	if (color == this->defaultColor())
		color = LDColor::nullColor;

	changeProperty(&m_color, color);
}

// =============================================================================
//
// Get a vertex by index
//
const Vertex& LDObject::vertex (int i) const
{
	return m_coords[i];
}

// =============================================================================
//
// Set a vertex to the given value
//
void LDObject::setVertex (int i, const Vertex& vert)
{
	changeProperty(&m_coords[i], vert);
}

LDMatrixObject::LDMatrixObject(const QMatrix4x4& matrix) :
	m_transformationMatrix {matrix} {}

Vertex LDMatrixObject::position() const
{
	QVector4D column = m_transformationMatrix.column(3);
	return {column.x(), column.y(), column.z()};
}

// =============================================================================
//
const QMatrix4x4& LDMatrixObject::transformationMatrix() const
{
	return m_transformationMatrix;
}

void LDMatrixObject::translate(const QVector3D& offset)
{
	QMatrix4x4 matrix = transformationMatrix();
	matrix.translate(offset);
	setTransformationMatrix(matrix);
}

void LDMatrixObject::setTransformationMatrix(const QMatrix4x4& newMatrix)
{
	changeProperty(&m_transformationMatrix, newMatrix);
}

LDError::LDError (QString contents, QString reason) :
	m_contents (contents),
	m_reason (reason) {}

QString LDError::reason() const
{
	return m_reason;
}

QString LDError::contents() const
{
	return m_contents;
}

Vertex LDBezierCurve::pointAt (qreal t) const
{
	if (t >= 0.0 and t <= 1.0)
	{
		Vertex result = pow(1.0 - t, 3) * vertex(0);
		result += (3 * pow(1.0 - t, 2) * t) * vertex(2).toVector();
		result += (3 * (1.0 - t) * pow(t, 2)) * vertex(3).toVector();
		result += pow(t, 3) * vertex(1).toVector();
		return result;
	}
	else
		return Vertex();
}

bool LDBezierCurve::isRasterizable() const
{
	return true;
}

void LDBezierCurve::rasterize(DocumentManager* context, Winding winding, Model& model, bool, bool)
{
	QVector<LDPolygon> polygons = rasterizePolygons(context, winding);

	for (LDPolygon& poly : polygons)
	{
		LDEdgeLine* line = model.emplace<LDEdgeLine>(poly.vertices[0], poly.vertices[1]);
		line->setColor(LDColor {poly.color});
	}
}

QVector<LDPolygon> LDBezierCurve::rasterizePolygons(DocumentManager*, Winding)
{
	QVector<LDPolygon> result;
	QVector<Vertex> parms;
	parms.append (pointAt (0.0));

	for (int i = 1; i < m_segments; ++i)
		parms.append (pointAt (double (i) / m_segments));

	parms.append (pointAt (1.0));
	LDPolygon poly;
	poly.color = color().index();
	poly.type = LDPolygon::Type::EdgeLine;

	for (int i = 0; i < m_segments; ++i)
	{
		poly.vertices[0] = parms[i];
		poly.vertices[1] = parms[i + 1];
		result << poly;
	}

	return result;
}

void LDBezierCurve::serialize(Serializer& serializer)
{
	LDObject::serialize(serializer);
	serializer << m_segments;
}

int LDBezierCurve::segments() const
{
	return m_segments;
}

void LDBezierCurve::setSegments(int newSegments)
{
	changeProperty(&m_segments, newSegments);
}

LDSubfileReference::LDSubfileReference(
	QString referenceName,
	const QMatrix4x4& matrix
) :
	LDMatrixObject {matrix},
	m_referenceName {referenceName} {}

// =============================================================================
//
LDDocument* LDSubfileReference::fileInfo(DocumentManager* context) const
{
	return context->getDocumentByName(m_referenceName);
}

QString LDSubfileReference::referenceName() const
{
	return m_referenceName;
}

void LDObject::getVertices(DocumentManager*, QSet<Vertex>& verts) const
{
	for (int i = 0; i < numVertices(); ++i)
		verts.insert(vertex(i));
}

void LDSubfileReference::getVertices (DocumentManager* context, QSet<Vertex>& verts) const
{
	verts.unite(fileInfo(context)->inlineVertices());
}

QString LDObject::objectListText() const
{
	if (numVertices() > 0)
	{
		QString result;

		for (int i = 0; i < numVertices(); ++i)
		{
			if (i != 0)
				result += ", ";

			result += vertex(i).toString (true);
		}

		return result;
	}
	else
	{
		return iconName();
	}
}

QVector<LDPolygon> LDObject::rasterizePolygons(DocumentManager*, Winding)
{
	return {};
}

QString LDError::objectListText() const
{
	return "ERROR: " + asText();
}

QString LDSubfileReference::objectListText() const
{
	QString result = format ("%1 %2, (", referenceName(), position().toString(true));

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j)
		result += format("%1%2", transformationMatrix()(i, j), (i != 2 or j != 2) ? " " : "");

	result += ')';
	return result;
}

bool LDObject::isInverted() const
{
	return m_hasInvertNext;
}

void LDObject::setInverted(bool value)
{
	changeProperty(&m_hasInvertNext, value);
}

void LDObject::serialize(Serializer& serializer)
{
	serializer << m_hasInvertNext;
	serializer << m_isHidden;
	serializer << m_color;
	serializer << m_randomColor;
	serializer << m_coords[0];
	serializer << m_coords[1];
	serializer << m_coords[2];
	serializer << m_coords[3];
}

void LDObject::restore(LDObjectState& archive)
{
	Serializer restorer {archive, Serializer::Restore};
	Serializer::Archive before = Serializer::store(this);
	serialize(restorer);
	emit modified(before, Serializer::store(this));
}

void LDMatrixObject::serialize(Serializer& serializer)
{
	LDObject::serialize(serializer);
	serializer << m_transformationMatrix;
}

void LDError::serialize(Serializer& serializer)
{
	LDObject::serialize(serializer);
	serializer << m_contents;
	serializer << m_reason;
}

void LDSubfileReference::serialize(Serializer& serializer)
{
	LDMatrixObject::serialize(serializer);
	serializer << m_referenceName;
}
