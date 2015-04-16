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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPENCODER_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPENCODER_HPP

#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapEncoder
		{
			typedef SparseGammaGapEncoder this_type;
			
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef libmaus::aio::SynchronousGenericOutput<uint64_t> stream_type;
			
			libmaus::aio::SynchronousGenericOutput<uint64_t> SGO;
			int64_t prevkey;
			libmaus::gamma::GammaEncoder<stream_type> genc;
		
			SparseGammaGapEncoder(std::ostream & out, int64_t const rprevkey = -1) : SGO(out,64*1024), prevkey(rprevkey), genc(SGO)
			{
			}
			
			void encode(uint64_t const key, uint64_t const val)
			{
				int64_t const dif = (static_cast<int64_t>(key)-prevkey)-1;
				genc.encode(dif);
				prevkey = key;
				assert ( val );
				genc.encode(val);
			}
			
			void term()
			{
				genc.encode(0);
				genc.encode(0);
				genc.flush();
				SGO.flush();
			}
			
			template<typename it>
			static void encodeArray(
				it const ita, 
				it const ite, 
				std::ostream & out
			)
			{
				std::sort(ita,ite);
				
				this_type enc(out);
				
				it itl = ita;
				
				while ( itl != ite )
				{
					it ith = itl;
					
					while ( ith != ite && *ith == *itl )
						++ith;
					
					enc.encode(*itl,ith-itl);
					
					itl = ith;
				}
				
				enc.term();
				out.flush();
			}

			template<typename it>
			static void encodeArray(
				it const ita, 
				it const ite, 
				std::string const & fn
			)
			{
				libmaus::aio::CheckedOutputStream COS(fn);
				encodeArray(ita,ite,COS);
			}
		};	
	}
}
#endif
