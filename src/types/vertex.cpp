/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright(C) 2013 - 2018 Teemu Piippo
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

#include "vertex.h"
#include "../format.h"

template<typename MatrixType>
void transformVertex(Vertex& vertex, const MatrixType& matrix)
{
	double newX = (matrix(0, 0) * vertex.x) + (matrix(0, 1) * vertex.y) + (matrix(0, 2) * vertex.z);
	double newY = (matrix(1, 0) * vertex.x) + (matrix(1, 1) * vertex.y) + (matrix(1, 2) * vertex.z);
	double newZ = (matrix(2, 0) * vertex.x) + (matrix(2, 1) * vertex.y) + (matrix(2, 2) * vertex.z);
	vertex.x = newX;
	vertex.y = newY;
	vertex.z = newZ;
}

void Vertex::transform(const QMatrix4x4& matrix)
{
	transformVertex(*this, matrix);
	x += matrix(0, 3);
	y += matrix(1, 3);
	z += matrix(2, 3);
}

void Vertex::rotate(const QQuaternion& orientation)
{
	*this = Vertex {0, 0, 0} + orientation.rotatedVector(toVector());
}

void Vertex::apply(ApplyFunction func)
{
	func(X, this->x);
	func(Y, this->y);
	func(Z, this->z);
}

void Vertex::apply(ApplyConstFunction func) const
{
	func(X, this->x);
	func(Y, this->y);
	func(Z, this->z);
}

double& Vertex::operator[](Axis axis)
{
	switch (axis)
	{
	case X:
		return this->x;

	case Y:
		return this->y;

	case Z:
		return this->z;

	default:
		return ::singleton<double>();
	}
}

double Vertex::operator[](Axis axis) const
{
	switch (axis)
	{
	case X:
		return this->x;

	case Y:
		return this->y;

	case Z:
		return this->z;

	default:
		return 0.0;
	}
}

void Vertex::setCoordinate(Axis axis, qreal value)
{
	(*this)[axis] = value;
}

QString Vertex::toString(bool mangled) const
{
	if (mangled)
		return ::format("(%1, %2, %3)", this->x, this->y, this->z);
	else
		return ::format("%1 %2 %3", this->x, this->y, this->z);
}

Vertex Vertex::fromVector(const QVector3D& vector)
{
	return {vector.x(), vector.y(), vector.z()};
}

Vertex Vertex::operator*(qreal scalar) const
{
	return {this->x * scalar, this->y * scalar, this->z * scalar};
}

Vertex& Vertex::operator+= (const QVector3D& other)
{
	this->x += other.x();
	this->y += other.y();
	this->z += other.z();
	return *this;
}

Vertex Vertex::operator+ (const QVector3D& other) const
{
	Vertex result(*this);
	result += other;
	return result;
}

QVector3D Vertex::toVector() const
{
	return {
		static_cast<float>(this->x),
		static_cast<float>(this->y),
		static_cast<float>(this->z)
	};
}

Vertex Vertex::operator-(const QVector3D& vector) const
{
	Vertex result = *this;
	result -= vector;
	return result;
}

Vertex& Vertex::operator-= (const QVector3D& vector)
{
	this->x -= vector.x();
	this->y -= vector.y();
	this->z -= vector.z();
	return *this;
}

QVector3D Vertex::operator-(const Vertex& other) const
{
	return {
		static_cast<float>(this->x - other.x),
		static_cast<float>(this->y - other.y),
		static_cast<float>(this->z - other.z)
	};
}

Vertex& Vertex::operator*= (qreal scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}

bool Vertex::operator== (const Vertex& other) const
{
	return this->x == other.x and this->y == other.y and this->z == other.z;
}

bool Vertex::operator!= (const Vertex& other) const
{
	return not(*this == other);
}

bool Vertex::operator<(const Vertex& other) const
{
	if (not qFuzzyCompare(this->x, other.x))
		return this->x < other.x;
	else if (not qFuzzyCompare(this->y, other.y))
		return this->y < other.y;
	else
		return this->z < other.z;
}

/*
 * Transforms this vertex with a tranformation matrix and returns the result.
 */
Vertex Vertex::transformed(const GLRotationMatrix& matrix) const
{
	return {
		matrix(0, 0) * this->x
			+ matrix(0, 1) * this->y
			+ matrix(0, 2) * this->z,
		matrix(1, 0) * this->x
			+ matrix(1, 1) * this->y
			+ matrix(1, 2) * this->z,
		matrix(2, 0) * this->x
			+ matrix(2, 1) * this->y
			+ matrix(2, 2) * this->z,
	};
}

/*
 * Returns the distance from one vertex to another.
 */
qreal distance(const Vertex& one, const Vertex& other)
{
	return (one - other).length();
}

/*
 * Returns a vertex with all coordinates inverted.
 */
Vertex operator-(const Vertex& vertex)
{
	return {-vertex.x, -vertex.y, -vertex.z};
}

/*
 * Inserts this vertex into a data stream. This is needed for vertices to be
 * stored in QSettings.
 */
QDataStream& operator<<(QDataStream& out, const Vertex& vertex)
{
	return out << vertex.x << vertex.y << vertex.z;
}

/*
 * Takes a vertex from a data stream.
 */
QDataStream& operator>>(QDataStream& in, Vertex& vertex)
{
	return in >> vertex.x >> vertex.y >> vertex.z;
}

unsigned int qHash(const Vertex& key)
{
	return qHash(key.x) ^ rotl10(qHash(key.y)) ^ rotl20(qHash(key.z));
}
