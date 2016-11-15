/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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


#include "main.h"
#include "ldObject.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "mainwindow.h"
#include "editHistory.h"
#include "glRenderer.h"
#include "colors.h"
#include "glCompiler.h"

ConfigOption (QString DefaultName = "")
ConfigOption (QString DefaultUser = "")
ConfigOption (bool UseCaLicense = true)

// List of all LDObjects
QMap<int32, LDObject*> g_allObjects;

enum { MAX_LDOBJECT_IDS = (1 << 24) };

#define LDOBJ_DEFAULT_CTOR(T,BASE) \
	T :: T (LDDocument* document) : \
		BASE (document) {}

// =============================================================================
// LDObject constructors
//
LDObject::LDObject (LDDocument* document) :
	m_isHidden (false),
	m_isSelected (false),
	m_isDestroyed (false),
	m_document (nullptr)
{
	if (document)
		document->addObject (this);

	memset (m_coords, 0, sizeof m_coords);

	// Let's hope that nobody goes to create 17 million objects anytime soon...
	static int32 nextId = 1; // 0 shalt be null
	if (nextId < MAX_LDOBJECT_IDS)
		m_id = nextId++;
	else
		m_id = 0;

	if (m_id != 0)
		g_allObjects[m_id] = this;

	m_randomColor = QColor::fromHsv (rand() % 360, rand() % 256, rand() % 96 + 128);
}

LDSubfileReference::LDSubfileReference (LDDocument* document) :
	LDMatrixObject (document) {}

LDOBJ_DEFAULT_CTOR (LDEmpty, LDObject)
LDOBJ_DEFAULT_CTOR (LDError, LDObject)
LDOBJ_DEFAULT_CTOR (LDLine, LDObject)
LDOBJ_DEFAULT_CTOR (LDTriangle, LDObject)
LDOBJ_DEFAULT_CTOR (LDCondLine, LDLine)
LDOBJ_DEFAULT_CTOR (LDQuad, LDObject)
LDOBJ_DEFAULT_CTOR (LDOverlay, LDObject)
LDOBJ_DEFAULT_CTOR (LDBfc, LDObject)
LDOBJ_DEFAULT_CTOR (LDComment, LDObject)
LDOBJ_DEFAULT_CTOR (LDBezierCurve, LDObject)

LDObject::~LDObject()
{
	if (not m_isDestroyed)
		print ("Warning: Object #%1 (%2) was not destroyed before being deleted\n", id(), this);
}

// =============================================================================
//
QString LDComment::asText() const
{
	return format ("0 %1", text());
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

// =============================================================================
//
QString LDLine::asText() const
{
	QString val = format ("2 %1", color());

	for (int i = 0; i < 2; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
QString LDTriangle::asText() const
{
	QString val = format ("3 %1", color());

	for (int i = 0; i < 3; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
QString LDQuad::asText() const
{
	QString val = format ("4 %1", color());

	for (int i = 0; i < 4; ++i)
		val += format (" %1", vertex (i));

	return val;
}

// =============================================================================
//
QString LDCondLine::asText() const
{
	QString val = format ("5 %1", color());
	
	// Add the coordinates
	for (int i = 0; i < 4; ++i)
		val += format (" %1", vertex (i));

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
QString LDEmpty::asText() const
{
	return "";
}

// =============================================================================
//
QString LDBfc::asText() const
{
	return format ("0 BFC %1", statementToString());
}

// =============================================================================
//
QList<LDTriangle*> LDQuad::splitToTriangles()
{
	// Create the two triangles based on this quadrilateral:
	// 0───3       0───3    3
	// │   │  --→  │  ╱    ╱│
	// │   │  --→  │ ╱    ╱ │
	// │   │  --→  │╱    ╱  │
	// 1───2       1    1───2
	LDTriangle* tri1 (new LDTriangle (vertex (0), vertex (1), vertex (3)));
	LDTriangle* tri2 (new LDTriangle (vertex (1), vertex (2), vertex (3)));

	// The triangles also inherit the quad's color
	tri1->setColor (color());
	tri2->setColor (color());

	return {tri1, tri2};
}

// =============================================================================
//
// Replace this LDObject with another LDObject. Object is deleted in the process.
//
void LDObject::replace (LDObject* other)
{
	int idx = lineNumber();

	if (idx != -1)
	{
		// Replace the instance of the old object with the new object
		document()->setObjectAt (idx, other);

		// Remove the old object
		destroy();
	}
}

void LDObject::replace (const LDObjectList& others)
{
	int idx = lineNumber();

	if (idx != -1 and not others.isEmpty())
	{
		for (int i = 1; i < others.size(); ++i)
			document()->insertObject (idx + i, others[i]);

		document()->setObjectAt (idx, others[0]);
		destroy();
	}
}

// =============================================================================
//
// Swap this object with another.
//
void LDObject::swap (LDObject* other)
{
	if (document() == other->document())
		document()->swapObjects (this, other);
}

int LDObject::triangleCount() const
{
	return 0;
}

int LDSubfileReference::triangleCount() const
{
	return fileInfo()->triangleCount();
}

int LDTriangle::triangleCount() const
{
	return 1;
}

int LDQuad::triangleCount() const
{
	return 2;
}

// =============================================================================
//
LDLine::LDLine (Vertex v1, Vertex v2, LDDocument* document) :
	LDObject (document)
{
	setVertex (0, v1);
	setVertex (1, v2);
}

// =============================================================================
//
LDTriangle::LDTriangle (const Vertex& v1, const Vertex& v2, const Vertex& v3, LDDocument* document) :
	LDObject (document)
{
	setVertex (0, v1);
	setVertex (1, v2);
	setVertex (2, v3);
}

// =============================================================================
//
LDQuad::LDQuad (const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4, LDDocument* document) :
	LDObject (document)
{
	setVertex (0, v1);
	setVertex (1, v2);
	setVertex (2, v3);
	setVertex (3, v4);
}

// =============================================================================
//
LDCondLine::LDCondLine (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, LDDocument* document) :
	LDLine (document)
{
	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

// =============================================================================
//
LDBezierCurve::LDBezierCurve(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3,
	LDDocument* document) :
	LDObject (document)
{
	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

// =============================================================================
//
// Deletes this object
//
void LDObject::destroy()
{
	deselect();

	// If this object was associated to a file, remove it off it now
	if (document())
		document()->forgetObject (this);

	// Delete the GL lists
	if (g_win)
		g_win->renderer()->forgetObject (this);

	// Remove this object from the list of LDObjects
	g_allObjects.erase (g_allObjects.find (id()));
	m_isDestroyed = true;
	delete this;
}

// =============================================================================
//
void LDObject::setDocument (LDDocument* document)
{
	m_document = document;

	if (document == nullptr)
		m_isSelected = false;
}

// =============================================================================
//
static void TransformObject (LDObject* obj, Matrix transform, Vertex pos, LDColor parentcolor)
{
	switch (obj->type())
	{
	case OBJ_Line:
	case OBJ_CondLine:
	case OBJ_Triangle:
	case OBJ_Quad:
		for (int i = 0; i < obj->numVertices(); ++i)
		{
			Vertex v = obj->vertex (i);
			v.transform (transform, pos);
			obj->setVertex (i, v);
		}
		break;

	case OBJ_SubfileReference:
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
LDObjectList LDSubfileReference::inlineContents (bool deep, bool render)
{
	LDObjectList objs = fileInfo()->inlineContents (deep, render);

	// Transform the objects
	for (LDObject* obj : objs)
		TransformObject (obj, transformationMatrix(), position(), color());

	return objs;
}

// =============================================================================
//
LDPolygon* LDObject::getPolygon()
{
	LDObjectType ot = type();
	int num = (ot == OBJ_Line)		? 2
			: (ot == OBJ_Triangle)	? 3
			: (ot == OBJ_Quad)		? 4
			: (ot == OBJ_CondLine)	? 5
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
	if (document())
	{
		for (int i = 0; i < document()->getObjectCount(); ++i)
		{
			if (document()->getObject (i) == this)
				return i;
		}
	}

	return -1;
}

// =============================================================================
//
// Get type name by enumerator
//
QString LDObject::typeName (LDObjectType type)
{
	return LDObject::getDefault (type)->typeName();
}

// =============================================================================
//
// Get a description of a list of LDObjects
//
QString LDObject::describeObjects (const LDObjectList& objs)
{
	QString text;

	if (objs.isEmpty())
		return "nothing"; // :)

	for (LDObjectType objType = OBJ_FirstType; objType < OBJ_NumTypes; ++objType)
	{
		int count = 0;

		for (LDObject* obj : objs)
		{
			if (obj->type() == objType)
				count++;
		}

		if (count == 0)
			continue;

		if (not text.isEmpty())
			text += ", ";

		QString noun = format ("%1%2", typeName (objType), plural (count));
		text += format ("%1 %2", count, noun);
	}

	return text;
}

// =============================================================================
//
// Object after this in the current file
//
LDObject* LDObject::next() const
{
	int idx = lineNumber();

	if (idx == -1 or idx == document()->getObjectCount() - 1)
		return nullptr;

	return document()->getObject (idx + 1);
}

// =============================================================================
//
// Object prior to this in the current file
//
LDObject* LDObject::previous() const
{
	int idx = lineNumber();

	if (idx <= 0)
		return nullptr;

	return document()->getObject (idx - 1);
}

// =============================================================================
//
// Is the previous object INVERTNEXT?
//
bool LDObject::previousIsInvertnext (LDBfc*& ptr)
{
	LDObject* prev = previous();

	if (prev and prev->type() == OBJ_Bfc and static_cast<LDBfc*> (prev)->statement() == BfcStatement::InvertNext)
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

LDDocument* LDObject::document() const
{
	return m_document;
}

bool LDObject::isDestroyed() const
{
	return m_isDestroyed;
}

// =============================================================================
//
// Returns a default-constructed LDObject by the given type
//
LDObject* LDObject::getDefault (const LDObjectType type)
{
	switch (type)
	{
	case OBJ_Comment:		return LDSpawn<LDComment>();
	case OBJ_Bfc:			return LDSpawn<LDBfc>();
	case OBJ_Line:			return LDSpawn<LDLine>();
	case OBJ_CondLine:		return LDSpawn<LDCondLine>();
	case OBJ_SubfileReference:		return LDSpawn<LDSubfileReference>();
	case OBJ_Triangle:		return LDSpawn<LDTriangle>();
	case OBJ_Quad:			return LDSpawn<LDQuad>();
	case OBJ_Empty:			return LDSpawn<LDEmpty>();
	case OBJ_Error:			return LDSpawn<LDError>();
	case OBJ_Overlay:		return LDSpawn<LDOverlay>();
	case OBJ_BezierCurve:	return LDSpawn<LDBezierCurve>();
	case OBJ_NumTypes:		break;
	}
	return nullptr;
}

// =============================================================================
//
void LDObject::invert() {}
void LDBfc::invert() {}
void LDEmpty::invert() {}
void LDComment::invert() {}
void LDError::invert() {}

// =============================================================================
//
void LDTriangle::invert()
{
	// Triangle goes 0 -> 1 -> 2, reversed: 0 -> 2 -> 1.
	// Thus, we swap 1 and 2.
	Vertex tmp = vertex (1);
	setVertex (1, vertex (2));
	setVertex (2, tmp);

	return;
}

// =============================================================================
//
void LDQuad::invert()
{
	// Quad:     0 -> 1 -> 2 -> 3
	// reversed: 0 -> 3 -> 2 -> 1
	// Thus, we swap 1 and 3.
	Vertex tmp = vertex (1);
	setVertex (1, vertex (3));
	setVertex (3, tmp);
}

// =============================================================================
//
void LDSubfileReference::invert()
{
	if (document() == nullptr)
		return;

	// Check whether subfile is flat
	int axisSet = (1 << X) | (1 << Y) | (1 << Z);
	LDObjectList objs = fileInfo()->inlineContents (true, false);

	for (LDObject* obj : objs)
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
		Matrix matrixModifier = IdentityMatrix;

		if (axisSet & (1 << X))
			matrixModifier[0] = -1;

		if (axisSet & (1 << Y))
			matrixModifier[4] = -1;

		if (axisSet & (1 << Z))
			matrixModifier[8] = -1;

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
			bfc->destroy();
			return;
		}
	}

	// Not inverted, thus prefix it with a new invertnext.
	document()->insertObject (idx, new LDBfc (BfcStatement::InvertNext));
}

// =============================================================================
//
void LDLine::invert()
{
	// For lines, we swap the vertices.
	Vertex tmp = vertex (0);
	setVertex (0, vertex (1));
	setVertex (1, tmp);
}

// =============================================================================
//
void LDCondLine::invert()
{
	// I don't think that a conditional line's control points need to be swapped, do they?
	Vertex tmp = vertex (0);
	setVertex (0, vertex (1));
	setVertex (1, tmp);
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
LDLine* LDCondLine::toEdgeLine()
{
	LDLine* replacement = new LDLine;

	for (int i = 0; i < replacement->numVertices(); ++i)
		replacement->setVertex (i, vertex (i));

	replacement->setColor (color());
	replace (replacement);
	return replacement;
}

// =============================================================================
//
LDObject* LDObject::fromID (int id)
{
	auto it = g_allObjects.find (id);

	if (it != g_allObjects.end())
		return *it;

	return nullptr;
}

// =============================================================================
//
QString LDOverlay::asText() const
{
	return format ("0 !LDFORGE OVERLAY %1 %2 %3 %4 %5 %6",
		fileName(), camera(), x(), y(), width(), height());
}

void LDOverlay::invert() {}

// =============================================================================
//
// Hook the set accessors of certain properties to this changeProperty function.
// It takes care of history management so we can capture low-level changes, this
// makes history stuff work out of the box.
//
template<typename T>
static void changeProperty (LDObject* obj, T* ptr, const T& val)
{
	int idx;

	if (*ptr == val)
		return;

	if (obj->document() and (idx = obj->lineNumber()) != -1)
	{
		QString before = obj->asText();
		*ptr = val;
		QString after = obj->asText();

		if (before != after)
		{
			obj->document()->addToHistory (new EditHistoryEntry (idx, before, after));
			g_win->renderer()->compileObject (obj);
			g_win->currentDocument()->redoVertices();
		}
	}
	else
	{
		*ptr = val;
	}
}

// =============================================================================
//
void LDObject::setColor (LDColor color)
{
	changeProperty (this, &m_color, color);
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
	changeProperty (this, &m_coords[i], vert);
}

LDMatrixObject::LDMatrixObject (LDDocument* document) :
	LDObject (document),
	m_position (Origin) {}

LDMatrixObject::LDMatrixObject (const Matrix& transform, const Vertex& pos, LDDocument* document) :
	LDObject (document),
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
	changeProperty (this, &m_position, a);
}

// =============================================================================
//
const Matrix& LDMatrixObject::transformationMatrix() const
{
	return m_transformationMatrix;
}

void LDMatrixObject::setTransformationMatrix (const Matrix& val)
{
	changeProperty (this, &m_transformationMatrix, val);
}

LDError::LDError (QString contents, QString reason, LDDocument* document) :
	LDObject (document),
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

LDComment::LDComment (QString text, LDDocument* document) :
	LDObject (document),
	m_text (text) {}

QString LDComment::text() const
{
	return m_text;
}

void LDComment::setText (QString value)
{
	changeProperty (this, &m_text, value);
}

LDBfc::LDBfc (const BfcStatement type, LDDocument* document) :
	LDObject (document),
	m_statement (type) {}

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

int LDOverlay::camera() const
{
	return m_camera;
}

void LDOverlay::setCamera (int value)
{
	m_camera = value;
}

int LDOverlay::x() const
{
	return m_x;
}

void LDOverlay::setX (int value)
{
	m_x = value;
}

int LDOverlay::y() const
{
	return m_y;
}

void LDOverlay::setY (int value)
{
	m_y = value;
}

int LDOverlay::width() const
{
	return m_width;
}

void LDOverlay::setWidth (int value)
{
	m_width = value;
}

int LDOverlay::height() const
{
	return m_height;
}

void LDOverlay::setHeight (int value)
{
	m_height = value;
}

QString LDOverlay::fileName() const
{
	return m_fileName;
}

void LDOverlay::setFileName (QString value)
{
	m_fileName = value;
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

LDObjectList LDBezierCurve::rasterize (int segments)
{
	QVector<LDPolygon> polygons = rasterizePolygons(segments);
	LDObjectList result;

	for (LDPolygon& poly : polygons)
	{
		LDLine* line = LDSpawn<LDLine> (poly.vertices[0], poly.vertices[1]);
		line->setColor (poly.color);
		result << line;
	}

	return result;
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

// =============================================================================
//
// Selects this object.
//
void LDObject::select()
{
	if (not isSelected() and document())
	{
		m_isSelected = true;
		document()->addToSelection (this);
	}
}

// =============================================================================
//
// Removes this object from selection
//
void LDObject::deselect()
{
	if (isSelected() and document())
	{
		m_isSelected = false;
		document()->removeFromSelection (this);

		// If this object is inverted with INVERTNEXT, deselect the INVERTNEXT as well.
		LDBfc* invertnext;

		if (previousIsInvertnext (invertnext))
			invertnext->deselect();
	}
}

// =============================================================================
//
LDObject* LDObject::createCopy() const
{
	LDObject* copy = ParseLine (asText());
	return copy;
}

// =============================================================================
//
LDDocument* LDSubfileReference::fileInfo() const
{
	return m_fileInfo;
}

void LDSubfileReference::setFileInfo (LDDocument* newReferee)
{
	changeProperty (this, &m_fileInfo, newReferee);

	if (document())
		document()->recountTriangles();

	// If it's an immediate subfile reference (i.e. this subfile is in an opened document), we need to pre-compile the
	// GL polygons for the document if they don't exist already.
	if (newReferee and
		newReferee->isCache() == false and
		newReferee->polygonData().isEmpty())
	{
		newReferee->initializeCachedData();
	}
};

void LDObject::getVertices (QSet<Vertex>& verts) const
{
	for (int i = 0; i < numVertices(); ++i)
		verts.insert(vertex(i));
}

void LDSubfileReference::getVertices (QSet<Vertex>& verts) const
{
	verts.unite(fileInfo()->inlineVertices());
}
