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
#if ! defined(LIBMAUS_BAMBAM_MDSTRINGCOMPUTATIONCONTEXT_HPP)
#define LIBMAUS_BAMBAM_MDSTRINGCOMPUTATIONCONTEXT_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bambam/BamAuxFilterVector.hpp>
#include <libmaus/bambam/CigarOperation.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct MdStringComputationContext
		{
			libmaus::autoarray::AutoArray<libmaus::bambam::cigar_operation> cigop;
			libmaus::autoarray::AutoArray<char> md;
			libmaus::autoarray::AutoArray<uint8_t> T0;
			libmaus::autoarray::AutoArray<uint8_t> T1;
			libmaus::bambam::BamAuxFilterVector auxvec;
			
			void checkSize(uint64_t const cigsum)
			{
				if ( 2*cigsum+1 > md.size() )
					md = libmaus::autoarray::AutoArray<char>(2*cigsum+1);
			}
			
			MdStringComputationContext()
			: T0(256,false), T1(256,false)
			{
				std::fill(T0.begin(),T0.end(),4);
				std::fill(T1.begin(),T1.end(),5);
				T0['A'] = T0['a'] =  T1['A'] = T1['a'] = 0;
				T0['C'] = T0['c'] =  T1['C'] = T1['c'] = 1;
				T0['G'] = T0['g'] =  T1['G'] = T1['g'] = 2;
				T0['T'] = T0['t'] =  T1['T'] = T1['t'] = 3;
				auxvec.set("MD");
				auxvec.set("NM");
			}

			static char * putNumber(char * p, uint32_t n)
			{
				uint32_t tn = n;
				unsigned int numlen = 0;
				unsigned int i = 0;
				char * c = 0;
				if ( !tn ) numlen = 1;
				while ( tn )
					tn /= 10, numlen++;
				c = (char *)alloca(numlen);
				tn = n;
				while ( i < numlen )
				{
					c[numlen-i-1] = (tn % 10)+'0';
					i++;
					tn /= 10;
				} 
				for ( i = 0; i < numlen; ++i )
					*(p++) = c[i];

				return p;
			}
		};
	}
}
#endif
