/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
#include <QObject>
#include <deque>
#include "common.h"

class LDObject;

typedef QChar qchar;
typedef QString str;
template<class T> class ConstListReverser;
template<class T> using c_rev = ConstListReverser<T>;
class strconfig;
class intconfig;
class floatconfig;
class QFile;
class QTextStream;

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef qint8 int8;
typedef qint16 int16;
typedef qint32 int32;
typedef qint64 int64;
typedef quint8 uint8;
typedef quint16 uint16;
typedef quint32 uint32;
typedef quint64 uint64;

template<class T> using initlist = std::initializer_list<T>;
template<class T, class R> using pair = std::pair<T, R>;
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
	matrix() {}
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
	vertex() {}
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
// List
// 
// Array class that wraps around std::deque
// =============================================================================
template<class T> class List {
public:
	typedef typename std::deque<T>::iterator it;
	typedef typename std::deque<T>::const_iterator c_it;
	typedef typename std::deque<T>::reverse_iterator r_it;
	typedef typename std::deque<T>::const_reverse_iterator cr_it;
	
	List() {}
	List (initlist<T> vals) {
		m_vect = vals;
	}
	
	it begin() {
		return m_vect.begin();
	}
	
	c_it begin() const {
		return m_vect.cbegin();
	}
	
	it end() {
		return m_vect.end();
	}
	
	c_it end() const {
		return m_vect.cend();
	}
	
	r_it rbegin() {
		return m_vect.rbegin();
	}
	
	cr_it crbegin() const {
		return m_vect.crbegin();
	}
	
	r_it rend() {
		return m_vect.rend();
	}
	
	cr_it crend() const {
		return m_vect.crend();
	}
	
	void erase (ulong pos) {
		assert (pos < size());
		m_vect.erase (m_vect.begin() + pos);
	}
	
	T& push_back (const T& value) {
		m_vect.push_back (value);
		return m_vect[m_vect.size() - 1];
	}
	
	void push_back (const List<T>& vals) {
		for (const T& val : vals)
			push_back (val);
	}
	
	bool pop (T& val) {
		if (size() == 0)
			return false;
		
		val = m_vect[size() - 1];
		erase (size() - 1);
		return true;
	}
	
	T& operator<< (const T& value) {
		return push_back (value);
	}
	
	void operator<< (const List<T>& vals) {
		push_back (vals);
	}
	
	bool operator>> (T& value) {
		return pop (value);
	}
	
	List<T> reverse() const {
		List<T> rev;
		
		for (const T& val : c_rev<T> (*this))
			rev << val;
		
		return rev;
	}
	
	void clear() {
		m_vect.clear();
	}
	
	void insert (ulong pos, const T& value) {
		m_vect.insert (m_vect.begin() + pos, value);
	}
	
	void makeUnique() {
		// Remove duplicate entries. For this to be effective, the List must be
		// sorted first.
		sort();
		it pos = std::unique (begin(), end());
		resize (std::distance (begin(), pos));
	}
	
	ulong size() const {
		return m_vect.size();
	}
	
	T& operator[] (ulong n) {
		assert (n < size());
		return m_vect[n];
	}
	
	const T& operator[] (ulong n) const {
		assert (n < size());
		return m_vect[n];
	}
	
	void resize (std::ptrdiff_t size) {
		m_vect.resize (size);
	}
	
	void sort() {
		std::sort (begin(), end());
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
	std::deque<T> m_vect;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// ListReverser (aka rev)
// 
// Helper class used to reverse-iterate Lists in range-for-loops.
// =============================================================================
template<class T> class ListReverser {
public:
	typedef typename List<T>::r_it it;
	
	ListReverser (List<T>& vect) {
		m_vect = &vect;
	}
	
	it begin() {
		return m_vect->rbegin();
	}
	
	it end() {
		return m_vect->rend();
	}
	
private:
	List<T>* m_vect;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// ConstListReverser (aka c_rev)
// 
// Like ListReverser, except works on const Lists.
// =============================================================================
template<class T> class ConstListReverser {
public:
	typedef typename List<T>::cr_it it;
	
	ConstListReverser (const List<T>& vect) {
		m_vect = &vect;
	}
	
	it begin() const {
		return m_vect->crbegin();
	}
	
	it end() const {
		return m_vect->crend();
	}
	
private:
	const List<T>* m_vect;
};

template<class T> using rev = ListReverser<T>;
template<class T> using c_rev = ConstListReverser<T>;

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// StringFormatArg
// 
// Converts a given value into a string that can be retrieved with ::value().
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
	StringFormatArg (const void* v);
	
	template<class T> StringFormatArg (const List<T>& v) {
		m_val = "{ ";
		
		uint i = 0;
		for (const T& it : v) {
			if (i++)
				m_val += ", ";
			
			StringFormatArg arg (it);
			m_val += arg.value();
		}
		
		if (i)
			m_val += " ";
		
		m_val += "}";
	}
	
	str value() const { return m_val; }
private:
	str m_val;
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
// File
// 
// A file interface with simple interface and support for range-for-loops.
// =============================================================================
class File {
private:
	// Iterator class to enable range-for-loop support. Rough hack.. don't use directly!
	class iterator {
	public:
		iterator() : m_file (null) {} // end iterator has m_file == null
		iterator (File* f);
		void operator++();
		str  operator*();
		bool operator== (iterator& other);
		bool operator!= (iterator& other);
	
	private:
		File* m_file;
		str m_text;
		bool m_gotdata = false;
	};

public:
	enum OpenType {
		Read,
		Write,
		Append
	};
	
	File();
	File (str path, File::OpenType rtype);
	File (FILE* fp, File::OpenType rtype);
	~File();
	
	bool         atEnd() const;
	iterator     begin();
	void         close();
	iterator&    end();
	bool         flush();
	bool         isNull() const;
	bool         readLine (str& line);
	void         rewind();
	bool         open (FILE* fp, OpenType rtype);
	bool         open (str path, OpenType rtype, FILE* fp = null);
	void         write (str msg);
	
	bool         operator!() const;
	operator bool() const;
	
private:
	QFile*       m_file;
	QTextStream* m_textstream;
	iterator     m_endIterator;
};

// =============================================================================
// LDBoundingBox
//
// The bounding box is the box that encompasses a given set of objects.
// v0 is the minimum vertex, v1 is the maximum vertex.
// =============================================================================
class LDBoundingBox {
	READ_PROPERTY (bool, empty, setEmpty)
	READ_PROPERTY (vertex, v0, setV0)
	READ_PROPERTY (vertex, v1, setV1)
	
public:
	LDBoundingBox();
	void reset();
	void calculate();
	double size() const;
	void calcObject (LDObject* obj);
	void calcVertex (const vertex& v);
	vertex center() const;
	
	LDBoundingBox& operator<< (LDObject* obj);
	LDBoundingBox& operator<< (const vertex& v);
};

// Formatter function
str DoFormat (List<StringFormatArg> args);

// printf replacement
void doPrint (File& f, initlist<StringFormatArg> args);
void doPrint (FILE* f, initlist<StringFormatArg> args); // heh

// log() - universal access to the message log. Defined here so that I don't have
// to include messagelog.h here and recompile everything every time that file changes.
void DoLog (std::initializer_list<StringFormatArg> args);

// Macros to access these functions
# ifndef IN_IDE_PARSER
#define fmt(...) DoFormat ({__VA_ARGS__})
# define print(...) doPrint (g_file_stdout, {__VA_ARGS__})
# define fprint(F, ...) doPrint (F, {__VA_ARGS__})
# define log(...) DoLog({ __VA_ARGS__ });
#else
str fmt (const char* fmtstr, ...);
void print (const char* fmtstr, ...);
void fprint (File& f, const char* fmtstr, ...);
void log (const char* fmtstr, ...);
#endif

extern File g_file_stdout;
extern File g_file_stderr;
extern const vertex g_origin; // Vertex at (0, 0, 0)
extern const matrix g_identity; // Identity matrix

static const double pi = 3.14159265358979323846f;

#endif // TYPES_H