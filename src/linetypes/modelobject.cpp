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

#include <assert.h>
#include "../documentmanager.h"
#include "../linetypes/modelobject.h"
#include "../lddocument.h"
#include "../miscallenous.h"
#include "../mainwindow.h"
#include "../editHistory.h"
#include "../canvas.h"
#include "../colors.h"
#include "../glcompiler.h"
#include "edgeline.h"
#include "triangle.h"
#include "quadrilateral.h"
#include "conditionaledge.h"
#include "comment.h"
#include "empty.h"

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
	QString val = format ("1 %1 %2 ", color(), position());
	val += transformationMatrix().toString();
	val += ' ';
	val += m_referenceName;
	return val;
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

// =============================================================================
//
QString LDBfc::asText() const
{
	return format ("0 BFC %1", statementToString());
}

int LDObject::triangleCount(DocumentManager*) const
{
	return 0;
}

int LDSubfileReference::triangleCount(DocumentManager* context) const
{
	return fileInfo(context)->triangleCount();
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
static void TransformObject (LDObject* obj, Matrix transform, Vertex pos, LDColor parentcolor)
{
	switch (obj->type())
	{
	case LDObjectType::EdgeLine:
	case LDObjectType::ConditionalEdge:
	case LDObjectType::Triangle:
	case LDObjectType::Quadrilateral:
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex v = obj->vertex (i);
			v.transform (transform, pos);
			obj->setVertex (i, v);
		}
		break;

	case LDObjectType::SubfileReference:
		{
			LDSubfileReference* ref = static_cast<LDSubfileReference*> (obj);
			Matrix newMatrix = transform * ref->transformationMatrix();
			Vertex newpos = ref->position();
			newpos.transform (transform, pos);
			ref->setPosition (newpos);
			ref->setTransformationMatrix (newMatrix);
		}
		break;

	default:
		break;
	}

	if (obj->color() == MainColor)
		obj->setColor (parentcolor);
}

// =============================================================================
// -----------------------------------------------------------------------------
void LDSubfileReference::inlineContents(DocumentManager* context, Model& model, bool deep, bool render)
{
	Model inlined {context};
	fileInfo(context)->inlineContents(inlined, deep, render);

	// Transform the objects
	for (LDObject* object : inlined)
		TransformObject(object, transformationMatrix(), position(), color());

	model.merge(inlined);
}

// =============================================================================
//
LDPolygon* LDObject::getPolygon()
{
	LDObjectType ot = type();
	int num = (ot == LDObjectType::EdgeLine)		? 2
			: (ot == LDObjectType::Triangle)	? 3
			: (ot == LDObjectType::Quadrilateral)		? 4
			: (ot == LDObjectType::ConditionalEdge)	? 5
			: 0;

	if (num == 0)
		return nullptr;

	LDPolygon* data = new LDPolygon;
	data->num = num;
	data->color = color().index();

	for (int i = 0; i < data->numVertices(); ++i)
		data->vertices[i] = vertex (i);

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
QList<LDPolygon> LDSubfileReference::inlinePolygons(DocumentManager* context)
{
	QList<LDPolygon> data = fileInfo(context)->inlinePolygons();

	for (LDPolygon& entry : data)
	{
		for (int i = 0; i < entry.numVertices(); ++i)
			entry.vertices[i].transform (transformationMatrix(), position());
	}

	return data;
}

// =============================================================================
//
// Moves this object using the given vertex as a movement List
//
void LDObject::move (Vertex vect)
{
	if (hasMatrix())
	{
		LDMatrixObject* mo = static_cast<LDMatrixObject*> (this);
		mo->setPosition (mo->position() + vect);
	}
	else
	{
		for (int i = 0; i < numVertices(); ++i)
			setVertex (i, vertex (i) + vect);
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
	return m_color;
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

	case LDObjectType::Bfc:
		return new LDBfc {};

	case LDObjectType::Comment:
		return new LDComment {};

	case LDObjectType::Error:
		return new LDError {};

	case LDObjectType::Empty:
		return new LDEmpty {};

	case LDObjectType::BezierCurve:
		return new LDBezierCurve {};

	case LDObjectType::_End:
		break;
	}

	return nullptr;
}

// =============================================================================
//
void LDObject::setColor (LDColor color)
{
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

LDMatrixObject::LDMatrixObject (const Matrix& transform, const Vertex& pos) :
	m_position (pos),
	m_transformationMatrix (transform) {}

void LDMatrixObject::setCoordinate (const Axis ax, double value)
{
	Vertex v = position();

	switch (ax)
	{
		case X: v.setX (value); break;
		case Y: v.setY (value); break;
		case Z: v.setZ (value); break;
	}

	setPosition (v);
}

const Vertex& LDMatrixObject::position() const
{
	return m_position;
}

// =============================================================================
//
void LDMatrixObject::setPosition (const Vertex& a)
{
	changeProperty(&m_position, a);
}

// =============================================================================
//
const Matrix& LDMatrixObject::transformationMatrix() const
{
	return m_transformationMatrix;
}

void LDMatrixObject::setTransformationMatrix (const Matrix& val)
{
	changeProperty(&m_transformationMatrix, val);
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

LDBfc::LDBfc (const BfcStatement type) :
    m_statement {type} {}

BfcStatement LDBfc::statement() const
{
	return m_statement;
}

void LDBfc::setStatement (BfcStatement value)
{
	m_statement = value;
}

QString LDBfc::statementToString() const
{
	return LDBfc::statementToString (statement());
}

QString LDBfc::statementToString (BfcStatement statement)
{
	static const char* statementStrings[] =
	{
		"CERTIFY CCW",
		"CCW",
		"CERTIFY CW",
		"CW",
		"NOCERTIFY",
		"INVERTNEXT",
		"CLIP",
		"CLIP CCW",
		"CLIP CW",
		"NOCLIP",
	};

	if ((int) statement >= 0 and (int) statement < countof (statementStrings))
		return QString::fromLatin1 (statementStrings[(int) statement]);
	else
		return "";
}

Vertex LDBezierCurve::pointAt (qreal t) const
{
	if (t >= 0.0 and t <= 1.0)
	{
		Vertex result;
		result += pow (1.0 - t, 3) * vertex (0);
		result += (3 * pow (1.0 - t, 2) * t) * vertex (2);
		result += (3 * (1.0 - t) * pow (t, 2)) * vertex (3);
		result += pow (t, 3) * vertex (1);
		return result;
	}
	else
		return Vertex();
}

void LDBezierCurve::rasterize(Model& model, int segments)
{
	QVector<LDPolygon> polygons = rasterizePolygons(segments);

	for (LDPolygon& poly : polygons)
	{
		LDEdgeLine* line = model.emplace<LDEdgeLine>(poly.vertices[0], poly.vertices[1]);
		line->setColor (poly.color);
	}
}

QVector<LDPolygon> LDBezierCurve::rasterizePolygons(int segments)
{
	QVector<LDPolygon> result;
	QVector<Vertex> parms;
	parms.append (pointAt (0.0));

	for (int i = 1; i < segments; ++i)
		parms.append (pointAt (double (i) / segments));

	parms.append (pointAt (1.0));
	LDPolygon poly;
	poly.color = color().index();
	poly.num = 2;

	for (int i = 0; i < segments; ++i)
	{
		poly.vertices[0] = parms[i];
		poly.vertices[1] = parms[i + 1];
		result << poly;
	}

	return result;
}

LDSubfileReference::LDSubfileReference(
	QString referenceName,
	const Matrix& transformationMatrix,
	const Vertex& position
) :
	LDMatrixObject {transformationMatrix, position},
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
		return typeName();
	}
}

QString LDError::objectListText() const
{
	return "ERROR: " + asText();
}

QString LDSubfileReference::objectListText() const
{
	QString result = format ("%1 %2, (", referenceName(), position().toString(true));

	for (int i = 0; i < 9; ++i)
		result += format("%1%2", transformationMatrix().value(i), (i != 8) ? " " : "");

	result += ')';
	return result;
}

QString LDBfc::objectListText() const
{
	return statementToString();
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

void LDMatrixObject::serialize(Serializer& serializer)
{
	LDObject::serialize(serializer);
	serializer << m_position;
	serializer << m_transformationMatrix;
}

void LDBfc::serialize(Serializer& serializer)
{
	LDObject::serialize(serializer);
	serializer << m_statement;
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
