//
// $Id$
//
//
// Original author: Andrew Dowsey <andrew.dowsey <a.t> liverpool.ac.uk>
//
// Copyright (C) 2015 biospi Laboratory, EEE, University of Liverpool, UK
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


#ifndef _SEAMASSRESTORATION_NETCDF4STORAGEMANAGER_HPP_
#define _SEAMASSRESTORATION_NETCDF4STORAGEMANAGER_HPP_


#include <SpatialIndex.h>


namespace SpatialIndex { namespace StorageManager {


SpatialIndex::IStorageManager* returnNetCDF4StorageManager(Tools::PropertySet& ps);
SpatialIndex::IStorageManager* createNewNetCDF4StorageManager(std::string& baseName, uint32_t pageSize);
SpatialIndex::IStorageManager* loadNetCDF4StorageManager(std::string& baseName);


class NetCDF4StorageManager : public SpatialIndex::IStorageManager
{
public:
	NetCDF4StorageManager(Tools::PropertySet&);

	virtual ~NetCDF4StorageManager();

	virtual void flush();
	virtual void loadByteArray(const id_type page, uint32_t& len, byte** data);
	virtual void storeByteArray(id_type& page, const uint32_t len, const byte* const data);
	virtual void deleteByteArray(const id_type page);

private:
	class Entry
	{
	public:
		byte* m_pData;
		uint32_t m_length;

		Entry(uint32_t l, const byte* const d) : m_pData(0), m_length(l)
		{
			m_pData = new byte[m_length];
			memcpy(m_pData, d, m_length);
		}

		~Entry() { delete[] m_pData; }
	};

	std::vector<Entry*> m_buffer;
	std::stack<id_type> m_emptyPages;
};


}}


#endif // _SEAMASSRESTORATION_NETCDF4STORAGEMANAGER_HPP_

