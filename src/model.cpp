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

#include "model.h"
#include "linetypes/modelobject.h"
#include "documentmanager.h"
#include "linetypes/comment.h"
#include "linetypes/conditionaledge.h"
#include "linetypes/edgeline.h"
#include "linetypes/empty.h"
#include "linetypes/quadrilateral.h"
#include "linetypes/triangle.h"

Model::Model(DocumentManager* manager) :
    QObject {manager},
    _manager {manager} {}

Model::~Model()
{
	for (int i = 0; i < countof(_objects); ++i)
		delete _objects[i];
}

/*
 * Takes an existing object and migrates it to the end of this model. The object is removed from its old model in the process.
 */
void Model::addObject(LDObject *object)
{
	insertObject(size(), object);
}

/*
 * Returns the amount of objects in this model.
 */
int Model::size() const
{
	return _objects.size();
}

/*
 * Returns the vector of objects in this model.
 */
const QVector<LDObject*>& Model::objects() const
{
	return _objects;
}

/*
 * Takes an existing object and migrates it to this model, at the specified position.
 */
void Model::insertObject(int position, LDObject* object)
{
	if (object->model() and object->model() != this)
		object->model()->withdraw(object);

	// TODO: check that the object isn't in the vector once there's a cheap way to do so!
	_objects.insert(position, object);
	_needsTriangleRecount = true;
	object->setDocument(this);
	emit objectAdded(object);
}

/*
 * Swaps one object with another, assuming they both are in this model.
 */
bool Model::swapObjects(LDObject* one, LDObject* other)
{
	int a = _objects.indexOf(one);
	int b = _objects.indexOf(other);

	if (a != b and a != -1 and b != -1)
	{
		_objects[b] = one;
		_objects[a] = other;
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Assigns a new object to the specified position in the model. The object that already is in the position is deleted in the process.
 */
bool Model::setObjectAt(int idx, LDObject* obj)
{
	if (idx < 0 or idx >= countof(_objects))
	{
		return false;
	}
	else
	{
		removeAt(idx);
		insertObject(idx, obj);
		return true;
	}
}

/*
 * Returns the object at the specified position, or null if not found.
 */
LDObject* Model::getObject(int position) const
{
	if (position >= 0 and position < countof(_objects))
		return _objects[position];
	else
		return nullptr;
}

/*
 * Removes the given object from the model.
 */
void Model::remove(LDObject* object)
{
	int position = object->lineNumber();
	if (_objects[position] == object)
		removeAt(position);
}

/*
 * Removes the object at the provided position.
 */
void Model::removeAt(int position)
{
	LDObject* object = withdrawAt(position);
	delete object;
}

/*
 * Replaces the given object with the contents of a model.
 */
void Model::replace(LDObject *object, Model &model)
{
	if (object->model() == this)
	{
		int position = object->lineNumber();

		for (int i = countof(model.objects()) - 1; i >= 1; i -= 1)
			insertObject(position + i, model.objects()[i]);

		setObjectAt(position, model.objects()[0]);
	}
}

/*
 * Signals the model to recount its triangles.
 */
void Model::recountTriangles()
{
	_needsTriangleRecount = true;
}

/*
 * Returns the triangle count in the model.
 */
int Model::triangleCount() const
{
	if (_needsTriangleRecount)
	{
		_triangleCount = 0;

		for (LDObject* object : objects())
			_triangleCount += object->triangleCount();

		_needsTriangleRecount = false;
	}

	return _triangleCount;
}

/*
 * Merges the given model into this model, starting at the given position. The other model is emptied in the process.
 */
void Model::merge(Model& other, int position)
{
	if (position < 0)
		position = countof(_objects);

	// Copy the vector of objects to copy, so that we don't change the vector while iterating over it.
	QVector<LDObject*> objectsCopy = other._objects;

	// Inform the contents of their new owner
	for (LDObject* object : objectsCopy)
	{
		insertObject(position, object);
		position += 1;
	}

	other.clear();
}

/*
 * Returns the begin-iterator into this model, so that models can be used in foreach-loops.
 */
QVector<LDObject*>::iterator Model::begin()
{
	return _objects.begin();
}

/*
 * Returns the end-iterator into this mode, so that models can be used in foreach-loops.
 */
QVector<LDObject*>::iterator Model::end()
{
	return _objects.end();
}

/*
 * Removes all objects in this model.
 */
void Model::clear()
{
	for (int i = _objects.size() - 1; i >= 0; i -= 1)
		removeAt(i);

	_triangleCount = 0;
	_needsTriangleRecount = false;
}

/*
 * Drops the object from the model. The object becomes a free object as a result (thus violating the invariant that every object
 * has a model!). The caller must immediately add the withdrawn object to another model.
 *
 * This private method is only used to implement public API.
 */
void Model::withdraw(LDObject* object)
{
	if (object->model() == this)
	{
		int position = object->lineNumber();

		if (_objects[position] == object)
			withdrawAt(position);
	}
}

/*
 * Drops an object from the model at the provided position. The caller must immediately put the result value object into a new model.
 */
LDObject* Model::withdrawAt(int position)
{
	LDObject* object = _objects[position];
	emit aboutToRemoveObject(object);
	_objects.removeAt(position);
	_needsTriangleRecount = true;
	return object;
}

/*
 * Returns whether or not this model is empty.
 */
bool Model::isEmpty() const
{
	return _objects.isEmpty();
}

/*
 * Returns the model's associated document manager. This pointer is used to resolve subfile references.
 */
DocumentManager* Model::documentManager() const
{
	return _manager;
}

// =============================================================================
//
static void CheckTokenCount (const QStringList& tokens, int num)
{
	if (countof(tokens) != num)
		throw QString (format ("Bad amount of tokens, expected %1, got %2", num, countof(tokens)));
}

// =============================================================================
//
static void CheckTokenNumbers (const QStringList& tokens, int min, int max)
{
	bool ok;
	QRegExp scientificRegex ("\\-?[0-9]+\\.[0-9]+e\\-[0-9]+");

	for (int i = min; i <= max; ++i)
	{
		// Check for floating point
		tokens[i].toDouble (&ok);
		if (ok)
			return;

		// Check hex
		if (tokens[i].startsWith ("0x"))
		{
			tokens[i].mid (2).toInt (&ok, 16);

			if (ok)
				return;
		}

		// Check scientific notation, e.g. 7.99361e-15
		if (scientificRegex.exactMatch (tokens[i]))
			return;

		throw QString (format ("Token #%1 was `%2`, expected a number (matched length: %3)",
		    (i + 1), tokens[i], scientificRegex.matchedLength()));
	}
}

// =============================================================================
//
static Vertex ParseVertex (QStringList& s, const int n)
{
	Vertex v;
	v.apply ([&] (Axis ax, double& a) { a = s[n + ax].toDouble(); });
	return v;
}

static qint32 StringToNumber (QString a, bool* ok = nullptr)
{
	int base = 10;

	if (a.startsWith ("0x"))
	{
		a.remove (0, 2);
		base = 16;
	}

	return a.toLong (ok, base);
}

// =============================================================================
// This is the LDraw code parser function. It takes in a string containing LDraw
// code and returns the object parsed from it. parseLine never returns null,
// the object will be LDError if it could not be parsed properly.
// =============================================================================
LDObject* Model::insertFromString(int position, QString line)
{
	try
	{
		QStringList tokens = line.split(" ", QString::SkipEmptyParts);

		if (countof(tokens) <= 0)
		{
			// Line was empty, or only consisted of whitespace
			return emplaceAt<LDEmpty>(position);
		}

		if (countof(tokens[0]) != 1 or not tokens[0][0].isDigit())
			throw QString ("Illogical line code");

		int num = tokens[0][0].digitValue();

		switch (num)
		{
		    case 0:
		    {
			    // Comment
			    QString commentText = line.mid (line.indexOf ("0") + 2);
				QString commentTextSimplified = commentText.simplified();

				// Handle BFC statements
				if (countof(tokens) > 2 and tokens[1] == "BFC")
				{
					for (BfcStatement statement : iterateEnum<BfcStatement>())
					{
						if (commentTextSimplified == format("BFC %1", LDBfc::statementToString (statement)))
							return emplaceAt<LDBfc>(position, statement);
					}

					// MLCAD is notorious for stuffing these statements in parts it
					// creates. The above block only handles valid statements, so we
					// need to handle MLCAD-style invertnext, clip and noclip separately.
					if (commentTextSimplified == "BFC CERTIFY INVERTNEXT")
						return emplaceAt<LDBfc>(position, BfcStatement::InvertNext);
					else if (commentTextSimplified == "BFC CERTIFY CLIP")
						return emplaceAt<LDBfc>(position, BfcStatement::Clip);
					else if (commentTextSimplified == "BFC CERTIFY NOCLIP")
						return emplaceAt<LDBfc>(position, BfcStatement::NoClip);
				}

				if (countof(tokens) > 2 and tokens[1] == "!LDFORGE")
				{
					// Handle LDForge-specific types, they're embedded into comments too
					if (tokens[2] == "BEZIER_CURVE")
					{
						CheckTokenCount (tokens, 16);
						CheckTokenNumbers (tokens, 3, 15);
						LDBezierCurve* obj = emplaceAt<LDBezierCurve>(position);
						obj->setColor (StringToNumber (tokens[3]));

						for (int i = 0; i < 4; ++i)
							obj->setVertex (i, ParseVertex (tokens, 4 + (i * 3)));

						return obj;
					}
				}

				// Just a regular comment:
				return emplaceAt<LDComment>(position, commentText);
		    }

		    case 1:
		    {
			    // Subfile
			    CheckTokenCount (tokens, 15);
				CheckTokenNumbers (tokens, 1, 13);
				LDDocument* document = _manager->getDocumentByName(tokens[14]);

				// If we cannot open the file, mark it an error. Note we cannot use LDParseError
				// here because the error object needs the document reference.
				if (not document)
				{
					LDError* obj = emplaceAt<LDError>(position, line, format ("Could not open %1", tokens[14]));
					obj->setFileReferenced (tokens[14]);
					return obj;
				}

				Vertex referncePosition = ParseVertex (tokens, 2);  // 2 - 4
				Matrix transform;

				for (int i = 0; i < 9; ++i)
					transform.value(i) = tokens[i + 5].toDouble(); // 5 - 13

				LDSubfileReference* obj = emplaceAt<LDSubfileReference>(position, document, transform, referncePosition);
				obj->setColor (StringToNumber (tokens[1]));
				return obj;
		    }

		    case 2:
		    {
			    CheckTokenCount (tokens, 8);
				CheckTokenNumbers (tokens, 1, 7);

				// Line
				LDEdgeLine* obj = emplaceAt<LDEdgeLine>(position);
				obj->setColor (StringToNumber (tokens[1]));

				for (int i = 0; i < 2; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 7

				return obj;
		    }

		    case 3:
		    {
			    CheckTokenCount (tokens, 11);
				CheckTokenNumbers (tokens, 1, 10);

				// Triangle
				LDTriangle* obj = emplaceAt<LDTriangle>(position);
				obj->setColor (StringToNumber (tokens[1]));

				for (int i = 0; i < 3; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 10

				return obj;
		    }

		    case 4:
		    case 5:
		    {
			    CheckTokenCount (tokens, 14);
				CheckTokenNumbers (tokens, 1, 13);

				// Quadrilateral / Conditional line
				LDObject* obj;

				if (num == 4)
					obj = emplaceAt<LDQuadrilateral>(position);
				else
					obj = emplaceAt<LDConditionalEdge>(position);

				obj->setColor (StringToNumber (tokens[1]));

				for (int i = 0; i < 4; ++i)
					obj->setVertex (i, ParseVertex (tokens, 2 + (i * 3)));   // 2 - 13

				return obj;
		    }

		    default:
			    throw QString {"Unknown line code number"};
		}
	}
	catch (QString& errorMessage)
	{
		// Strange line we couldn't parse
		return emplaceAt<LDError>(position, line, errorMessage);
	}
}

/*
 * Given an LDraw object string, parses it and inserts it into the model.
 */
LDObject* Model::addFromString(QString line)
{
	return insertFromString(size(), line);
}

/*
 * Replaces the given object with a new one that is parsed from the given LDraw object string.
 * If the parsing fails, the object is replaced with an error object.
 */
LDObject* Model::replaceWithFromString(LDObject* object, QString line)
{
	if (object and object->model() == this)
	{
		int position = object->lineNumber();
		removeAt(position);
		return insertFromString(position, line);
	}
	else
		return nullptr;
}

int countof(Model& model)
{
	return model.size();
}
