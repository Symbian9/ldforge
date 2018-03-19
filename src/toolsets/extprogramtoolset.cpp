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

#include <QProcess>
#include <QTemporaryFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QSettings>
#include <QFileInfo>
#include "../guiutilities.h"
#include "../main.h"
#include "../miscallenous.h"
#include "../mainwindow.h"
#include "../lddocument.h"
#include "../editHistory.h"
#include "../documentmanager.h"
#include "../grid.h"
#include "../parser.h"
#include "../dialogs/externalprogrampathdialog.h"
#include "extprogramtoolset.h"
#include "ui_ytruderdialog.h"
#include "ui_intersectordialog.h"
#include "ui_rectifierdialog.h"
#include "ui_covererdialog.h"
#include "ui_isecalcdialog.h"
#include "ui_edger2dialog.h"

ExtProgramToolset::ExtProgramToolset (MainWindow* parent) :
	Toolset (parent)
{
	extProgramInfo[Isecalc].name = "Isecalc";
	extProgramInfo[Intersector].name = "Intersector";
	extProgramInfo[Coverer].name = "Coverer";
	extProgramInfo[Ytruder].name = "Ytruder";
	extProgramInfo[Rectifier].name = "Rectifier";
	extProgramInfo[Edger2].name = "Edger2";
}

bool ExtProgramToolset::makeTempFile (QTemporaryFile& tmp, QString& fname)
{
	if (not tmp.open())
		return false;

	fname = tmp.fileName();
	tmp.close();
	return true;
}

bool ExtProgramToolset::programUsesWine (ExtProgramType program)
{
#ifndef Q_OS_WIN32
	return getWineSetting (program);
#else
	return false;
#endif
}

bool ExtProgramToolset::getWineSetting (ExtProgramType program)
{
	return m_window->getConfigValue (externalProgramName (program) + "UsesWine").toBool();
}

QString ExtProgramToolset::getPathSetting (ExtProgramType program)
{
	return m_window->getConfigValue (externalProgramName (program) + "Path").toString();
}

void ExtProgramToolset::setPathSetting (ExtProgramType program, QString value)
{
	m_window->getSettings()->setValue (externalProgramName (program) + "Path", QVariant::fromValue (value));
}

void ExtProgramToolset::setWineSetting (ExtProgramType program, bool value)
{
	m_window->getSettings()->setValue (externalProgramName (program) + "UsesWine", QVariant::fromValue (value));
}

QString ExtProgramToolset::externalProgramName (ExtProgramType program)
{
	return extProgramInfo[program].name;
}

bool ExtProgramToolset::checkExtProgramPath (ExtProgramType program)
{
	QString path = getPathSetting (program);

	if (not path.isEmpty())
		return true;

	ExternalProgramPathDialog* dialog = new ExternalProgramPathDialog (externalProgramName (program));

	if (dialog->exec() and not dialog->path().isEmpty())
	{
		setPathSetting (program, dialog->path());
		return true;
	}

	return false;
}

// =============================================================================
//
QString ExtProgramToolset::errorCodeString (ExtProgramType program, QProcess& process)
{
	switch (process.error())
	{
	case QProcess::FailedToStart:
		if (programUsesWine (program))
			return tr ("Program failed to start, make sure that Wine is installed and check your permissions.");

		return tr ("Program failed to start, %1check your permissions");

	case QProcess::Crashed:
		return tr ("Crashed.");

	case QProcess::WriteError:
	case QProcess::ReadError:
		return tr ("I/O error.");

	case QProcess::UnknownError:
		return tr ("Unknown error");

	case QProcess::Timedout:
		return tr ("Timed out (30 seconds)");
	}

	return "";
}

// =============================================================================
//
void ExtProgramToolset::writeObjects (const QVector<LDObject*>& objects, QFile& f)
{
	for (LDObject* obj : objects)
	{
		if (obj->type() == LDObjectType::SubfileReference)
		{
			LDSubfileReference* ref = static_cast<LDSubfileReference*> (obj);
			Model model {m_documents};
			ref->inlineContents(m_documents, CounterClockwise, model, true, false);
			writeObjects(model.objects(), f);
		}
		else if (obj->type() == LDObjectType::BezierCurve)
		{
			LDBezierCurve* curve = static_cast<LDBezierCurve*> (obj);
			Model model {m_documents};
			curve->rasterize(model, grid()->bezierCurveSegments());
			writeObjects(model.objects(), f);
		}
		else
			f.write ((obj->asText() + "\r\n").toUtf8());
	}
}

// =============================================================================
//
void ExtProgramToolset::writeObjects (const QVector<LDObject*>& objects, QString filename)
{
	// Write the input file
	QFile f (filename);

	if (not f.open (QIODevice::WriteOnly | QIODevice::Text))
	{
		QString message = format(tr("Couldn't open temporary file %1 for writing: %2\n"), filename, f.errorString());
		QMessageBox::critical(m_window, tr("Error"), message);
		return;
	}

	writeObjects (objects, f);
	f.close();

#ifdef DEBUG
	QFile::copy (filename, "debug_lastInput");
#endif
}

// =============================================================================
//
void ExtProgramToolset::writeSelection (QString fname)
{
	writeObjects(selectedObjects().toList().toVector(), fname);
}

// =============================================================================
//
void ExtProgramToolset::writeColorGroup (LDColor color, QString fname)
{
	QVector<LDObject*> objects;

	for (LDObject* obj : currentDocument()->objects())
	{
		if (not obj->isColored() or obj->color() != color)
			continue;

		objects << obj;
	}

	writeObjects (objects, fname);
}

// =============================================================================
//
bool ExtProgramToolset::runExtProgram (ExtProgramType program, QString argvstr)
{
	QString path = getPathSetting (program);
	QTemporaryFile input;
	QStringList argv = argvstr.split (" ", QString::SkipEmptyParts);

#ifndef Q_OS_WIN32
	if (programUsesWine (program))
	{
		argv.insert (0, path);
		path = "wine";
	}
#endif // Q_OS_WIN32

	print ("Running command: %1 %2\n", path, argv.join (" "));

	if (not input.open())
		return false;

	QProcess process;

	// Begin!
	process.setStandardInputFile (input.fileName());
	process.start (path, argv);

	if (not process.waitForStarted())
	{
		QString message = format("Couldn't start %1: %2\n", externalProgramName(program), errorCodeString(program, process));
		QMessageBox::critical(m_window, tr("Error running external program"), message);
		return false;
	}

	// Write an enter, the utility tools all expect one
	input.write ("\n");

	// Wait while it runs
	process.waitForFinished();

	QString errorMessage = "";

	if (process.exitStatus() != QProcess::NormalExit)
		errorMessage = errorCodeString (program, process);

	// Check the return code
	if (process.exitCode() != 0)
		errorMessage = format ("Program exited abnormally (return code %1).",  process.exitCode());

	if (not errorMessage.isEmpty())
	{
		QString message = format(tr("%1 failed: %2\n"), externalProgramName(program), errorMessage);
		QMessageBox::critical(m_window, tr("External program failed"), message);
		QString filename ("externalProgramOutput.txt");
		QFile file (filename);

		if (file.open (QIODevice::WriteOnly | QIODevice::Text))
		{
			file.write (process.readAllStandardOutput());
			file.write (process.readAllStandardError());
			print ("Wrote output and error logs to %1", QFileInfo (file).absoluteFilePath());
		}
		else
		{
			print ("Couldn't open %1 for writing: %2",
				QFileInfo (filename).absoluteFilePath(), file.errorString());
		}

		return false;
	}

	return true;
}

// =============================================================================
//
void ExtProgramToolset::insertOutput (QString fname, bool replace, QList<LDColor> colorsToReplace)
{
#ifdef DEBUG
	QFile::copy (fname, "./debug_lastOutput");
#endif

	// Read the output file
	QFile f (fname);

	if (not f.open (QIODevice::ReadOnly))
	{
		QString message = format(tr("Couldn't open temporary file %1 for reading.\n"), fname);
		QMessageBox::critical(m_window, tr("Error running external program"), message);
		return;
	}

	Model model {m_documents};
	Parser parser {f};
	parser.parseBody(model);

	// If we replace the objects, delete the selection now.
	if (replace)
		m_window->deleteSelection();

	for (LDColor color : colorsToReplace)
		m_window->deleteByColor (color);

	// Insert the new objects
	mainWindow()->clearSelection();

	for (LDObject* object : model.objects())
	{
		if (object->isScemantic())
			currentDocument()->insertCopy(currentDocument()->size(), object);
	}

	m_window->doFullRefresh();
}

// =============================================================================
// Interface for Ytruder
// =============================================================================
void ExtProgramToolset::ytruder()
{
	setlocale (LC_ALL, "C");

	if (not checkExtProgramPath (Ytruder))
		return;

	QDialog* dlg = new QDialog;
	Ui::YtruderUI ui;
	ui.setupUi (dlg);

	if (not dlg->exec())
		return;

	// Read the user's choices
	const enum { Distance, Symmetry, Projection, Radial } mode =
		ui.mode_distance->isChecked()   ? Distance :
		ui.mode_symmetry->isChecked()   ? Symmetry :
		ui.mode_projection->isChecked() ? Projection : Radial;

	const Axis axis =
		ui.axis_x->isChecked() ? X :
		ui.axis_y->isChecked() ? Y : Z;

	const double depth = ui.planeDepth->value(),
				 condAngle = ui.condAngle->value();

	QTemporaryFile indat, outdat;
	QString inDATName, outDATName;

	// Make temp files for the input and output files
	if (not makeTempFile (indat, inDATName) or not makeTempFile (outdat, outDATName))
		return;

	// Compose the command-line arguments
	QString argv = joinStrings (
	{
		(axis == X) ? "-x" : (axis == Y) ? "-y" : "-z",
		(mode == Distance) ? "-d" : (mode == Symmetry) ? "-s" : (mode == Projection) ? "-p" : "-r",
		depth,
		"-a",
		condAngle,
		inDATName,
		outDATName
	});

	writeSelection (inDATName);

	if (not runExtProgram (Ytruder, argv))
		return;

	insertOutput (outDATName, false, {});
}

// =============================================================================
// Rectifier interface
// =============================================================================
void ExtProgramToolset::rectifier()
{
	setlocale (LC_ALL, "C");

	if (not checkExtProgramPath (Rectifier))
		return;

	QDialog* dlg = new QDialog;
	Ui::RectifierUI ui;
	ui.setupUi (dlg);

	if (not dlg->exec())
		return;

	QTemporaryFile indat, outdat;
	QString inDATName, outDATName;

	// Make temp files for the input and output files
	if (not makeTempFile (indat, inDATName) or not makeTempFile (outdat, outDATName))
		return;

	// Compose arguments
	QString argv = joinStrings (
	{
		(not ui.cb_condense->isChecked()) ? "-q" : "",
		(not ui.cb_subst->isChecked()) ? "-r" : "",
		(ui.cb_condlineCheck->isChecked()) ? "-a" : "",
		(ui.cb_colorize->isChecked()) ? "-c" : "",
		"-t",
		ui.dsb_coplthres->value(),
		inDATName,
		outDATName
	});

	writeSelection (inDATName);

	if (not runExtProgram (Rectifier, argv))
		return;

	insertOutput (outDATName, true, {});
}

// =============================================================================
// Intersector interface
// =============================================================================
void ExtProgramToolset::intersector()
{
	setlocale (LC_ALL, "C");

	if (not checkExtProgramPath (Intersector))
		return;

	QDialog* dlg = new QDialog;
	Ui::IntersectorUI ui;
	ui.setupUi (dlg);
	guiUtilities()->fillUsedColorsToComboBox (ui.cmb_incol);
	guiUtilities()->fillUsedColorsToComboBox (ui.cmb_cutcol);
	ui.cb_repeat->setWhatsThis ("If this is set, " APPNAME " runs Intersector a second time with inverse files to cut "
								" the cutter group with the input group. Both groups are cut by the intersection.");
	ui.cb_edges->setWhatsThis ("Makes " APPNAME " try run Isecalc to create edgelines for the intersection.");

	LDColor inCol, cutCol;
	const bool repeatInverse = ui.cb_repeat->isChecked();

	if (not dlg->exec())
		return;

	inCol = ui.cmb_incol->itemData (ui.cmb_incol->currentIndex()).toInt();
	cutCol = ui.cmb_cutcol->itemData (ui.cmb_cutcol->currentIndex()).toInt();

	// Five temporary files!
	// indat = input group file
	// cutdat = cutter group file
	// outdat = primary output
	// outdat2 = inverse output
	// edgesdat = edges output (isecalc)
	QTemporaryFile indat, cutdat, outdat, outdat2, edgesdat;
	QString inDATName, cutDATName, outDATName, outDAT2Name, edgesDATName;

	if (not makeTempFile (indat, inDATName) or
		not makeTempFile (cutdat, cutDATName) or
		not makeTempFile (outdat, outDATName) or
		not makeTempFile (outdat2, outDAT2Name) or
		not makeTempFile (edgesdat, edgesDATName))
	{
		return;
	}

	QString parms = joinStrings (
	{
		(ui.cb_colorize->isChecked()) ? "-c" : "",
		(ui.cb_nocondense->isChecked()) ? "-t" : "",
		"-s",
		ui.dsb_prescale->value()
	});

	QString argv_normal = joinStrings (
	{
		parms,
		inDATName,
		cutDATName,
		outDATName
	});

	QString argv_inverse = joinStrings (
	{
		parms,
		cutDATName,
		inDATName,
		outDAT2Name
	});

	writeColorGroup (inCol, inDATName);
	writeColorGroup (cutCol, cutDATName);

	if (not runExtProgram (Intersector, argv_normal))
		return;

	insertOutput (outDATName, false, {inCol});

	if (repeatInverse and runExtProgram (Intersector, argv_inverse))
		insertOutput (outDAT2Name, false, {cutCol});

	if (ui.cb_edges->isChecked()
		and checkExtProgramPath (Isecalc)
		and runExtProgram (Isecalc, joinStrings ({inDATName, cutDATName, edgesDATName})))
	{
		insertOutput (edgesDATName, false, {});
	}
}

// =============================================================================
//
void ExtProgramToolset::coverer()
{
	setlocale (LC_ALL, "C");

	if (not checkExtProgramPath (Coverer))
		return;

	QDialog* dlg = new QDialog;
	Ui::CovererUI ui;
	ui.setupUi (dlg);
	guiUtilities()->fillUsedColorsToComboBox (ui.cmb_col1);
	guiUtilities()->fillUsedColorsToComboBox (ui.cmb_col2);

	if (not dlg->exec())
		return;

	LDColor in1Col = ui.cmb_col1->itemData (ui.cmb_col1->currentIndex()).toInt();
	LDColor in2Col = ui.cmb_col2->itemData (ui.cmb_col2->currentIndex()).toInt();
	QTemporaryFile in1dat, in2dat, outdat;
	QString in1DATName, in2DATName, outDATName;

	if (not makeTempFile (in1dat, in1DATName) or
		not makeTempFile (in2dat, in2DATName) or
		not makeTempFile (outdat, outDATName))
	{
		return;
	}

	QString argv = joinStrings (
	{
		(ui.cb_oldsweep->isChecked() ? "-s" : ""),
		(ui.cb_reverse->isChecked() ? "-r" : ""),
		(ui.dsb_segsplit->value() != 0 ? format ("-l %1", ui.dsb_segsplit->value()) : ""),
		(ui.sb_bias->value() != 0 ? format ("-s %1", ui.sb_bias->value()) : ""),
		in1DATName,
		in2DATName,
		outDATName
	});

	writeColorGroup (in1Col, in1DATName);
	writeColorGroup (in2Col, in2DATName);

	if (not runExtProgram (Coverer, argv))
		return;

	insertOutput (outDATName, false, {});
}

// =============================================================================
//
void ExtProgramToolset::isecalc()
{
	setlocale (LC_ALL, "C");

	if (not checkExtProgramPath (Isecalc))
		return;

	Ui::IsecalcUI ui;
	QDialog* dlg = new QDialog;
	ui.setupUi (dlg);

	guiUtilities()->fillUsedColorsToComboBox (ui.cmb_col1);
	guiUtilities()->fillUsedColorsToComboBox (ui.cmb_col2);

	LDColor in1Col, in2Col;

	if (not dlg->exec())
		return;

	in1Col = ui.cmb_col1->itemData (ui.cmb_col1->currentIndex()).toInt();
	in2Col = ui.cmb_col2->itemData (ui.cmb_col2->currentIndex()).toInt();

	QTemporaryFile in1dat, in2dat, outdat;
	QString in1DATName, in2DATName, outDATName;

	if (not makeTempFile (in1dat, in1DATName) or
		not makeTempFile (in2dat, in2DATName) or
		not makeTempFile (outdat, outDATName))
	{
		return;
	}

	QString argv = joinStrings (
	{
		in1DATName,
		in2DATName,
		outDATName
	});

	writeColorGroup (in1Col, in1DATName);
	writeColorGroup (in2Col, in2DATName);
	runExtProgram (Isecalc, argv);
	insertOutput (outDATName, false, {});
}

// =============================================================================
//
void ExtProgramToolset::edger2()
{
	setlocale (LC_ALL, "C");

	if (not checkExtProgramPath (Edger2))
		return;

	QDialog* dlg = new QDialog;
	Ui::Edger2Dialog ui;
	ui.setupUi (dlg);

	if (not dlg->exec())
		return;

	QTemporaryFile in, out;
	QString inName, outName;

	if (not makeTempFile (in, inName) or not makeTempFile (out, outName))
		return;

	int unmatched = ui.unmatched->currentIndex();

	QString argv = joinStrings (
	{
		format ("-p %1", ui.precision->value()),
		format ("-af %1", ui.flatAngle->value()),
		format ("-ac %1", ui.condAngle->value()),
		format ("-ae %1", ui.edgeAngle->value()),
		ui.delLines->isChecked()     ? "-de" : "",
		ui.delCondLines->isChecked() ? "-dc" : "",
		ui.colored->isChecked()      ? "-c" : "",
		ui.bfc->isChecked()          ? "-b" : "",
		ui.convex->isChecked()       ? "-cx" : "",
		ui.concave->isChecked()      ? "-cv" : "",
		unmatched == 0 ? "-u+" : (unmatched == 2 ? "-u-" : ""),
		inName,
		outName,
	});

	writeSelection (inName);

	if (not runExtProgram (Edger2, argv))
		return;

	insertOutput (outName, true, {});
}
