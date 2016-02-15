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

#include <QDir>
#include <QRegExp>
#include <QFileDialog>
#include "ldDocument.h"
#include "mainwindow.h"
#include "primitives.h"
#include "ui_makeprim.h"
#include "miscallenous.h"
#include "colors.h"
#include "ldpaths.h"
#include "documentmanager.h"

QList<PrimitiveCategory*> m_categories;

static const QStringList g_radialNameRoots =
{
	"edge",
	"cyli",
	"disc",
	"ndis",
	"ring",
	"con"
};

PrimitiveScanner* PrimitiveManager::activeScanner()
{
	return m_activeScanner;
}

QString PrimitiveManager::getPrimitivesCfgPath() const
{
	return qApp->applicationDirPath() + DIRSLASH "prims.cfg";
}

// =============================================================================
//
void PrimitiveManager::loadPrimitives()
{
	// Try to load prims.cfg
	QFile conf (getPrimitivesCfgPath());

	if (not conf.open (QIODevice::ReadOnly))
	{
		// No prims.cfg, build it
		startScan();
	}
	else
	{
		while (not conf.atEnd())
		{
			QString line = conf.readLine();

			if (line.endsWith ("\n"))
				line.chop (1);

			if (line.endsWith ("\r"))
				line.chop (1);

			int space = line.indexOf (" ");

			if (space == -1)
				continue;

			Primitive info;
			info.name = line.left (space);
			info.title = line.mid (space + 1);
			m_primitives << info;
		}

		populateCategories();
		print ("%1 primitives loaded.\n", m_primitives.size());
	}
}

// =============================================================================
//
// TODO: replace with QDirIterator
//
static void GetRecursiveFilenames (QDir dir, QList<QString>& fnames) __attribute__((deprecated));
static void GetRecursiveFilenames (QDir dir, QList<QString>& fnames)
{
	QFileInfoList flist = dir.entryInfoList (QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

	for (const QFileInfo& info : flist)
	{
		if (info.isDir())
			GetRecursiveFilenames (QDir (info.absoluteFilePath()), fnames);
		else
			fnames << info.absoluteFilePath();
	}
}

// =============================================================================
//
PrimitiveScanner::PrimitiveScanner (PrimitiveManager* parent) :
	QObject(parent),
	HierarchyElement(parent),
	m_manager(parent),
	m_i(0)
{
	QDir dir = LDPaths::primitivesDir();
	m_baselen = dir.absolutePath().length();
	GetRecursiveFilenames(dir, m_files);
	emit starting(m_files.size());
	print("Scanning primitives...");
}

const QList<Primitive>& PrimitiveScanner::scannedPrimitives() const
{
	return m_prims;
}

// =============================================================================
//
void PrimitiveScanner::work()
{
	int max = qMin (m_i + 100, m_files.size());

	for (; m_i < max; ++m_i)
	{
		QString filename = m_files[m_i];
		QFile file (filename);

		if (not file.open (QIODevice::ReadOnly))
			continue;

		Primitive info;
		info.name = filename.mid (m_baselen + 1);  // make full path relative
		info.name.replace ('/', '\\');  // use DOS backslashes, they're expected
		info.category = nullptr;
		QByteArray titledata = file.readLine();

		if (titledata != QByteArray())
			info.title = QString::fromUtf8 (titledata);

		info.title = info.title.simplified();

		if (info.title[0] == '0')
		{
			info.title.remove (0, 1);  // remove 0
			info.title = info.title.simplified();
		}

		m_prims << info;
	}

	if (m_i == m_files.size())
	{
		// Done with primitives, now save to a config file
		QString path = m_manager->getPrimitivesCfgPath();
		QFile configFile (path);

		if (configFile.open (QIODevice::WriteOnly | QIODevice::Text))
		{
			for (Primitive& info : m_prims)
				fprint (configFile, "%1 %2\r\n", info.name, info.title);

			configFile.close();
		}
		else
		{
			errorPrompt(m_window, format("Couldn't write primitive list %1: %2", path, configFile.errorString()));
		}

		emit workDone();
	}
	else
	{
		// Defer to event loop, pick up the work later
		emit update (m_i);
		QMetaObject::invokeMethod (this, "work", Qt::QueuedConnection);
	}
}

// =============================================================================
//
void PrimitiveManager::startScan()
{
	if (m_activeScanner == nullptr)
	{
		loadCategories();
		m_activeScanner = new PrimitiveScanner(this);
		m_activeScanner->work();
		connect(m_activeScanner, &PrimitiveScanner::workDone, this, &PrimitiveManager::scanDone);
	}
}

void PrimitiveManager::scanDone()
{
	if (m_activeScanner)
	{
		m_primitives = m_activeScanner->scannedPrimitives();
		populateCategories();
		print ("%1 primitives scanned", m_primitives.size());
		delete m_activeScanner;
		m_activeScanner = nullptr;
	}
}

// =============================================================================
//
PrimitiveCategory::PrimitiveCategory (QString name, QObject* parent) :
	QObject (parent),
	m_name (name) {}

// =============================================================================
//
void PrimitiveManager::clearCategories()
{
	for (PrimitiveCategory* category : m_categories)
		delete category;

	m_categories.clear();
}

// =============================================================================
//
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

	// Sort the categories. Note that we do this here because we need the existing
	// order for regex matching.
	qSort (m_categories.begin(), m_categories.end(),
		[](PrimitiveCategory* const& a, PrimitiveCategory* const& b) -> bool
		{
			return a->name() < b->name();
		});
}

// =============================================================================
//
void PrimitiveManager::loadCategories()
{
	clearCategories();
	QString path = ":/data/primitive-categories.cfg";
	QFile f (path);

	if (not f.open (QIODevice::ReadOnly))
	{
		Critical (format (QObject::tr ("Failed to open primitive categories: %1"), f.errorString()));
		return;
	}

	PrimitiveCategory* category = nullptr;

	while (not f.atEnd())
	{
		QString line = QString::fromUtf8(f.readLine()).trimmed();

		if (line.length() == 0 or line[0] == '#')
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
				print (tr ("Warning: unknown pattern type \"%1\" on line \"%2\""), typechar, line);
				continue;
			}

			QRegExp regex (line.mid (colon + 1));
			PrimitiveCategory::RegexEntry entry = { regex, type };
			category->patterns << entry;
		}
		else
		{
			print ("Warning: Rules given before the first category name");
		}
	}

	if (category->isValidToInclude())
		m_categories << category;

	// Add a category for unmatched primitives.
	// Note: if this function is called the second time, g_unmatched has been
	// deleted at the beginning of the function and is dangling at this point.
	m_unmatched = new PrimitiveCategory (tr ("Other"));
	m_categories << m_unmatched;
	f.close();
}

// =============================================================================
//
bool PrimitiveCategory::isValidToInclude()
{
	return not patterns.isEmpty();
}

QString PrimitiveCategory::name() const
{
	return m_name;
}

// =============================================================================
//
static double getRadialPoint (int i, int divs, double (*func) (double))
{
	return (*func) ((i * 2 * Pi) / divs);
}

// =============================================================================
//
// TODO: this doesn't really belong here.
//
void PrimitiveManager::makeCircle (int segs, int divs, double radius, QList<QLineF>& lines)
{
	for (int i = 0; i < segs; ++i)
	{
		double x0 = radius * getRadialPoint (i, divs, cos),
			x1 = radius * getRadialPoint (i + 1, divs, cos),
			z0 = radius * getRadialPoint (i, divs, sin),
			z1 = radius * getRadialPoint (i + 1, divs, sin);

		lines << QLineF (QPointF (x0, z0), QPointF (x1, z1));
	}
}

// =============================================================================
//
LDObjectList PrimitiveManager::makePrimitiveBody (PrimitiveType type, int segs, int divs, int num)
{
	LDObjectList objs;
	QList<int> conditionalLineSegments;
	QList<QLineF> circle;

	makeCircle (segs, divs, 1, circle);

	for (int i = 0; i < segs; ++i)
	{
		double x0 = circle[i].x1();
		double x1 = circle[i].x2();
		double z0 = circle[i].y1();
		double z1 = circle[i].y2();

		switch (type)
		{
		case Circle:
			{
				Vertex v0 (x0, 0.0f, z0),
				  v1 (x1, 0.0f, z1);

				LDLine* line (LDSpawn<LDLine>());
				line->setVertex (0, v0);
				line->setVertex (1, v1);
				line->setColor (EdgeColor);
				objs << line;
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

					y0 = y1 = 0.0f;
					y2 = y3 = 1.0f;
				}
				else
				{
					x2 = x1 * (num + 1);
					x3 = x0 * (num + 1);
					z2 = z1 * (num + 1);
					z3 = z0 * (num + 1);

					x0 *= num;
					x1 *= num;
					z0 *= num;
					z1 *= num;

					if (type == Ring)
						y0 = y1 = y2 = y3 = 0.0f;
					else
					{
						y0 = y1 = 1.0f;
						y2 = y3 = 0.0f;
					}
				}

				Vertex v0 (x0, y0, z0);
				Vertex v1 (x1, y1, z1);
				Vertex v2 (x2, y2, z2);
				Vertex v3 (x3, y3, z3);

				LDQuad* quad (LDSpawn<LDQuad> (v0, v1, v2, v3));
				quad->setColor (MainColor);

				if (type == Cylinder)
					quad->invert();

				objs << quad;

				if (type == Cylinder or type == Cone)
					conditionalLineSegments << i;
			}
			break;

		case Disc:
		case DiscNegative:
			{
				double x2, z2;

				if (type == Disc)
					x2 = z2 = 0.0f;
				else
				{
					x2 = (x0 >= 0.0f) ? 1.0f : -1.0f;
					z2 = (z0 >= 0.0f) ? 1.0f : -1.0f;
				}

				Vertex v0 (x0, 0.0f, z0),
					   v1 (x1, 0.0f, z1),
					   v2 (x2, 0.0f, z2);

				// Disc negatives need to go the other way around, otherwise
				// they'll end up upside-down.
				LDTriangle* seg (LDSpawn<LDTriangle>());
				seg->setColor (MainColor);
				seg->setVertex (type == Disc ? 0 : 2, v0);
				seg->setVertex (1, v1);
				seg->setVertex (type == Disc ? 2 : 0, v2);
				objs << seg;
			}
			break;
		}
	}

	// If this is not a full circle, we need a conditional line at the other
	// end, too.
	if (segs < divs and not conditionalLineSegments.isEmpty())
		conditionalLineSegments << segs;

	for (int i : conditionalLineSegments)
	{
		Vertex v0 (getRadialPoint (i, divs, cos), 0.0f, getRadialPoint (i, divs, sin));
		Vertex v1;
		Vertex v2 (getRadialPoint (i + 1, divs, cos), 0.0f, getRadialPoint (i + 1, divs, sin));
		Vertex v3 (getRadialPoint (i - 1, divs, cos), 0.0f, getRadialPoint (i - 1, divs, sin));

		if (type == Cylinder)
		{
			v1 = Vertex (v0[X], 1.0f, v0[Z]);
		}
		else if (type == Cone)
		{
			v1 = Vertex (v0[X] * (num + 1), 0.0f, v0[Z] * (num + 1));
			v0.setX (v0.x() * num);
			v0.setY (1.0);
			v0.setZ (v0.z() * num);
		}

		LDCondLine* line = (LDSpawn<LDCondLine>());
		line->setColor (EdgeColor);
		line->setVertex (0, v0);
		line->setVertex (1, v1);
		line->setVertex (2, v2);
		line->setVertex (3, v3);
		objs << line;
	}

	return objs;
}

// =============================================================================
//
QString PrimitiveManager::primitiveTypeName (PrimitiveType type)
{
	// Not translated as primitives are in English.
	const char* names[] = { "Circle", "Cylinder", "Disc", "Disc Negative", "Ring", "Cone" };

	if (type >= 0 and type < countof(names))
		return names[type];
	else
		return "Unknown";
}

// =============================================================================
//
QString PrimitiveManager::makeRadialFileName (PrimitiveType type, int segs, int divs, int num)
{
	int numerator = segs;
	int denominator = divs;

	// Simplify the fractional part, but the denominator must be at least 4.
	simplify (numerator, denominator);

	if (denominator < 4)
	{
		const int factor = 4 / denominator;
		numerator *= factor;
		denominator *= factor;
	}

	// Compose some general information: prefix, fraction, root, ring number
	QString prefix = (divs == LowResolution) ? "" : format ("%1/", divs);
	QString frac = format ("%1-%2", numerator, denominator);
	QString root = g_radialNameRoots[type];
	QString numstr = (type == Ring or type == Cone) ? format ("%1", num) : "";

	// Truncate the root if necessary (7-16rin4.dat for instance).
	// However, always keep the root at least 2 characters.
	int extra = (frac.length() + numstr.length() + root.length()) - 8;
	root.chop (qBound (0, extra, 2));

	// Stick them all together and return the result.
	return prefix + frac + root + numstr + ".dat";
}

// =============================================================================
//
LDDocument* PrimitiveManager::generatePrimitive (PrimitiveType type, int segments, int divisions, int number)
{
	// Make the description
	QString fraction = QString::number ((float) segments / divisions);
	QString name = makeRadialFileName (type, segments, divisions, number);
	QString description;

	// Ensure that there's decimals, even if they're 0.
	if (fraction.indexOf (".") == -1)
		fraction += ".0";

	if (type == Ring or type == Cone)
	{
		QString spacing =
			(number < 10) ? "  " :
			(number < 100) ? " "  : "";

		description = format ("%1 %2%3 x %4", primitiveTypeName (type), spacing, number, fraction);
	}
	else
		description = format ("%1 %2", primitiveTypeName (type), fraction);

	// Prepend "Hi-Res" if 48/ primitive.
	if (divisions == HighResolution)
		description.insert (0, "Hi-Res ");

	LDDocument* document = m_window->newDocument();
	document->setDefaultName (name);

	QString author = APPNAME;
	QString license = "";

	if (not m_config->defaultName().isEmpty())
	{
		license = PreferredLicenseText();
		author = format ("%1 [%2]", m_config->defaultName(), m_config->defaultUser());
	}

	LDObjectList objs;

	objs << LDSpawn<LDComment> (description)
		 << LDSpawn<LDComment> (format ("Name: %1", name))
		 << LDSpawn<LDComment> (format ("Author: %1", author))
		 << LDSpawn<LDComment> (format ("!LDRAW_ORG Unofficial_%1Primitive", divisions == HighResolution ?  "48_" : ""))
		 << LDSpawn<LDComment> (license)
		 << LDSpawn<LDEmpty>()
		 << LDSpawn<LDBfc> (BfcStatement::CertifyCCW)
		 << LDSpawn<LDEmpty>();

	document->openForEditing();
	document->history()->setIgnoring (false);
	document->addObjects (objs);
	document->addObjects (makePrimitiveBody (type, segments, divisions, number));
	document->addHistoryStep();
	return document;
}

// =============================================================================
//
// Gets a primitive by the given specs. If the primitive cannot be found, it will
// be automatically generated.
//
LDDocument* PrimitiveManager::getPrimitive (PrimitiveType type, int segs, int divs, int num)
{
	QString name = makeRadialFileName (type, segs, divs, num);
	LDDocument* document = m_window->documents()->getDocumentByName (name);

	if (document)
		return document;

	return generatePrimitive (type, segs, divs, num);
}

// =============================================================================
//
PrimitivePrompt::PrimitivePrompt (QWidget* parent, Qt::WindowFlags f) :
	QDialog (parent, f)
{
	ui = new Ui_MakePrimUI;
	ui->setupUi (this);
	connect (ui->cb_hires, SIGNAL (toggled (bool)), this, SLOT (hiResToggled (bool)));
}

// =============================================================================
//
PrimitivePrompt::~PrimitivePrompt()
{
	delete ui;
}

// =============================================================================
//
void PrimitivePrompt::hiResToggled (bool on)
{
	ui->sb_segs->setMaximum (on ? HighResolution : LowResolution);

	// If the current value is 16 and we switch to hi-res, default the
	// spinbox to 48.
	if (on and ui->sb_segs->value() == LowResolution)
		ui->sb_segs->setValue (HighResolution);
}

// =============================================================================
//
PrimitiveManager::PrimitiveManager(QObject* parent) :
	QObject(parent),
	HierarchyElement(parent),
	m_activeScanner(nullptr),
	m_unmatched(nullptr) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void PrimitiveManager::populateTreeWidget(QTreeWidget* tree, const QString& selectByDefault)
{
	tree->clear();

	for (PrimitiveCategory* category : m_categories)
	{
		PrimitiveTreeItem* parentItem = new PrimitiveTreeItem (tree, nullptr);
		parentItem->setText (0, category->name());
		QList<QTreeWidgetItem*> subfileItems;

		for (Primitive& prim : category->primitives)
		{
			PrimitiveTreeItem* item = new PrimitiveTreeItem (parentItem, &prim);
			item->setText (0, format ("%1 - %2", prim.name, prim.title));
			subfileItems << item;

			// If this primitive is the one the current object points to,
			// select it by default
			if (selectByDefault == prim.name)
				tree->setCurrentItem (item);
		}

		tree->addTopLevelItem (parentItem);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
//
PrimitiveTreeItem::PrimitiveTreeItem (QTreeWidgetItem* parent, Primitive* info) :
	QTreeWidgetItem (parent),
	m_primitive (info) {}

// ---------------------------------------------------------------------------------------------------------------------
//
PrimitiveTreeItem::PrimitiveTreeItem (QTreeWidget* parent, Primitive* info) :
	QTreeWidgetItem (parent),
	m_primitive (info) {}

// ---------------------------------------------------------------------------------------------------------------------
//
Primitive* PrimitiveTreeItem::primitive() const
{
	return m_primitive;
}
