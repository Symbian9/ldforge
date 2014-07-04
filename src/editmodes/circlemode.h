#include "abstracteditmode.h"

class CircleMode : public AbstractDrawMode
{
	DEFINE_CLASS (CircleMode, AbstractDrawMode)

public:
	CircleMode (GLRenderer* renderer);

	virtual void render (QPainter& painter) const override;
	virtual EditModeType type() const override;

	double					getCircleDrawDist (int pos) const;
	Matrix					getCircleDrawMatrix (double scale);
};
