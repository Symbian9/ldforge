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

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include "glcompiler.h"
#include "miscallenous.h"
#include "guiutilities.h"
#include "documentmanager.h"
#include "grid.h"

void checkGLError(HierarchyElement* element, QString file, int line)
{
	struct ErrorInfo
	{
		GLenum	value;
		QString	text;
	};

	static const ErrorInfo knownErrors[] =
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

	GLenum errorNumber = glGetError();

	if (errorNumber != GL_NO_ERROR)
	{
		QString errorMessage;

		for (const ErrorInfo& error : knownErrors)
		{
			if (error.value == errorNumber)
			{
				errorMessage = error.text;
				break;
			}
		}

		element->print("OpenGL ERROR: at %1:%2: %3", Basename(file), line, errorMessage);
	}
}

/*
 * Constructs a GL compiler.
 */
GLCompiler::GLCompiler (GLRenderer* renderer) :
	HierarchyElement (renderer),
	m_renderer (renderer)
{
	connect(renderer->model(), SIGNAL(objectAdded(LDObject*)), this, SLOT(compileObject(LDObject*)));
	connect(renderer->model(), SIGNAL(objectModified(LDObject*)), this, SLOT(compileObject(LDObject*)));
	connect(renderer->model(), SIGNAL(aboutToRemoveObject(LDObject*)), this, SLOT(forgetObject(LDObject*)), Qt::DirectConnection);
	connect(renderer, SIGNAL(objectHighlightingChanged(LDObject*)), this, SLOT(compileObject(LDObject*)));
	connect(m_window, SIGNAL(gridChanged()), this, SLOT(recompile()));

	for (LDObject* object : renderer->model()->objects())
		stageForCompilation(object);
}

/*
 * Initializes the VBOs after OpenGL is initialized.
 */
void GLCompiler::initialize()
{
	initializeOpenGLFunctions();
	glGenBuffers(countof(m_vbo), &m_vbo[0]);
	CHECK_GL_ERROR();
}

/*
 * Destructs the VBOs when the compiler is deleted.
 */
GLCompiler::~GLCompiler()
{
	glDeleteBuffers(countof(m_vbo), &m_vbo[0]);
	CHECK_GL_ERROR();
}

/*
 * Returns an index color for the LDObject ID given. This color represents the object in the picking scene.
 */
QColor GLCompiler::indexColorForID (qint32 id) const
{
	// Calculate a color based from this index. This method caters for
	// 16777216 objects. I don't think that will be exceeded anytime soon. :)
	int r = (id / 0x10000) % 0x100;
	int g = (id / 0x100) % 0x100;
	int b = id % 0x100;
	return {r, g, b};
}

/*
 * Returns the suitable color for the polygon.
 * - polygon is the polygon to colorise.
 * - polygonOwner is the LDObject from which the polygon originated.
 * - subclass provides context for the polygon.
 */
QColor GLCompiler::getColorForPolygon(LDPolygon& polygon, LDObject* polygonOwner, VboSubclass subclass)
{
	QColor color;

	switch (subclass)
	{
	case VboSubclass::Surfaces:
	case VboSubclass::Normals:
	case VboSubclass::_End:
		// Surface and normal VBOs contain vertex data, not colors. So we can't return anything meaningful.
		return {};

	case VboSubclass::BfcFrontColors:
		// Use the constant green color for BFC front colors
		return {64, 192, 80};

	case VboSubclass::BfcBackColors:
		// Use the constant red color for BFC back colors
		return {208, 64, 64};

	case VboSubclass::PickColors:
		// For the picking scene, determine the color from the owner's ID.
		return indexColorForID(polygonOwner->id());

	case VboSubclass::RandomColors:
		// For the random color scene, the owner object has rolled up a random color. Use that.
		color = polygonOwner->randomColor();
		break;

	case VboSubclass::RegularColors:
		// For normal colors, use the polygon's color.
		if (polygon.color == MainColor)
		{
			// If it's the main color, use the polygon owner's color.
			if (polygonOwner->color() == MainColor)
			{
				// If that also is the main color, then we whatever the user has configured the main color to look like.
				color = guiUtilities()->mainColorRepresentation();
			}
			else
			{
				color = polygonOwner->color().faceColor();
			}
		}
		else if (polygon.color == EdgeColor)
		{
			// Edge color is black, unless we have a dark background, in which case lines need to be bright.
			color = luma(m_config->backgroundColor()) > 40 ? Qt::black : Qt::white;
		}
		else
		{
			// Not main or edge color, use the polygon's color as is.
			color = LDColor {polygon.color}.faceColor();
		}
		break;
	}

	if (color.isValid())
	{
		// We may wish to apply blending on the color to indicate selection or highlight.
		double blendAlpha = 0.0;

		if (polygonOwner->isSelected())
			blendAlpha = 1.0;
		else if (polygonOwner == m_renderer->objectAtCursor())
			blendAlpha = 0.5;

		if (blendAlpha != 0.0)
		{
			QColor selectedColor = m_config->selectColorBlend();
			double denominator = blendAlpha + 1.0;
			color.setRed((color.red() + (selectedColor.red() * blendAlpha)) / denominator);
			color.setGreen((color.green() + (selectedColor.green() * blendAlpha)) / denominator);
			color.setBlue((color.blue() + (selectedColor.blue() * blendAlpha)) / denominator);
		}
	}
	else
	{
		// The color was unknown. Use main color to make the polygon at least not appear pitch-black.
		if (polygon.num != 2 and polygon.num != 5)
			color = guiUtilities()->mainColorRepresentation();
		else
			color = Qt::black;

		// Warn about the unknown color, but only once.
		static QSet<int> warnedColors;
		if (not warnedColors.contains(polygon.color))
		{
			print(tr("Unknown color %1!\n"), polygon.color);
			warnedColors.insert(polygon.color);
		}
	}

	return color;
}

/*
 * Tells the compiler that a merge of VBOs is required.
 */
void GLCompiler::needMerge()
{
	for (int i = 0; i < countof (m_vboChanged); ++i)
		m_vboChanged[i] = true;
}

/*
 * Stages the given object for compilation.
 */
void GLCompiler::stageForCompilation(LDObject* obj)
{
	m_staged << obj;
}

/*
 * Removes an object from the set of objects to be compiled.
 */
void GLCompiler::unstage(LDObject* obj)
{
	m_staged.remove (obj);
}

/*
 * Compiles all staged objects.
 */
void GLCompiler::compileStaged()
{
	for (LDObject* object : m_staged)
		compileObject(object);

	m_staged.clear();
}

/*
 * Prepares a VBO for rendering. The VBO is merged if needed.
 */
void GLCompiler::prepareVBO (int vbonum)
{
	// Compile anything that still awaits it
	compileStaged();

	if (m_vboChanged[vbonum])
	{
		// Merge the VBO into a vector of floats.
		QVector<GLfloat> vbodata;

		for (auto it = m_objectInfo.begin(); it != m_objectInfo.end();)
		{
			if (it.key() == nullptr)
			{
				it = m_objectInfo.erase(it);
			}
			else
			{
				if (not it.key()->isHidden())
					vbodata += it->data[vbonum];

				++it;
			}
		}

		// Transfer the VBO to the graphics processor.
		glBindBuffer (GL_ARRAY_BUFFER, m_vbo[vbonum]);
		glBufferData (GL_ARRAY_BUFFER, countof(vbodata) * sizeof(GLfloat), vbodata.constData(), GL_STATIC_DRAW);
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERROR();
		m_vboChanged[vbonum] = false;
		m_vboSizes[vbonum] = countof(vbodata);
	}
}

/*
 * Removes the data related to the given object.
 */
void GLCompiler::dropObjectInfo(LDObject* object)
{
	if (m_objectInfo.contains(object))
	{
		// If we have data relating to this object, remove it. The VBOs have changed now and need to be merged.
		m_objectInfo.remove(object);
		needMerge();
	}
}

/*
 * Makes the compiler forget about the given object completely.
 */
void GLCompiler::forgetObject(LDObject* object)
{
	dropObjectInfo(object);
	unstage(object);
}

/*
 * Compiles a single object.
 */
void GLCompiler::compileObject(LDObject* object)
{
	if (object == nullptr)
		return;

	ObjectVboData info;
	dropObjectInfo(object);

	switch (object->type())
	{
	// Note: We cannot split quads into triangles here, it would mess up the wireframe view.
	// Quads must go into separate vbos.
	case LDObjectType::Triangle:
	case LDObjectType::Quadrilateral:
	case LDObjectType::EdgeLine:
	case LDObjectType::ConditionalEdge:
		{
			LDPolygon* poly = object->getPolygon();
			poly->id = object->id();
			compilePolygon (*poly, object, info);
			delete poly;
			break;
		}

	// TODO: try use interfaces to remove these special treatments?
	case LDObjectType::SubfileReference:
		{
			LDSubfileReference* subfileReference = static_cast<LDSubfileReference*>(object);
			auto data = subfileReference->inlinePolygons();

			for (LDPolygon& poly : data)
			{
				poly.id = object->id();
				compilePolygon (poly, object, info);
			}
			break;
		}

	case LDObjectType::BezierCurve:
		{
			LDBezierCurve* curve = static_cast<LDBezierCurve*>(object);
			for (LDPolygon& polygon : curve->rasterizePolygons(grid()->bezierCurveSegments()))
			{
				polygon.id = object->id();
				compilePolygon (polygon, object, info);
			}
		}
		break;

	default:
		break;
	}

	m_objectInfo[object] = info;
	needMerge();
}

/*
 * Inserts a single polygon into VBOs.
 */
void GLCompiler::compilePolygon(LDPolygon& poly, LDObject* polygonOwner, ObjectVboData& objectInfo)
{
	VboClass surface;
	int vertexCount;

	switch (poly.num)
	{
	case 2:	surface = VboClass::Lines;				vertexCount = 2; break;
	case 3:	surface = VboClass::Triangles;			vertexCount = 3; break;
	case 4:	surface = VboClass::Quads;				vertexCount = 4; break;
	case 5:	surface = VboClass::ConditionalLines;	vertexCount = 2; break;
	default: return;
	}

	// Determine the normals for the polygon.
	Vertex normals[4];
	auto vertexRing = ring(poly.vertices, vertexCount);

	for (int i = 0; i < vertexCount; ++i)
	{
		const Vertex& v1 = vertexRing[i - 1];
		const Vertex& v2 = vertexRing[i];
		const Vertex& v3 = vertexRing[i + 1];
		normals[i] = Vertex::crossProduct(v3 - v2, v1 - v2).normalized();
	}

	for (VboSubclass complement : iterateEnum<VboSubclass>())
	{
		const int vbonum = vboNumber (surface, complement);
		QVector<GLfloat>& vbodata = objectInfo.data[vbonum];
		const QColor color = getColorForPolygon (poly, polygonOwner, complement);

		for (int vert = 0; vert < vertexCount; ++vert)
		{
			if (complement == VboSubclass::Surfaces)
			{
				// Write coordinates. Apparently Z must be flipped too?
				vbodata	<< poly.vertices[vert].x()
						<< -poly.vertices[vert].y()
						<< -poly.vertices[vert].z();
			}
			else if (complement == VboSubclass::Normals)
			{
				vbodata << normals[vert].x()
				        << -normals[vert].y()
				        << -normals[vert].z();
			}
			else if (complement == VboSubclass::InvertedNormals)
			{
				vbodata << -normals[vert].x();
				vbodata << +normals[vert].y();
				vbodata << +normals[vert].z();
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

int GLCompiler::vboNumber (VboClass surface, VboSubclass complement)
{
	return (static_cast<int>(surface) * EnumLimits<VboSubclass>::Count) + static_cast<int>(complement);
}


GLuint GLCompiler::vbo (int vbonum) const
{
	return m_vbo[vbonum];
}


int GLCompiler::vboSize (int vbonum) const
{
	return m_vboSizes[vbonum];
}

/*
 * Recompiles the entire model.
 */
void GLCompiler::recompile()
{
	for (LDObject* object : m_renderer->model()->objects())
		compileObject(object);
}
