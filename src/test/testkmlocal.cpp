/*
    libmaus2
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
#include <libmaus2/LibMausConfig.hpp>

#include <libmaus2/clustering/KMeans.hpp>
#include <libmaus2/random/UniformUnitRandom.hpp>

#if defined(LIBMAUS2_HAVE_KMLOCAL)
#include <libmaus2/fastx/CompactFastQMultiBlockReader.hpp>
#include <libmaus2/fastx/CompactFastQBlockGenerator.hpp>
#include <libmaus2/fastx/CompactFastQContainerDictionaryCreator.hpp>
#include <libmaus2/fastx/CompactFastQContainer.hpp>
#include <libmaus2/fastx/FqWeightQuantiser.hpp>
#include <libmaus2/fastx/CompactFastEncoder.hpp>

int kmlocalmain(::libmaus2::util::ArgInfo const & arginfo)
{
	try
	{
	
		{
		std::vector<double> V;
		V.push_back(0);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(2);
		V.push_back(3);
		V.push_back(4);
		V.push_back(5);
		V.push_back(6);
		V.push_back(7);
		V.push_back(8);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(9);
		V.push_back(10);
		
		for ( uint64_t k = 1; k <= V.size(); ++k )
		{	
			std::cerr << "k==" << k << " " << std::set<double>(V.begin(),V.end()).size() << std::endl;
			::libmaus2::quantisation::Quantiser::unique_ptr_type quant(libmaus2::quantisation::ClusterComputation::constructQuantiser(V,k));
		}
		}

		// ::libmaus2::fastx::FqWeightQuantiser::statsRun(arginfo,arginfo.restargs,std::cerr);	
		std::string scont = ::libmaus2::fastx::CompactFastQBlockGenerator::encodeCompactFastQContainer(arginfo.restargs,0,3);
		std::istringstream textistr(scont);
		::libmaus2::fastx::CompactFastQContainer CFQC(textistr);
		::libmaus2::fastx::CompactFastQContainer::pattern_type cpat;
		
		for ( uint64_t i = 0; i < CFQC.size(); ++i )
		{
			CFQC.getPattern(cpat,i);
			std::cout << cpat;
		}

		#if 1
		std::ostringstream ostr;
		::libmaus2::fastx::CompactFastQBlockGenerator::encodeCompactFastQFile(arginfo.restargs,0,256,6/* qbits */,ostr);
		std::istringstream istr(ostr.str());
		// CompactFastQSingleBlockReader<std::istream> CFQSBR(istr);
		::libmaus2::fastx::CompactFastQMultiBlockReader<std::istream> CFQMBR(istr);
		::libmaus2::fastx::CompactFastQMultiBlockReader<std::istream>::pattern_type pattern;
		while ( CFQMBR.getNextPatternUnlocked(pattern) )
		{
			std::cout << pattern;
		}
		#endif

		// ::libmaus2::fastx::FqWeightQuantiser::rephredFastq(arginfo.restargs,arginfo);
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
#endif

#include <libmaus2/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	::libmaus2::util::ArgInfo arginfo(argc,argv);

	#if 0
	{
		uint64_t const n = 32*1024;
		std::vector<double> V(n);
		#if 0
		V.push_back(0);
		V.push_back(0);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(2);
		V.push_back(2);
		#endif
		
		for ( uint64_t i = 0; i < V.size(); ++i )
			V[i] = libmaus2::random::UniformUnitRandom::uniformUnitRandom();
		
		// V.push_back(2);
		uint64_t const loops = 100;
		
		for ( uint64_t k = 1; k < V.size(); ++k )
		{
			std::vector<double> const R = libmaus2::clustering::KMeans::kmeans(V.begin(), V.size(), k);
		
			std::cerr << "k=" << k << std::endl;
		
			#if 0
			for ( uint64_t i = 0; i < R.size(); ++i )
				std::cerr << "R[" << i << "]=" << R[i] << std::endl;
			#endif
		}
	}
	#endif

	{
		uint64_t const n = 32*1024;
		std::vector<std::vector<double> > V(n);
		#if 0
		V.push_back(0);
		V.push_back(0);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(1);
		V.push_back(2);
		V.push_back(2);
		#endif
		
		for ( uint64_t i = 0; i < V.size(); ++i )
		{	
			for ( uint64_t j = 0; j < 3; ++j )
				V[i].push_back(libmaus2::random::UniformUnitRandom::uniformUnitRandom());
		}
		
		// V.push_back(2);
		uint64_t const loops = 100;
		
		for ( uint64_t k = 1; k < V.size(); ++k )
		{
			#if 0
			std::vector<std::vector<double>> const R = libmaus2::clustering::KMeans::kmeans(V, k);
			std::vector<std::vector<double>> const Rnopp = libmaus2::clustering::KMeans::kmeans(V, k, false);
		
			std::cerr << "k3=" << k << " error " << libmaus2::clustering::KMeans::error(V,R) << " " << libmaus2::clustering::KMeans::error(V,Rnopp) << std::endl;
			#endif

			std::vector< std::vector<double> > const Rnopp = libmaus2::clustering::KMeans::kmeans(V, k, false);
			std::cerr << "k3=" << k << " error " << libmaus2::clustering::KMeans::error(V,Rnopp) << " " << libmaus2::clustering::KMeans::silhouette(V,Rnopp) << std::endl;
		
			#if 0
			for ( uint64_t i = 0; i < R.size(); ++i )
				std::cerr << "R[" << i << "]=" << R[i] << std::endl;
			#endif
		}
	}

	
	#if defined(LIBMAUS2_HAVE_KMLOCAL)
	return kmlocalmain(arginfo);
	#else
	std::cerr << "kmlocal is not available." << std::endl;
	return EXIT_FAILURE;
	#endif
}
