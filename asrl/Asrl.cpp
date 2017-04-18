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


#include "Asrl.hpp"
#include "BasisMatrixGroup.hpp"
#include "OptimizerSrl.hpp"
#include "OptimizerAccelerationEve1.hpp"
#include <iostream>
#include <iomanip>
using namespace std;
using namespace kernel;


void Asrl::notice()
{
	cout << "seaMass-ASRL : Copyright (C) 2016 - biospi Laboratory, University of Bristol, UK" << endl;
	cout << "This program comes with ABSOLUTELY NO WARRANTY." << endl;
	cout << "This is free software, and you are welcome to redistribute it under certain conditions." << endl;
}


Asrl::Asrl(Input& input, double shrinkage, bool taperShrinkage, double tolerance) : shrinkage_(shrinkage), taperShrinkage_(taperShrinkage), tolerance_(tolerance), iteration_(0)
{
    if (getDebugLevel() % 10 >= 1)
    {
        cout << getTimeStamp() << "  Initialising basis functions ..." << endl;
    }

    if (input.gN == 0)
    {
        new BasisMatrix(bases_, input.aM, input.aN, input.aVs, input.aIs, input.aJs, BasisMatrix::Transient::NO);
    }
    else
    {
        new BasisMatrixGroup(bases_, input.aM, input.aN, input.aVs, input.aIs, input.aJs, input.gM, input.gVs, input.gIs, input.gJs, BasisMatrix::Transient::NO);
    }

    vector<li> dummy; // remove and allow sending from input.xs
    innerOptimizer_ = new OptimizerSrl(bases_, input.bs, dummy);
    optimizer_ = new OptimizerAccelerationEve1(innerOptimizer_);
    optimizer_->init((fp)shrinkage_);
}


Asrl::~Asrl()
{
	delete optimizer_;
	delete innerOptimizer_;
}


bool Asrl::step()
{
	if (iteration_ == 0 && getDebugLevel() % 10 >= 1)
	{
		li nnz = 0;
		li nx = 0;
		for (ii j = 0; j < (ii)bases_.size(); j++)
		{
            if (static_cast<Basis*>(bases_[j])->getTransient() == Basis::Transient::NO)
			{
                for (size_t k = 0; k < optimizer_->xs()[j].size(); k++)
                {
                    nnz += optimizer_->xs()[j][k].nnz();
                    nx += optimizer_->xs()[j][k].size();
                }
			}
		}

        cout << getTimeStamp();
        cout << "   it:     0 nx: " << setw(10) << nx << " nnz: " << setw(10) << nnz;
        cout << " tol:  " << fixed << setprecision(8) << setw(10) << tolerance_ << endl;
	}

	iteration_++;
	double grad = optimizer_->step();

	if (getDebugLevel() % 10 >= 1)
	{
		li nnz = 0;
		for (ii j = 0; j < (ii)bases_.size(); j++)
		{
			if (static_cast<Basis*>(bases_[j])->getTransient() == Basis::Transient::NO)
			{
                for (size_t k = 0; k < optimizer_->xs()[j].size(); k++)
                {
                    nnz += optimizer_->xs()[j][k].nnz();
                }
			}
		}
        cout << getTimeStamp();
		cout << "   it: " << setw(5) << iteration_;
		cout << " shrink: ";
		cout.unsetf(ios::floatfield); 
		cout << setprecision(4) << setw(6) << shrinkage_;
		cout << " nnz: " << setw(10) << nnz;
		cout << " grad: " << fixed << setprecision(8) << setw(10) << grad << endl;
	}

	if (grad <= tolerance_)
	{
		if (shrinkage_ == 0 || !taperShrinkage_)
		{
			if (getDebugLevel() % 10 == 0) cout << "o" << endl;
			return false;
		}
		else
		{
			if (getDebugLevel() % 10 == 0) cout << "o" << flush;
			shrinkage_ *= (shrinkage_ > 0.0625 ? 0.5 : 0.0);
			optimizer_->init((fp)shrinkage_);
		}
	}
	else
	{
		if (getDebugLevel() % 10 == 0) cout << "." << flush;
	}
    
    if (grad != grad)
    {
        cout << "ARGGH!" << endl;
        return false;
    }
    
	return true;
}


ii Asrl::getIteration() const
{
	return iteration_;
}


void Asrl::getOutput(Output& output) const
{
    if (getDebugLevel() % 10 >= 1)
         cout << getTimeStamp() << "  Getting output ..." << endl;

    output.xs.resize(optimizer_->xs()[0][0].size());
    optimizer_->xs()[0][0].exportTo(output.xs.data());

	vector<MatrixSparse> aXs;
	bases_[0]->synthesis(aXs, optimizer_->xs()[0], false);
	output.aXs.resize(aXs[0].size());
    aXs[0].exportTo(output.aXs.data());

	BasisMatrixGroup* basisMatrixGroup = dynamic_cast<BasisMatrixGroup*>(bases_[0]);
	if (basisMatrixGroup)
	{
		vector<MatrixSparse> gXs;
		basisMatrixGroup->groupSynthesis(gXs, optimizer_->xs()[0], false);
		output.gXs.resize(gXs[0].size());
        gXs[0].exportTo(output.gXs.data());
	}
}