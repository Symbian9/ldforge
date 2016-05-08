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

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include "glCompiler.h"
#include "ldObject.h"
#include "colors.h"
#include "ldDocument.h"
#include "miscallenous.h"
#include "glRenderer.h"
#include "dialogs.h"
#include "guiutilities.h"
#include "documentmanager.h"
#include "grid.h"

struct GLErrorInfo
{
	GLenum	value;
	QString	text;
};

static const GLErrorInfo g_GLErrors[] =
{
	{ GL_NO_ERROR,						"No error" },
	{ GL_INVALID_ENUM,					"Unacceptable enumerator passed" },
	{ GL_INVALID_VALUE,					"Numeric argument out of range" },
	{ GL_INVALID_OPERATION,				"The operation is not allowed to be done in this state" },
	{ GL_INVALID_FRAMEBUFFER_OPERATION,	"Framebuffer object is not complete"},
	{ GL_OUT_OF_MEMORY,					"Out of memory" },
	{ GL_STACK_UNDERFLOW,				"The operation would have caused an underflow" },
	{ GL_STACK_OVERFLOW,				"The operation would have caused an overflow" },
};

ConfigOption (QString SelectColorBlend = "#0080FF")

// static QMap<LDObject*, String> g_objectOrigins;

void CheckGLErrorImpl (const char* file, int line)
{
	QString errmsg;
	GLenum errnum = glGetError();

	if (errnum == GL_NO_ERROR)
		return;

	for (const GLErrorInfo& err : g_GLErrors)
	{
		if (err.value == errnum)
		{
			errmsg = err.text;
			break;
		}
	}

	print ("OpenGL ERROR: at %1:%2: %3", Basename (QString (file)), line, errmsg);
}


GLCompiler::GLCompiler (GLRenderer* renderer) :
	HierarchyElement (renderer),
	m_renderer (renderer)
{
	needMerge();
	memset (m_vboSizes, 0, sizeof m_vboSizes);
}


void GLCompiler::initialize()
{
	initializeOpenGLFunctions();
	glGenBuffers (NumVbos, &m_vbo[0]);
	CHECK_GL_ERROR();
}


GLCompiler::~GLCompiler()
{
	glDeleteBuffers (NumVbos, &m_vbo[0]);
	CHECK_GL_ERROR();
}


QColor GLCompiler::indexColorForID (int id) const
{
	// Calculate a color based from this index. This method caters for
	// 16777216 objects. I don't think that will be exceeded anytime soon. :)
	int r = (id / 0x10000) % 0x100,
		g = (id / 0x100) % 0x100,
		b = id % 0x100;

	return QColor (r, g, b);
}


QColor GLCompiler::getColorForPolygon (LDPolygon& poly, LDObject* topobj, ComplementVboType complement) const
{
	QColor qcol;
	static const QColor bfcFrontColor (64, 192, 80);
	static const QColor bfcBackColor (208, 64, 64);

	switch (complement)
	{
	case SurfacesVboComplement:
		return QColor();

	case BfcFrontColorsVboComplement:
		qcol = bfcFrontColor;
		break;

	case BfcBackColorsVboComplement:
		qcol = bfcBackColor;
		break;

	case PickColorsVboComplement:
		return indexColorForID (topobj->id());

	case RandomColorsVboComplement:
		qcol = topobj->randomColor();
		break;

	case NormalColorsVboComplement:
		if (poly.color == MainColor)
		{
			if (topobj->color() == MainColor)
				qcol = guiUtilities()->mainColorRepresentation();
			else
				qcol = topobj->color().faceColor();
		}
		else if (poly.color == EdgeColor)
		{
			qcol = luma (QColor (m_config->backgroundColor())) > 40 ? Qt::black : Qt::white;
		}
		else
		{
			LDColor col = poly.color;

			if (col.isValid())
				qcol = col.faceColor();
		}
		break;
	}

	if (not qcol.isValid())
	{
		// The color was unknown. Use main color to make the polygon at least
		// not appear pitch-black.
		if (poly.num != 2 and poly.num != 5)
			qcol = guiUtilities()->mainColorRepresentation();
		else
			qcol = Qt::black;

		// Warn about the unknown color, but only once.
		static QList<int> warnedColors;
		if (not warnedColors.contains (poly.color))
		{
			print ("Unknown color %1!\n", poly.color);
			warnedColors << poly.color;
		}

		return qcol;
	}

	double blendAlpha = 0.0;

	if (topobj->isSelected())
		blendAlpha = 1.0;
	else if (topobj == m_renderer->objectAtCursor())
		blendAlpha = 0.5;

	if (blendAlpha != 0.0)
	{
		QColor selcolor (m_config->selectColorBlend());
		double denom = blendAlpha + 1.0;
		qcol.setRed ((qcol.red() + (selcolor.red() * blendAlpha)) / denom);
		qcol.setGreen ((qcol.green() + (selcolor.green() * blendAlpha)) / denom);
		qcol.setBlue ((qcol.blue() + (selcolor.blue() * blendAlpha)) / denom);
	}

	return qcol;
}


void GLCompiler::needMerge()
{
	for (int i = 0; i < countof (m_vboChanged); ++i)
		m_vboChanged[i] = true;
}


void GLCompiler::stageForCompilation (LDObject* obj)
{
	/*
	g_objectOrigins[obj] = format ("%1:%2 (%3)",
		obj->document()->getDisplayName(), obj->lineNumber(), obj->typeName());
	*/

	m_staged << obj;
}


void GLCompiler::unstage (LDObject* obj)
{
	m_staged.remove (obj);
}


void GLCompiler::compileDocument (LDDocument* doc)
{
	if (doc)
	{
		for (LDObject* obj : doc->objects())
			compileObject (obj);
	}
}


void GLCompiler::compileStaged()
{
	for (QSetIterator<LDObject*> it (m_staged); it.hasNext();)
		compileObject (it.next());

	m_staged.clear();
}


void GLCompiler::prepareVBO (int vbonum)
{
	// Compile anything that still awaits it
	compileStaged();

	if (not m_vboChanged[vbonum])
		return;

	QVector<GLfloat> vbodata;

	for (auto it = m_objectInfo.begin(); it != m_objectInfo.end();)
	{
		if (it.key() == nullptr)
		{
			it = m_objectInfo.erase (it);
			continue;
		}

		if (it.key()->document() == currentDocument() and not it.key()->isHidden())
			vbodata += it->data[vbonum];

		++it;
	}

	glBindBuffer (GL_ARRAY_BUFFER, m_vbo[vbonum]);
	glBufferData (GL_ARRAY_BUFFER, vbodata.size() * sizeof(GLfloat), vbodata.constData(), GL_STATIC_DRAW);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	CHECK_GL_ERROR();
	m_vboChanged[vbonum] = false;
	m_vboSizes[vbonum] = vbodata.size();
}


void GLCompiler::dropObjectInfo (LDObject* obj)
{
	if (m_objectInfo.contains (obj))
	{
		m_objectInfo.remove (obj);
		needMerge();
	}
}


void GLCompiler::compileObject (LDObject* obj)
{
	if (obj == nullptr or obj->document() == nullptr or obj->document()->isCache())
		return;

	ObjectVBOInfo info;
	info.isChanged = true;
	dropObjectInfo (obj);

	switch (obj->type())
	{
	// Note: We cannot split quads into triangles here, it would mess up the wireframe view.
	// Quads must go into separate vbos.
	case OBJ_Triangle:
	case OBJ_Quad:
	case OBJ_Line:
	case OBJ_CondLine:
		{
			LDPolygon* poly = obj->getPolygon();
			poly->id = obj->id();
			compilePolygon (*poly, obj, &info);
			delete poly;
			break;
		}

	case OBJ_SubfileReference:
		{
			LDSubfileReference* ref = static_cast<LDSubfileReference*> (obj);
			auto data = ref->inlinePolygons();

			for (LDPolygon& poly : data)
			{
				poly.id = obj->id();
				compilePolygon (poly, obj, &info);
			}
			break;
		}

	case OBJ_BezierCurve:
		{
			LDBezierCurve* curve = static_cast<LDBezierCurve*> (obj);
			for (LDPolygon& polygon : curve->rasterizePolygons(grid()->bezierCurveSegments()))
			{
				polygon.id = obj->id();
				compilePolygon (polygon, obj, &info);
			}
		}
		break;

	default:
		break;
	}

	m_objectInfo[obj] = info;
	needMerge();
}


void GLCompiler::compilePolygon (LDPolygon& poly, LDObject* topobj, ObjectVBOInfo* objinfo)
{
	SurfaceVboType surface;
	int numverts;

	switch (poly.num)
	{
	case 2:	surface = LinesVbo;				numverts = 2; break;
	case 3:	surface = TrianglesVbo;			numverts = 3; break;
	case 4:	surface = QuadsVbo;				numverts = 4; break;
	case 5:	surface = ConditionalLinesVbo;	numverts = 2; break;
	default: return;
	}

	for (ComplementVboType complement : iterateEnum<ComplementVboType>())
	{
		const int vbonum = vboNumber (surface, complement);
		QVector<GLfloat>& vbodata = objinfo->data[vbonum];
		const QColor color = getColorForPolygon (poly, topobj, complement);

		for (int vert = 0; vert < numverts; ++vert)
		{
			if (complement == SurfacesVboComplement)
			{
				// Write coordinates. Apparently Z must be flipped too?
				vbodata	<< poly.vertices[vert].x()
						<< -poly.vertices[vert].y()
						<< -poly.vertices[vert].z();
			}
			else
			{
				vbodata	<< ((GLfloat) color.red()) / 255.0f
						<< ((GLfloat) color.green()) / 255.0f
						<< ((GLfloat) color.blue()) / 255.0f
						<< ((GLfloat) color.alpha()) / 255.0f;
			}
		}
	}
}


void GLCompiler::setRenderer (GLRenderer* renderer)
{
	m_renderer = renderer;
}


int GLCompiler::vboNumber (SurfaceVboType surface, ComplementVboType complement)
{
	return (surface * EnumLimits<ComplementVboType>::Count) + complement;
}


GLuint GLCompiler::vbo (int vbonum) const
{
	return m_vbo[vbonum];
}


int GLCompiler::vboSize (int vbonum) const
{
	return m_vboSizes[vbonum];
}