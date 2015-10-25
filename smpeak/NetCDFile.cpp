#include "NetCDFile.hpp"

NetCDFile::NetCDFile(const string _fileName, int omode) : fileName(_fileName)
{
	switch(omode)
	{
	case NC_NOWRITE:
		if ((retval = nc_open(fileName.c_str(), omode, &ncid)))
			err(retval);
		fileStatus = true;
		break;
	case NC_WRITE:
		if ((retval = nc_open(fileName.c_str(), omode, &ncid)))
			err(retval);
		fileStatus = true;
		break;
	case NC_NETCDF4:
	   if ((retval = nc_create(fileName.c_str(), omode|NC_CLOBBER, &ncid)))
	      err(retval);
	   fileStatus = true;
	   break;
	}
}

void NetCDFile::open(const string _fileName, int omode)
{
	if (fileStatus == false)
	{
		fileName=_fileName;
		switch(omode)
		{
		case NC_NOWRITE:
			if ((retval = nc_open(fileName.c_str(), omode, &ncid)))
				err(retval);
			fileStatus = true;
			break;
		case NC_WRITE:
			if ((retval = nc_open(fileName.c_str(), omode, &ncid)))
				err(retval);
			fileStatus = true;
			break;
		case NC_NETCDF4:
			if ((retval = nc_create(fileName.c_str(), omode|NC_CLOBBER, &ncid)))
				err(retval);
			fileStatus = true;
			break;
		}
	}
	else
	{
		cout<<"File already opened"<<endl;
		exit(ERRCODE);
	}
}

void NetCDFile::close(void)
{
	if (fileStatus == true)
	{
		if ((retval = nc_close(ncid)))
			err(retval);
		fileStatus = false;
	}
	else
	{
		cout<<"No file to close"<<endl;
		exit(ERRCODE);
	}
}

NetCDFile::~NetCDFile()
{
	if(fileStatus == true)
	{
		if((retval = nc_close(ncid)))
			err(retval);
	}
}

int NetCDFile::search_Group(const string dataSet, int grpid)
{
	if(grpid == 0) grpid = ncid;

	int nGrps;
	vector<int> ngrpids;
	string strGrp;
	size_t strGrpL;

	int nVars;
	int varid;

	// How many Groups
	if(( retval = nc_inq_grps(grpid, &nGrps, NULL) ))
		err(retval);
	if(nGrps > 0 )
	{
		ngrpids.resize(nGrps);
		if(( retval = nc_inq_grps(grpid, NULL, &ngrpids[0]) ))
			err(retval);
	}

	if(( retval = nc_inq_grpname_len(grpid, &strGrpL)))
		err(retval);
	strGrp.resize(strGrpL);
	if(( retval = nc_inq_grpname_full(grpid,NULL,&strGrp[0])))
		err(retval);

	// Scan Groups
	for(int i = 0; i < nGrps; ++i)
	{
		search_Group(dataSet, ngrpids[i]);
	}
	// Scan Variables
	if(( retval = nc_inq_varids(grpid, &nVars, NULL) ))
		err(retval);
	if(nVars > 0)
	{
		retval = nc_inq_varid(grpid, dataSet.c_str(), &varid);
		if(retval == NC_NOERR)
		{
			this->dataSetList.push_back(InfoGrpVar(grpid,varid,strGrp,dataSet));
			return 1;
		}
	}
	return 0;
}

void NetCDFile::err(int e)
{
	cout<<"Error: "<<nc_strerror(e)<<endl;
	exit(ERRCODE);
}


void mzMLdump(const string fileName, string data)
{
	ofstream out(fileName.c_str());
	out<<data;
	out.close();
}