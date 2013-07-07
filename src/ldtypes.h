/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
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

#ifndef LDTYPES_H
#define LDTYPES_H

#include "common.h"
#include "types.h"

#define LDOBJ(T) \
	virtual ~LD##T () {} \
	virtual LDObject::Type getType () const { \
		return LDObject::T; \
	} \
	virtual str raw (); \
	virtual LD##T* clone () { \
		return new LD##T (*this); \
	} \
	virtual void move (vertex vVector); \
	virtual void invert ();

#define LDOBJ_NAME( N )        virtual str typeName() const { return #N; }
#define LDOBJ_VERTICES( V )    virtual short vertices() const { return V; }
#define LDOBJ_SETCOLORED( V )  virtual bool isColored() const { return V; }
#define LDOBJ_COLORED          LDOBJ_SETCOLORED( true )
#define LDOBJ_UNCOLORED        LDOBJ_SETCOLORED( false )

#define LDOBJ_CUSTOM_SCEMANTIC virtual bool isScemantic() const
#define LDOBJ_SCEMANTIC        LDOBJ_CUSTOM_SCEMANTIC { return true; }
#define LDOBJ_NON_SCEMANTIC    LDOBJ_CUSTOM_SCEMANTIC { return false; }

#define LDOBJ_SETMATRIX(V)     virtual bool hasMatrix() const { return V; }
#define LDOBJ_HAS_MATRIX       LDOBJ_SETMATRIX( true )
#define LDOBJ_NO_MATRIX        LDOBJ_SETMATRIX( false )

class QListWidgetItem;
class LDSubfile;

// =============================================================================
// LDMatrixObject
// 
// Common code for objects with matrices
// =============================================================================
class LDMatrixObject {
	DECLARE_PROPERTY (matrix, transform, setTransform)
	DECLARE_PROPERTY (vertex, position, setPosition)
	PROPERTY (LDObject*, linkPointer, setLinkPointer)
	
public:
	LDMatrixObject () {}
	LDMatrixObject (const matrix& transform, const vertex& pos) :
		PROP_NAME (transform) (transform), PROP_NAME (position) (pos) {}
	
	const double& setCoordinate (const Axis ax, double value) {
		vertex v = position ();
		v[ax] = value;
		setPosition (v);
		
		return position ()[ax];
	}
};

// =============================================================================
// LDObject
// 
// Base class object for all LD* types. Each LDObject represents a single line
// in the LDraw code file. The virtual method getType returns an enumerator
// which is a token of the object's type. The object can be casted into
// sub-classes based on this enumerator.
// =============================================================================
class LDObject {
	PROPERTY (bool, hidden, setHidden)
	PROPERTY (bool, selected, setSelected)
	PROPERTY (LDObject*, parent, setParent)
	READ_PROPERTY (int, id, setID)
	DECLARE_PROPERTY (short, color, setColor)
	
public:
	// Object type codes. Codes are sorted in order of significance.
	enum Type {
		Subfile,        // Object represents a sub-file reference
		Quad,           // Object represents a quadrilateral
		Triangle,       // Object represents a triangle
		Line,           // Object represents a line
		CondLine,       // Object represents a conditional line
		Vertex,         // Object is a vertex, LDForge extension object
		BFC,            // Object represents a BFC statement
		Overlay,        // Object contains meta-info about an overlay image.
		Comment,        // Object represents a comment
		Gibberish,      // Object is the result of failed parsing
		Empty,          // Object represents an empty line
		Unidentified,   // Object is an uninitialized (SHOULD NEVER HAPPEN)
		NumTypes        // Amount of object types
	};
	
	LDObject ();
	virtual ~LDObject ();
	
	// Index (i.e. line number) of this object
	long getIndex (LDOpenFile* pFile) const;
	
	// OpenGL list for this object
	uint glLists[4];
	
	// Type enumerator of this object
	virtual LDObject::Type getType () const {
		return LDObject::Unidentified;
	};
	
	// A string that represents this line
	virtual str raw () {
		return "";
	}
	
	// Creates a new LDObject identical to this one and returns a pointer to it.
	virtual LDObject* clone () {
		return new LDObject (*this);
	}
	
	// Replace this LDObject with another LDObject. This method deletes the
	// object and any pointers to it become invalid.
	void replace (LDObject* replacement);
	
	// Swap this object with another.
	void swap (LDObject* other);
	
	// Moves this object using the given vertex as a movement vector
	virtual void move (vertex vect);
	
	// What object in the current file ultimately references this?
	LDObject* topLevelParent ();
	
	// Number of vertices this object has
	virtual short vertices () const { return 0; }
	
	// Is this object colored?
	virtual bool isColored () const { return false; }
	
	// Does this object have meaning in the part model?
	virtual bool isScemantic () const { return false; }
	
	// Type name of this object
	virtual str typeName() const { return "unknown"; }
	static str typeName( LDObject::Type type );
	
	// Returns a sample object by the given value
	static LDObject* getDefault (const LDObject::Type type);
	
	static void moveObjects (vector<LDObject*> objs, const bool bUp);
	static str objectListContents (const vector<LDObject*>& objs);
	static LDObject* fromID( int id );
	
	// Object list entry for this object
	QListWidgetItem* qObjListEntry;
	
	virtual bool        hasMatrix       () const { return false; }
	virtual void        invert          ();
	LDObject*           next            () const;
	LDObject*           prev            () const;
	void                setVertex       (int i, const vertex& vert);
	const vertex&       getVertex       (int i) const;
	void                setVertexCoord  (int i, const Axis ax, double value);

protected:
	bool m_glinit;
	friend class GLRenderer;
	
private:
	vertex m_coords[4];
};

// =============================================================================
// LDGibberish
//
// Represents a line in the LDraw file that could not be properly parsed. It is
// represented by a (!) ERROR in the code view. It exists for the purpose of
// allowing garbage lines be debugged and corrected within LDForge. The member
// zContent contains the contents of the unparsable line.
// =============================================================================
class LDGibberish : public LDObject {
public:
	LDOBJ (Gibberish)
	LDOBJ_NAME( error )
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDGibberish ();
	LDGibberish (str _zContent, str _zReason);
	
	// Content of this unknown line
	str contents;
	
	// Why is this gibberish?
	str reason;
};

// =============================================================================
// LDEmptyLine 
// 
// Represents an empty line in the LDraw code file.
// =============================================================================
class LDEmpty : public LDObject {
public:
	LDOBJ (Empty)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
};

// =============================================================================
// LDComment
//
// Represents a code-0 comment in the LDraw code file. Member zText contains
// the text of the comment.
// =============================================================================
class LDComment : public LDObject {
public:
	LDOBJ (Comment)
	LDOBJ_NAME( comment )
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDComment () {}
	LDComment (str text) : text (text) {}
	
	str text; // The text of this comment
};

// =============================================================================
// LDBFC
// 
// Represents a 0 BFC statement in the LDraw code. eStatement contains the type
// of this statement.
// =============================================================================
class LDBFC : public LDObject {
public:
	enum Type { CertifyCCW, CCW, CertifyCW, CW, NoCertify, InvertNext, NumStatements };
	
	LDOBJ (BFC)
	LDOBJ_NAME( bfc )
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_CUSTOM_SCEMANTIC { return (type == InvertNext); }
	LDOBJ_NO_MATRIX
	
	LDBFC () {}
	LDBFC (const LDBFC::Type type) : type (type) {}
	
	// Statement strings
	static const char* statements[];
	
	Type type;
};

// =============================================================================
// LDSubfile
//
// Represents a single code-1 subfile reference.
// =============================================================================
class LDSubfile : public LDObject, public LDMatrixObject {
	PROPERTY (LDOpenFile*, fileInfo, setFileInfo)
	
public:
	LDOBJ (Subfile)
	LDOBJ_NAME( subfile )
	LDOBJ_VERTICES (0)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_HAS_MATRIX
	
	LDSubfile () {
		setLinkPointer (this);
	}
	
	// Inlines this subfile. Note that return type is an array of heap-allocated
	// LDObject-clones, they must be deleted one way or another.
	vector<LDObject*> inlineContents (bool deep, bool cache);
};

// =============================================================================
// LDLine
//
// Represents a single code-2 line in the LDraw code file. v0 and v1 are the end
// points of the line. The line is colored with dColor unless uncolored mode is
// set.
// =============================================================================
class LDLine : public LDObject {
public:
	LDOBJ (Line)
	LDOBJ_NAME( line )
	LDOBJ_VERTICES (2)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDLine () {}
	LDLine (vertex v1, vertex v2);
};

// =============================================================================
// LDCondLine
//
// Represents a single code-5 conditional line. The end-points v0 and v1 are
// inherited from LDLine, c0 and c1 are the control points of this line.
// =============================================================================
class LDCondLine : public LDLine {
public:
	LDOBJ (CondLine)
	LDOBJ_NAME( condline )
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDCondLine () {}
	LDLine* demote ();
};

// =============================================================================
// LDTriangle
//
// Represents a single code-3 triangle in the LDraw code file. Vertices v0, v1
// and v2 contain the end-points of this triangle. dColor is the color the
// triangle is colored with.
// =============================================================================
class LDTriangle : public LDObject {
public:
	LDOBJ (Triangle)
	LDOBJ_NAME( triangle )
	LDOBJ_VERTICES (3)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDTriangle () {}
	LDTriangle (vertex v0, vertex v1, vertex v2) {
		setVertex (0, v0);
		setVertex (1, v1);
		setVertex (2, v2);
	}
};

// =============================================================================
// LDQuad
//
// Represents a single code-4 quadrilateral. v0, v1, v2 and v3 are the end points
// of the quad, dColor is the color used for the quad.
// =============================================================================
class LDQuad : public LDObject {
public:
	LDOBJ (Quad)
	LDOBJ_NAME( quad )
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDQuad () {}
	
	// Split this quad into two triangles (note: heap-allocated)
	vector<LDTriangle*> splitToTriangles ();
};

// =============================================================================
// LDVertex
// 
// The vertex is an LDForce-specific extension which represents a single
// vertex which can be used as a parameter to tools or to store coordinates
// with. Vertices are a part authoring tool and they should not appear in
// finished parts.
// =============================================================================
class LDVertex : public LDObject {
public:
	LDOBJ (Vertex)
	LDOBJ_NAME( vertex )
	LDOBJ_VERTICES (0) // TODO: move pos to vaCoords[0]
	LDOBJ_COLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
	
	LDVertex () {}
	
	vertex pos;
};

// =============================================================================
// LDOverlay
// 
// Overlay image meta, stored in the header of parts so as to preserve overlay
// information.
// =============================================================================
class LDOverlay : public LDObject
{
public:
	LDOBJ( Overlay )
	LDOBJ_NAME( overlay )
	LDOBJ_VERTICES( 0 )
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
	PROPERTY( int, camera, setCamera )
	PROPERTY( int, x, setX )
	PROPERTY( int, y, setY )
	PROPERTY( int, width, setWidth )
	PROPERTY( int, height, setHeight )
	PROPERTY( str, filename, setFilename )
};

#endif // LDTYPES_H