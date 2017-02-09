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
#include "../main.h"
#include "../model.h"

class QPainter;
class Canvas;
class QMouseEvent;
class QKeyEvent;

enum class EditModeType
{
	Select,
	Draw,
	Rectangle,
	Circle,
	MagicWand,
	LinePath,
	Curve,
};

/*
 * Base class of all editing modes.
 */
class AbstractEditMode : public QObject, public HierarchyElement
{
	Q_OBJECT

public:
	struct MouseEventData
	{
		QMouseEvent* ev;
		Qt::KeyboardModifiers keymods;
		bool mouseMoved;
		Qt::MouseButtons releasedButtons;
	};

	AbstractEditMode(Canvas* canvas);

	virtual bool			allowFreeCamera() const = 0;
	virtual void			render (QPainter&) const {}
	Canvas* renderer() const;
	virtual EditModeType	type() const = 0;
	virtual bool			mousePressed (QMouseEvent*) { return false; }
	virtual bool			mouseReleased (MouseEventData const&) { return false; }
	virtual bool			mouseDoubleClicked (QMouseEvent*) { return false; }
	virtual bool			mouseMoved (QMouseEvent*) { return false; }
	virtual bool			keyReleased (QKeyEvent*) { return false; }

	static AbstractEditMode* createByType (Canvas* renderer, EditModeType type);

private:
	Canvas* m_canvas;
};

/*
 * Base class for draw-like edit modes
 */
class AbstractDrawMode : public AbstractEditMode
{
	DEFINE_CLASS (AbstractDrawMode, AbstractEditMode)

protected:
	QList<Vertex>			m_drawedVerts;
	QBrush					m_polybrush;

public:
	AbstractDrawMode (Canvas* canvas);

	void addDrawnVertex(const Vertex& pos);
	bool allowFreeCamera() const override final;
	virtual void endDraw();
	void drawLineLength(QPainter& painter, const Vertex& v0, const Vertex& v1, const QPointF& v0p, const QPointF& v1p) const;
	void finishDraw(Model& model);
	Vertex getCursorVertex() const;
	bool keyReleased(QKeyEvent* ev) override;
	virtual int maxVertices() const;
	bool mouseReleased(const AbstractEditMode::MouseEventData& data) override;
	virtual bool preAddVertex(Vertex const&);
	void renderPolygon(QPainter& painter, const QVector<Vertex>& poly3d, bool withlengths, bool withangles) const;
};

/*
 * Base class for select-like edit modes
 */
class AbstractSelectMode : public AbstractEditMode
{
	DEFINE_CLASS (AbstractSelectMode, AbstractEditMode)

public:
	AbstractSelectMode (Canvas* canvas);

	virtual bool allowFreeCamera() const override
	{
		return true;
	}
};
