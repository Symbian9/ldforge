#pragma once
#include "../main.h"
#include "../glRenderer.h"

class QPainter;
class GLRenderer;

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
	QBrush		_polybrush;

public:
	struct MouseEventData
	{
		QMouseEvent* ev;
		Qt::KeyboardModifiers keymods;
		bool mouseMoved;
		Qt::MouseButtons releasedButtons;
	};

	AbstractEditMode (GLRenderer* renderer);

	virtual bool			allowFreeCamera() const = 0;
	virtual void			render (QPainter& painter) const {};
	GLRenderer*				renderer() const;
	virtual EditModeType	type() const = 0;
	virtual void			mousePressed (MouseEventData const& data) {}
	virtual void			mouseReleased (MouseEventData const& data) {}

	static AbstractEditMode* createByType (GLRenderer* renderer, EditModeType type);
};

//
// Base class for draw-like edit modes
//
class AbstractDrawMode : public AbstractEditMode
{
	QList<Vertex>			m_drawedVerts;
	Vertex					m_rectverts[4];

public:
	AbstractDrawMode (GLRenderer* renderer);

	virtual bool allowFreeCamera() const override
	{
		return false;
	}

	virtual void mouseReleased (MouseEventData const& data) override;
	void addDrawnVertex (const Vertex& pos);

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
public:
	AbstractSelectMode (GLRenderer* renderer);

	virtual bool allowFreeCamera() const override
	{
		return true;
	}
};
