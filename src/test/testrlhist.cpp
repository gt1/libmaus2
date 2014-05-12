/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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

#include <libmaus/huffman/RLDecoder.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		std::string const fn = arginfo.getRestArg<std::string>(0);
		libmaus::util::Histogram::unique_ptr_type thist(libmaus::huffman::RLDecoder::getRunLengthHistogram(fn));
		std::map<uint64_t,uint64_t> const M = thist->getByType<uint64_t>();

		uint64_t runtotal = 0;
		uint64_t total = 0;
		for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin();
			ita != M.end(); ++ita )
		{
			runtotal += ita->second;
			total += ita->first * ita->second;
		}
			
		uint64_t const n = libmaus::huffman::RLDecoder::getLength(fn);
		
		std::cout << "[S]\taverage run-length\t" << thist->avg() << std::endl;
		
		assert ( total == n );

		uint64_t acc = 0;
		bool accthresset = false;
		uint64_t accthres = 0;
		double const thres = 0.99999;
		
		for ( 
			std::map<uint64_t,uint64_t>::const_iterator ita = M.begin();
			ita != M.end(); 
			++ita 
		)
		{
			acc += ita->second;
			if ( ! accthresset && static_cast<double>(acc) / runtotal >= thres )
			{
				accthres = ita->first;
				accthresset = true;
			}
			std::cout << ita->first << "\t" << ita->second << "\t" << static_cast<double>(acc) / runtotal << std::endl;
		}
		
		uint64_t eacc = 0;
		uint64_t ediv = 0;
		for ( 
			std::map<uint64_t,uint64_t>::const_iterator ita = M.begin();
			ita != M.end(); 
			++ita 
		)
			if ( ita->first <= accthres )
			{
				eacc += ita->first * ita->second;
				ediv += ita->second;
			}
		
		std::cerr << "[S]\taverage run-length below acc thres\t" << thres << "\t" << static_cast<double>(eacc)/ediv << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
