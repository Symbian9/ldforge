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
#include "main.h"

//
// Implements a ring finding algorithm. It is passed two radii and it tries to
// find solutions of rings that would fill the given space.
//
// Note: it is not fool-proof and does not always yield a solution.
//
class RingFinder
{
public:
	// A single component in a solution
	struct Component
	{
		int num;
		double scale;
	};

	// A solution whose components fill the desired ring space.
	class Solution
	{
	public:
		inline void addComponent (const Component& a);
		inline const QVector<Component>& getComponents() const;
		void scaleComponents (double scale);
		bool isSuperiorTo (const Solution* other) const;

	private:
		QVector<Component> m_components;
	};

	RingFinder();

	inline const QVector<Solution>& allSolutions() const;
	inline const Solution* bestSolution() const;
	bool findRings (double r0, double r1);

private:
	QVector<Solution> m_solutions;
	const Solution*   m_bestSolution;
	int               m_stack;

	bool findRingsRecursor (double r0, double r1, Solution& currentSolution);
};

//
// Gets the components of a solution
//
inline const QVector<RingFinder::Component>& RingFinder::Solution::getComponents() const
{
	return m_components;
}

//
// Adds a component to a solution
//
inline void RingFinder::Solution::addComponent (const Component& a)
{
	m_components.push_back (a);
}

//
// Returns the solution that was considered best. Returns null
// if there isn't any suitable solution.
//
inline const RingFinder::Solution* RingFinder::bestSolution() const
{
	return m_bestSolution;
}

//
// Returns all found solutions.
//
inline const QVector<RingFinder::Solution>& RingFinder::allSolutions() const
{
	return m_solutions;
}

extern RingFinder g_RingFinder;
