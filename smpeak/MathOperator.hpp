//
// $Id$
//
//
// Original author: Ranjeet Bhamber <ranjeet <a.t> liverpool.ac.uk>
//
// Copyright (C) 2015  Biospi Laboratory for Medical Bioinformatics, University of Liverpool, UK
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

#ifndef SMPEAK_MATHOPERATOR_HPP_
#define SMPEAK_MATHOPERATOR_HPP_

#include "peakcore.hpp"

template<class T>
class OpUnit
{
protected:
	void apply(lli row, lli col, T** alpha){};
	void axisRT(hsize_t dims, int _offset, double rt_res, vector<double> &_rt);
	void axisMZ(hsize_t dims, int _offset, double mz_res, vector<double> &_mz);
	~OpUnit(){};
};

template<class T>
class OpNablaH : public OpUnit<T>
{
protected:
	void apply(lli row, lli col, T** alpha);
	void axisMZ(hsize_t dims, int _offset, double mz_res, vector<double> &_mz);
	~OpNablaH(){};
};

template<class T>
class OpNabla2H : public OpUnit<T>
{
protected:
	void apply(lli row, lli col, T** alpha);
	void axisMZ(hsize_t dims, int _offset, double mz_res, vector<double> &_mz);
	~OpNabla2H(){};
};

template<class T>
class OpNablaV : public OpUnit<T>
{
protected:
	void apply(lli row, lli col, T** alpha);
	void axisRT(hsize_t dims, int _offset, double rt_res, vector<double> &_rt);
	~OpNablaV(){};
};


template<class T>
class OpNabla2V : public OpUnit<T>
{
protected:
	void apply(lli row, lli col, T** alpha);
	void axisRT(hsize_t dims, int _offset, double rt_res, vector<double> &_rt);
	~OpNabla2V(){};
};

#include"MathOperator.tpp"

#endif /* SMPEAK_MATHOPERATOR_HPP_ */
