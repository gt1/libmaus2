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

#if ! defined(COMPUTEFASTXINTERVALS_HPP)
#define COMPUTEFASTXINTERVALS_HPP

#include <libmaus/fastx/FastInterval.hpp>
#include <iostream>

namespace libmaus
{
	namespace fastx
	{
		template<typename reader_type>
		struct ComputeFastXIntervals
		{
			static std::vector< ::libmaus::fastx::FastInterval > computeFastXIntervals(
				std::vector<std::string> const & filenames,
				uint64_t const indexstep = 512ull*1024ull,
				std::ostream & logfile = std::cerr)
			{
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(filenames.size()); ++i )
					reader_type::buildIndex(filenames[i],indexstep);
			
				logfile << "Computing fastx intervals...";
				std::vector< ::libmaus::fastx::FastInterval > V = reader_type::buildIndex(filenames, indexstep );
				logfile << "done." << std::endl;
				return V;
			}		
		};
	}
}
#endif
