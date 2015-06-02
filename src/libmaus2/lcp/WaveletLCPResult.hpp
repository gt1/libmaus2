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
#if ! defined(LIBMAUS2_LCP_WAVELETLCPRESULT_HPP)
#define LIBMAUS2_LCP_WAVELETLCPRESULT_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace lcp
	{		
		struct WaveletLCPResult
		{
			typedef WaveletLCPResult this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef uint8_t small_elem_type;
			::libmaus2::autoarray::AutoArray<uint8_t>::unique_ptr_type WLCP;
			::libmaus2::autoarray::AutoArray<uint64_t>::unique_ptr_type U;
			::libmaus2::rank::ERank222B::unique_ptr_type Urank;
			::libmaus2::autoarray::AutoArray<uint64_t>::unique_ptr_type LLCP;
			
			typedef libmaus2::util::ConstIterator<WaveletLCPResult,uint64_t> const_iterator;
			
			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,WLCP->size()-1);
			}
			
			void serialise(std::ostream & out) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,U.get()!=0);
				WLCP->serialize(out);
				if ( U.get() )
				{
					U->serialize(out);
					LLCP->serialize(out);
				}
			}
			
			static unique_ptr_type load(std::string const & filename)
			{
				libmaus2::aio::CheckedInputStream istr(filename);
				
				unique_ptr_type P ( new this_type(istr) );
				
				if ( ! istr )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to load file " << filename << " in WaveletLCPResult::load()" << std::endl;
					se.finish();
					throw se;					
				}
				
				return UNIQUE_PTR_MOVE(P);
			}
			
			WaveletLCPResult(std::istream & in)
			{
				bool const haveU = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				::libmaus2::autoarray::AutoArray<uint8_t>::unique_ptr_type tWLCP(new ::libmaus2::autoarray::AutoArray<uint8_t>(in));
				WLCP = UNIQUE_PTR_MOVE(tWLCP);
				if ( haveU )
				{
					::libmaus2::autoarray::AutoArray<uint64_t>::unique_ptr_type tU(new ::libmaus2::autoarray::AutoArray<uint64_t>(in));
					U = UNIQUE_PTR_MOVE(tU);
					::libmaus2::autoarray::AutoArray<uint64_t>::unique_ptr_type tLLCP(new ::libmaus2::autoarray::AutoArray<uint64_t>(in));
					LLCP = UNIQUE_PTR_MOVE(tLLCP);
					uint64_t const n = WLCP->size()-1;
					uint64_t const n64 = (n+63)/64;
					::libmaus2::rank::ERank222B::unique_ptr_type tUrank(new ::libmaus2::rank::ERank222B(U->get(),n64*64));
					Urank = UNIQUE_PTR_MOVE(tUrank);
				}
			}
			
			WaveletLCPResult(uint64_t const n)
			: WLCP(::libmaus2::autoarray::AutoArray<uint8_t>::unique_ptr_type(new ::libmaus2::autoarray::AutoArray<uint8_t>(n+1)))
			{
			
			}
			
			bool isUnset(uint64_t i) const
			{
				if ( (*WLCP)[i] != std::numeric_limits<small_elem_type>::max() )
					return false;
				else if ( !(U.get()) )
					return true;
				else
				{
					assert ( ::libmaus2::bitio::getBit(U->get(),i) );
					return (*LLCP) [ Urank->rank1(i)-1 ] == std::numeric_limits<uint64_t>::max();
				}
			}
			
			void set(uint64_t const i, uint64_t const v)
			{
				assert ( (*WLCP)[i] == std::numeric_limits<small_elem_type>::max() ) ;
				assert ( U->get() );
				assert ( ::libmaus2::bitio::getBit(U->get(),i) );
				(*LLCP) [ Urank->rank1(i)-1 ] = v;
			}
			
			uint64_t operator[](uint64_t const i) const
			{
				if ( ! U )
					return (*WLCP)[i];
				else if ( ! ::libmaus2::bitio::getBit(U->get(),i) )
					return (*WLCP)[i];
				else
					return (*LLCP) [ Urank->rank1(i)-1 ];
			}
			
			uint64_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
			
			void setupLargeValueVector(uint64_t const n, small_elem_type const unset)
			{
				// set up large value bit vector
				uint64_t const n64 = (n+63)/64;
				::libmaus2::autoarray::AutoArray<uint64_t>::unique_ptr_type tU(new ::libmaus2::autoarray::AutoArray<uint64_t>(n64));
				this->U = UNIQUE_PTR_MOVE(tU);
				for ( uint64_t i = 0; i < n; ++i )
					if ( (*WLCP)[i] == unset )
						::libmaus2::bitio::putBit(this->U->get(),i,1);
				// set up rank dictionary for large value bit vector
				::libmaus2::rank::ERank222B::unique_ptr_type tUrank(new ::libmaus2::rank::ERank222B(this->U->get(),n64*64));
				this->Urank = UNIQUE_PTR_MOVE(tUrank);
				// set up array for large values
				::libmaus2::autoarray::AutoArray<uint64_t>::unique_ptr_type tLLCP(new ::libmaus2::autoarray::AutoArray<uint64_t>(this->Urank->rank1(n-1)));
				this->LLCP = UNIQUE_PTR_MOVE(tLLCP);
				// mark all large values as unset
				for ( uint64_t i = 0; i < this->LLCP->size(); ++i )
					(*(this->LLCP))[i] = std::numeric_limits<uint64_t>::max();				
			}
		};
	}
}
#endif
