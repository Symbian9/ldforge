/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

#define LDOBJ(T)												\
public:															\
	static constexpr LDObjectType SubclassType = OBJ_##T;		\
	LD##T (LDDocument* document = nullptr);						\
																\
	virtual LDObjectType type() const override					\
	{															\
		return OBJ_##T;											\
	}															\
																\
	virtual QString asText() const override;					\
	virtual void invert() override;								\

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
class LDSubfile;
class LDDocument;

class LDBfc;

//
// Object type codes.
//
enum LDObjectType
{
	OBJ_Subfile,		//	Object represents a	sub-file reference
	OBJ_Quad,			//	Object represents a	quadrilateral
	OBJ_Triangle,		//	Object represents a	triangle
	OBJ_Line,			//	Object represents a	line
	OBJ_CondLine,		//	Object represents a	conditional line
	OBJ_Bfc,			//	Object represents a	BFC statement
	OBJ_Overlay,		//	Object contains meta-info about an overlay image.
	OBJ_Comment,		//	Object represents a	comment
	OBJ_Error,			//	Object is the result of failed parsing
	OBJ_Empty,			//	Object represents an empty line
	OBJ_BezierCurve,	//	Object represents a Bézier curve

	OBJ_NumTypes,       // Amount of object types
	OBJ_FirstType = OBJ_Subfile
};

MAKE_ITERABLE_ENUM (LDObjectType)

//
// LDObject
//
// Base class object for all object types. Each LDObject represents a single line
// in the LDraw code file. The virtual method getType returns an enumerator
// which is a token of the object's type. The object can be casted into
// sub-classes based on this enumerator.
//
class LDObject
{
public:
	LDObject (LDDocument* document = nullptr);

	virtual QString asText() const = 0; // This object as LDraw code
	LDColor color() const;
	LDObject* createCopy() const;
	virtual LDColor defaultColor() const = 0; // What color does the object default to?
	void deselect(); 
	void destroy();
	LDDocument* document() const;
	LDPolygon* getPolygon();
	virtual void getVertices (QVector<Vertex>& verts) const;
	virtual bool hasMatrix() const = 0; // Does this object have a matrix and position? (see LDMatrixObject)
	qint32 id() const;
	virtual void invert() = 0; // Inverts this object (winding is reversed)
	virtual bool isColored() const = 0;
	bool isDestroyed() const;
	bool isHidden() const;
	virtual bool isScemantic() const = 0; // Does this object have meaning in the part model?
	bool isSelected() const;
	int lineNumber() const;
	void move (Vertex vect);
	LDObject* next() const;
	virtual int numVertices() const = 0;
	LDObject* previous() const;
	bool previousIsInvertnext (LDBfc*& ptr);
	QColor randomColor() const;
	void replace (LDObject* other);
	void select();
	void setColor (LDColor color);
	void setDocument (LDDocument* document);
	void setHidden (bool value);
	void setVertex (int i, const Vertex& vert);
	void swap (LDObject* other);
	LDObject* topLevelParent();
	virtual LDObjectType type() const = 0;
	virtual QString typeName() const = 0;
	const Vertex& vertex (int i) const;

	static QString describeObjects (const LDObjectList& objs);
	static LDObject* fromID (int id);
	static LDObject* getDefault (const LDObjectType type);
	static void moveObjects (LDObjectList objs, const bool up); // TODO: move this to LDDocument?
	static QString typeName (LDObjectType type);

protected:
	virtual ~LDObject();

private:
	bool m_isHidden;
	bool m_isSelected;
	bool m_isDestroyed;
	LDObject* m_parent;
	LDDocument* m_document;
	qint32 m_id;
	LDColor m_color;
	QColor m_randomColor;
	Vertex m_coords[4];
};

template<typename T, typename... Args>
T* LDSpawn (Args... args)
{
	static_assert (std::is_base_of<LDObject, T>::value,
		"spawn may only be used with LDObject-derivatives");
	T* result = new T (args..., nullptr);

	// Set default color. Relying on virtual functions, this cannot be done in the c-tor.
	// TODO: store -1 as the default color
	if (result->isColored())
		result->setColor (result->defaultColor());

	return result;
}

//
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
	LDMatrixObject (LDDocument* document = nullptr);
	LDMatrixObject (const Matrix& transform, const Vertex& pos, LDDocument* document = nullptr);

	const Vertex& position() const;
	void setCoordinate (const Axis ax, double value);
	void setPosition (const Vertex& a);
	const Matrix& transform() const;
	void setTransform (const Matrix& value);

private:
	Matrix m_transform;
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
	LDError (QString contents, QString reason, LDDocument* document = nullptr);
	QString reason() const;
	QString contents() const;
	QString fileReferenced() const;
	void setFileReferenced (QString value);

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
	LDComment (QString text, LDDocument* document = nullptr);
	QString text() const;
	void setText (QString value);

private:
	QString m_text;
};

//
//
// Represents a 0 BFC statement in the LDraw code.
//
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

	NumValues,
	FirstValue = CertifyCCW
};

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
	LDBfc (const BfcStatement type, LDDocument* document = nullptr);

	BfcStatement statement() const;
	void setStatement (BfcStatement value);
	QString statementToString() const;

	static QString statementToString (BfcStatement statement);

private:
	BfcStatement m_statement;
};

//
// LDSubfile
//
// Represents a single code-1 subfile reference.
//
class LDSubfile : public LDMatrixObject
{
	LDOBJ (Subfile)
	LDOBJ_NAME (subfile)
	LDOBJ_VERTICES (0)
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (MainColor)
	LDOBJ_SCEMANTIC
	LDOBJ_HAS_MATRIX

public:
	// Inlines this subfile.
	LDDocument* fileInfo() const;
	virtual void getVertices (QVector<Vertex>& verts) const override;
	LDObjectList inlineContents (bool deep, bool render);
	QList<LDPolygon> inlinePolygons();
	void setFileInfo (LDDocument* fileInfo);

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

public:
	LDLine (Vertex v1, Vertex v2, LDDocument* document = nullptr);
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
	LDCondLine (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3, LDDocument* document = nullptr);
	LDLine* toEdgeLine();
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
	LDTriangle (Vertex const& v1, Vertex const& v2, Vertex const& v3, LDDocument* document = nullptr);
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
	LDQuad (const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4, LDDocument* document = nullptr);

	// Split this quad into two triangles
	QList<LDTriangle*> splitToTriangles();
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
	LDBezierCurve (const Vertex& v0, const Vertex& v1,
		const Vertex& v2, const Vertex& v3, LDDocument* document = nullptr);
	LDLine* toEdgeLine();
};

// Other common LDraw stuff
static const QString CALicenseText ("!LICENSE Redistributable under CCAL version 2.0 : "
	"see CAreadme.txt");

enum
{
	LowResolution = 16,
	HighResolution = 48
};

QString PreferredLicenseText();