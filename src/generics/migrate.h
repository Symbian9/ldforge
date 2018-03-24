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
#include <algorithm>
#include "range.h"

template<typename T>
void migrate(T& vector, int first, int last, int destination)
{
	if (destination < first)
	{
		int n = first - destination;
		for (int i : range(first, first + 1, last))
		{
			for (int step : range(i - 1, i - 2, i - n))
				std::swap(vector[step + 1], vector[step]);
		}
	}
	else if (destination > last)
	{
		int n = destination - last - 1;

		for (int i : range(last, last - 1, first))
		{
			for (int step : range(i + 1, i + 2, i + n))
				std::swap(vector[step - 1], vector[step]);
		}
	}
}
