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

#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

// =============================================================================
#include <QString>
#include <QKeySequence>

typedef QChar qchar;
typedef QString str;

#define MAX_INI_LINE 512
#define MAX_CONFIG 512

#define cfg(T, NAME, DEFAULT) T##config NAME (DEFAULT, #NAME)
#define extern_cfg(T, NAME)   extern T##config NAME

// =========================================================
class config {
public:
	enum Type {
		Type_none,
		Type_int,
		Type_str,
		Type_float,
		Type_bool,
		Type_keyseq,
	};

	const char* name;

	virtual Type getType() const {
		return Type_none;
	}

	virtual void resetValue() {}
	virtual bool isDefault() const {
		return false;
	}

	// ------------------------------------------
	static bool load();
	static bool save();
	static void reset();
	static str dirpath();
	static str filepath();
};

void addConfig (config* ptr);

// =============================================================================
#define DEFINE_UNARY_OPERATOR(T, OP) \
	T operator OP() { \
		return OP value; \
	} \
	 
#define DEFINE_BINARY_OPERATOR(T, OP) \
	T operator OP (const T other) { \
		return value OP other; \
	} \
	 
#define DEFINE_ASSIGN_OPERATOR(T, OP) \
	T& operator OP (const T other) { \
		return value OP other; \
	} \
	 
#define DEFINE_COMPARE_OPERATOR(T, OP) \
	bool operator OP (const T other) { \
		return value OP other; \
	} \
	 
#define DEFINE_CAST_OPERATOR(T) \
	operator T() { \
		return (T) value; \
	} \
	 
#define DEFINE_ALL_COMPARE_OPERATORS(T) \
	DEFINE_COMPARE_OPERATOR (T, ==) \
	DEFINE_COMPARE_OPERATOR (T, !=) \
	DEFINE_COMPARE_OPERATOR (T, >) \
	DEFINE_COMPARE_OPERATOR (T, <) \
	DEFINE_COMPARE_OPERATOR (T, >=) \
	DEFINE_COMPARE_OPERATOR (T, <=) \
	 
#define DEFINE_INCREMENT_OPERATORS(T) \
	T operator++() { return ++value; } \
	T operator++(int) { return value++; } \
	T operator--() { return --value; } \
	T operator--(int) { return value--; }

#define CONFIGTYPE(T) \
	class T##config : public config

#define IMPLEMENT_CONFIG(T) \
	T value, defval; \
	\
	T##config (T _defval, const char* _name) { \
		value = defval = _defval; \
		name = _name; \
		addConfig (this); \
	} \
	operator const T&() const { \
		return value; \
	} \
	config::Type getType() const override { \
		return config::Type_##T; \
	} \
	virtual void resetValue() { \
		value = defval; \
	} \
	virtual bool isDefault() const { \
		return value == defval; \
	}

// =============================================================================
CONFIGTYPE (int) {
public:
	IMPLEMENT_CONFIG (int)
	DEFINE_ALL_COMPARE_OPERATORS (int)
	DEFINE_INCREMENT_OPERATORS (int)
	DEFINE_BINARY_OPERATOR (int, +)
	DEFINE_BINARY_OPERATOR (int, -)
	DEFINE_BINARY_OPERATOR (int, *)
	DEFINE_BINARY_OPERATOR (int, /)
	DEFINE_BINARY_OPERATOR (int, %)
	DEFINE_BINARY_OPERATOR (int, ^)
	DEFINE_BINARY_OPERATOR (int, |)
	DEFINE_BINARY_OPERATOR (int, &)
	DEFINE_BINARY_OPERATOR (int, >>)
	DEFINE_BINARY_OPERATOR (int, <<)
	DEFINE_UNARY_OPERATOR (int, !)
	DEFINE_UNARY_OPERATOR (int, ~)
	DEFINE_UNARY_OPERATOR (int, -)
	DEFINE_UNARY_OPERATOR (int, +)
	DEFINE_ASSIGN_OPERATOR (int, =)
	DEFINE_ASSIGN_OPERATOR (int, +=)
	DEFINE_ASSIGN_OPERATOR (int, -=)
	DEFINE_ASSIGN_OPERATOR (int, *=)
	DEFINE_ASSIGN_OPERATOR (int, /=)
	DEFINE_ASSIGN_OPERATOR (int, %=)
	DEFINE_ASSIGN_OPERATOR (int, >>=)
	DEFINE_ASSIGN_OPERATOR (int, <<=)
};

// =============================================================================
CONFIGTYPE (str) {
public:
	IMPLEMENT_CONFIG (str)

	DEFINE_COMPARE_OPERATOR (str, ==)
	DEFINE_COMPARE_OPERATOR (str, !=)
	DEFINE_ASSIGN_OPERATOR (str, =)
	DEFINE_ASSIGN_OPERATOR (str, +=)
	
	qchar operator[] (int n) {
		return value[n];
	}
};

// =============================================================================
CONFIGTYPE (float) {
public:
	IMPLEMENT_CONFIG (float)
	DEFINE_ALL_COMPARE_OPERATORS (float)
	DEFINE_INCREMENT_OPERATORS (float)
	DEFINE_BINARY_OPERATOR (float, +)
	DEFINE_BINARY_OPERATOR (float, -)
	DEFINE_BINARY_OPERATOR (float, *)
	DEFINE_UNARY_OPERATOR (float, !)
	DEFINE_ASSIGN_OPERATOR (float, =)
	DEFINE_ASSIGN_OPERATOR (float, +=)
	DEFINE_ASSIGN_OPERATOR (float, -=)
	DEFINE_ASSIGN_OPERATOR (float, *=)
};

// =============================================================================
CONFIGTYPE (bool) {
public:
	IMPLEMENT_CONFIG (bool)
	DEFINE_ALL_COMPARE_OPERATORS (bool)
	DEFINE_ASSIGN_OPERATOR (bool, =)
};

// =============================================================================
typedef QKeySequence keyseq;

CONFIGTYPE (keyseq) {
public:
	IMPLEMENT_CONFIG (keyseq)
	DEFINE_ALL_COMPARE_OPERATORS (keyseq)
	DEFINE_ASSIGN_OPERATOR (keyseq, =)
};

#endif // CONFIG_H