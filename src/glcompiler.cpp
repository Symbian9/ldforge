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

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>
#include <GL/glext.h>
#include "glcompiler.h"
#include "guiutilities.h"
#include "documentmanager.h"
#include "grid.h"
#include "algorithms/invert.h"
#include "generics/ring.h"

void checkGLError(QString file, int line)
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

		print("OpenGL ERROR: at %1:%2: %3", file, line, errorMessage);
	}
}

/*
 * Constructs a GL compiler.
 */
GLCompiler::GLCompiler (GLRenderer* renderer) :
	HierarchyElement (renderer),
	m_renderer (renderer)
{
	connect(
		renderer->model(),
		SIGNAL(rowsInserted(QModelIndex, int, int)),
		this,
		SLOT(handleRowInsertion(QModelIndex, int, int))
	);
	connect(
		renderer->model(),
		SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),
		this,
		SLOT(handleRowRemoval(QModelIndex, int, int))
	);
	connect(
		renderer->model(),
		SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
		this,
		SLOT(handleDataChange(QModelIndex, QModelIndex))
	);
	connect(
		renderer,
		SIGNAL(objectHighlightingChanged(QModelIndex, QModelIndex)),
		this,
		SLOT(handleObjectHighlightingChanged(QModelIndex, QModelIndex))
	);
	connect(m_window, SIGNAL(gridChanged()), this, SLOT(recompile()));

	for (QModelIndex index : renderer->model()->indices())
	{
		print("%1", index);
		stageForCompilation(index);
	}
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
QColor GLCompiler::getColorForPolygon(
	LDPolygon& polygon,
	const QModelIndex& polygonOwnerIndex,
	VboSubclass subclass
) {
	QColor color;
	LDObject* polygonOwner = m_renderer->model()->lookup(polygonOwnerIndex);

	switch (subclass)
	{
	case VboSubclass::Surfaces:
	case VboSubclass::Normals:
	case VboSubclass::InvertedNormals:
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
		// For the picking scene, use unique picking colors provided by the model.
		return m_renderer->model()->pickingColorForObject(polygonOwnerIndex);

	case VboSubclass::RandomColors:
		// For the random color scene, the owner object has rolled up a random color. Use that.
		color = polygonOwner->randomColor();
		break;

	case VboSubclass::RegularColors:
		// For normal colors, use the polygon's color.
		if (LDColor {polygon.color} == MainColor)
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
		else if (LDColor {polygon.color} == EdgeColor)
		{
			// Edge color is black, unless we have a dark background, in which case lines need to be bright.
			color = luma(config::backgroundColor()) > 40 ? Qt::black : Qt::white;
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

		if (this->_selectionModel and this->_selectionModel->isSelected(polygonOwnerIndex))
			blendAlpha = 1.0;
		else if (polygonOwnerIndex == m_renderer->objectAtCursor())
			blendAlpha = 0.5;

		if (blendAlpha != 0.0)
		{
			QColor selectedColor = config::selectColorBlend();
			double denominator = blendAlpha + 1.0;
			color.setRed((color.red() + (selectedColor.red() * blendAlpha)) / denominator);
			color.setGreen((color.green() + (selectedColor.green() * blendAlpha)) / denominator);
			color.setBlue((color.blue() + (selectedColor.blue() * blendAlpha)) / denominator);
		}
	}
	else
	{
		// The color was unknown. Use main color to make the polygon at least not appear pitch-black.
		if (polygon.type != LDPolygon::Type::EdgeLine and polygon.type != LDPolygon::Type::ConditionalEdge)
			color = guiUtilities()->mainColorRepresentation();
		else
			color = Qt::black;

		// Warn about the unknown color, but only once.
		static QSet<LDColor> warnedColors;
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
void GLCompiler::stageForCompilation(const QModelIndex& index)
{
	m_staged.insert(index);
}

/*
 * Removes an object from the set of objects to be compiled.
 */
void GLCompiler::unstage(const QModelIndex& index)
{
	m_staged.remove(index);
}

/*
 * Compiles all staged objects.
 */
void GLCompiler::compileStaged()
{
	for (const QModelIndex& index : m_staged)
		compileObject(index);

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

		for (
			auto iterator = m_objectInfo.begin();
			iterator != m_objectInfo.end();
		) {
			if (not iterator.key().isValid())
			{
				iterator = m_objectInfo.erase(iterator);
			}
			else
			{
				LDObject* object = m_renderer->model()->lookup(iterator.key());
				if (not object->isHidden())
					vbodata += iterator->data[vbonum];

				++iterator;
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
void GLCompiler::dropObjectInfo(const QModelIndex& index)
{
	if (m_objectInfo.contains(index))
	{
		// If we have data relating to this object, remove it.
		// The VBOs have changed now and need to be merged.
		m_objectInfo.remove(index);
		this->needBoundingBoxRebuild = true;
		needMerge();
	}
}

/*
 * Makes the compiler forget about the given object completely.
 */
void GLCompiler::forgetObject(QModelIndex index)
{
	dropObjectInfo(index);
	unstage(index);
}

/*
 * Compiles a single object.
 */
void GLCompiler::compileObject(const QModelIndex& index)
{
	LDObject* object = m_renderer->model()->lookup(index);

	if (object == nullptr)
		return;

	ObjectVboData info;
	dropObjectInfo(index);

	switch (object->type())
	{
	// Note: We cannot split quads into triangles here, it would mess up the
	// wireframe view. Quads must go into separate vbos.
	case LDObjectType::Triangle:
	case LDObjectType::Quadrilateral:
	case LDObjectType::EdgeLine:
	case LDObjectType::ConditionalEdge:
		{
			LDPolygon polygon = object->getPolygon();
			compilePolygon(polygon, index, info);
		}
		break;

	default:
		if (object->isRasterizable())
		{
			auto data = object->rasterizePolygons(m_documents, m_renderer->model()->winding());

			for (LDPolygon& poly : data)
				compilePolygon(poly, index, info);
		}
		break;
	}

	m_objectInfo[index] = info;
	needMerge();
}

/*
 * Inserts a single polygon into VBOs.
 */
void GLCompiler::compilePolygon(
	LDPolygon& poly,
	const QModelIndex& polygonOwnerIndex,
	ObjectVboData& objectInfo
) {
	if (m_renderer->model()->winding() == Clockwise)
		::invertPolygon(poly);

	VboClass surface;

	switch (poly.type)
	{
	case LDPolygon::Type::EdgeLine:
		surface = VboClass::Lines;
		break;

	case LDPolygon::Type::Triangle:
		surface = VboClass::Triangles;
		break;

	case LDPolygon::Type::Quadrilateral:
		surface = VboClass::Quads;
		break;

	case LDPolygon::Type::ConditionalEdge:
		surface = VboClass::ConditionalLines;
		break;

	default:
		return;
	}

	// Determine the normals for the polygon.
	QVector3D normals[4];
	auto vertexRing = ring(poly.vertices, poly.numPolygonVertices());

	for (int i = 0; i < poly.numPolygonVertices(); ++i)
	{
		const Vertex& v1 = vertexRing[i - 1];
		const Vertex& v2 = vertexRing[i];
		const Vertex& v3 = vertexRing[i + 1];
		normals[i] = QVector3D::crossProduct(v3 - v2, v1 - v2).normalized();
	}

	// Transform vertices so that they're suitable for GL rendering
	for (int i = 0; i < poly.numPolygonVertices(); i += 1)
	{
		poly.vertices[i].y = -poly.vertices[i].y;
		poly.vertices[i].z = -poly.vertices[i].z;

		// Add these vertices to the bounding box (unless we're going to do it over
		// from scratch afterwards)
		if (not this->needBoundingBoxRebuild)
			this->boundingBox.consider(poly.vertices[i]);
	}

	for (VboSubclass complement : iterateEnum<VboSubclass>())
	{
		const int vbonum = vboNumber (surface, complement);
		QVector<GLfloat>& vbodata = objectInfo.data[vbonum];
		const QColor color = getColorForPolygon (poly, polygonOwnerIndex, complement);

		for (int vert = 0; vert < poly.numPolygonVertices(); ++vert)
		{
			if (complement == VboSubclass::Surfaces)
			{
				// Write coordinates. Apparently Z must be flipped too?
				vbodata	<< poly.vertices[vert].x
						<< poly.vertices[vert].y
						<< poly.vertices[vert].z;
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

/*
 * Returns the center point of the model.
 */
Vertex GLCompiler::modelCenter()
{
	// If there's something still queued for compilation, we need to build those first so
	// that they get into the bounding box.
	this->compileStaged();

	// If the bounding box is invalid, rebuild it now.
	if (this->needBoundingBoxRebuild)
	{
		this->boundingBox = {};
		QMapIterator<QPersistentModelIndex, ObjectVboData> iterator {m_objectInfo};

		while (iterator.hasNext())
		{
			iterator.next();

			for (VboClass vboclass : {
				VboClass::Lines,
				VboClass::Triangles,
				VboClass::Quads,
				VboClass::ConditionalLines
			}) {
				// Read in the surface vertices and add them to the bounding box.
				int vbonum = vboNumber(vboclass, VboSubclass::Surfaces);
				const auto& vector = iterator.value().data[vbonum];

				for (int i = 0; i + 2 < countof(vector); i += 3)
					this->boundingBox.consider({vector[i], vector[i + 1], vector[i + 2]});
			}
		}

		this->needBoundingBoxRebuild = false;
	}

	if (not this->boundingBox.isEmpty())
		return this->boundingBox.center();
	else
		return {};
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

void GLCompiler::fullUpdate()
{
	m_objectInfo.clear();
	recompile();
}

/*
 * Recompiles the entire model.
 */
void GLCompiler::recompile()
{
	for (QModelIndex index : m_renderer->model()->indices())
		compileObject(index);

	emit sceneChanged();
}

void GLCompiler::handleRowInsertion(const QModelIndex&, int first, int last)
{
	for (int row = first; row <= last; row += 1)
		m_staged.insert(m_renderer->model()->index(row));

	emit sceneChanged();
}

void GLCompiler::handleRowRemoval(const QModelIndex&, int first, int last)
{
	for (int row = last; row >= first; row -= 1)
		forgetObject(m_renderer->model()->index(row));

	this->needBoundingBoxRebuild = true;
	emit sceneChanged();
}

void GLCompiler::handleDataChange(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	for (int row = topLeft.row(); row <= bottomRight.row(); row += 1)
		m_staged.insert(m_renderer->model()->index(row));

	this->needBoundingBoxRebuild = true;
	emit sceneChanged();
}

void GLCompiler::handleObjectHighlightingChanged(
	const QModelIndex& oldIndex,
	const QModelIndex& newIndex
) {
	m_staged.insert(oldIndex);
	m_staged.insert(newIndex);
	emit sceneChanged();
}

void GLCompiler::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	for (const QModelIndex& index : selected.indexes())
		m_staged.insert(index);

	for (const QModelIndex& index : deselected.indexes())
		m_staged.insert(index);

	m_renderer->update();
	emit sceneChanged();
}

QItemSelectionModel* GLCompiler::selectionModel() const
{
	return _selectionModel;
}

void GLCompiler::setSelectionModel(QItemSelectionModel* selectionModel)
{
	if (this->_selectionModel)
		disconnect(this->_selectionModel, 0, 0, 0);

	this->_selectionModel = selectionModel;

	if (this->_selectionModel)
	{
		connect(
			this->_selectionModel,
			SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			this,
			SLOT(selectionChanged(const QItemSelection&, const QItemSelection&))
		);
		connect(
			this->_selectionModel,
			SIGNAL(destroyed(QObject*)),
			this,
			SLOT(clearSelectionModel())
		);
	}

	emit sceneChanged();
}

void GLCompiler::clearSelectionModel()
{
	this->setSelectionModel(nullptr);
	emit sceneChanged();
}
