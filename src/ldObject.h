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
#include "main.h"
#include "basics.h"
#include "glShared.h"
#include "colors.h"

class Model;

#define LDOBJ(T)												\
public:															\
	static constexpr LDObjectType SubclassType = OBJ_##T;		\
																\
	virtual LDObjectType type() const override					\
	{															\
		return OBJ_##T;											\
	}															\
																\
	virtual QString asText() const override;					\
	virtual void invert() override;								\
protected:														\
	friend class Model;											\
	LD##T (Model* model = nullptr);								\

#define LDOBJ_NAME(N)          public: virtual QString typeName() const override { return #N; }
#define LDOBJ_VERTICES(V)      public: virtual int numVertices() const override { return V; }
#define LDOBJ_SETCOLORED(V)    public: virtual bool isColored() const override { return V; }
#define LDOBJ_COLORED          LDOBJ_SETCOLORED (true)
#define LDOBJ_UNCOLORED        LDOBJ_SETCOLORED (false) LDOBJ_DEFAULTCOLOR (MainColor)
#define LDOBJ_DEFAULTCOLOR(V)  public: virtual LDColor defaultColor() const override { return (V); }

#define LDOBJ_CUSTOM_SCEMANTIC public: virtual bool isScemantic() const override
#define LDOBJ_SCEMANTIC        LDOBJ_CUSTOM_SCEMANTIC { return true; }
#define LDOBJ_NON_SCEMANTIC    LDOBJ_CUSTOM_SCEMANTIC { return false; }

#define LDOBJ_SETMATRIX(V)     public: virtual bool hasMatrix() const override { return V; }
#define LDOBJ_HAS_MATRIX       LDOBJ_SETMATRIX (true)
#define LDOBJ_NO_MATRIX        LDOBJ_SETMATRIX (false)

class QListWidgetItem;
class LDSubfileReference;
class LDDocument;

class LDBfc;

//
// Object type codes.
//
enum LDObjectType
{
	OBJ_SubfileReference,	//	Object represents a	sub-file reference
	OBJ_Quad,				//	Object represents a	quadrilateral
	OBJ_Triangle,			//	Object represents a	triangle
	OBJ_Line,				//	Object represents a	line
	OBJ_CondLine,			//	Object represents a	conditional line
	OBJ_Bfc,				//	Object represents a	BFC statement
	OBJ_Overlay,			//	Object contains meta-info about an overlay image.
	OBJ_Comment,			//	Object represents a	comment
	OBJ_Error,				//	Object is the result of failed parsing
	OBJ_Empty,				//	Object represents an empty line
	OBJ_BezierCurve,		//	Object represents a Bézier curve

	OBJ_NumTypes,			// Amount of object types
	OBJ_FirstType = OBJ_SubfileReference
};

MAKE_ITERABLE_ENUM (LDObjectType, OBJ_SubfileReference, OBJ_BezierCurve)

//
// LDObject
//
// Base class object for all object types. Each LDObject represents a single line
// in the LDraw code file. The virtual method getType returns an enumerator
// which is a token of the object's type. The object can be casted into
// sub-classes based on this enumerator.
//
class LDObject : public QObject
{
    Q_OBJECT

public:
	virtual QString asText() const = 0; // This object as LDraw code
    LDColor color() const;
	virtual LDColor defaultColor() const = 0; // What color does the object default to?
	Model* model() const;
	LDPolygon* getPolygon();
	virtual void getVertices (QSet<Vertex>& verts) const;
	virtual bool hasMatrix() const = 0; // Does this object have a matrix and position? (see LDMatrixObject)
	qint32 id() const;
	virtual void invert() = 0; // Inverts this object (winding is reversed)
	virtual bool isColored() const = 0;
	bool isHidden() const;
	virtual bool isScemantic() const = 0; // Does this object have meaning in the part model?
	bool isSelected() const;
	int lineNumber() const;
	void move (Vertex vect);
	LDObject* next() const;
	virtual int numVertices() const = 0;
	virtual QString objectListText() const;
	LDObject* previous() const;
	bool previousIsInvertnext (LDBfc*& ptr);
	QColor randomColor() const;
	void setColor (LDColor color);
	void setHidden (bool value);
	void setVertex (int i, const Vertex& vert);
	void swap (LDObject* other);
	virtual int triangleCount() const;
	virtual LDObjectType type() const = 0;
	virtual QString typeName() const = 0;
	const Vertex& vertex (int i) const;

	static LDObject* fromID(int32 id);

signals:
	void codeChanged(int position, QString before, QString after);

protected:
	friend class Model;
	LDObject (Model* model = nullptr);
	virtual ~LDObject();
	void setDocument(Model* model);

private:
	bool m_isHidden;
	bool m_isSelected;
	Model* _model;
	qint32 m_id;
	LDColor m_color;
	QColor m_randomColor;
	Vertex m_coords[4];
};

//
// Common code for objects with matrices. This class is multiple-derived in
// and thus not used directly other than as a common storage point for matrices
// and vertices.
//
// In 0.1-alpha, there was a separate 'radial' type which had a position and
// matrix as well. Even though right now only LDSubfile uses this, I'm keeping
// this class distinct in case I get new extension ideas. :)
//
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

//
//
// Represents a line in the LDraw file that could not be properly parsed. It is
// represented by a (!) ERROR in the code view. It exists for the purpose of
// allowing garbage lines be debugged and corrected within LDForge.
//
class LDError : public LDObject
{
	LDOBJ (Error)
	LDOBJ_NAME (error)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	QString reason() const;
	QString contents() const;
	QString fileReferenced() const;
	void setFileReferenced (QString value);
	QString objectListText() const override;

protected:
	LDError (QString contents, QString reason, Model* model = nullptr);

private:
	QString m_fileReferenced; // If this error was caused by inability to open a file, what file was that?
	QString m_contents; // The LDraw code that was being parsed
	QString m_reason;
};

//
//
// Represents an empty line in the LDraw code file.
//
class LDEmpty : public LDObject
{
	LDOBJ (Empty)
	LDOBJ_NAME (empty)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	QString objectListText() const override;
};

//
//
// Represents a code-0 comment in the LDraw code file.
//
class LDComment : public LDObject
{
	LDOBJ (Comment)
	LDOBJ_NAME (comment)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	QString objectListText() const override;
	QString text() const;
	void setText (QString value);

protected:
	LDComment (QString text, Model* model = nullptr);

private:
	QString m_text;
};

//
//
// Represents a 0 BFC statement in the LDraw code.
//
enum BfcStatement
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
};

MAKE_ITERABLE_ENUM(BfcStatement, CertifyCCW, NoClip)

class LDBfc : public LDObject
{
public:
	LDOBJ (Bfc)
	LDOBJ_NAME (bfc)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_CUSTOM_SCEMANTIC { return (statement() == BfcStatement::InvertNext); }
	LDOBJ_NO_MATRIX

public:
    QString objectListText() const override;
	BfcStatement statement() const;
	void setStatement (BfcStatement value);
	QString statementToString() const;

	static QString statementToString (BfcStatement statement);

protected:
	LDBfc (const BfcStatement type, Model* model = nullptr);

private:
	BfcStatement m_statement;
};

//
// LDReference
//
// Represents a single code-1 subfile reference.
//
class LDSubfileReference : public LDMatrixObject
{
	LDOBJ (SubfileReference)
	LDOBJ_NAME (subfilereference)
	LDOBJ_VERTICES (0)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (MainColor)
	LDOBJ_SCEMANTIC
	LDOBJ_HAS_MATRIX

public:
	// Inlines this subfile.
	LDDocument* fileInfo() const;
	virtual void getVertices (QSet<Vertex>& verts) const override;
	void inlineContents(Model& model, bool deep, bool render);
	QList<LDPolygon> inlinePolygons();
	QString objectListText() const override;
	void setFileInfo (LDDocument* fileInfo);
	int triangleCount() const override;

protected:
	LDSubfileReference(LDDocument* reference, const Matrix& transformationMatrix, const Vertex& position, Model* model = nullptr);

private:
	LDDocument* m_fileInfo;
};

//
// LDLine
//
// Represents a single code-2 line in the LDraw code file.
//
class LDLine : public LDObject
{
	LDOBJ (Line)
	LDOBJ_NAME (line)
	LDOBJ_VERTICES (2)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (EdgeColor)
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

protected:
	LDLine (Vertex v1, Vertex v2, Model* model = nullptr);
};

//
// LDCondLine
//
// Represents a single code-5 conditional line.
//
class LDCondLine : public LDLine
{
	LDOBJ (CondLine)
	LDOBJ_NAME (condline)
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (EdgeColor)
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	LDLine* becomeEdgeLine();

protected:
	LDCondLine (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, Model* model = nullptr);
};

//
// LDTriangle
//
// Represents a single code-3 triangle in the LDraw code file. Vertices v0, v1
// and v2 contain the end-points of this triangle. dColor is the color the
// triangle is colored with.
//
class LDTriangle : public LDObject
{
	LDOBJ (Triangle)
	LDOBJ_NAME (triangle)
	LDOBJ_VERTICES (3)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (MainColor)
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	int triangleCount() const override;

protected:
	LDTriangle (Vertex const& v1, Vertex const& v2, Vertex const& v3, Model* model = nullptr);
};

//
// LDQuad
//
// Represents a single code-4 quadrilateral. v0, v1, v2 and v3 are the end points
// of the quad, dColor is the color used for the quad.
//
class LDQuad : public LDObject
{
	LDOBJ (Quad)
	LDOBJ_NAME (quad)
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (MainColor)
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	int triangleCount() const override;

protected:
	LDQuad (const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4, Model* model = nullptr);
};

//
// LDOverlay
//
// Overlay image meta, stored in the header of parts so as to preserve overlay information.
//
class LDOverlay : public LDObject
{
	LDOBJ (Overlay)
	LDOBJ_NAME (overlay)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	int camera() const;
	QString fileName() const;
	int height() const;
	QString objectListText() const override;
	void setCamera (int value);
	void setFileName (QString value);
	void setHeight (int value);
	void setWidth (int value);
	void setX (int value);
	void setY (int value);
	int width() const;
	int x() const;
	int y() const;

private:
	int m_camera;
	int m_x;
	int m_y;
	int m_width;
	int m_height;
	QString m_fileName;
};

class LDBezierCurve : public LDObject
{
	LDOBJ (BezierCurve)
	LDOBJ_NAME (beziercurve)
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (EdgeColor)
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	Vertex pointAt (qreal t) const;
	void rasterize(Model& model, int segments);
	QVector<LDPolygon> rasterizePolygons (int segments);

protected:
	LDBezierCurve (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, Model* model = nullptr);
};

enum
{
	LowResolution = 16,
	HighResolution = 48
};
