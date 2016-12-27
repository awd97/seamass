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


#include "BasisBsplineScale.hpp"

#include "Bspline.hpp"

#include <limits>
#include <iomanip>
#include <cmath>


using namespace std;


BasisBsplineScale::
BasisBsplineScale(vector<Basis*>& bases, int parentIndex, short dimension, Transient transient, int order)
	: BasisBspline(bases, static_cast<BasisBspline*>(bases[parentIndex])->getGridInfo().dimensions, transient, parentIndex), dimension_(dimension)
{
#ifndef NDEBUG
	cout << " " << getIndex() << " BasisBsplineScale";
    if (getTransient() == Basis::Transient::YES) cout << " (t)";
	cout << endl;
#endif

	const GridInfo parentGridInfo = static_cast<BasisBspline*>(bases[parentIndex])->getGridInfo();
	gridInfo() = parentGridInfo;
	gridInfo().scale[dimension] = parentGridInfo.scale[dimension] - 1;
	gridInfo().offset[dimension] = parentGridInfo.offset[dimension] / 2;
	gridInfo().extent[dimension] = (parentGridInfo.offset[dimension] + parentGridInfo.extent[dimension] - 1 - order) / 2 + order + 1 - gridInfo().offset[dimension];
	ii m = parentGridInfo.extent[dimension];
	ii n = gridInfo().extent[dimension];

	ii stride = 1;
	for (ii j = 0; j < dimension; j++) stride *= gridInfo().extent[j];

	// create our kernel
	ii nh = order + 2;
	vector<fp> hs(nh);
	double sum = 0.0;
	for (ii i = 0; i < nh; i++)
	{
		hs[i] = (fp) (1.0 / pow(2.0, (double)order) * Bspline::factorial(order + 1) / (double)(Bspline::factorial(i)*Bspline::factorial(order + 1 - i)));
		sum += hs[i];
	}
	for (ii i = 0; i < nh; i++)
	{
		hs[i] /= (fp) sum;
	}

	// create A as a temporary COO matrix
	vector<fp> acoo(nh * n);
	vector<ii> rowind(nh * n);
	vector<ii> colind(nh * n);

	ii nnz = 0;
	ii offset = order + ((parentGridInfo.offset[dimension] + 1) % 2);
	for (ii j = 0; j < n; j++)
	{
		for (ii i = 0; i < nh; i++)
		{
			rowind[nnz] = 2 * j + i - offset;
			if (rowind[nnz] < 0 || rowind[nnz] >= m) continue;
			acoo[nnz] = hs[i];
			colind[nnz] = j;

			nnz++;
		}
	}

    aT_.init(n, m, nnz, acoo.data(), colind.data(), rowind.data());
	a_.init(m, n, nnz, acoo.data(), rowind.data(), colind.data());
    
    nnzRows_ = n;

#ifndef NDEBUG
	cout << "  parent=" << getParentIndex() << " dimension=" << dimension << " " << gridInfo() << " mem=";
	cout << fixed << setprecision(2) << (a_.mem() + aT_.mem()) / 1024.0 / 1024.0 << "Mb" << endl;
#endif
}


BasisBsplineScale::~BasisBsplineScale()
{
}


void
BasisBsplineScale::
synthesis(MatrixSparse& f, const MatrixSparse& x, bool accumulate) const
{
#ifndef NDEBUG
	cout << " " << getIndex() << " BasisBsplineScale::synthesis" << endl;
#endif

	f.mul(accumulate ? MatrixSparse::Accumulate::YES : MatrixSparse::Accumulate::NO, x, MatrixSparse::Transpose::NO, aT_);
}


void BasisBsplineScale::analysis(MatrixSparse& xE, const MatrixSparse& fE, bool sqrA) const
{
#ifndef NDEBUG
	cout << " " << getIndex() << " BasisBsplineScale::analysis" << endl;
#endif

	if (sqrA)
	{
		MatrixSparse t;
		t.copy(a_);
		t.elementwiseSqr();
		xE.mul(MatrixSparse::Accumulate::NO, fE, MatrixSparse::Transpose::NO, t);
	}
	else
	{
		xE.mul(MatrixSparse::Accumulate::NO, fE, MatrixSparse::Transpose::NO, a_);
	}
}


void BasisBsplineScale::deleteRows(const MatrixSparse& x, ii threshold)
{
    /*if(nnzRows_ - x.nnz() >= threshold)
    {
        // delete rows in aTs we don't need anymore
        aT_.deleteRows(x);
        a_.copy(aT_, MatrixSparse::Operation::TRANSPOSE);
        
        nnzRows_ = x.nnz();
    }*/
}

