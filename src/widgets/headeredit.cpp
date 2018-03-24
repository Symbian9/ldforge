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

#include "headeredit.h"
#include "ui_headeredit.h"
#include "../lddocument.h"
#include "../headerhistorymodel.h"

static const QStringList categories {
	"",
	"Animal", "Antenna", "Arch", "Arm", "Bar", "Baseplate", "Belville", "Boat", "Bracket",
	"Brick", "Canvas", "Car", "Clikits", "Cockpit", "Cone", "Constraction",
	"Constraction Accessory", "Container", "Conveyor", "Crane", "Cylinder", "Dish", "Door",
	"Electric", "Exhaust", "Fence", "Figure", "Figure Accessory", "Flag", "Forklift", "Freestyle",
	"Garage", "Glass", "Grab", "Hinge", "Homemaker", "Hose", "Ladder", "Lever", "Magnet", "Minifig",
	"Minifig Accessory", "Minifig Footwear", "Minifig Headwear", "Minifig Hipwear",
	"Minifig Neckwear", "Monorail", "Panel", "Plane", "Plant", "Plate", "Platform", "Propellor",
	"Rack", "Roadsign", "Rock", "Scala", "Screw", "Sheet", "Slope", "Sphere", "Staircase",
	"Sticker", "Support", "Tail", "Tap", "Technic", "Tile", "Tipper", "Tractor", "Trailer",
	"Train", "Turntable", "Tyre", "Vehicle", "Wedge", "Wheel", "Winch", "Window", "Windscreen",
	"Wing", "Znap",
};

HeaderEdit::HeaderEdit(QWidget* parent) :
	QWidget {parent},
	ui {*new Ui_HeaderEdit},
	headerHistoryModel {new HeaderHistoryModel {nullptr, this}}
{
	ui.setupUi(this);

	this->ui.category->addItems(::categories);
	this->ui.category->setItemText(0, "(unspecified)");
	this->ui.history->setModel(this->headerHistoryModel);

	connect(
		ui.description,
		&QLineEdit::textChanged,
		[&](const QString& text)
		{
			if (this->hasValidHeader())
			{
				this->m_header->description = text;
				emit descriptionChanged(text);
			}
		}
	);
	connect(
		ui.author,
		&QLineEdit::textChanged,
		[&](const QString& text)
		{
			if (this->hasValidHeader())
				this->m_header->author = text;
		}
	);
	connect(
		ui.winding,
		qOverload<int>(&QComboBox::currentIndexChanged),
		[&](int index)
		{
			if (this->hasValidHeader())
				this->m_model->setWinding(static_cast<Winding>(index));
		}
	);
	connect(
		ui.license,
		qOverload<int>(&QComboBox::currentIndexChanged),
		[&](int index)
		{
			if (this->m_header)
				this->m_header->license = static_cast<decltype(LDHeader::license)>(index);
		}
	);
	connect(
		ui.category,
		qOverload<int>(&QComboBox::currentIndexChanged),
		[&](int index)
		{
			if (this->hasValidHeader())
				this->m_header->category = ::categories.value(index);
		}
	);
	connect(
		ui.alias,
		&QCheckBox::stateChanged,
		[&](int state)
		{
			if (this->hasValidHeader())
				assignFlag<LDHeader::Alias>(this->m_header->qualfiers, state == Qt::Checked);
		}
	);
	connect(
		ui.physicalColor,
		&QCheckBox::stateChanged,
		[&](int state)
		{
			if (this->hasValidHeader())
			{
				assignFlag<LDHeader::Physical_Color>(
					this->m_header->qualfiers,
					state == Qt::Checked
				);
			}
		}
	);
	connect(
		ui.flexibleSection,
		&QCheckBox::stateChanged,
		[&](int state)
		{
			if (this->hasValidHeader())
			{
				assignFlag<LDHeader::Flexible_Section>(
					this->m_header->qualfiers,
					state == Qt::Checked
				);
			}
		}
	);
	connect(
		ui.historyNew,
		&QPushButton::clicked,
		[&]()
		{
			if (this->hasValidHeader())
			{
				const QModelIndex index = this->ui.history->selectionModel()->currentIndex();
				int row;

				if (index.isValid())
					row = index.row() + 1;
				else
					row = this->headerHistoryModel->rowCount();

				this->headerHistoryModel->insertRows(row, 1, {});
			}
		}
	);
	connect(
		ui.historyDelete,
		&QPushButton::clicked,
		[&]()
		{
			const QModelIndex index = this->ui.history->selectionModel()->currentIndex();

			if (this->hasValidHeader() and index.isValid())
				this->headerHistoryModel->removeRows(index.row(), 1, {});
		}
	);
	connect(ui.historyMoveUp, &QPushButton::clicked, [&](){ this->moveRows(-1); });
	connect(ui.historyMoveDown, &QPushButton::clicked, [&](){ this->moveRows(+2); });
	this->setEnabled(this->hasValidHeader());
}

void HeaderEdit::moveRows(int direction)
{
	if (this->hasValidHeader())
	{
		const QModelIndex index = this->ui.history->selectionModel()->currentIndex();
		this->headerHistoryModel->moveRows({}, index.row(), 1, {}, index.row() + direction);
	}
}

HeaderEdit::~HeaderEdit()
{
	delete this->headerHistoryModel;
	delete &this->ui;
}

void HeaderEdit::setDocument(LDDocument* document)
{
	LDHeader* header = &document->header;
	this->m_model = document;
	this->m_header = header;
	this->ui.description->setText(header->description);
	this->ui.author->setText(header->author);
	this->ui.category->setCurrentIndex(::categories.indexOf(header->category));
	this->ui.license->setCurrentIndex(static_cast<int>(header->license));
	this->ui.alias->setChecked(header->qualfiers & LDHeader::Alias);
	this->ui.physicalColor->setChecked(header->qualfiers & LDHeader::Physical_Color);
	this->ui.flexibleSection->setChecked(header->qualfiers & LDHeader::Flexible_Section);
	this->ui.cmdline->setText(header->cmdline);
	this->ui.winding->setCurrentIndex(document->winding());
	this->ui.keywords->document()->setPlainText(header->keywords);
	this->ui.help->document()->setPlainText(header->help);
	this->headerHistoryModel->setHeader(header);
	this->setEnabled(this->hasValidHeader());
}

bool HeaderEdit::hasValidHeader() const
{
	return this->m_header != nullptr and this->m_header->type != LDHeader::NoHeader;
}
