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
#include <functional>
#include <QVector3D>
#include "../basics.h"

struct Vertex
{
	qreal x, y, z;

	using ApplyFunction = std::function<void(Axis, double&)>;
	using ApplyConstFunction = std::function<void(Axis, double)>;

	void apply(ApplyFunction func);
	void apply(ApplyConstFunction func) const;
	QString toString(bool mangled = false) const;
	QVector3D toVector() const;
	void transform(const class Matrix& matrix, const Vertex& pos);
	Vertex transformed(const GLRotationMatrix& matrix) const;
	void setCoordinate(Axis ax, qreal value);

	Vertex& operator+=(const QVector3D& other);
	Vertex operator+(const QVector3D& other) const;
	QVector3D operator-(const Vertex& other) const;
	Vertex operator-(const QVector3D& vector) const;
	Vertex& operator-=(const QVector3D& vector);
	Vertex& operator*=(qreal scalar);
	Vertex operator*(qreal scalar) const;
	bool operator<(const Vertex& other) const;
	double& operator[](Axis ax);
	double operator[](Axis ax) const;
	bool operator==(const Vertex& other) const;
	bool operator!=(const Vertex& other) const;
};

inline Vertex operator*(qreal scalar, const Vertex& vertex)
{
	return vertex * scalar;
}

Q_DECLARE_METATYPE(Vertex)
unsigned int qHash(const Vertex& key);
QDataStream& operator<<(QDataStream& out, const Vertex& vertex);
QDataStream& operator>>(QDataStream& in, Vertex& vertex);
