/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Teemu Piippo
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
#include "mainWindow.h"
#include "editHistory.h"
#include "glRenderer.h"
#include "colors.h"
#include "glCompiler.h"

CFGENTRY (String, DefaultName, "")
CFGENTRY (String, DefaultUser, "")
CFGENTRY (Bool, UseCALicense, true)

// List of all LDObjects
QMap<int32, LDObject*> g_allObjects;
static int32 g_idcursor = 1; // 0 shalt be null

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
	qObjListEntry (null)
{
	if (document)
		document->addObject (this);

	memset (m_coords, 0, sizeof m_coords);
	chooseID();

	if (id() != 0)
		g_allObjects[id()] = this;

	setRandomColor (QColor::fromHsv (rand() % 360, rand() % 256, rand() % 96 + 128));
}

LDSubfile::LDSubfile (LDDocument* document) :
	LDMatrixObject (document) {}

LDOBJ_DEFAULT_CTOR (LDEmpty, LDObject)
LDOBJ_DEFAULT_CTOR (LDError, LDObject)
LDOBJ_DEFAULT_CTOR (LDLine, LDObject)
LDOBJ_DEFAULT_CTOR (LDTriangle, LDObject)
LDOBJ_DEFAULT_CTOR (LDCondLine, LDLine)
LDOBJ_DEFAULT_CTOR (LDQuad, LDObject)
LDOBJ_DEFAULT_CTOR (LDVertex, LDObject)
LDOBJ_DEFAULT_CTOR (LDOverlay, LDObject)
LDOBJ_DEFAULT_CTOR (LDBFC, LDObject)
LDOBJ_DEFAULT_CTOR (LDComment, LDObject)

LDObject::~LDObject()
{
	// Don't bother during program termination
	if (IsExiting() == false)
	{
		// If this object was selected, unselect it now
		if (isSelected() and document() != null)
			deselect();

		// If this object was associated to a file, remove it off it now
		if (document() != null)
			document()->forgetObject (this);

		// Delete the GL lists
		if (g_win != null)
			g_win->R()->forgetObject (this);

		// Remove this object from the list of LDObjects
		g_allObjects.erase (g_allObjects.find (id()));
	}
}

// =============================================================================
//
void LDObject::chooseID()
{
	// Let's hope that nobody goes to create 17 million objects anytime soon...
	if (g_idcursor < MAX_LDOBJECT_IDS)
		setID (g_idcursor++);
	else
		setID (0);
}

// =============================================================================
//
void LDObject::setVertexCoord (int i, Axis ax, double value)
{
	Vertex v = vertex (i);
	v.setCoordinate (ax, value);
	setVertex (i, v);
}

// =============================================================================
//
QString LDComment::asText() const
{
	return format ("0 %1", text());
}

// =============================================================================
//
QString LDSubfile::asText() const
{
	QString val = format ("1 %1 %2 ", color(), position());
	val += transform().toString();
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

// =============================================================================
//
QString LDError::asText() const
{
	return contents();
}

// =============================================================================
//
QString LDVertex::asText() const
{
	return format ("0 !LDFORGE VERTEX %1 %2", color(), pos);
}

// =============================================================================
//
QString LDEmpty::asText() const
{
	return "";
}

// =============================================================================
//
const char* LDBFC::StatementStrings[] =
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

QString LDBFC::asText() const
{
	return format ("0 BFC %1", StatementStrings[int (m_statement)]);
}

// =============================================================================
//
QList<LDTriangle*> LDQuad::splitToTriangles()
{
	// Create the two triangles based on this quadrilateral:
	// 0---3       0---3    3
	// |   |       |  /    /|
	// |   |  ==>  | /    / |
	// |   |       |/    /  |
	// 1---2       1    1---2
	LDTriangle* tri1 (new LDTriangle (vertex (0), vertex (1), vertex (3)));
	LDTriangle* tri2 (new LDTriangle (vertex (1), vertex (2), vertex (3)));

	// The triangles also inherit the quad's color
	tri1->setColor (color());
	tri2->setColor (color());

	return {tri1, tri2};
}

// =============================================================================
//
void LDObject::replace (LDObject* other)
{
	int idx = lineNumber();

	if (idx != -1)
	{
		// Replace the instance of the old object with the new object
		document()->setObject (idx, other);

		// Remove the old object
		destroy();
	}
}

// =============================================================================
//
void LDObject::swap (LDObject* other)
{
	assert (document() == other->document());
	document()->swapObjects (this, other);
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
void LDObject::destroy()
{
	delete this;
}

// =============================================================================
//
void LDObject::setDocument (LDDocument* const& a)
{
	m_document = a;

	if (a == null)
		setSelected (false);
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

	case OBJ_Subfile:
		{
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			Matrix newMatrix = transform * ref->transform();
			Vertex newpos = ref->position();
			newpos.transform (transform, pos);
			ref->setPosition (newpos);
			ref->setTransform (newMatrix);
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
LDObjectList LDSubfile::inlineContents (bool deep, bool render)
{
	LDObjectList objs = fileInfo()->inlineContents (deep, render);

	// Transform the objects
	for (LDObject* obj : objs)
	{
		// assert (obj->type() != OBJ_Subfile);
		// Set the parent now so we know what inlined the object.
		obj->setParent (this);
		TransformObject (obj, transform(), position(), color());
	}

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
		return null;

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
QList<LDPolygon> LDSubfile::inlinePolygons()
{
	QList<LDPolygon> data = fileInfo()->inlinePolygons();

	for (LDPolygon& entry : data)
	{
		for (int i = 0; i < entry.numVertices(); ++i)
			entry.vertices[i].transform (transform(), position());
	}

	return data;
}

// =============================================================================
// -----------------------------------------------------------------------------
long LDObject::lineNumber() const
{
	assert (document() != null);

	for (int i = 0; i < document()->getObjectCount(); ++i)
	{
		if (document()->getObject (i) == this)
			return i;
	}

	return -1;
}

// =============================================================================
//
void LDObject::moveObjects (LDObjectList objs, const bool up)
{
	if (objs.isEmpty())
		return;

	// If we move down, we need to iterate the array in reverse order.
	long const start = up ? 0 : (objs.size() - 1);
	long const end = up ? objs.size() : -1;
	long const incr = up ? 1 : -1;
	LDObjectList objsToCompile;
	LDDocument* file = objs[0]->document();

	for (long i = start; i != end; i += incr)
	{
		LDObject* obj = objs[i];

		long const idx = obj->lineNumber();
		long const target = idx + (up ? -1 : 1);

		if ((up and idx == 0) or (not up and idx == (long) file->objects().size() - 1l))
		{
			// One of the objects hit the extrema. If this happens, this should be the first
			// object to be iterated on. Thus, nothing has changed yet and it's safe to just
			// abort the entire operation.
			assert (i == start);
			return;
		}

		objsToCompile << obj;
		objsToCompile << file->getObject (target);

		obj->swap (file->getObject (target));
	}

	RemoveDuplicates (objsToCompile);

	// The objects need to be recompiled, otherwise their pick lists are left with
	// the wrong index colors which messes up selection.
	for (LDObject* obj : objsToCompile)
		g_win->R()->compileObject (obj);
}

// =============================================================================
//
QString LDObject::typeName (LDObjectType type)
{
	return LDObject::getDefault (type)->typeName();
}

// =============================================================================
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

		QString noun = format ("%1%2", typeName (objType), Plural (count));

		// Plural of "vertex" is "vertices", correct that
		if (objType == OBJ_Vertex and count != 1)
			noun = "vertices";

		text += format ("%1 %2", count, noun);
	}

	return text;
}

// =============================================================================
//
LDObject* LDObject::topLevelParent()
{
	LDObject* it;
	
	for (it = this; it->parent(); it = it->parent())
		;

	return it;
}

// =============================================================================
//
LDObject* LDObject::next() const
{
	int idx = lineNumber();
	assert (idx != -1);

	if (idx == document()->getObjectCount() - 1)
		return nullptr;

	return document()->getObject (idx + 1);
}

// =============================================================================
//
LDObject* LDObject::previous() const
{
	int idx = lineNumber();
	assert (idx != -1);

	if (idx == 0)
		return nullptr;

	return document()->getObject (idx - 1);
}

// =============================================================================
//
bool LDObject::previousIsInvertnext (LDBFC*& ptr)
{
	LDObject* prev = previous();

	if (prev and prev->type() == OBJ_BFC and static_cast<LDBFC*> (prev)->statement() == BFCStatement::InvertNext)
	{
		ptr = static_cast<LDBFC*> (prev);
		return true;
	}

	return false;
}

// =============================================================================
//
void LDObject::move (Vertex vect)
{
	if (hasMatrix())
	{
		LDMatrixObject* mo = static_cast<LDMatrixObject*> (this);
		mo->setPosition (mo->position() + vect);
	}
	else if (type() == OBJ_Vertex)
	{
		// ugh
		static_cast<LDVertex*> (this)->pos += vect;
	}
	else
	{
		for (int i = 0; i < numVertices(); ++i)
			setVertex (i, vertex (i) + vect);
	}
}

// =============================================================================
//
LDObject* LDObject::getDefault (const LDObjectType type)
{
	switch (type)
	{
		case OBJ_Comment:		return LDSpawn<LDComment>();
		case OBJ_BFC:			return LDSpawn<LDBFC>();
		case OBJ_Line:			return LDSpawn<LDLine>();
		case OBJ_CondLine:		return LDSpawn<LDCondLine>();
		case OBJ_Subfile:		return LDSpawn<LDSubfile>();
		case OBJ_Triangle:		return LDSpawn<LDTriangle>();
		case OBJ_Quad:			return LDSpawn<LDQuad>();
		case OBJ_Empty:			return LDSpawn<LDEmpty>();
		case OBJ_Error:			return LDSpawn<LDError>();
		case OBJ_Vertex:		return LDSpawn<LDVertex>();
		case OBJ_Overlay:		return LDSpawn<LDOverlay>();
		case OBJ_NumTypes:		assert (false);
	}
	return nullptr;
}

// =============================================================================
//
void LDObject::invert() {}
void LDBFC::invert() {}
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
void LDSubfile::invert()
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

		setTransform (transform() * matrixModifier);
		return;
	}

	// Subfile is not flat. Resort to invertnext.
	int idx = lineNumber();

	if (idx > 0)
	{
		LDBFC* bfc = dynamic_cast<LDBFC*> (previous());

		if (bfc and bfc->statement() == BFCStatement::InvertNext)
		{
			// This is prefixed with an invertnext, thus remove it.
			bfc->destroy();
			return;
		}
	}

	// Not inverted, thus prefix it with a new invertnext.
	document()->insertObj (idx, new LDBFC (BFCStatement::InvertNext));
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

void LDVertex::invert() {}

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
			obj->document()->addToHistory (new EditHistory (idx, before, after));
			g_win->R()->compileObject (obj);
			CurrentDocument()->redoVertices();
		}
	}
	else
	{
		*ptr = val;
	}
}

// =============================================================================
//
void LDObject::setColor (LDColor const& val)
{
	changeProperty (this, &m_color, val);
}

// =============================================================================
//
const Vertex& LDObject::vertex (int i) const
{
	return m_coords[i];
}

// =============================================================================
//
void LDObject::setVertex (int i, const Vertex& vert)
{
	changeProperty (this, &m_coords[i], vert);
}

// =============================================================================
//
void LDMatrixObject::setPosition (const Vertex& a)
{
	changeProperty (this, &m_position, a);
}

// =============================================================================
//
void LDMatrixObject::setTransform (const Matrix& val)
{
	changeProperty (this, &m_transform, val);
}

// =============================================================================
//
void LDObject::select()
{
	if (document() != null)
		document()->addToSelection (this);
}

// =============================================================================
//
void LDObject::deselect()
{
	if (document() != null)
	{
		document()->removeFromSelection (this);

		// If this object is inverted with INVERTNEXT, deselect the INVERTNEXT as well.
		LDBFC* invertnext;

		if (previousIsInvertnext (invertnext))
			invertnext->deselect();
	}
}

// =============================================================================
//
QString PreferredLicenseText()
{
	return (cfg::UseCALicense ? CALicenseText : "");
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
void LDSubfile::setFileInfo (LDDocument* const& a)
{
	changeProperty (this, &m_fileInfo, a);

	// If it's an immediate subfile reference (i.e. this subfile belongs in an
	// explicit file), we need to pre-compile the GL polygons for the document
	// if they don't exist already.
	if (a != null and
		a->isImplicit() == false and
		a->polygonData().isEmpty())
	{
		a->initializeCachedData();
	}
};

void LDObject::getVertices (QVector<Vertex>& verts) const
{
	for (int i = 0; i < numVertices(); ++i)
		verts << vertex (i);
}

void LDSubfile::getVertices (QVector<Vertex>& verts) const
{
	verts << fileInfo()->inlineVertices();
}

void LDVertex::getVertices (QVector<Vertex>& verts) const
{
	verts.append (pos);
}
