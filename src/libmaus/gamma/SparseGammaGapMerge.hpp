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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPMERGE_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPMERGE_HPP

#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapMerge
		{
			static void merge(
				std::istream & stream_in_a,
				std::istream & stream_in_b,
				std::ostream & stream_out
			)
			{
				libmaus::aio::SynchronousGenericInput<uint64_t> SGIa(stream_in_a,64*1024);
				libmaus::aio::SynchronousGenericInput<uint64_t> SGIb(stream_in_b,64*1024);
				libmaus::aio::SynchronousGenericOutput<uint64_t> SGO(stream_out,64*1024);
				
				libmaus::gamma::GammaDecoder< libmaus::aio::SynchronousGenericInput<uint64_t> > adec(SGIa);
				libmaus::gamma::GammaDecoder< libmaus::aio::SynchronousGenericInput<uint64_t> > bdec(SGIb);
				libmaus::gamma::GammaEncoder< libmaus::aio::SynchronousGenericOutput<uint64_t> > oenc(SGO);
				
				std::pair<uint64_t,uint64_t> aval;
				std::pair<uint64_t,uint64_t> bval;

				aval.first = adec.decode();
				aval.second = adec.decode();

				bval.first = bdec.decode();
				bval.second = bdec.decode();
				
				int64_t prevkey = -1;
				
				while ( aval.second && bval.second )
				{
					if ( aval.first == bval.first )
					{
						oenc.encode(static_cast<int64_t>(aval.first) - prevkey - 1);
						oenc.encode(aval.second + bval.second);
						
						prevkey = aval.first;
						
						aval.first += adec.decode() + 1;
						aval.second = adec.decode();
						bval.first += bdec.decode() + 1;
						bval.second = bdec.decode();
					}
					else if ( aval.first < bval.first )
					{
						oenc.encode(static_cast<int64_t>(aval.first) - prevkey - 1);
						oenc.encode(aval.second);
						
						prevkey = aval.first;
						
						aval.first += adec.decode() + 1;
						aval.second = adec.decode();
					}
					else // if ( bval.first < aval.first )
					{
						oenc.encode(static_cast<int64_t>(bval.first) - prevkey - 1);
						oenc.encode(bval.second);
						
						prevkey = bval.first;
						
						bval.first += bdec.decode() + 1;
						bval.second = bdec.decode();
					}
				}
				
				while ( aval.second )
				{
					oenc.encode(static_cast<int64_t>(aval.first) - prevkey - 1);
					oenc.encode(aval.second);
						
					prevkey = aval.first;
						
					aval.first += adec.decode() + 1;
					aval.second = adec.decode();				
				}
				
				while ( bval.second )
				{
					oenc.encode(static_cast<int64_t>(bval.first) - prevkey - 1);
					oenc.encode(bval.second);
						
					prevkey = bval.first;
						
					bval.first += bdec.decode() + 1;
					bval.second = bdec.decode();
				}
				
				oenc.encode(0);
				oenc.encode(0);
				oenc.flush();
				SGO.flush();
				stream_out.flush();
			}		
		};
	}
}
#endif
