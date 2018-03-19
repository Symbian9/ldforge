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

#include "../linetypes/modelobject.h"
#include "../lddocument.h"

/*
 * Returns whether or not the document is flat.
 * If it is flat, the result is stored in *axis.
 */
bool isflat(Model* model, Axis* flatDimension)
{
	// The dimensions that this model is potentially flat in.
	QVector<Axis> dimensions = {X, Y, Z};

	// Iterate through everything in the subfile. If there is any vertex with a coordinate not at
	// zero, the subfile is not flat in that dimension.
	for (LDObject* subfileObject : model->objects())
	{
		for (int i = 0; i < subfileObject->numVertices(); ++i)
		{
			Vertex const& v_i = subfileObject->vertex(i);

			if (not qFuzzyCompare(v_i.x(), 0.f))
				dimensions.removeOne(X);

			if (not qFuzzyCompare(v_i.y(), 0.f))
				dimensions.removeOne(Y);

			if (not qFuzzyCompare(v_i.z(), 0.f))
				dimensions.removeOne(Z);
		}

		// If there are no more dimensions left, we can exit the loop.
		if (dimensions.isEmpty())
			break;
	}

	if (dimensions.size() == 1)
	{
		// The model is flat in one dimension, return that.
		// If the model is flat in two or three dimensions, it's not really a valid model.
		*flatDimension = dimensions[0];
		return true;
	}
	else
	{
		// The model is not flat.
		return false;
	}
}

/*
 * Returns a matrix that causes a flip on the given dimension.
 */
Matrix flipmatrix(Axis dimension)
{
	Matrix result = Matrix::identity;

	switch (dimension)
	{
	case X:
		result(0, 0) = -1;
		break;

	case Y:
		result(1, 1) = -1;
		break;

	case Z:
		result(2, 2) = -1;
		break;
	}

	return result;
}

/*
 * Inverts an LDObject so that its winding is changed.
 */
void invert(LDObject* obj, DocumentManager* context)
{
	if (obj->numPolygonVertices() > 0)
	{
		// Object is vertex based, so change the order of the vertices.
		QVector<Vertex> vertices;
		vertices.resize(obj->numPolygonVertices());

		for (int i = 0; i < vertices.size(); i += 1)
			vertices[vertices.size() - 1 - i] = obj->vertex(i);

		for (int i = 0; i < vertices.size(); i += 1)
			obj->setVertex(i, vertices[i]);
	}
	else if (obj->type() == LDObjectType::SubfileReference)
	{
		// Check whether subfile is flat. If it is, flip it on the axis on which it is flat.
		Model model {context};
		LDSubfileReference* reference = static_cast<LDSubfileReference*>(obj);
		reference->fileInfo(context)->inlineContents(model, true, false);
		Axis flatDimension;

		if (::isflat(&model, &flatDimension))
		{
			reference->setTransformationMatrix(
				reference->transformationMatrix() * ::flipmatrix(flatDimension)
			);
		}
		else
		{
			// Subfile is not flat. Resort to invertnext.
			reference->setInverted(not reference->isInverted());
		}
	}
}

void invertPolygon(LDPolygon& polygon)
{
	switch (polygon.numPolygonVertices())
	{
	case 2:
	case 3:
		std::swap(polygon.vertices[0], polygon.vertices[1]);
		break;

	case 4:
		std::swap(polygon.vertices[1], polygon.vertices[3]);
		break;
	}
}
