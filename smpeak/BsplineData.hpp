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

#ifndef SMPEAK_BSPLINEDATA_HPP_
#define SMPEAK_BSPLINEDATA_HPP_

#include "peakcore.hpp"
#include "SMData.hpp"


template<typename R = double, typename T = float>
class BsplineData
{
public:
	BsplineData(DataAxis<T,R> &bs, DataAxis<T,R> &dbs, DataAxis<T,R> &d2bs);
	BsplineData(DataAxis<T,R> &bs, DataAxis<T,R> &dhbs, DataAxis<T,R> &d2hbs,
				DataAxis<T,R> &dvbs, DataAxis<T,R> &d2vbs);
	vector<DataAxis<T,R>* > get(void);
private:
	vector<DataAxis<T,R>* > bspObjP;
};


template<typename R,typename T>
BsplineData<R,T>::BsplineData(DataAxis<T,R> &bs, DataAxis<T,R> &dbs, DataAxis<T,R> &d2bs)
{
	bspObjP.push_back(&bs);
	bspObjP.push_back(&dbs);
	bspObjP.push_back(&d2bs);
}

template<typename R,typename T>
BsplineData<R,T>::BsplineData(DataAxis<T,R> &bs, DataAxis<T,R> &dhbs, DataAxis<T,R> &d2hbs,
		DataAxis<T,R> &dvbs, DataAxis<T,R> &d2vbs)
{
	bspObjP.push_back(&bs);
	bspObjP.push_back(&dhbs);
	bspObjP.push_back(&d2hbs);
	bspObjP.push_back(&dvbs);
	bspObjP.push_back(&d2vbs);
}

template<typename R,typename T>
vector<DataAxis<T,R>* > BsplineData<R,T>::get(void)
{
	return bspObjP;
}


#endif /* SMPEAK_BSPLINEDATA_HPP_ */
