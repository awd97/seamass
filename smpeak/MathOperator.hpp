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


template<class T>
void OpUnit<T>::axisRT(hsize_t dims, int _offset, double rt_res, vector<double> &_rt)
{
	_rt.resize(dims);
	double offset = double(_offset) - 1;
	double ppbrt = 1.0 / (pow(2.0, rt_res));
	#pragma omp parallel for
	for (lli i = 0; i < _rt.size(); ++i) {
		_rt[i] = (offset + i) * ppbrt;
	}
}

template<class T>
void OpUnit<T>::axisMZ(hsize_t dims, int _offset, double mz_res, vector<double> &_mz)
{
	_mz.resize(dims);
	double offset = double(_offset) - 1;
	double ppbmz = 1.0033548378 / (pow(2.0, mz_res) * 60.0);
	#pragma omp parallel for
	for (lli i = 0; i < _mz.size(); ++i) {
		_mz[i] = (offset + i) * ppbmz;
	}
}


template<class T>
void OpNablaH<T>::apply(lli row, lli col, T** alpha)
{
	#pragma omp parallel for
	for(lli i=0; i < row; ++i)
	{
		T Nm1=alpha[i][0];
		for(lli j=1; j < col; ++j)
		{
			T N=alpha[i][j];
			alpha[i][j]=alpha[i][j]-Nm1;
			Nm1=N;
		}
	}
	#pragma omp parallel for
	for(lli i=0; i < row; ++i)
		alpha[i][0]=0.0;
}

template<class T>
void OpNablaH<T>::axisMZ(hsize_t dims, int _offset, double mz_res, vector<double> &_mz)
{
	_mz.resize(dims);
	double offset = double(_offset) - 1.5;
	double ppbmz = 1.0033548378 / (pow(2.0, mz_res) * 60.0);
	#pragma omp parallel for
	for (lli i = 0; i < _mz.size(); ++i) {
		_mz[i] = (offset + i) * ppbmz;
	}
}


template<class T>
void OpNabla2H<T>::apply(lli row, lli col, T** alpha)
{
	#pragma omp parallel for
	for(lli i=0; i < row; ++i)
	{
		T Nm1=alpha[i][1];
		T Nm2=alpha[i][0];
		for(lli j=2; j < col; ++j)
		{
			T N=alpha[i][j];
			alpha[i][j]=alpha[i][j]-2.0*Nm1+Nm2;
			Nm2=Nm1;
			Nm1=N;
		}
	}
	#pragma omp parallel for
	for(lli j=0; j < 2; ++j)
		for(lli i=0; i < row; ++i)
			alpha[i][j]=0.0;
}

template<class T>
void OpNabla2H<T>::axisMZ(hsize_t dims, int _offset, double mz_res, vector<double> &_mz)
{
	_mz.resize(dims);
	double offset = double(_offset) - 2;
	double ppbmz = 1.0033548378 / (pow(2.0, mz_res) * 60.0);
	#pragma omp parallel for
	for (lli i = 0; i < _mz.size(); ++i) {
		_mz[i] = (offset + i) * ppbmz;
	}
}


template<class T>
void OpNablaV<T>::apply(lli row, lli col, T** alpha)
{
	#pragma omp parallel for
	for(lli i=0; i < col; ++i)
	{
		T Nm1=alpha[0][i];
		for(lli j=1; j < row; ++j)
		{
			T N=alpha[j][i];
			alpha[j][i]=alpha[j][i]-Nm1;
			Nm1=N;
		}
	}
	#pragma omp parallel for
	for(lli i=0; i < col; ++i)
		alpha[0][i]=0.0;
}

template<class T>
void OpNablaV<T>::axisRT(hsize_t dims, int _offset, double rt_res, vector<double> &_rt)
{
	_rt.resize(dims);
	double offset = double(_offset) - 1.5;
	double ppbrt = 1.0 / (pow(2.0, rt_res));
	#pragma omp parallel for
	for (lli i = 0; i < _rt.size(); ++i) {
		_rt[i] = (offset + i) * ppbrt;
	}
}


template<class T>
void OpNabla2V<T>::apply(lli row, lli col, T** alpha)
{
	#pragma omp parallel for
	for(lli i=0; i < col; ++i)
	{
		T Nm1=alpha[1][i];
		T Nm2=alpha[0][i];
		for(lli j=2; j < row; ++j)
		{
			T N=alpha[j][i];
			alpha[j][i]=alpha[j][i]-2.0*Nm1+Nm2;
			Nm2=Nm1;
			Nm1=N;
		}
	}
	#pragma omp parallel for
	for(lli i=0; i < 2; ++i)
		for(lli j=0; j < col; ++j)
			alpha[i][j]=0.0;
}

template<class T>
void OpNabla2V<T>::axisRT(hsize_t dims, int _offset, double rt_res, vector<double> &_rt)
{
	_rt.resize(dims);
	double offset = double(_offset) - 2;
	double ppbrt = 1.0 / (pow(2.0, rt_res));
	#pragma omp parallel for
	for (lli i = 0; i < _rt.size(); ++i) {
		_rt[i] = (offset + i) * ppbrt;
	}
}

#endif /* SMPEAK_MATHOPERATOR_HPP_ */