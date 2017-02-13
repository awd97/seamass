//
// Original author: Andrew Dowsey <andrew.dowsey <a.t> bristol.ac.uk>
//
// Copyright (C) 2016  biospi Laboratory, University of Bristol, UK
//
// This file is part of seaMass.
//
// seaMass is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// seaMass is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with seaMass.  If not, see <http://www.gnu.org/licenses/>.
//


#ifndef _SEAMASS_CORE_BASISBSPLINE_HPP_
#define _SEAMASS_CORE_BASISBSPLINE_HPP_


#include <vector>

#include "Basis.hpp"


class BasisBspline : public Basis
{
public:
	struct GridInfo
	{
		short dimensions;  	      // dimension of each b-spline coefficients grid
		std::vector<short> scale; // dyadic scale for each dimension
		std::vector<ii> offset;   // coefficient offset for each dimension
		std::vector<ii> extent;   // number of coefficients for each dimension (make up the columns)
		ii count;                 // number of grids

		GridInfo(short dimensions_);
		~GridInfo();

		ii m() const;           // number of rows in resulting matrix
        ii n() const;           // number of columns in resulting matrix
		li size() const;        // number of coefficients across all grids

		void operator=(const GridInfo& gridInfo);
	};

	BasisBspline(std::vector<Basis*>& bases, short dimensions, Transient transient, int parentIndex = -1);
	virtual ~BasisBspline();

	ii getM() const;
	ii getN() const;
	const GridInfo& getGridInfo() const;

protected:
	GridInfo& gridInfo();

private:
	GridInfo gridInfo_;
};

std::ostream& operator<<(std::ostream& os, const BasisBspline::GridInfo& gridInfo);

#endif

