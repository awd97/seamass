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

#include "NetCDF4StorageManager.hpp"


using namespace SpatialIndex;
using namespace SpatialIndex::StorageManager;


SpatialIndex::IStorageManager* SpatialIndex::StorageManager::returnNetCDF4StorageManager(Tools::PropertySet& ps)
{
	IStorageManager* sm = new NetCDF4StorageManager(ps);
	return sm;
}


SpatialIndex::IStorageManager* SpatialIndex::StorageManager::createNewNetCDF4StorageManager(std::string& baseName, uint32_t pageSize)
{
	Tools::Variant var;
	Tools::PropertySet ps;

	var.m_varType = Tools::VT_BOOL;
	var.m_val.blVal = true;
	ps.setProperty("Overwrite", var);
		// overwrite the file if it exists.

	var.m_varType = Tools::VT_PCHAR;
	var.m_val.pcVal = const_cast<char*>(baseName.c_str());
	ps.setProperty("FileName", var);
		// .idx and .dat extensions will be added.

	var.m_varType = Tools::VT_ULONG;
	var.m_val.ulVal = pageSize;
	ps.setProperty("PageSize", var);
		// specify the page size. Since the index may also contain user defined data
		// there is no way to know how big a single node may become. The storage manager
		// will use multiple pages per node if needed. Off course this will slow down performance.

	return returnNetCDF4StorageManager(ps);
}


SpatialIndex::IStorageManager* SpatialIndex::StorageManager::loadNetCDF4StorageManager(std::string& baseName)
{
	Tools::Variant var;
	Tools::PropertySet ps;

	var.m_varType = Tools::VT_PCHAR;
	var.m_val.pcVal = const_cast<char*>(baseName.c_str());
	ps.setProperty("FileName", var);
		// .idx and .dat extensions will be added.

	return returnNetCDF4StorageManager(ps);
}


NetCDF4StorageManager::NetCDF4StorageManager(Tools::PropertySet&)
{
}


NetCDF4StorageManager::~NetCDF4StorageManager()
{
	for (std::vector<Entry*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it) delete *it;
}


void NetCDF4StorageManager::flush()
{
	std::cout << "flush " << std::endl;
}


void NetCDF4StorageManager::loadByteArray(const id_type page, uint32_t& len, byte** data)
{
	std::cout << "loadByteArray " << page << ", " << len << std::endl;

	Entry* e;
	try
	{
		e = m_buffer.at(page);
		if (e == 0) throw InvalidPageException(page);
	}
	catch (std::out_of_range)
	{
		throw InvalidPageException(page);
	}

	len = e->m_length;
	*data = new byte[len];

	memcpy(*data, e->m_pData, len);
}


void NetCDF4StorageManager::storeByteArray(id_type& page, const uint32_t len, const byte* const data)
{
	std::cout << "storeByteArray " << page << ", " << len << std::endl;

	if (page == NewPage)
	{
		Entry* e = new Entry(len, data);

		if (m_emptyPages.empty())
		{
			m_buffer.push_back(e);
			page = m_buffer.size() - 1;
		}
		else
		{
			page = m_emptyPages.top(); m_emptyPages.pop();
			m_buffer[page] = e;
		}
	}
	else
	{
		Entry* e_old;
		try
		{
			e_old = m_buffer.at(page);
			if (e_old == 0) throw InvalidPageException(page);
		}
		catch (std::out_of_range)
		{
			throw InvalidPageException(page);
		}

		Entry* e = new Entry(len, data);

		delete e_old;
		m_buffer[page] = e;
	}
}


void NetCDF4StorageManager::deleteByteArray(const id_type page)
{
	std::cout << "deleteByteArray " << page << std::endl;

	Entry* e;
	try
	{
		e = m_buffer.at(page);
		if (e == 0) throw InvalidPageException(page);
	}
	catch (std::out_of_range)
	{
		throw InvalidPageException(page);
	}

	m_buffer[page] = 0;
	m_emptyPages.push(page);

	delete e;
}

