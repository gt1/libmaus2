/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <cstdio>
#include <libmaus/quantisation/KMLocalInterface.hpp>
#include <libmaus/quantisation/KmeansDataType.hpp>

#if defined(LIBMAUS_HAVE_KMLOCAL)
#include "KMlocal.h"
#include <vector>
#include <iostream>
#include <algorithm>

static std::vector<double> computeKMeans(
	KMdata & datapoints,
	uint64_t const k,
	uint64_t const runs,
	bool const debug
	)
{
	KMterm const term(runs, 0, 0, 0, 0.10, 0.10, 3, 0.50, 10, 0.95);
	datapoints.buildKcTree();
	KMfilterCenters ctrs(k, datapoints);
	KMlocalHybrid kmAlg(ctrs, term);
	ctrs = kmAlg.execute();

	std::vector<double> centrevector;
	for ( uint64_t i = 0; i < k; ++i )
	{
		centrevector.push_back(ctrs[i][0]);
		if ( debug )
			std::cerr << "centre[" << i << "]=" << ctrs[i][0] << std::endl;
	}

	std::sort(centrevector.begin(),centrevector.end());
	
	return centrevector;
}

static std::vector<double> * copyVector(std::vector<double> const & V)
{
	try
	{
		std::vector<double> * P = new std::vector<double>(V.size());
		std::copy ( V.begin(), V.end(), P->begin() );
		return P;
	}
	catch(...)
	{
		return 0;
	}
}

static std::vector<double> * kmeans(std::vector<double>::const_iterator vita, uint64_t const rn, uint64_t const k, uint64_t const runs, bool const debug = false)
{
	try
	{
		KMdata datapoints(1, rn);
		for ( uint64_t i = 0; i < rn; ++i )
			datapoints[i][0] = vita[i];
		
		std::vector<double> const Q = computeKMeans(datapoints,k,runs,debug);
	
		return copyVector(Q);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return 0;
	}
}
static std::vector<double> * kmeans(std::vector<unsigned int>::const_iterator vita, uint64_t const rn, uint64_t const k, uint64_t const runs, bool const debug = false)
{
	try
	{
		KMdata datapoints(1, rn);
		for ( uint64_t i = 0; i < rn; ++i )
			datapoints[i][0] = vita[i];
		
		std::vector<double> const Q = computeKMeans(datapoints,k,runs,debug);
	
		return copyVector(Q);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return 0;
	}
}
static std::vector<double> * kmeans(std::vector<uint64_t>::const_iterator vita, uint64_t const rn, uint64_t const k, uint64_t const runs, bool const debug = false)
{
	try
	{
		KMdata datapoints(1, rn);
		for ( uint64_t i = 0; i < rn; ++i )
			datapoints[i][0] = vita[i];
		
		std::vector<double> const Q = computeKMeans(datapoints,k,runs,debug);
	
		return copyVector(Q);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return 0;
	}
}


static void * kmeansWrapperByType(unsigned int const type, void * pvita, uint64_t const rn, uint64_t const k, uint64_t const runs, unsigned int const debug)
{
	typedef libmaus::quantisation::kmeans_data_type data_type_type;

	switch ( static_cast< data_type_type >(type) )
	{
		case ::libmaus::quantisation::type_double: return kmeans(*(reinterpret_cast<std::vector<double>::const_iterator *>(pvita)),rn,k,runs,debug);
		case ::libmaus::quantisation::type_unsigned_int: return kmeans(*(reinterpret_cast<std::vector<unsigned int>::const_iterator *>(pvita)),rn,k,runs,debug);
		case ::libmaus::quantisation::type_uint64_t: return kmeans(*(reinterpret_cast<std::vector<uint64_t>::const_iterator *>(pvita)),rn,k,runs,debug);
		default: return 0;
	}
}

extern "C" {
	void * libmaus_quantisation_kmeansWrapperByTypeC(unsigned int const type, void * pvita, uint64_t const rn, uint64_t const k, uint64_t const runs, unsigned int const debug)
	{
		return kmeansWrapperByType(type,pvita,rn,k,runs,debug);
	}
}
#else
extern "C" {
	void * libmaus_quantisation_kmeansWrapperByTypeC(unsigned int const /* type */, void * /* pvita */, uint64_t const /* rn */, uint64_t const /* k */, uint64_t const /* runs */, unsigned int const /* debug */)
	{
		fprintf(stderr,"libmaus_quantisation_kmeansWrapperByTypeC called, but clustering code is not present.");
		return 0;
	}
}
#endif
