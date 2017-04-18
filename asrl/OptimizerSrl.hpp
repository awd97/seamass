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


#ifndef SEAMASS_ASRL_OPTIMIZERSRL_HPP
#define SEAMASS_ASRL_OPTIMIZERSRL_HPP


#include "Optimizer.hpp"


class OptimizerSrl : public Optimizer
{
public:    
    OptimizerSrl(const std::vector<Basis*>& bases, const std::vector<fp> binCounts, const std::vector<li> spectrumIndex, fp pruneThreshold = (fp)0.001);
	virtual ~OptimizerSrl();
    
	void init(fp lamba);
	fp step();
	void synthesis(std::vector<MatrixSparse>& f, ii basis = -1);

	ii getIteration() const;
	const std::vector<Basis*>& getBases() const;

	std::vector< std::vector<MatrixSparse> >& xs();
	std::vector< std::vector<MatrixSparse> >& l2s();
	std::vector< std::vector<MatrixSparse> >& l1l2s();

private:
	const std::vector<Basis*>& bases_;
    std::vector<Matrix> b_;
	fp pruneThreshold_;

	fp lambda_;
	int iteration_;

	std::vector< std::vector<MatrixSparse> > xs_;
	std::vector< std::vector<MatrixSparse> > l2s_;
	std::vector< std::vector<MatrixSparse> > l1l2s_;
    
    double synthesisDuration_;
    double errorDuration_;
    double analysisDuration_;
    double shrinkageDuration_;
    double updateDuration_;
};


#endif
