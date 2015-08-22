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

class LDBFC;

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
	OBJ_Vertex,			//	Object is a	vertex,	LDForge extension object
	OBJ_BFC,			//	Object represents a	BFC statement
	OBJ_Overlay,		//	Object contains meta-info about an overlay image.
	OBJ_Comment,		//	Object represents a	comment
	OBJ_Error,			//	Object is the result of failed parsing
	OBJ_Empty,			//	Object represents an empty line

	OBJ_NumTypes,       // Amount of object types
	OBJ_FirstType = OBJ_Subfile
};

NUMERIC_ENUM_OPERATORS (LDObjectType)

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
	PROPERTY (public,		bool,				isHidden,		setHidden,		STOCK_WRITE)
	PROPERTY (public,		bool,				isSelected,		setSelected,	STOCK_WRITE)
	PROPERTY (public,		LDObject*,			parent,			setParent,		STOCK_WRITE)
	PROPERTY (public,		LDDocument*,		document,		setDocument,	CUSTOM_WRITE)
	PROPERTY (private,		int32,				id,				setID,			STOCK_WRITE)
	PROPERTY (public,		LDColor,			color,			setColor,		CUSTOM_WRITE)
	PROPERTY (private,		QColor,				randomColor,	setRandomColor,	STOCK_WRITE)

public:
	LDObject (LDDocument* document = nullptr);
	virtual ~LDObject();

	// This object as LDraw code
	virtual QString				asText() const = 0;

	// Makes a copy of this object
	LDObject*					createCopy() const;

	// What color does the object default to?
	virtual LDColor				defaultColor() const = 0;

	// Deletes this object
	void						destroy();

	// Removes this object from selection
	void						deselect();

	virtual void				getVertices (QVector<Vertex>& verts) const;

	// Does this object have a matrix and position? (see LDMatrixObject)
	virtual bool				hasMatrix() const = 0;

	// Inverts this object (winding is reversed)
	virtual void				invert() = 0;

	// Is this object colored?
	virtual bool				isColored() const = 0;

	// Does this object have meaning in the part model?
	virtual bool				isScemantic() const = 0;

	// Index (i.e. line number) of this object
	long						lineNumber() const;

	// Moves this object using the given vertex as a movement List
	void						move (Vertex vect);

	// Object after this in the current file
	LDObject*					next() const;

	// Number of vertices this object has
	virtual int					numVertices() const = 0;

	// Object prior to this in the current file
	LDObject*					previous() const;

	// Is the previous object INVERTNEXT?
	bool						previousIsInvertnext (LDBFC*& ptr);

	// Replace this LDObject with another LDObject. Object is deleted in the process.
	void						replace (LDObject* other);

	// Selects this object.
	void						select();

	// Set a vertex to the given value
	void						setVertex (int i, const Vertex& vert);

	// Set a single coordinate of a vertex
	void						setVertexCoord (int i, Axis ax, double value);

	// Swap this object with another.
	void						swap (LDObject* other);

	// What object in the current file ultimately references this?
	LDObject*					topLevelParent();

	// Type enumerator of this object
	virtual LDObjectType		type() const = 0;

	// Type name of this object
	virtual QString				typeName() const = 0;

	// Get a vertex by index
	const Vertex&				vertex (int i) const;

	// Get type name by enumerator
	static QString typeName (LDObjectType type);

	// Returns a default-constructed LDObject by the given type
	static LDObject* getDefault (const LDObjectType type);

	// TODO: move this to LDDocument?
	static void moveObjects (LDObjectList objs, const bool up);

	// Get a description of a list of LDObjects
	static QString describeObjects (const LDObjectList& objs);
	static LDObject* fromID (int id);
	LDPolygon* getPolygon();

	// TODO: make this private!
	QListWidgetItem* qObjListEntry;

private:
	Vertex m_coords[4];

	void chooseID();
};

//
// Makes a new LDObject. This makes the shared pointer always use the custom
// deleter so that all deletions go through finalDelete();
//
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
	PROPERTY (public,	Matrix,				transform,		setTransform,	CUSTOM_WRITE)
	Vertex m_position;

public:
	LDMatrixObject (LDDocument* document = nullptr) :
		LDObject (document),
		m_position (Origin) {}

	LDMatrixObject (const Matrix& transform, const Vertex& pos, LDDocument* document = nullptr) :
		LDObject (document),
		m_transform (transform),
		m_position (pos) {}

	inline const Vertex& position() const
	{
		return m_position;
	}

	void setCoordinate (const Axis ax, double value)
	{
		Vertex v = position();

		switch (ax)
		{
			case X: v.setX (value); break;
			case Y: v.setY (value); break;
			case Z: v.setZ (value); break;
		}

		setPosition (v);
	}

	void setPosition (const Vertex& a);
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
	PROPERTY (public,	QString,	fileReferenced, setFileReferenced,	STOCK_WRITE)
	PROPERTY (private,	QString,	contents,		setContents,		STOCK_WRITE)
	PROPERTY (private,	QString,	reason,			setReason,			STOCK_WRITE)

public:
	LDError (QString contents, QString reason, LDDocument* document = nullptr) :
		LDObject (document),
		m_contents (contents),
		m_reason (reason) {}
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
	PROPERTY (public, QString, text, setText, STOCK_WRITE)
	LDOBJ (Comment)
	LDOBJ_NAME (comment)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	LDComment (QString text, LDDocument* document = nullptr) :
		LDObject (document),
		m_text (text) {}
};

//
//
// Represents a 0 BFC statement in the LDraw code.
//
enum class BFCStatement
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

class LDBFC : public LDObject
{
public:
	LDOBJ (BFC)
	LDOBJ_NAME (bfc)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_CUSTOM_SCEMANTIC { return (statement() == BFCStatement::InvertNext); }
	LDOBJ_NO_MATRIX
	PROPERTY (public, BFCStatement, statement, setStatement, STOCK_WRITE)

public:
	LDBFC (const BFCStatement type, LDDocument* document = nullptr) :
		LDObject (document),
		m_statement (type) {}

	// Statement strings
	static const char* StatementStrings[];
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
	PROPERTY (public, LDDocument*, fileInfo, setFileInfo, CUSTOM_WRITE)

public:
	enum InlineFlag
	{
		DeepInline     = (1 << 0),
		CacheInline    = (1 << 1),
		RendererInline = (1 << 2),
		DeepCacheInline = (DeepInline | CacheInline),
	};

	Q_DECLARE_FLAGS (InlineFlags, InlineFlag)

	// Inlines this subfile.
	LDObjectList inlineContents (bool deep, bool render);
	QList<LDPolygon> inlinePolygons();
	virtual void getVertices (QVector<Vertex>& verts) const override;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (LDSubfile::InlineFlags)

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
// LDVertex
//
// The vertex is an LDForce-specific extension which represents a single
// vertex which can be used as a parameter to tools or to store coordinates
// with. Vertices are a part authoring tool and they should not appear in
// finished parts.
//
class LDVertex : public LDObject
{
	LDOBJ (Vertex)
	LDOBJ_NAME (vertex)
	LDOBJ_VERTICES (0) // TODO: move pos to m_vertices[0]
	LDOBJ_COLORED
	LDOBJ_DEFAULTCOLOR (MainColor)
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

public:
	Vertex pos;
	virtual void getVertices (QVector<Vertex>& verts) const override;
};

//
// LDOverlay
//
// Overlay image meta, stored in the header of parts so as to preserve overlay
// information.
//
class LDOverlay : public LDObject
{
	LDOBJ (Overlay)
	LDOBJ_NAME (overlay)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
	PROPERTY (public,	int,		camera,		setCamera,		STOCK_WRITE)
	PROPERTY (public,	int,		x,			setX,			STOCK_WRITE)
	PROPERTY (public,	int,		y,			setY,			STOCK_WRITE)
	PROPERTY (public,	int,		width,		setWidth,		STOCK_WRITE)
	PROPERTY (public,	int,		height,		setHeight,		STOCK_WRITE)
	PROPERTY (public,	QString,	fileName,	setFileName,	STOCK_WRITE)
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

template<typename T>
inline void DynamicExecute (LDObject* obj, std::function<void (T*)> func)
{
	static_assert (std::is_base_of<LDObject, T>::value,
		"DynamicExecute may only be used with LDObject-derivatives");

	if (obj->type() == T::SubclassType)
		func (static_cast<T*> (obj));
}

struct LDIterationBreakage {};

template<typename T>
inline void LDIterate (LDObjectList const& objs,
	std::function<void (T*)> func)
{
	static_assert (std::is_base_of<LDObject, T>::value,
		"LDIterate may only be used with LDObject-derivatives");

	try
	{
		for (LDObject* const& obj : objs)
			DynamicExecute<T> (obj, func);
	}
	catch (LDIterationBreakage) {}
}

inline void Break()
{
	throw LDIterationBreakage();
}
