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
#include <QRegExp>
#include <QDialog>
#include <QTreeWidgetItem>
#include <QDirIterator>
#include "main.h"
#include "model.h"
#include "hierarchyelement.h"

class LDDocument;
class Ui_GeneratePrimitiveDialog;
class PrimitiveCategory;
class PrimitiveScanner;

struct Primitive
{
	QString name;
	QString title;
	PrimitiveCategory* category;
};

struct PrimitiveModel
{
	enum Type
	{
		Circle,
		Cylinder,
		Disc,
		DiscNegative,
		Ring,
		Cone,
	} type;
	int segments;
	int divisions;
	int ringNumber;

	QString typeName() const;
	void generateBody(Model& model) const;
	static QString typeName(Type type);
	QString makeFileName() const;
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

class PrimitiveManager : public QAbstractItemModel, HierarchyElement
{
	Q_OBJECT

public:
	PrimitiveManager(QObject* parent);

	PrimitiveScanner* activeScanner();
	LDDocument* generatePrimitive(const PrimitiveModel &spec);
	LDDocument* getPrimitive(const PrimitiveModel &spec);
	QString getPrimitivesCfgPath() const;
	void loadPrimitives();
	void startScan();

	int	columnCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	QList<PrimitiveCategory*> m_categories;
	PrimitiveScanner* m_activeScanner;
	QVector<Primitive> m_primitives;
	PrimitiveCategory* m_unmatched;

	void loadCategories();
	void populateCategories();
	void clearCategories();
};

/*
 * PrimitiveScanner
 *
 * Worker object that scans the primitives folder for primitives and builds an index of them.
 */
class PrimitiveScanner : public QObject, HierarchyElement
{
	Q_OBJECT

public:
	PrimitiveScanner(PrimitiveManager* parent);
	const QVector<Primitive>& scannedPrimitives() const;

public slots:
	void work();

signals:
	void workDone();

private:
	PrimitiveManager* m_manager;
	QVector<Primitive> m_scannedPrimitives;
	QDirIterator m_iterator;
	int m_basePathLength;
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
