/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2018 Teemu Piippo
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

#include <QApplication>
#include <QMessageBox>
#include "lddocument.h"
#include "mainwindow.h"
#include "primitives.h"
#include "colors.h"
#include "documentmanager.h"
#include "editHistory.h"
#include "algorithms/geometry.h"
#include "linetypes/comment.h"
#include "linetypes/conditionaledge.h"
#include "linetypes/edgeline.h"
#include "linetypes/empty.h"
#include "linetypes/quadrilateral.h"
#include "linetypes/triangle.h"

PrimitiveManager::PrimitiveManager(QObject* parent) :
	QAbstractItemModel {parent},
	HierarchyElement {parent},
	m_activeScanner {nullptr},
	m_unmatched {nullptr} {}


PrimitiveScanner* PrimitiveManager::activeScanner()
{
	return m_activeScanner;
}


QString PrimitiveManager::getPrimitivesCfgPath() const
{
	return QDir {qApp->applicationDirPath()}.filePath("prims.cfg");
}


void PrimitiveManager::loadPrimitives()
{
	// Try to load prims.cfg
	QFile primitivesFile = {getPrimitivesCfgPath()};

	if (not primitivesFile.open (QIODevice::ReadOnly))
	{
		// No prims.cfg, build it
		startScan();
	}
	else
	{
		emit layoutAboutToBeChanged();
		m_primitives.clear();

		while (not primitivesFile.atEnd())
		{
			QString line = primitivesFile.readLine().simplified();
			int space = line.indexOf(" ");

			if (space != -1)
			{
				Primitive info;
				info.name = line.left(space);
				info.title = line.mid(space + 1);
				m_primitives.append(info);
			}
		}

		populateCategories();
		emit layoutChanged();
		print(tr("%1 primitives loaded.") + "\n", countof(m_primitives));
	}
}


void PrimitiveManager::startScan()
{
	if (m_activeScanner == nullptr)
	{
		loadCategories();
		m_activeScanner = new PrimitiveScanner {this};
		m_activeScanner->work();
		connect(m_activeScanner, &PrimitiveScanner::workDone, this, [&]()
		{
			if (m_activeScanner)
			{
				emit layoutAboutToBeChanged();
				m_primitives = m_activeScanner->scannedPrimitives();
				populateCategories();
				emit layoutChanged();
				print(tr("%1 primitives scanned"), countof(m_primitives));
				delete m_activeScanner;
				m_activeScanner = nullptr;
			}
		});
	}
}


void PrimitiveManager::clearCategories()
{
	for (PrimitiveCategory* category : m_categories)
		delete category;

	m_categories.clear();
}


void PrimitiveManager::populateCategories()
{
	loadCategories();

	for (PrimitiveCategory* category : m_categories)
		category->primitives.clear();

	for (Primitive& primitive : m_primitives)
	{
		bool matched = false;
		primitive.category = nullptr;

		// Go over the categories and their regexes, if and when there's a match,
		// the primitive's category is set to the category the regex beloings to.
		for (PrimitiveCategory* category : m_categories)
		{
			for (PrimitiveCategory::RegexEntry& entry : category->patterns)
			{
				switch (entry.type)
				{
				case PrimitiveCategory::FilenamePattern:
					// f-regex, check against filename
					matched = entry.regex.exactMatch (primitive.name);
					break;

				case PrimitiveCategory::TitlePattern:
					// t-regex, check against title
					matched = entry.regex.exactMatch (primitive.title);
					break;
				}

				if (matched)
				{
					primitive.category = category;
					break;
				}
			}

			// Drop off if a category was decided on.
			if (primitive.category)
				break;
		}

		// If there was a match, add the primitive to the category.
		// Otherwise, add it to the list of unmatched primitives.
		if (primitive.category)
			primitive.category->primitives << primitive;
		else
			m_unmatched->primitives << primitive;
	}

	// Sort the categories. Note that we only do this here because we needed the original order for pattern matching.
	::sort(m_categories.begin(), m_categories.end(),
		[](PrimitiveCategory* const& one, PrimitiveCategory* const& other) -> bool
		{
			return one->name() < other->name();
		});
}


void PrimitiveManager::loadCategories()
{
	clearCategories();
	QString path = ":/data/primitive-categories.cfg";
	QFile categoriesFile = {path};

	if (not categoriesFile.open (QIODevice::ReadOnly))
	{
		QString message = format(tr("Failed to open primitive categories: %1"), categoriesFile.errorString());
		QMessageBox::critical(m_window, tr("Cannot open categories"), message);
		return;
	}

	PrimitiveCategory* category = nullptr;

	while (not categoriesFile.atEnd())
	{
		QString line = QString::fromUtf8(categoriesFile.readLine()).trimmed();

		if (line.isEmpty() or line[0] == '#')
			continue;

		int colon = line.indexOf (":");
		if (colon == -1)
		{
			if (category and category->isValidToInclude())
			{
				m_categories << category;
			}
			else if (category)
			{
				print (tr ("Warning: Category \"%1\" left without patterns"), category->name());
				delete category;
			}

			category = new PrimitiveCategory (line);
		}
		else if (category)
		{
			QString typechar = line.left (colon);
			PrimitiveCategory::PatternType type = PrimitiveCategory::FilenamePattern;

			if (typechar == "f")
			{
				type = PrimitiveCategory::FilenamePattern;
			}
			else if (typechar == "t")
			{
				type = PrimitiveCategory::TitlePattern;
			}
			else
			{
				print(tr("Warning: unknown pattern type \"%1\" on line \"%2\""), typechar, line);
				continue;
			}

			QRegExp regex (line.mid (colon + 1));
			PrimitiveCategory::RegexEntry entry = { regex, type };
			category->patterns << entry;
		}
		else
		{
			print("Warning: Rules given before the first category name");
		}
	}

	if (category->isValidToInclude())
		m_categories << category;

	// Add a category for unmatched primitives.
	// Note: if this function is called the second time, m_unmatched has been
	// deleted at the beginning of the function and is dangling at this point.
	m_unmatched = new PrimitiveCategory {tr("Other")};
	m_categories.append(m_unmatched);
	categoriesFile.close();
}

void PrimitiveModel::generateBody(Model& model) const
{
	QVector<int> conditionalLineSegments;
	QVector<QLineF> circle = makeCircle(segments, divisions, 1);

	for (int i = 0; i < segments; ++i)
	{
		double x0 = circle[i].x1();
		double x1 = circle[i].x2();
		double z0 = circle[i].y1();
		double z1 = circle[i].y2();

		switch (type)
		{
		case Circle:
		    {
			    LDEdgeLine* line = model.emplace<LDEdgeLine>();
				line->setVertex(0, Vertex {x0, 0.0f, z0});
				line->setVertex(1, Vertex {x1, 0.0f, z1});
				line->setColor(EdgeColor);
			}
			break;

		case Cylinder:
		case Ring:
		case Cone:
			{
				double x2, x3, z2, z3;
				double y0, y1, y2, y3;

				if (type == Cylinder)
				{
					x2 = x1;
					x3 = x0;
					z2 = z1;
					z3 = z0;
					y0 = y1 = 0.0;
					y2 = y3 = 1.0;
				}
				else
				{
					x2 = x1 * (ringNumber + 1);
					x3 = x0 * (ringNumber + 1);
					z2 = z1 * (ringNumber + 1);
					z3 = z0 * (ringNumber + 1);
					x0 *= ringNumber;
					x1 *= ringNumber;
					z0 *= ringNumber;
					z1 *= ringNumber;

					if (type == Ring)
					{
						y0 = y1 = y2 = y3 = 0.0;
					}
					else
					{
						y0 = y1 = 1.0;
						y2 = y3 = 0.0;
					}
				}

				Vertex v0 = {x0, y0, z0};
				Vertex v1 = {x1, y1, z1};
				Vertex v2 = {x2, y2, z2};
				Vertex v3 = {x3, y3, z3};

				if (type == Cylinder)
					qSwap(v1, v3);

				LDQuadrilateral* quad = model.emplace<LDQuadrilateral>(v0, v1, v2, v3);
				quad->setColor(MainColor);

				if (type == Cylinder or type == Cone)
					conditionalLineSegments.append(i);
			}
			break;

		case Disc:
		case DiscNegative:
			{
				double x2, z2;

				if (type == Disc)
				{
					x2 = z2 = 0.0;
				}
				else
				{
					x2 = (x0 >= 0.0) ? 1.0 : -1.0;
					z2 = (z0 >= 0.0) ? 1.0 : -1.0;
				}

				Vertex v0 = {x0, 0.0, z0};
				Vertex v1 = {x1, 0.0, z1};
				Vertex v2 = {x2, 0.0, z2};

				// Disc negatives need to go the other way around, otherwise
				// they'll end up upside-down.
				LDTriangle* segment = model.emplace<LDTriangle>();
				segment->setColor(MainColor);
				segment->setVertex(type == Disc ? 0 : 2, v0);
				segment->setVertex(1, v1);
				segment->setVertex(type == Disc ? 2 : 0, v2);
			}
			break;
		}
	}

	// If this is not a full circle, we need a conditional line at the other
	// end, too.
	if (segments < divisions and not conditionalLineSegments.isEmpty())
		conditionalLineSegments << segments;

	for (int i : conditionalLineSegments)
	{
		QPointF p0 = ::pointOnLDrawCircumference(i, divisions);
		QPointF p2 = ::pointOnLDrawCircumference(i + 1, divisions);
		QPointF p3 = ::pointOnLDrawCircumference(i - 1, divisions);
		Vertex v0 = {p0.x(), 0.0, p0.y()};
		Vertex v1;
		Vertex v2 = {p2.x(), 0.0, p2.y()};
		Vertex v3 = {p3.x(), 0.0, p3.y()};

		if (type == Cylinder)
		{
			v1 = {v0.x, 1.0f, v0.z};
		}
		else if (type == Cone)
		{
			v1 = {v0.x * (ringNumber + 1), 0.0, v0.z * (ringNumber + 1)};
			v0 = {v0.x * ringNumber, 1.0, v0.z * ringNumber};
		}

		LDConditionalEdge* line = model.emplace<LDConditionalEdge>();
		line->setColor(EdgeColor);
		line->setVertex(0, v0);
		line->setVertex(1, v1);
		line->setVertex(2, v2);
		line->setVertex(3, v3);
	}
}


QString PrimitiveModel::typeName() const
{
	return typeName(type);
}


QString PrimitiveModel::typeName(PrimitiveModel::Type type)
{
	// Not translated as primitives are in English.
	const char* names[] = {"Circle", "Cylinder", "Disc", "Disc Negative", "Ring", "Cone"};

	if (type >= 0 and type < countof(names))
		return names[type];
	else
		return "Unknown";
}


QString PrimitiveModel::makeFileName(FilenameStyle style) const
{
	int numerator = segments;
	int denominator = divisions;

	// Simplify the fractional part, but the denominator must be at least 4.
	simplify(numerator, denominator);

	if (denominator < 4)
	{
		int factor = 4 / denominator;
		numerator *= factor;
		denominator *= factor;
	}

	// Compose some general information: prefix, fraction, root, ring number
	QString prefix = (divisions == MediumResolution) ? "" : format ("%1\\", divisions);
	QString frac = format ("%1-%2", numerator, denominator);
	static const char* roots[] = {"edge", "cyli", "disc", "ndis", "ring", "con"};
	QString root = roots[type];
	QString numberString = (type == Ring or type == Cone) ? format ("%1", ringNumber) : "";

	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	if (style == LegacyStyleName)
	{
		int extra = (countof(frac) + countof(numberString) + countof(root)) - 8;
		root.chop(qBound(0, extra, 2));
	}

	// Stick them all together and return the result.
	return prefix + frac + root + numberString + ".dat";
}


LDDocument* PrimitiveManager::generatePrimitive(const PrimitiveModel& spec)
{
	// Make the description
	QString fraction = QString::number ((float) spec.segments / spec.divisions);
	QString fileName = spec.makeFileName(PrimitiveModel::NewStyleName);
	QString description;

	// Ensure that there's decimals, even if they're 0.
	if (fraction.indexOf(".") == -1)
		fraction += ".0";

	if (spec.type == PrimitiveModel::Ring or spec.type == PrimitiveModel::Cone)
	{
		QString spacing =
			(spec.ringNumber < 10) ? "  " :
			(spec.ringNumber < 100) ? " "  : "";
		description = format("%1 %2%3 x %4", PrimitiveModel::typeName(spec.type), spacing, spec.ringNumber, fraction);
	}
	else
		description = format("%1 %2", PrimitiveModel::typeName(spec.type), fraction);

	// Prepend "Hi-Res" or "Lo-Res" as appropriate.
	if (spec.divisions == HighResolution)
		description.insert (0, "Hi-Res ");
	else if (spec.divisions == LowResolution)
		description.insert (0, "Lo-Res ");

	LDDocument* document = m_window->newDocument();
	document->setDefaultName(fileName);

	if (not config::defaultName().isEmpty())
	{
		document->header.license = LDHeader::defaultLicense();
		document->header.author = format("%1 [%2]", config::defaultName(), config::defaultUser());
	}
	else
	{
		document->header.author = APPNAME;
	}

	document->setFrozen(false);
	document->header.name = fileName;
	document->header.description = description;

	if (spec.divisions == HighResolution)
		document->header.type = LDHeader::Primitive_48;
	else if (spec.divisions == LowResolution)
		document->header.type = LDHeader::Primitive_8;
	else
		document->header.type = LDHeader::Primitive;

	if (config::useCaLicense())
		document->header.license = LDHeader::CaLicense;
	else
		document->header.license =LDHeader::UnspecifiedLicense;

	document->setWinding(CounterClockwise);
	spec.generateBody(*document);
	document->history()->setIgnoring(false);
	return document;
}

/*
 * PrimitiveManager :: getPrimitive
 *
 * Gets a primitive by the given model. If the primitive cannot be found, it will be automatically generated.
 */
LDDocument* PrimitiveManager::getPrimitive(const PrimitiveModel& model)
{
	// Try find with the new style name.
	QString name = model.makeFileName(PrimitiveModel::NewStyleName);
	LDDocument* document = m_window->documents()->getDocumentByName(name);

	if (not document)
	{
		// Not found, try the legacy name
		QString name = model.makeFileName(PrimitiveModel::LegacyStyleName);
		document = m_window->documents()->getDocumentByName(name);
	}

	if (not document)
	{
		// Not found either, generate it.
		document = generatePrimitive(model);
		m_window->openDocumentForEditing(document);
	}

	return document;
}

/*
 * Returns the amount of columns in the primitives tree (1)
 */
int	PrimitiveManager::columnCount(const QModelIndex&) const
{
	return 1;
}

/*
 * For an index that points to a primitive, returns the category that contains it
 */
static PrimitiveCategory* categoryForPrimitiveIndex(const QModelIndex& primitiveIndex)
{
	return static_cast<PrimitiveCategory*>(primitiveIndex.internalPointer());
}

/*
 * Returns data from the tree model.
 */
QVariant PrimitiveManager::data(const QModelIndex& index, int role) const
{
	if (index.isValid())
	{
		if (categoryForPrimitiveIndex(index) != nullptr)
		{
			// Index points to a primitive, return primitive information.
			Primitive& primitive = categoryForPrimitiveIndex(index)->primitives[index.row()];

			switch(role)
			{
			case Qt::DisplayRole:
				return format("%1 - %2", primitive.name, primitive.title);

			case Qt::DecorationRole:
				return MainWindow::getIcon("subfilereference");

			case PrimitiveNameRole:
				return primitive.name;

			case PrimitiveDescriptionRole:
				return primitive.title;

			default:
				return {};
			}
		}
		else
		{
			// Index points to a category, return category information.
			PrimitiveCategory* category = this->m_categories[index.row()];

			switch (role)
			{
			case Qt::DisplayRole:
				return category->name();

			case Qt::DecorationRole:
				return MainWindow::getIcon("folder");

			default:
				return {};
			}
		}
	}
	else
	{
		// Index is invalid.
		return {};
	}
}

/*
 * For a row and parent index, returns a child index.
 */
QModelIndex PrimitiveManager::index(int row, int, const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		if (categoryForPrimitiveIndex(parent))
		{
			// Parent is a primitive index. Primitives cannot have children so return an
			// invalid index.
			return {};
		}
		else
		{
			// Parent is a category, return an index to a primitive
			PrimitiveCategory* category = m_categories[parent.row()];

			// Create an index inside the category
			if (row >= 0 and row < category->primitives.size())
				return this->createIndex(row, 0, category);
			else
				return {};
		}
	}
	else
	{
		// Create a top-level index pointing to a category
		if (row >= 0 and row < this->m_categories.size())
			return this->createIndex(row, 0, nullptr);
		else
			return {};
	}
}

/*
 * For a primitive index, find the category index that contains it.
 */
QModelIndex PrimitiveManager::parent(const QModelIndex &index) const
{
	int row = this->m_categories.indexOf(categoryForPrimitiveIndex(index));

	if (row != -1)
		return this->createIndex(row, 0, nullptr);
	else
		return {};
}

/*
 * Returns the amount of rows contained inside the given index.
 */
int PrimitiveManager::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		if (categoryForPrimitiveIndex(parent))
		{
			// Primitives don't have child nodes, so return 0.
			return 0;
		}
		else
		{
			// For categories, return the amount of primitives contained.
			return this->m_categories[parent.row()]->primitives.size();
		}
	}
	else
	{
		// For top-level, return the amount of categories.
		return this->m_categories.size();
	}
}

/*
 * Returns a static "Primitives" text for the header.
 */
QVariant PrimitiveManager::headerData(int section, Qt::Orientation, int role) const
{
	if (section == 0 and role == Qt::DisplayRole)
		return tr("Primitives");
	else
		return {};
}

//
// ---------------------------------------------------------------------------------------------------------------------
//


PrimitiveCategory::PrimitiveCategory (QString name, QObject* parent) :
	QObject (parent),
	m_name (name) {}


bool PrimitiveCategory::isValidToInclude()
{
	return not patterns.isEmpty();
}


QString PrimitiveCategory::name() const
{
	return m_name;
}


/*
 * PrimitiveScanner :: PrimitiveScanner
 *
 * Constructs a primitive scanner.
 */
PrimitiveScanner::PrimitiveScanner(PrimitiveManager* parent) :
	QObject(parent),
	HierarchyElement(parent),
	m_manager(parent)
{
	for (const Library& library : config::libraries())
	{
		QDir dir {library.path};
		if (dir.exists("p") and QFileInfo {dir.filePath("p")}.isDir())
		{
			directories.push(dir.filePath("p"));

			if (dir.exists("p/48") and QFileInfo {dir.filePath("p/48")}.isDir())
				directories.push(dir.filePath("p/48"));
		}
	}

	print("Scanning primitives...");
}

PrimitiveScanner::~PrimitiveScanner()
{
	delete this->currentIterator;
}

/*
 * PrimitiveScanner :: scannedPrimitives
 *
 * Returns a vector containing all the primitives found.
 */
const QVector<Primitive> &PrimitiveScanner::scannedPrimitives() const
{
	return m_scannedPrimitives;
}

/*
 * PrimitiveScanner :: work
 *
 * Does one step of work, processes up to 100 primitives.
 * If the scanner does not finish work by this function call, it will ask the event loop to call this method again.
 */
void PrimitiveScanner::work()
{
	while (this->currentIterator == nullptr or not this->currentIterator->hasNext())
	{
		delete this->currentIterator;
		this->currentIterator = nullptr;

		if (this->directories.isEmpty())
		{
			// If there are no more primitives to iterate, we're done. Now save this information into a cache file.
			std::sort(
				m_scannedPrimitives.begin(),
				m_scannedPrimitives.end(),
				[](const Primitive& one, const Primitive& other) -> bool
				{
					return one.title < other.title;
				}
			);
			QString path = m_manager->getPrimitivesCfgPath();
			QFile configFile = {path};

			if (configFile.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				for (Primitive& primitive : m_scannedPrimitives)
					fprint(configFile, "%1 %2\r\n", primitive.name, primitive.title);

				configFile.close();
			}
			else
			{
				QMessageBox::critical(
					m_window,
					tr("Error"),
					format(
						tr("Couldn't write primitive list %1: %2"),
						path,
						configFile.errorString()
					)
				);
			}

			emit workDone();
			return;
		}
		else
		{
			this->currentIterator = new QDirIterator {this->directories.pop()};
			this->m_basePathLength = this->currentIterator->path().length();
		}
	}

	for (int i = 0; this->currentIterator->hasNext() and i < 100; ++i)
	{
		QString filename = this->currentIterator->next();
		QFile file = {filename};

		if (file.open (QIODevice::ReadOnly))
		{
			Primitive primitive;
			primitive.name = LDDocument::shortenName(filename);
			primitive.category = nullptr;
			primitive.title = QString::fromUtf8(file.readLine().simplified());

			if (primitive.title[0] == '0')
			{
				primitive.title.remove(0, 1);  // remove 0
				primitive.title = primitive.title.trimmed();
			}

			m_scannedPrimitives << primitive;
		}
	}

	// Defer to event loop, pick up the work later.
	QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection);
}
