/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#include "gui.h"
#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlistwidget.h>

class ConfigDialog : public QDialog {
	Q_OBJECT
	
public:
	QTabWidget* qTabs;
	QWidget* qMainTab, *qShortcutsTab, *qQuickColorTab;
	
	// =========================================================================
	// Main tab widgets
	QLabel* qLDrawPathLabel;
	QLabel* qGLBackgroundLabel, *qGLForegroundLabel, *qGLForegroundAlphaLabel;
	QLabel* qGLLineThicknessLabel, *qToolBarIconSizeLabel;
	QLineEdit* qLDrawPath;
	QPushButton* qLDrawPathFindButton;
	QPushButton* qGLBackgroundButton, *qGLForegroundButton;
	QCheckBox* qLVColorize, *qGLColorBFC;
	QSlider* qGLForegroundAlpha, *qGLLineThickness, *qToolBarIconSize;
	
	// =========================================================================
	// Shortcuts tab
	QListWidget* qShortcutList;
	QPushButton* qSetShortcut, *qResetShortcut, *qClearShortcut;
	std::vector<QListWidgetItem*> qaShortcutItems;
	
	// =========================================================================
	// Quick color toolbar tab
	QListWidget* qQuickColorList;
	QPushButton* qAddColor, *qDelColor, *qChangeColor, *qAddColorSeparator,
		*qMoveColorUp, *qMoveColorDown, *qClearColors;
	std::vector<QListWidgetItem*> qaQuickColorItems;
	std::vector<quickColorMetaEntry> quickColorMeta;
	
	// =========================================================================
	QDialogButtonBox* qButtons;
	
	ConfigDialog (ForgeWindow* parent);
	~ConfigDialog ();
	static void staticDialog ();
	
private:
	void initMainTab ();
	void initShortcutsTab ();
	void initQuickColorTab ();
	
	void makeSlider (QSlider*& qSlider, short int dMin, short int dMax, short int dDefault);
	void setButtonBackground (QPushButton* qButton, str zValue);
	void pickColor (strconfig& cfg, QPushButton* qButton);
	void updateQuickColorList (quickColorMetaEntry* pSel = nullptr);
	void setShortcutText (QListWidgetItem* qItem, actionmeta meta);
	long getItemRow (QListWidgetItem* qItem, std::vector<QListWidgetItem*>& haystack);
	str makeColorToolBarString ();
	QListWidgetItem* getSelectedQuickColor ();
	
private slots:
	void slot_findLDrawPath ();
	void slot_setGLBackground ();
	void slot_setGLForeground ();
	
	void slot_setShortcut ();
	void slot_resetShortcut ();
	void slot_clearShortcut ();
	
	void slot_setColor ();
	void slot_delColor ();
	void slot_addColorSeparator ();
	void slot_moveColor ();
	void slot_clearColors ();
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
class KeySequenceDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit KeySequenceDialog (QKeySequence seq, QWidget* parent = nullptr, Qt::WindowFlags f = 0);
	static bool staticDialog (actionmeta& meta, QWidget* parent = nullptr);
	
	QLabel* qOutput;
	QDialogButtonBox* qButtons;
	QKeySequence seq;
	
private:
	void updateOutput ();
	
private slots:
	virtual void keyPressEvent (QKeyEvent* ev);
};