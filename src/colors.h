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
#include <QColor>
#include "main.h"

#define SHARED_POINTER_DERIVATIVE(T) \
public: \
	using Self = T; \
	using DataType = T##Data; \
	using Super = QSharedPointer<DataType>; \
	\
	T() : Super() {} \
	T (DataType* data) : Super (data) {} \
	T (Super const& other) : Super (other) {} \
	T (QWeakPointer<DataType> const& other) : Super (other) {} \
	\
	template <typename Deleter> \
	T (DataType* data, Deleter dlt) : Super (data, dlt) {} \
	\
	inline bool			operator== (decltype(nullptr)) { return data() == nullptr; } \
	inline bool			operator!= (decltype(nullptr)) { return data() != nullptr; } \
	inline DataType*	operator->() const = delete;

#define SHARED_POINTER_DATA_ACCESS(N) \
	public: inline decltype(DataType::m_##N) const& N() const { return data()->m_##N; }

class LDColor;

struct LDColorData
{
	QString m_name;
	QString m_hexcode;
	QColor m_faceColor;
	QColor m_edgeColor;
	qint32 m_index;
};

class LDColor : public QSharedPointer<LDColorData>
{
	SHARED_POINTER_DERIVATIVE (LDColor)
	SHARED_POINTER_DATA_ACCESS (edgeColor)
	SHARED_POINTER_DATA_ACCESS (faceColor)
	SHARED_POINTER_DATA_ACCESS (hexcode)
	SHARED_POINTER_DATA_ACCESS (index)
	SHARED_POINTER_DATA_ACCESS (name)

public:
	QString				indexString() const;
	bool				isDirect() const;

	bool				operator== (Self const& other);
	static void			addLDConfigColor (qint32 index, LDColor color);
	static LDColor		fromIndex (qint32 index);
};

void InitColors();
int Luma (const QColor& col);
int CountLDConfigColors();

// Main and edge colors
LDColor MainColor();
LDColor EdgeColor();

enum
{
	MainColorIndex = 16,
	EdgeColorIndex = 24,
};
