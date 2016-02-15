/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2016 Teemu Piippo
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
#include <QRegExp>
#include <QDialog>
#include <QTreeWidgetItem>
#include "main.h"

class LDDocument;
class Ui_MakePrimUI;
class PrimitiveCategory;

struct Primitive
{
	QString name;
	QString title;
	PrimitiveCategory* category;
};

class PrimitiveCategory : public QObject
{
	Q_OBJECT

public:
	enum PatternType
	{
		FilenamePattern,
		TitlePattern
	};

	struct RegexEntry
	{
		QRegExp		regex;
		PatternType	type;
	};

	QList<RegexEntry> patterns;
	QList<Primitive> primitives;

	explicit PrimitiveCategory (QString name, QObject* parent = 0);
	bool isValidToInclude();
	QString name() const;

private:
	QString m_name;
};

//
// Worker object that scans the primitives folder for primitives and
// builds an index of them.
//
class PrimitiveScanner : public QObject, HierarchyElement
{
	Q_OBJECT

public:
	PrimitiveScanner(PrimitiveManager* parent);
	const QList<Primitive>& scannedPrimitives() const;

public slots:
	void work();

signals:
	void starting(int num);
	void workDone();
	void update(int i);

private:
	PrimitiveManager* m_manager;
	QList<Primitive> m_prims;
	QStringList m_files;
	int m_i;
	int m_baselen;
};

extern QList<PrimitiveCategory*> m_categories;

enum PrimitiveType
{
	Circle,
	Cylinder,
	Disc,
	DiscNegative,
	Ring,
	Cone,
};

class PrimitivePrompt : public QDialog
{
	Q_OBJECT

public:
	explicit PrimitivePrompt (QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	virtual ~PrimitivePrompt();
	Ui_MakePrimUI* ui;

public slots:
	void hiResToggled (bool on);
};

class PrimitiveManager : public QObject, HierarchyElement
{
	Q_OBJECT

public:
	PrimitiveManager(QObject* parent);

	PrimitiveScanner* activeScanner();
	LDDocument* generatePrimitive(PrimitiveType type, int segs, int divs, int num);
	LDDocument* getPrimitive(PrimitiveType type, int segs, int divs, int num);
	QString getPrimitivesCfgPath() const;
	void loadPrimitives();
	void makeCircle(int segs, int divs, double radius, QList<QLineF>& lines);
	QString makeRadialFileName(PrimitiveType type, int segs, int divs, int num);
	void populateTreeWidget(QTreeWidget* tree, const QString& selectByDefault = QString());
	QString primitiveTypeName(PrimitiveType type);
	Q_SLOT void scanDone();
	void startScan();

private:
	QList<PrimitiveCategory*> m_categories;
	PrimitiveScanner* m_activeScanner;
	QList<Primitive> m_primitives;
	PrimitiveCategory* m_unmatched;

	LDObjectList makePrimitiveBody (PrimitiveType type, int segs, int divs, int num);
	void loadCategories();
	void populateCategories();
	void clearCategories();
};

class PrimitiveTreeItem : public QTreeWidgetItem
{
public:
	PrimitiveTreeItem (QTreeWidgetItem* parent, Primitive* info);
	PrimitiveTreeItem (QTreeWidget* parent, Primitive* info);
	Primitive* primitive() const;

private:
	Primitive* m_primitive;
};
