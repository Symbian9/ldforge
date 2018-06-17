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

#pragma once
#include "../main.h"
#include "../colors.h"
#include "../serializer.h"

class Model;
class LDDocument;
class DocumentManager;
struct LDPolygon;

/*
 * Object type codes.
 */
enum class LDObjectType
{
	SubfileReference,	//	Object represents a	sub-file reference
	Quadrilateral,		//	Object represents a	quadrilateral
	Triangle,			//	Object represents a	triangle
	EdgeLine,			//	Object represents a	line
	ConditionalEdge,	//	Object represents a	conditional line
	Comment,			//	Object represents a	comment
	Error,				//	Object is the result of failed parsing
	Empty,				//	Object represents an empty line
	BezierCurve,		//	Object represents a Bézier curve
	CircularPrimitive,
	_End
};

Q_DECLARE_METATYPE(LDObjectType)
MAKE_ITERABLE_ENUM(LDObjectType)

inline int qHash(LDObjectType type)
{
	return qHash(static_cast<int>(type));
}

/*
 * Represents one line of code in an LDraw model file.
 */
class LDObject : public QObject
{
	Q_OBJECT

public:
	LDObject();

	virtual QString asText() const = 0; // This object as LDraw code
	LDColor color() const;
	virtual LDColor defaultColor() const; // What color does the object default to?
	LDPolygon getPolygon();
	virtual void getVertices (DocumentManager *context, QSet<Vertex>& verts) const;
	virtual bool hasMatrix() const; // Does this object have a matrix and position? (see LDMatrixObject)
	virtual bool isColored() const;
	bool isHidden() const;
	virtual bool isRasterizable() const { return false; } // Can this object be rasterized?
	virtual bool isScemantic() const; // Does this object have meaning in the part model?
	void move (const QVector3D& vector);
	virtual int numVertices() const;
	virtual int numPolygonVertices() const;
	virtual QString objectListText() const;
	QColor randomColor() const;
	virtual void rasterize(DocumentManager*, Winding, Model&, bool, bool) {}
	virtual QVector<LDPolygon> rasterizePolygons(DocumentManager*, Winding);
	void setColor (LDColor color);
	void setHidden (bool value);
	void setVertex (int i, const Vertex& vert);
	bool isInverted() const;
	void setInverted(bool value);
	virtual int triangleCount(DocumentManager* context) const;
	virtual LDObjectType type() const = 0;
	virtual QString iconName() const = 0;
	const Vertex& vertex (int i) const;
	virtual void serialize(class Serializer& serializer);
	void restore(LDObjectState& archive);

	static LDObject* newFromType(LDObjectType type);

signals:
	void modified(const LDObjectState& before, const LDObjectState& after);

protected:
	virtual Winding nativeWinding(DocumentManager*) const;
	template<typename T>
	void changeProperty(T* property, const T& value);

private:
	bool m_hasInvertNext = false;
	bool m_isHidden;
	LDColor m_color = LDColor::nullColor;
	QColor m_randomColor;
	Vertex m_coords[4];
};

Q_DECLARE_METATYPE(LDObject*)

/*
 * Base class for objects with matrices.
 */
class LDMatrixObject : public LDObject
{
public:
	LDMatrixObject() = default;
	LDMatrixObject(const QMatrix4x4& matrix);

	bool hasMatrix() const override { return true; }
	Vertex position() const;
	void setTransformationMatrix(const QMatrix4x4& newMatrix);
	const QMatrix4x4& transformationMatrix() const;
	void translate(const QVector3D& offset);
	void serialize(class Serializer& serializer) override;

protected:
	bool shouldInvert(Winding winding, DocumentManager* context);

private:
	QMatrix4x4 m_transformationMatrix;
};

/*
 * Represents a line in the LDraw file that could not be properly parsed.
 */
class LDError : public LDObject
{
public:
	LDError() = default;
	LDError(QString contents, QString reason);

	virtual LDObjectType type() const override
	{
		return LDObjectType::Error;
	}

	virtual QString asText() const override;
	QString reason() const;
	QString contents() const;
	QString objectListText() const override;
	bool isColored() const override { return false; }
	QString iconName() const override { return "error"; }
	void serialize(class Serializer& serializer) override;

private:
	QString m_contents; // The LDraw code that was being parsed
	QString m_reason;
};

/*
 * Represents a single code-1 subfile reference.
 */
class LDSubfileReference : public LDMatrixObject
{
public:
	static const LDObjectType SubclassType = LDObjectType::SubfileReference;

	LDSubfileReference() = default;
	LDSubfileReference(QString referenceName, const QMatrix4x4& matrix = {});

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	LDDocument* fileInfo(DocumentManager *context) const;
	bool isRasterizable() const override { return true; }
	virtual void getVertices(DocumentManager *context, QSet<Vertex>& verts) const override;
	void rasterize(
		DocumentManager* context,
		Winding parentWinding,
		Model& model,
		bool deep,
		bool render
	) override;
	QVector<LDPolygon> rasterizePolygons(DocumentManager* context, Winding parentWinding) override;
	QString objectListText() const override;
	QString referenceName() const;
	int triangleCount(DocumentManager *context) const override;
	QString iconName() const override { return "subfilereference"; }
	void serialize(class Serializer& serializer) override;
	void setReferenceName(const QString& newReferenceName);

protected:
	Winding nativeWinding(DocumentManager* context) const override;

private:
	QString m_referenceName;
};

/*
 * Models a Bézier curve. It is stored as a special comment in the LDraw code file and can be inlined down into line segments.
 */
class LDBezierCurve : public LDObject
{
public:
	static const LDObjectType SubclassType = LDObjectType::BezierCurve;

	LDBezierCurve() = default;
	LDBezierCurve(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3);

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	Vertex pointAt (qreal t) const;
	void rasterize(Model& model, int segments);
	QVector<LDPolygon> rasterizePolygons (int segments);
	int numVertices() const override { return 4; }
	LDColor defaultColor() const override { return EdgeColor; }
	QString iconName() const override { return "beziercurve"; }
};

enum
{
	LowResolution = 8,
	MediumResolution = 16,
	HighResolution = 48
};

/*
 * Changes a property in a manner that emits the appropriate signal to notify that the object changed.
 */
template<typename T>
void LDObject::changeProperty(T* property, const T& value)
{
	if (*property != value)
	{
		Serializer::Archive before = Serializer::store(this);
		*property = value;
		emit modified(before, Serializer::store(this));
	}
}
