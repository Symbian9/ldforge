#pragma once
#include "../main.h"

class QPainter;
class GLRenderer;
class QMouseEvent;

enum class EditModeType
{
	Select,
	Draw,
	Circle,
	MagicWand,
};

class AbstractEditMode
{
	GLRenderer* _renderer;

public:
	struct MouseEventData
	{
		QMouseEvent* ev;
		Qt::KeyboardModifiers keymods;
		bool mouseMoved;
		Qt::MouseButtons releasedButtons;
	};

	AbstractEditMode (GLRenderer* renderer);
	virtual ~AbstractEditMode();

	virtual bool			allowFreeCamera() const = 0;
	virtual void			render (QPainter&) const {};
	GLRenderer*				renderer() const;
	virtual EditModeType	type() const = 0;
	virtual bool			mousePressed (QMouseEvent*) { return false; }
	virtual bool			mouseReleased (MouseEventData const&) { return false; }
	virtual bool			mouseDoubleClicked (QMouseEvent*) { return false; }
	virtual bool			mouseMoved (QMouseEvent*) { return false; }

	static AbstractEditMode* createByType (GLRenderer* renderer, EditModeType type);
};

//
// Base class for draw-like edit modes
//
class AbstractDrawMode : public AbstractEditMode
{
	DEFINE_CLASS (AbstractDrawMode, AbstractEditMode)

protected:
	QList<Vertex>			_drawedVerts;
	Vertex					_rectverts[4];
	QBrush					_polybrush;

public:
	AbstractDrawMode (GLRenderer* renderer);

	virtual bool allowFreeCamera() const override
	{
		return false;
	}

	bool mouseReleased (const AbstractEditMode::MouseEventData& data) override;
	void addDrawnVertex (const Vertex& pos);
	void finishDraw (LDObjectList& objs);

	virtual bool preAddVertex (Vertex const&)
	{
		return false;
	}
};

//
// Base class for select-like edit modes
//
class AbstractSelectMode : public AbstractEditMode
{
	DEFINE_CLASS (AbstractSelectMode, AbstractEditMode)

public:
	AbstractSelectMode (GLRenderer* renderer);

	virtual bool allowFreeCamera() const override
	{
		return true;
	}
};
