/**
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
**/

#if ! defined(SAMPLEDSA_HPP)
#define SAMPLEDSA_HPP

#include <libmaus/lf/LF.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace fm
	{
		template<typename lf_type>
		struct SampledSA
		{
			typedef SampledSA<lf_type> sampled_sa_type;
			typedef typename ::libmaus::util::unique_ptr < sampled_sa_type > :: type unique_ptr_type;

			lf_type const * lf;
			
			uint64_t padn;
			uint64_t sasamplingrate;
			::libmaus::autoarray::AutoArray<uint64_t> RSA;
			::libmaus::rank::ERank222B::unique_ptr_type ARSA;
			::libmaus::autoarray::AutoArray<uint64_t> SSA;

			static uint64_t readUnsignedInt(std::istream & in)
			{
				uint64_t i;
				::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}

			static uint64_t readUnsignedInt(std::istream & in, uint64_t & s)
			{
				uint64_t i;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}
				
			static ::libmaus::autoarray::AutoArray<uint64_t> readArray64(std::istream & in)
			{
				::libmaus::autoarray::AutoArray<uint64_t> A;
				A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray<uint64_t> readArray64(std::istream & in, uint64_t & s)
			{
				::libmaus::autoarray::AutoArray<uint64_t> A;
				s += A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray<uint32_t> readArray32(std::istream & in)
			{
				::libmaus::autoarray::AutoArray<uint32_t> A;
				A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray<uint32_t> readArray32(std::istream & in, uint64_t & s)
			{
				::libmaus::autoarray::AutoArray<uint32_t> A;
				s += A.deserialize(in);
				return A;
			}

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,sasamplingrate);
				s += RSA.serialize(out);
				s += SSA.serialize(out);
				return s;
			}
			
			uint64_t deserialize(std::istream & in)
			{
				padn = ((lf->getN() + 63)/64)*64;
				uint64_t s = 0;
				sasamplingrate = readUnsignedInt(in,s);
				RSA = readArray64(in,s);
				ARSA = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type ( new ::libmaus::rank::ERank222B(RSA.get(), padn) ));
				SSA = readArray64(in,s);
				// std::cerr << "SA: " << s << " bytes = " << s*8 << " bits = " << (s+(1024*1024-1))/(1024*1024) << " mb" << " samplingrate = " << sasamplingrate << std::endl;
				return s;
			}
			
			SampledSA(lf_type const * rlf, std::istream & in) : lf(rlf) { deserialize(in); }
			SampledSA(lf_type const * rlf, std::istream & in, uint64_t & s) : lf(rlf) { s += deserialize(in); }

			SampledSA( 
				lf_type const * rlf,
				uint64_t rsasamplingrate,
				bool const verbose = false
			)
			: lf(rlf), padn(((lf->getN() + 63)/64)*64), sasamplingrate(rsasamplingrate), RSA(padn/64,false),
			  SSA ( (lf->getN() + sasamplingrate - 1)/sasamplingrate )
			{
				if ( verbose )
					std::cerr << "Computing Sampled SA...";
			
				if ( verbose )
					std::cerr << "(erasing RSA";
				std::fill(RSA.get(), RSA.get()+padn/64, 0);
				if ( verbose )
					std::cerr << ")";

				// find rank of position 0 (i.e. search terminating symbol)
				if ( verbose )
					std::cerr << "(zeroPosRank";
				uint64_t r = lf->zeroPosRank();
				if ( verbose )
					std::cerr << ")";

				uint64_t const rr = r;

				// fill vector
				if ( verbose )
					std::cerr << "(fillingBitVector";
				for ( int64_t i = (lf->getN()-1); i >= 0; --i )
				{
					if ( (i & (32*1024*1024-1)) == 0 && verbose )
						std::cerr << "(" << i/(1024*1024) << ")";
						
					r = (*lf)(r); // LF mapping

					if ( i % sasamplingrate == 0 )
						::libmaus::bitio::putBits ( RSA.get(), r, 1, 1);
					else
						::libmaus::bitio::putBits ( RSA.get(), r, 1, 0);			
				}
				if ( verbose )
					std::cerr << ")";

				assert ( r == rr );

				ARSA = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type ( new ::libmaus::rank::ERank222B(RSA.get(), padn) ));

				if ( verbose )
					std::cerr << "(fillingSampledSuffixArray";
				for ( int64_t i = (lf->getN()-1); i >= 0; --i )
				{
					if ( (i & (1024*1024-1)) == 0 && verbose )
						std::cerr << "(" << i/(1024*1024) << ")";
						
					r = (*lf)(r); // LF mapping

					if ( (i & (sasamplingrate-1)) == 0 )
						SSA [ ARSA->rank1(r) - 1 ] = i;
				}
				if ( verbose )
					std::cerr << "done)";

				assert ( r == rr );

				#if defined(SAMPLEDSA_DEBUG)
				for ( int64_t i = (lf->getN()-1); i >= 0; --i )
				{
					r = (*lf)(r); // LF mapping

					assert ( static_cast<uint64_t>(i) == (*this)[r] );
				}
				#endif

				assert ( r == rr );
				
				if ( verbose )
					std::cerr << "done." << std::endl;
			}

			uint64_t operator[](uint64_t r) const
			{	
				uint64_t o = 0;
				uint64_t tr = r;

				while ( ! ::libmaus::bitio::getBit( RSA.get(), tr) )
					o++, tr = (*lf)(tr); // LF mapping
				
				uint64_t i = ( SSA [ ARSA->rank1(tr) - 1 ] + o ) % lf->getN();
				
				return i;
			}
		};

		template<typename lf_type>
		struct SimpleSampledSA
		{
			typedef SimpleSampledSA<lf_type> sampled_sa_type;
			typedef typename ::libmaus::util::unique_ptr < sampled_sa_type > :: type unique_ptr_type;

			lf_type const * lf;
			
			uint64_t sasamplingrate;
			uint64_t sasamplingmask;
			unsigned int sasamplingshift;
			::libmaus::autoarray::AutoArray<uint64_t> SSA;
			
			void setSamplingRate(uint64_t const samplingrate)
			{
				sasamplingrate = samplingrate;
				sasamplingmask = sasamplingrate-1;

				sasamplingshift = 0;
				uint64_t tsasamplingrate = sasamplingrate;
				while ( !(tsasamplingrate & 1) )
				{
					tsasamplingrate >>= 1;
					sasamplingshift++;
				}
				assert ( tsasamplingrate == 1 );
				assert ( (1ull << sasamplingshift) == sasamplingrate );
			}

			static uint64_t readUnsignedInt(std::istream & in)
			{
				uint64_t i;
				::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}

			static uint64_t readUnsignedInt(std::istream & in, uint64_t & s)
			{
				uint64_t i;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&i);
				return i;
			}
				
			static ::libmaus::autoarray::AutoArray<uint64_t> readArray64(std::istream & in)
			{
				::libmaus::autoarray::AutoArray<uint64_t> A;
				A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray<uint64_t> readArray64(std::istream & in, uint64_t & s)
			{
				::libmaus::autoarray::AutoArray<uint64_t> A;
				s += A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray<uint32_t> readArray32(std::istream & in)
			{
				::libmaus::autoarray::AutoArray<uint32_t> A;
				A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray<uint32_t> readArray32(std::istream & in, uint64_t & s)
			{
				::libmaus::autoarray::AutoArray<uint32_t> A;
				s += A.deserialize(in);
				return A;
			}

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,sasamplingrate);
				s += SSA.serialize(out);
				return s;
			}
			
			uint64_t deserialize(std::istream & in)
			{
				uint64_t s = 0;
				setSamplingRate(readUnsignedInt(in,s));
				SSA = readArray64(in,s);
				// std::cerr << "SA: " << s << " bytes = " << s*8 << " bits = " << (s+(1024*1024-1))/(1024*1024) << " mb" << " samplingrate = " << sasamplingrate << std::endl;
				return s;
			}
			
			static unique_ptr_type load(lf_type const * lf, std::string const & filename)
			{
				libmaus::aio::CheckedInputStream CIS(filename);
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(lf,CIS)));
			}
			
			SimpleSampledSA(lf_type const * rlf, std::istream & in) : lf(rlf) { deserialize(in); }
			SimpleSampledSA(lf_type const * rlf, std::istream & in, uint64_t & s) : lf(rlf) { s += deserialize(in); }

			SimpleSampledSA(lf_type const * rlf,
				uint64_t const rsasamplingrate,
				::libmaus::autoarray::AutoArray<uint64_t> & rSSA)
			: lf(rlf), SSA(rSSA) 
			{
				setSamplingRate(rsasamplingrate);
			}

			SimpleSampledSA(lf_type const * rlf, uint64_t rsasamplingrate, bool const verbose = false)
			: lf(rlf), 
			  sasamplingrate(rsasamplingrate),
			  SSA ( (lf->getN() + sasamplingrate - 1)/sasamplingrate )
			{
				setSamplingRate(rsasamplingrate);
			
				if ( verbose )
					std::cerr << "Computing simple Sampled SA...";
			
				// find rank of position 0 (i.e. search terminating symbol)
				if ( verbose )
					std::cerr << "(zeroPosRank";
				uint64_t r = lf->zeroPosRank();
				if ( verbose )
					std::cerr << ")";

				uint64_t const rr = r;

				if ( verbose )
					std::cerr << "(fillingSimpleSampledSuffixArray";
				for ( int64_t i = (lf->getN()-1); i >= 0; --i )
				{
					if ( (i & (1024*1024-1)) == 0 && verbose )
						std::cerr << "(" << i/(1024*1024) << ")";
						
					r = (*lf)(r); // LF mapping

					if ( (r & (sasamplingrate-1)) == 0 )
						SSA [ r / sasamplingrate ] = i;
				}
				if ( verbose )
					std::cerr << "done)";

				assert ( r == rr );

				#if defined(SAMPLEDSA_DEBUG)
				std::cerr << "(Checking...";
				for ( int64_t i = (lf->getN()-1); i >= 0; --i )
				{
					if ( (i & (32*1024-1)) == 0 )
						std::cerr << "(" << i/(32*1024) << ")";

					r = (*lf)(r); // LF mapping

					if ( i != (*this)[r] )
					{
						std::cerr 
							<< "r=" << r << " i=" << i << " t=" << (*this)[r] << std::endl;
					}
					
					assert ( static_cast<uint64_t>(i) == (*this)[r] );
				}
				std::cerr << "done)";
				#endif

				assert ( r == rr );
				
				if ( verbose )
					std::cerr << "done." << std::endl;
			}

			uint64_t operator[](uint64_t r) const
			{
				// assert ( r < lf->getN() );
			
				uint64_t o = 0;
				uint64_t tr = r;

				while ( (tr & sasamplingmask) )
					o++, tr = (*lf)(tr); // LF mapping
				
				uint64_t i = SSA [ tr >> sasamplingshift ] + o;
				
				if ( i >= lf->getN() )
					i -= lf->getN();
				
				// std::cerr << "r=" << r << " i=" << i << std::endl;
				
				return i;
			}
		};
		}
	}
	#endif
