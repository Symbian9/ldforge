/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2015 Teemu Piippo
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

#include "ringFinder.h"
#include "miscallenous.h"

RingFinder g_RingFinder;

RingFinder::RingFinder() {}

// =============================================================================
//
bool RingFinder::findRingsRecursor (double r0, double r1, Solution& currentSolution)
{
	// Don't recurse too deep.
	if (m_stack >= 5 or r1 < r0)
		return false;

	// Find the scale and number of a ring between r1 and r0.
	double scale = r1 - r0;
	double num = r0 / scale;

	// If the ring number is integral, we have found a fitting ring to r0 -> r1!
	if (isInteger (num))
	{
		Component cmp;
		cmp.scale = scale;
		cmp.num = (int) round (num);
		currentSolution.addComponent (cmp);

		// If we're still at the first recursion, this is the only
		// ring and there's nothing left to do. Guess we found the winner.
		if (m_stack == 0)
		{
			m_solutions.push_back (currentSolution);
			return true;
		}
	}
	else
	{
		// Try find solutions by splitting the ring in various positions.
		if (isZero (r1 - r0))
			return false;

		double interval;

		// Determine interval. The smaller delta between radii, the more precise
		// interval should be used. We can't really use a 0.5 increment when
		// calculating rings to 10 -> 105... that would take ages to process!
		if (r1 - r0 < 0.5)
			interval = 0.1;
		else if (r1 - r0 < 10)
			interval = 0.5;
		else if (r1 - r0 < 50)
			interval = 1;
		else
			interval = 5;

		// Now go through possible splits and try find rings for both segments.
		for (double r = r0 + interval; r < r1; r += interval)
		{
			Solution sol = currentSolution;

			m_stack++;
			bool res = findRingsRecursor (r0, r, sol) and findRingsRecursor (r, r1, sol);
			m_stack--;

			if (res)
			{
				// We succeeded in finding radii for this segment. If the stack is 0, this
				// is the first recursion to this function. Thus there are no more ring segments
				// to process and we can add the solution.
				//
				// If not, when this function ends, it will be called again with more arguments.
				// Accept the solution to this segment by setting currentSolution to sol, and
				// return true to continue processing.
				if (m_stack == 0)
					m_solutions.push_back (sol);
				else
				{
					currentSolution = sol;
					return true;
				}
			}
		}

		return false;
	}

	return true;
}

//
// This is the main algorithm of the ring finder. It tries to use math
// to find the one ring between r0 and r1. If it fails (the ring number
// is non-integral), it finds an intermediate radius (ceil of the ring
// number times scale) and splits the radius at this point, calling this
// function again to try find the rings between r0 - r and r - r1.
//
// This does not always yield into usable results. If at some point r ==
// r0 or r == r1, there is no hope of finding the rings, at least with
// this algorithm, as it would fall into an infinite recursion.
//
bool RingFinder::findRings (double r0, double r1)
{
	m_solutions.clear();
	Solution sol;

	// If we're dealing with fractional radii, try upscale them into integral
	// ones. This should yield in more reliable and more optimized results.
	// For instance, using r0=1.5, r1=3.5 causes the algorithm to fail but
	// r0=3, r1=7 (scaled up by 2) yields a 2-component solution. We can then
	// downscale the radii back by dividing the scale fields of the solution
	// components.
	double scale = 1.0;

	if (not isZero (scale = r0 - floor (r0)) or not isZero (scale = r1 - floor (r1)))
	{
		double r0f = r0 / scale;
		double r1f = r1 / scale;

		if (isInteger (r0f) and isInteger (r1f))
		{
			r0 = r0f;
			r1 = r1f;
		}
		// If the numbers are both at most one-decimal fractions, we can use a scale of 10
		else if (isInteger (r0 * 10) and isInteger (r1 * 10))
		{
			scale = 0.1;
			r0 *= 10;
			r1 *= 10;
		}
	}
	else
	{
		scale = 1.0;
	}

	// Recurse in and try find solutions.
	findRingsRecursor (r0, r1, sol);

	// If we had upscaled our radii, downscale back now.
	if (scale != 1.0)
	{
		for (Solution& sol : m_solutions)
			sol.scaleComponents (scale);
	}

	// Compare the solutions and find the best one. The solution class has an operator>
	// overload to compare two solutions.
	m_bestSolution = nullptr;

	for (Solution const& sol : m_solutions)
	{
		if (m_bestSolution == nullptr or sol.isSuperiorTo (m_bestSolution))
			m_bestSolution = &sol;
	}

	return (m_bestSolution != nullptr);
}

//
// Compares this solution with @other and determines which
// one is superior.
//
// A solution is considered superior if solution has less
// components than the other one. If both solution have an
// equal amount components, the solution with a lesser maximum
// ring number is found superior, as such solutions should
// yield less new primitives and cleaner definitions.
//
// The solution which is found superior to every other solution
// will be the one returned by RingFinder::bestSolution().
//
bool RingFinder::Solution::isSuperiorTo (const Solution* other) const
{
	// If one solution has less components than the other one, it is definitely
	// better.
	if (getComponents().size() != other->getComponents().size())
		return getComponents().size() < other->getComponents().size();

	// Calculate the maximum ring number. Since the solutions have equal
	// ring counts, the solutions with lesser maximum rings should result
	// in cleaner code and less new primitives, right?
	int maxA = 0,
		maxB = 0;

	for (int i = 0; i < getComponents().size(); ++i)
	{
		maxA = qMax (getComponents()[i].num, maxA);
		maxB = qMax (other->getComponents()[i].num, maxB);
	}

	if (maxA != maxB)
		return maxA < maxB;

	// Solutions have equal rings and equal maximum ring numbers. Let's
	// just say this one is better, at this point it does not matter which
	// one is chosen.
	return true;
}

void RingFinder::Solution::scaleComponents (double scale)
{
	for (Component& cmp : m_components)
		cmp.scale *= scale;
}
