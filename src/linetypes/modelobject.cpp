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

// List of all LDObjects
QMap<qint32, LDObject*> g_allObjects;

enum { MAX_LDOBJECT_IDS = (1 << 24) };

#define LDOBJ_DEFAULT_CTOR(T,BASE) \
	T :: T (Model* model) : \
	    BASE {model} {}

// =============================================================================
// LDObject constructors
//
LDObject::LDObject (Model* model) :
    m_isHidden {false},
    m_isSelected {false},
    _model {model},
    m_coords {Origin}
{
	assert(_model != nullptr);

	// Let's hope that nobody goes to create 17 million objects anytime soon...
	static qint32 nextId = 1; // 0 shalt be null
	if (nextId < MAX_LDOBJECT_IDS)
		m_id = nextId++;
	else
		m_id = 0;

	if (m_id != 0)
		g_allObjects[m_id] = this;

	m_randomColor = QColor::fromHsv (rand() % 360, rand() % 256, rand() % 96 + 128);
}

LDSubfileReference::LDSubfileReference (Model* model) :
    LDMatrixObject (model) {}

LDOBJ_DEFAULT_CTOR (LDError, LDObject)
LDOBJ_DEFAULT_CTOR (LDBfc, LDObject)
LDOBJ_DEFAULT_CTOR (LDBezierCurve, LDObject)

LDObject::~LDObject()
{
	// Remove this object from the list of LDObjects
	g_allObjects.erase(g_allObjects.find(id()));
}

// =============================================================================
//
QString LDSubfileReference::asText() const
{
	QString val = format ("1 %1 %2 ", color(), position());
	val += transformationMatrix().toString();
	val += ' ';
	val += fileInfo()->name();
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

// =============================================================================
//
// Swap this object with another.
//
void LDObject::swap (LDObject* other)
{
	if (model() == other->model())
		model()->swapObjects (this, other);
}

int LDObject::triangleCount() const
{
	return 0;
}

int LDSubfileReference::triangleCount() const
{
	return fileInfo()->triangleCount();
}

int LDObject::numVertices() const
{
	return 0;
}

// =============================================================================
//
LDBezierCurve::LDBezierCurve(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, Model* model) :
    LDObject {model}
{
	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

// =============================================================================
//
void LDObject::setDocument (Model* model)
{
	_model = model;
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
void LDSubfileReference::inlineContents(Model& model, bool deep, bool render)
{
	Model inlined {this->model()->documentManager()};
	fileInfo()->inlineContents(inlined, deep, render);

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
	data->id = id();
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
QList<LDPolygon> LDSubfileReference::inlinePolygons()
{
	QList<LDPolygon> data = fileInfo()->inlinePolygons();

	for (LDPolygon& entry : data)
	{
		for (int i = 0; i < entry.numVertices(); ++i)
			entry.vertices[i].transform (transformationMatrix(), position());
	}

	return data;
}

// =============================================================================
//
// Index (i.e. line number) of this object
//
int LDObject::lineNumber() const
{
	if (model())
	{
		for (int i = 0; i < model()->size(); ++i)
		{
			if (model()->getObject(i) == this)
				return i;
		}
	}

	return -1;
}

// =============================================================================
//
// Object after this in the current file
//
LDObject* LDObject::next() const
{
	return model()->getObject(lineNumber() + 1);
}

// =============================================================================
//
// Object prior to this in the current file
//
LDObject* LDObject::previous() const
{
	return model()->getObject(lineNumber() - 1);
}

// =============================================================================
//
// Is the previous object INVERTNEXT?
//
bool LDObject::previousIsInvertnext (LDBfc*& ptr)
{
	LDObject* prev = previous();

	if (prev and prev->type() == LDObjectType::Bfc and static_cast<LDBfc*> (prev)->statement() == BfcStatement::InvertNext)
	{
		ptr = static_cast<LDBfc*> (prev);
		return true;
	}

	return false;
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

bool LDObject::isSelected() const
{
	return m_isSelected;
}

qint32 LDObject::id() const
{
	return m_id;
}

LDColor LDObject::color() const
{
	return m_color;
}

QColor LDObject::randomColor() const
{
	return m_randomColor;
}

Model* LDObject::model() const
{
	return _model;
}

// =============================================================================
//
void LDObject::invert() {}
void LDBfc::invert() {}
void LDError::invert() {}

// =============================================================================
//
void LDSubfileReference::invert()
{
	if (model() == nullptr)
		return;

	// Check whether subfile is flat
	int axisSet = (1 << X) | (1 << Y) | (1 << Z);
	Model model {this->model()->documentManager()};
	fileInfo()->inlineContents(model, true, false);

	for (LDObject* obj : model.objects())
	{
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex const& vrt = obj->vertex (i);

			if (axisSet & (1 << X) and vrt.x() != 0.0)
				axisSet &= ~(1 << X);

			if (axisSet & (1 << Y) and vrt.y() != 0.0)
				axisSet &= ~(1 << Y);

			if (axisSet & (1 << Z) and vrt.z() != 0.0)
				axisSet &= ~(1 << Z);
		}

		if (axisSet == 0)
			break;
	}

	if (axisSet != 0)
	{
		// Subfile has all vertices zero on one specific plane, so it is flat.
		// Let's flip it.
		Matrix matrixModifier = Matrix::identity;

		if (axisSet & (1 << X))
			matrixModifier(0, 0) = -1;

		if (axisSet & (1 << Y))
			matrixModifier(1, 1) = -1;

		if (axisSet & (1 << Z))
			matrixModifier(2, 2) = -1;

		setTransformationMatrix (transformationMatrix() * matrixModifier);
		return;
	}

	// Subfile is not flat. Resort to invertnext.
	int idx = lineNumber();

	if (idx > 0)
	{
		LDBfc* bfc = dynamic_cast<LDBfc*> (previous());

		if (bfc and bfc->statement() == BfcStatement::InvertNext)
		{
			// This is prefixed with an invertnext, thus remove it.
			this->model()->remove(bfc);
			return;
		}
	}

	// Not inverted, thus prefix it with a new invertnext.
	this->model()->emplaceAt<LDBfc>(idx, BfcStatement::InvertNext);
}

// =============================================================================
//
void LDBezierCurve::invert()
{
	// A Bézier curve's control points probably need to be, though.
	Vertex tmp = vertex (1);
	setVertex (1, vertex (0));
	setVertex (0, tmp);
	tmp = vertex (3);
	setVertex (3, vertex (2));
	setVertex (2, tmp);
}

// =============================================================================
//
LDObject* LDObject::fromID(qint32 id)
{
	return g_allObjects.value(id);
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

LDMatrixObject::LDMatrixObject (Model* model) :
    LDObject (model),
	m_position (Origin) {}

LDMatrixObject::LDMatrixObject (const Matrix& transform, const Vertex& pos, Model* model) :
    LDObject (model),
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

LDError::LDError (QString contents, QString reason, Model* model) :
    LDObject (model),
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

QString LDError::fileReferenced() const
{
	return m_fileReferenced;
}

void LDError::setFileReferenced (QString value)
{
	m_fileReferenced = value;
}

LDBfc::LDBfc (const BfcStatement type, Model* model) :
    LDObject {model},
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
	poly.id = id();
	poly.num = 2;

	for (int i = 0; i < segments; ++i)
	{
		poly.vertices[0] = parms[i];
		poly.vertices[1] = parms[i + 1];
		result << poly;
	}

	return result;
}

LDSubfileReference::LDSubfileReference(LDDocument* reference, const Matrix& transformationMatrix,
                                       const Vertex& position, Model* model) :
    LDMatrixObject {transformationMatrix, position, model},
    m_fileInfo {reference} {}

// =============================================================================
//
LDDocument* LDSubfileReference::fileInfo() const
{
	return m_fileInfo;
}

void LDSubfileReference::setFileInfo (LDDocument* newReferee)
{
	changeProperty(&m_fileInfo, newReferee);

	if (model())
		model()->recountTriangles();

	// If it's an immediate subfile reference (i.e. this subfile is in an opened document), we need to pre-compile the
	// GL polygons for the document if they don't exist already.
	if (newReferee and
		newReferee->isFrozen() == false and
		newReferee->polygonData().isEmpty())
	{
		newReferee->initializeCachedData();
	}
}

void LDObject::getVertices (QSet<Vertex>& verts) const
{
	for (int i = 0; i < numVertices(); ++i)
		verts.insert(vertex(i));
}

void LDSubfileReference::getVertices (QSet<Vertex>& verts) const
{
	verts.unite(fileInfo()->inlineVertices());
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
	QString result = format ("%1 %2, (", fileInfo()->getDisplayName(), position().toString(true));

	for (int i = 0; i < 9; ++i)
		result += format("%1%2", transformationMatrix().value(i), (i != 8) ? " " : "");

	result += ')';
	return result;
}

QString LDBfc::objectListText() const
{
	return statementToString();
}
