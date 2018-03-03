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

#pragma once
#include <type_traits>
#include "../main.h"
#include "../basics.h"
#include "../glShared.h"
#include "../colors.h"

class Model;
class LDDocument;

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
	Bfc,				//	Object represents a	BFC statement
	Comment,			//	Object represents a	comment
	Error,				//	Object is the result of failed parsing
	Empty,				//	Object represents an empty line
	BezierCurve,		//	Object represents a Bézier curve
	_End
};

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
	virtual QString asText() const = 0; // This object as LDraw code
	LDColor color() const;
	virtual LDColor defaultColor() const; // What color does the object default to?
	Model* model() const;
	LDPolygon* getPolygon();
	virtual void getVertices (QSet<Vertex>& verts) const;
	virtual bool hasMatrix() const; // Does this object have a matrix and position? (see LDMatrixObject)
	qint32 id() const;
	virtual void invert(); // Inverts this object (winding is reversed)
	virtual bool isColored() const;
	bool isHidden() const;
	virtual bool isScemantic() const; // Does this object have meaning in the part model?
	bool isSelected() const;
	void move (Vertex vect);
	LDObject* next();
	virtual int numVertices() const;
	virtual int numPolygonVertices() const;
	virtual QString objectListText() const;
	QColor randomColor() const;
	void setColor (LDColor color);
	void setHidden (bool value);
	void setVertex (int i, const Vertex& vert);
	void swap (LDObject* other);
	bool isInverted() const;
	void setInverted(bool value);
	virtual int triangleCount() const;
	virtual LDObjectType type() const = 0;
	virtual QString typeName() const = 0;
	const Vertex& vertex (int i) const;

	static LDObject* fromID(qint32 id);

signals:
	void codeChanged(QString before, QString after);

protected:
	friend class Model;
	LDObject (Model* model = nullptr);
	virtual ~LDObject();
	void setDocument(Model* model);

	template<typename T>
	void changeProperty(T* property, const T& value);

private:
	bool m_hasInvertNext = false;
	bool m_isHidden;
	bool m_isSelected;
	Model* _model;
	qint32 m_id;
	LDColor m_color;
	QColor m_randomColor;
	Vertex m_coords[4];
};

Q_DECLARE_METATYPE(LDObject*)

/*
 * Base class for objects with matrices.
 */
class LDMatrixObject : public LDObject
{
	Vertex m_position;

public:
	const Vertex& position() const;
	void setCoordinate (const Axis ax, double value);
	void setPosition (const Vertex& a);
	void setTransformationMatrix (const Matrix& value);
	const Matrix& transformationMatrix() const;

protected:
	LDMatrixObject (Model* model = nullptr);
	LDMatrixObject (const Matrix& transformationMatrix, const Vertex& pos, Model* model = nullptr);

private:
	Matrix m_transformationMatrix;
};

/*
 * Represents a line in the LDraw file that could not be properly parsed.
 */
class LDError : public LDObject
{
public:
	static constexpr LDObjectType SubclassType = LDObjectType::Error;

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	virtual void invert() override;
	QString reason() const;
	QString contents() const;
	QString fileReferenced() const;
	void setFileReferenced (QString value);
	QString objectListText() const override;
	bool isColored() const override { return false; }
	QString typeName() const override { return "error"; }

protected:
	friend class Model;
	LDError (Model* model);
	LDError (QString contents, QString reason, Model* model = nullptr);

private:
	QString m_fileReferenced; // If this error was caused by inability to open a file, what file was that?
	QString m_contents; // The LDraw code that was being parsed
	QString m_reason;
};

/*
 * Represents a 0 BFC statement in the LDraw code.
 */
enum class BfcStatement
{
	CertifyCCW,
	CCW,
	CertifyCW,
	CW,
	NoCertify,
	InvertNext,
	Clip,
	ClipCCW,
	ClipCW,
	NoClip,
	_End
};

MAKE_ITERABLE_ENUM(BfcStatement)

class LDBfc : public LDObject
{
	public:
	static constexpr LDObjectType SubclassType = LDObjectType::Bfc;

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	virtual void invert() override;
protected:
	friend class Model;
	LDBfc (Model* model);

public:
	bool isScemantic() const override { return statement() == BfcStatement::InvertNext; }
    QString objectListText() const override;
	BfcStatement statement() const;
	void setStatement (BfcStatement value);
	QString statementToString() const;
	bool isColored() const override { return false; }
	QString typeName() const override { return "bfc"; }

	static QString statementToString (BfcStatement statement);

protected:
	LDBfc (const BfcStatement type, Model* model = nullptr);

private:
	BfcStatement m_statement;
};

/*
 * Represents a single code-1 subfile reference.
 */
class LDSubfileReference : public LDMatrixObject
{
public:
	static constexpr LDObjectType SubclassType = LDObjectType::SubfileReference;

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	virtual void invert() override;
	LDDocument* fileInfo() const;
	virtual void getVertices (QSet<Vertex>& verts) const override;
	void inlineContents(Model& model, bool deep, bool render);
	QList<LDPolygon> inlinePolygons();
	QString objectListText() const override;
	void setFileInfo (LDDocument* fileInfo);
	int triangleCount() const override;
	bool hasMatrix() const override { return true; }
	QString typeName() const override { return "subfilereference"; }

protected:
	friend class Model;
	LDSubfileReference (Model* model);
	LDSubfileReference(LDDocument* reference, const Matrix& transformationMatrix, const Vertex& position, Model* model = nullptr);

private:
	LDDocument* m_fileInfo;
};

/*
 * Models a Bézier curve. It is stored as a special comment in the LDraw code file and can be inlined down into line segments.
 */
class LDBezierCurve : public LDObject
{
public:
	static constexpr LDObjectType SubclassType = LDObjectType::BezierCurve;

	virtual LDObjectType type() const override
	{
		return SubclassType;
	}

	virtual QString asText() const override;
	virtual void invert() override;
	Vertex pointAt (qreal t) const;
	void rasterize(Model& model, int segments);
	QVector<LDPolygon> rasterizePolygons (int segments);
	int numVertices() const override { return 4; }
	LDColor defaultColor() const override { return EdgeColor; }
	QString typeName() const override { return "beziercurve"; }

protected:
	friend class Model;
	LDBezierCurve (Model* model);
	LDBezierCurve (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, Model* model = nullptr);
};

enum
{
	LowResolution = 16,
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
		QString before = asText();
		*property = value;
		emit codeChanged(before, asText());
	}
}
