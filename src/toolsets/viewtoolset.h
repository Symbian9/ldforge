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

#pragma once
#include "toolset.h"
#include "glrenderer.h"

class ViewToolset : public Toolset
{
	Q_OBJECT

public:
	ViewToolset (MainWindow* parent);

	Q_INVOKABLE void axes();
	Q_INVOKABLE void bfcView();
	Q_INVOKABLE void clearCullDepth();
	Q_INVOKABLE void clearDrawPlane();
	Q_INVOKABLE void drawAngles();
	Q_INVOKABLE void drawConditionalLines();
	Q_INVOKABLE void drawEdgeLines();
	Q_INVOKABLE void drawSurfaces();
	Q_INVOKABLE void jumpTo();
	Q_INVOKABLE void lighting();
	Q_INVOKABLE void randomColors();
	Q_INVOKABLE void resetView();
	Q_INVOKABLE void screenshot();
	Q_INVOKABLE void selectAll();
	Q_INVOKABLE void selectByColor();
	Q_INVOKABLE void selectByType();
	Q_INVOKABLE void setDrawPlane();
	Q_INVOKABLE void setCullDepth();
	Q_INVOKABLE void visibilityHide();
	Q_INVOKABLE void visibilityReveal();
	Q_INVOKABLE void visibilityToggle();
	Q_INVOKABLE void wireframe();

	Q_INVOKABLE void newTopCamera();
	Q_INVOKABLE void newFrontCamera();
	Q_INVOKABLE void newLeftCamera();
	Q_INVOKABLE void newBottomCamera();
	Q_INVOKABLE void newBackCamera();
	Q_INVOKABLE void newRightCamera();
	Q_INVOKABLE void newFreeCamera();
	Q_INVOKABLE void selectTopCamera();
	Q_INVOKABLE void selectFrontCamera();
	Q_INVOKABLE void selectLeftCamera();
	Q_INVOKABLE void selectBottomCamera();
	Q_INVOKABLE void selectBackCamera();
	Q_INVOKABLE void selectRightCamera();
	Q_INVOKABLE void selectFreeCamera();

private:
	void createNewCamera(gl::CameraType cameraType);
	void selectCamera(gl::CameraType cameraType);
};
