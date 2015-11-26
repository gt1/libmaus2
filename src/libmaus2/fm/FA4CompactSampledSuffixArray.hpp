/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_FM_FA4COMPACTSAMPLEDSUFFIXARRAY_HPP)
#define LIBMAUS2_FM_FA4COMPACTSAMPLEDSUFFIXARRAY_HPP

#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace fm
	{
		struct FA4CompactSampledSuffixArray
		{
			typedef FA4CompactSampledSuffixArray this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t samplingrate;
			uint64_t samplingmask;
			libmaus2::bitio::SynchronousCompactArray CA;

			FA4CompactSampledSuffixArray(std::istream & in)
			: samplingrate(libmaus2::util::NumberSerialisation::deserialiseNumber(in)), samplingmask(samplingrate-1), CA(in)
			{

			}

			template<typename rank_type, typename index_type>
			FA4CompactSampledSuffixArray(rank_type const & Prank, index_type const & Pindex, std::string const & isaname)
			: samplingrate(1), samplingmask(samplingrate-1), CA(Prank.size(),Pindex.coordbits,0 /* no padding */,false /* do not erase */)
			{
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISAISI(new libmaus2::aio::InputStreamInstance(isaname));
				uint64_t isasamplingrate;
				libmaus2::serialize::Serialize<uint64_t>::deserialize(*PISAISI,&isasamplingrate);
				uint64_t isaarraysize;
				libmaus2::serialize::Serialize<uint64_t>::deserialize(*PISAISI,&isaarraysize);

				uint64_t const isaintervalsize = (isaarraysize+numthreads-1)/numthreads;
				uint64_t const isaintervals = (isaarraysize+isaintervalsize-1)/isaintervalsize;
				std::vector < std::pair<uint64_t,uint64_t> > Visa(isaintervals);

				for ( uint64_t t = 0; t < isaintervals; ++t )
				{
					uint64_t const ilow = t * isaintervalsize;
					assert ( ilow < isaarraysize );
					uint64_t const p = ilow * isasamplingrate;
					PISAISI->seekg((2+ilow)*sizeof(uint64_t));
					uint64_t r;
					libmaus2::serialize::Serialize<uint64_t>::deserialize(*PISAISI,&r);
					Visa[t] = std::pair<uint64_t,uint64_t>(p,r);
				}

				assert ( Visa.size() == 0 || Visa[0].first == 0 );

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( uint64_t i = 0; i < Visa.size(); ++i )
				{
					uint64_t i1 = (i+1)%Visa.size();
					uint64_t p = Visa[i1].first;
					uint64_t r = Visa[i1].second;
					if ( ! p )
						p = Prank.size();
					uint64_t pstop = Visa[i].first;

					while ( p-- != pstop )
					{
						r = Prank.simpleLF(r);
						if ( ! (r&samplingmask) )
							CA.set(r, Pindex.mapCoordinatesToWord(p));
					}
				}
			}


			static unique_ptr_type load(std::istream & in)
			{
				unique_ptr_type tptr(new this_type(in));
				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type load(std::string const & s)
			{
				libmaus2::aio::InputStreamInstance ISI(s);
				unique_ptr_type tptr(new this_type(ISI));
				return UNIQUE_PTR_MOVE(tptr);
			}

			template<typename rank_type, typename index_type>
			static unique_ptr_type construct(rank_type const & Prank, index_type const & Pindex, std::string const & isaname)
			{
				unique_ptr_type tptr(new this_type(Prank,Pindex,isaname));
				return UNIQUE_PTR_MOVE(tptr);
			}

			uint64_t operator[](uint64_t const i) const
			{
				return CA[i];
			}

			uint64_t size() const
			{
				return CA.size();
			}
		};
	}
}
#endif
