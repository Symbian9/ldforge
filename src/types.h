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

#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <vector>
#include "common.h"

// Null pointer
static const std::nullptr_t null = nullptr;

typedef QChar qchar;
typedef QString str;
template<class T> class ConstVectorReverser;
template<class T> using c_rev = ConstVectorReverser<T>;
class strconfig;
class intconfig;
class floatconfig;
class QFile;
class QTextStream;

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

// Typedef out the _t suffices :)
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

template<class T> using initlist = std::initializer_list<T>;
using std::size_t;

enum Axis { X, Y, Z };
static const Axis g_Axes[3] = { X, Y, Z };

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// matrix
// 
// A mathematical 3 x 3 matrix
// =============================================================================
class matrix {
public:
	matrix () {}
	matrix (initlist<double> vals);
	matrix (double fillval);
	matrix (double vals[]);
	
	double			determinant	() const;
	matrix			mult			(matrix other) const;
	void			puts			() const;
	str				stringRep		() const;
	void			zero			();
	double&			val				(const uint idx) { return m_vals[idx]; }
	const double&	val				(const uint idx) const { return m_vals[idx]; }
	
	matrix&			operator=		(matrix other);
	matrix			operator*		(matrix other) const { return mult (other); }
	double&			operator[]		(const uint idx) { return m_vals[idx]; }
	const double&	operator[]		(const uint idx) const { return m_vals[idx]; }

private:
	double m_vals[9];
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// vertex
// 
// Vertex class, contains a single point in 3D space. Not to be confused with
// LDVertex, which is a vertex used in an LDraw part file.
// =============================================================================
class vertex {
public:
	vertex () {}
	vertex (double x, double y, double z);
	
	double&			coord			(const ushort n) { return m_coords[n]; }
	const double&	coord			(const ushort n) const { return m_coords[n]; }
	vertex			midpoint		(const vertex& other);
	void			move			(const vertex& other);
	str				stringRep		(bool mangled) const;
	void			transform		(matrix matr, vertex pos);
	double&			x				() { return m_coords[X]; }
	const double&	x				() const { return m_coords[X]; }
	double&			y				() { return m_coords[Y]; }
	const double&	y				() const { return m_coords[Y]; }
	double&			z				() { return m_coords[Z]; }
	const double&	z				() const { return m_coords[Z]; }
	
	vertex&			operator+=		(const vertex& other);
	vertex			operator+		(const vertex& other) const;
	vertex			operator/		(const double d) const;
	vertex&			operator/=		(const double d);
	bool			operator==		(const vertex& other) const;
	bool			operator!=		(const vertex& other) const;
	vertex			operator-		() const;
	int				operator<		(const vertex& other) const;
	double&			operator[]		(const Axis ax);
	const double&	operator[]		(const Axis ax) const;
	double&			operator[]		(const int ax);
	const double&	operator[]		(const int ax) const;

private:
	double m_coords[3];
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// vector
// 
// Array class that wraps around std::vector
// =============================================================================
template<class T> class vector {
public:
	typedef typename std::vector<T>::iterator it;
	typedef typename std::vector<T>::const_iterator c_it;
	typedef typename std::vector<T>::reverse_iterator r_it;
	typedef typename std::vector<T>::const_reverse_iterator cr_it;
	
	vector () {}
	vector (initlist<T> vals) {
		m_vect = vals;
	}
	
	it begin () {
		return m_vect.begin ();
	}
	
	c_it begin () const {
		return m_vect.cbegin ();
	}
	
	it end () {
		return m_vect.end ();
	}
	
	c_it end () const {
		return m_vect.cend ();
	}
	
	r_it rbegin () {
		return m_vect.rbegin ();
	}
	
	cr_it crbegin () const {
		return m_vect.crbegin ();
	}
	
	r_it rend () {
		return m_vect.rend ();
	}
	
	cr_it crend () const {
		return m_vect.crend ();
	}
	
	void erase (ulong pos) {
		assert (pos < size ());
		m_vect.erase (m_vect.begin () + pos);
	}
	
	void push_back (const T& value) {
		m_vect.push_back (value);
	}
	
	void push_back (const vector<T>& vals) {
		for (const T& val : vals)
			push_back (val);
	}
	
	bool pop (T& val) {
		if (size () == 0)
			return false;
		
		val = m_vect[size () - 1];
		erase (size () - 1);
		return true;
	}
	
	vector<T>& operator<< (const T& value) {
		push_back (value);
		return *this;
	}
	
	vector<T>& operator<< (const vector<T>& vals) {
		push_back (vals);
		return *this;
	}
	
	bool operator>> (T& value) {
		return pop (value);
	}
	
	vector<T> reverse () const {
		vector<T> rev;
		
		for (const T& val : c_rev<T> (*this))
			rev << val;
		
		return rev;
	}
	
	void clear () {
		m_vect.clear ();
	}
	
	void insert (ulong pos, const T& value) {
		m_vect.insert (m_vect.begin () + pos, value);
	}
	
	void makeUnique () {
		// Remove duplicate entries. For this to be effective, the vector must be
		// sorted first.
		sort ();
		it pos = std::unique (begin (), end ());
		resize (std::distance (begin (), pos));
	}
	
	ulong size () const {
		return m_vect.size ();
	}
	
	T& operator[] (ulong n) {
		assert (n < size ());
		return m_vect[n];
	}
	
	const T& operator[] (ulong n) const {
		assert (n < size ());
		return m_vect[n];
	}
	
	void resize (std::ptrdiff_t size) {
		m_vect.resize (size);
	}
	
	void sort () {
		std::sort (begin (), end ());
	}
	
	ulong find (const T& needle) {
		ulong i = 0;
		for (const T& hay : *this) {
			if (hay == needle)
				return i;
			
			i++;
		}
		
		return -1u;
	}
	
private:
	std::vector<T> m_vect;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// VectorReverser (aka rev)
// 
// Helper class used to reverse-iterate vectors in range-for-loops.
// =============================================================================
template<class T> class VectorReverser {
public:
	typedef typename vector<T>::r_it it;
	
	VectorReverser (vector<T>& vect) {
		m_vect = &vect;
	}
	
	it begin () {
		return m_vect->rbegin ();
	}
	
	it end () {
		return m_vect->rend ();
	}
	
private:
	vector<T>* m_vect;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// ConstVectorReverser (aka c_rev)
// 
// Like VectorReverser, except works on const vectors.
// =============================================================================
template<class T> class ConstVectorReverser {
public:
	typedef typename vector<T>::cr_it it;
	
	ConstVectorReverser (const vector<T>& vect) {
		m_vect = &vect;
	}
	
	it begin () const {
		return m_vect->crbegin ();
	}
	
	it end () const {
		return m_vect->crend ();
	}
	
private:
	const vector<T>* m_vect;
};

template<class T> using rev = VectorReverser<T>;
template<class T> using c_rev = ConstVectorReverser<T>;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// StringFormatArg
// 
// Converts a given value into a string that can be retrieved with ::value ().
// Used as the argument type to the formatting functions, hence its name.
// =============================================================================
class StringFormatArg {
public:
	StringFormatArg (const str& v);
	StringFormatArg (const char& v);
	StringFormatArg (const uchar& v);
	StringFormatArg (const qchar& v);
	
#define NUMERIC_FORMAT_ARG(T,C) \
	StringFormatArg (const T& v) { \
		char valstr[32]; \
		sprintf (valstr, "%" #C, v); \
		m_val = valstr; \
	}
	
	NUMERIC_FORMAT_ARG (int, d)
	NUMERIC_FORMAT_ARG (short, d)
	NUMERIC_FORMAT_ARG (long, ld)
	NUMERIC_FORMAT_ARG (uint, u)
	NUMERIC_FORMAT_ARG (ushort, u)
	NUMERIC_FORMAT_ARG (ulong, lu)
	
	StringFormatArg (const float& v);
	StringFormatArg (const double& v);
	StringFormatArg (const vertex& v);
	StringFormatArg (const matrix& v);
	StringFormatArg (const char* v);
	StringFormatArg (const strconfig& v);
	StringFormatArg (const intconfig& v);
	StringFormatArg (const floatconfig& v);
	
	str value () const { return m_val; }
private:
	str m_val;
};

// Formatter function
str DoFormat (vector<StringFormatArg> args);
#ifndef IN_IDE_PARSER
#define fmt(...) DoFormat ({__VA_ARGS__})
#else
str fmt (const char* fmtstr, ...);
#endif

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// File
// 
// A file interface with simple interface and support for range-for-loops.
// =============================================================================
class File {
public:
	// Iterator class to enable range-for-loop support. Rough hack.. don't use directly!
	class iterator {
	public:
		iterator () : m_file (null) {} // end iterator has m_file == null
		iterator (File* f) : m_file (f), m_text (m_file->readLine ()) {}
		void operator++ ();
		str  operator* ();
		bool operator== (iterator& other);
		bool operator!= (iterator& other);
	
	private:
		File* m_file;
		str m_text;
	};
	
	enum OpenType {
		Read,
		Write,
		Append
	};
	
	File (const std::nullptr_t&);
	File (str path, File::OpenType rtype);
	File (FILE* fp, File::OpenType rtype);
	~File ();
	
	bool         atEnd         () const;
	iterator     begin         ();
	void         close         ();
	iterator&    end           ();
	bool         flush         ();
	bool         isNull        () const;
	str          readLine      ();
	bool         open          (FILE* fp, OpenType rtype);
	bool         open          (str path, OpenType rtype, FILE* fp = null);
	bool         operator!     () const;
	void         write         (str msg);
	
private:
	QFile*       m_file;
	QTextStream* m_textstream;
	iterator     m_endIterator;
};

// Null-file, equivalent to a null FILE*
extern const File nullfile;

#endif // TYPES_H