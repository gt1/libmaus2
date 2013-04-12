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
#if ! defined(LIBMAUS_SUFFIXSORT_CIRCULARSUFFIXCOMPARATOR_HPP)
#define LIBMAUS_SUFFIXSORT_CIRCULARSUFFIXCOMPARATOR_HPP

#include <libmaus/aio/CircularWrapper.hpp>
#include <libmaus/bitio/CompactDecoderBuffer.hpp>
#include <stdexcept>

namespace libmaus
{
	namespace suffixsort
	{
		struct CircularWrapperFactory
		{
			typedef ::libmaus::aio::CircularWrapper wrapper_type;
			typedef wrapper_type::unique_ptr_type wrapper_ptr_type;
			typedef int int_type;
			
			static wrapper_ptr_type construct(std::string const & filename, uint64_t const offset)
			{
				return UNIQUE_PTR_MOVE(wrapper_ptr_type(new wrapper_type(filename,offset)));
			}
		};

		struct CompactDecoderWrapperFactory
		{
			typedef ::libmaus::aio::CompactCircularWrapper wrapper_type;
			typedef wrapper_type::unique_ptr_type wrapper_ptr_type;
			typedef int int_type;
				
			static wrapper_ptr_type construct(std::string const & filename, uint64_t const offset)
			{
				wrapper_ptr_type W(new wrapper_type(filename,offset));
				return UNIQUE_PTR_MOVE(W);
			}
		};

		struct PacDecoderWrapperFactory
		{
			typedef ::libmaus::aio::PacCircularWrapper wrapper_type;
			typedef wrapper_type::unique_ptr_type wrapper_ptr_type;
			typedef int int_type;
				
			static wrapper_ptr_type construct(std::string const & filename, uint64_t const offset)
			{
				wrapper_ptr_type W(new wrapper_type(filename,offset));
				return UNIQUE_PTR_MOVE(W);
			}
		};

		struct PacTermDecoderWrapperFactory
		{
			typedef ::libmaus::aio::PacTermCircularWrapper wrapper_type;
			typedef wrapper_type::unique_ptr_type wrapper_ptr_type;
			typedef int int_type;
				
			static wrapper_ptr_type construct(std::string const & filename, uint64_t const offset)
			{
				wrapper_ptr_type W(new wrapper_type(filename,offset));
				return UNIQUE_PTR_MOVE(W);
			}
		};

		struct Utf8DecoderWrapperFactory
		{
			typedef ::libmaus::aio::Utf8CircularWrapperWrapper wrapper_type;
			typedef wrapper_type::unique_ptr_type wrapper_ptr_type;
			typedef wint_t int_type;

			static wrapper_ptr_type construct(std::string const & filename, uint64_t const offset)
			{
				wrapper_ptr_type W(new wrapper_type(filename,offset));
				return UNIQUE_PTR_MOVE(W);
			}
		};
	
		template<typename _factory_type>
		struct CircularSuffixComparatorTemplate
		{
			typedef _factory_type factory_type;
			typedef CircularSuffixComparatorTemplate<factory_type> this_type;
		
			std::string const & filename;
			uint64_t const fs;
			
			CircularSuffixComparatorTemplate(
				std::string const & rfilename, 
				uint64_t const rfs
			)
			: filename(rfilename), fs(rfs)
			{
			}
			
			bool operator()(uint64_t pa, uint64_t pb) const
			{
				assert ( fs );
				
				pa %= fs;
				pb %= fs;
				
				if ( pa == pb )
					return false;
				
				typename factory_type::wrapper_ptr_type cwa = UNIQUE_PTR_MOVE(factory_type::construct(filename,pa));
				typename factory_type::wrapper_ptr_type cwb = UNIQUE_PTR_MOVE(factory_type::construct(filename,pb));
			
				for ( uint64_t i = 0; i < fs; ++i )
				{
					typename factory_type::int_type const ca = cwa->get();
					typename factory_type::int_type const cb = cwb->get();
					
					assert ( ca >= 0 );
					assert ( cb >= 0 );
					
					if ( ca != cb )
						return ca < cb;
				}
				
				return pa < pb;
			}
			
			template<typename iterator>
			bool operator()(iterator texta, iterator texte, uint64_t pb) const
			{
				assert ( fs );
				pb %= fs;
				
				typename factory_type::wrapper_ptr_type cwb = UNIQUE_PTR_MOVE(factory_type::construct(filename,pb));
				
				while ( texta != texte )
				{
					typename factory_type::int_type const ca = *(texta++);
					typename factory_type::int_type const cb = cwb->get();
					
					assert ( ca >= 0 );
					assert ( cb >= 0 );

					if ( ca != cb )
						return ca < cb;
				}
				
				std::runtime_error rt("CircularSuffixComparator::operator(iterator,iterator,uint64_t): comparison extends beyond end of given text.");
				throw rt;
			}

			// search for smallest suffix in SA that equals q or is larger than q
			template<typename saidx_t>
			static uint64_t suffixSearch(saidx_t const * SA, uint64_t const n, uint64_t const o, uint64_t const q, std::string const & filename, uint64_t const fs)
			{
				uint64_t l = 0, r = n;
				this_type CSC(filename,fs);
				
				// binary search
				while ( r-l > 2 )
				{
					uint64_t const m = (l+r)>>1;		

					// is m too small? i.e. SA[m] < q
					if ( CSC(SA[m]+o,q) )
					{
						l = m+1;
					}
					// m is large enough
					else
					{
						r = m+1;
					}
				}
				
				// ! (SA[l] >= q) <=> q < SA[l]
				while ( l < r && CSC(SA[l]+o,q) )
					++l;
				
				if ( l < n )
				{
					// SA[l] >= q
					assert ( ! CSC(SA[l]+o,q) );
				}
				
				return l;
			}

			// search for smallest suffix in SA that equals q or is larger than q
			template<typename saidx_t, typename text_iterator>
			static uint64_t suffixSearchInternal(
				saidx_t const * SA, 
				text_iterator texta, text_iterator texte, 
				uint64_t const n, uint64_t const q, std::string const & filename, uint64_t const fs)
			{
				uint64_t l = 0, r = n;
				this_type CSC(filename,fs);
				
				// binary search
				while ( r-l > 2 )
				{
					uint64_t const m = (l+r)>>1;		

					// is m too small? i.e. SA[m] < q
					if ( CSC(texta + SA[m],texte,q) )
					{
						l = m+1;
					}
					// m is large enough
					else
					{
						r = m+1;
					}
				}
				
				// ! (SA[l] >= q) <=> q < SA[l]
				while ( l < r && CSC(texta + SA[l],texte,q) )
					++l;
				
				if ( l < n )
				{
					// SA[l] >= q
					assert ( ! CSC(texta + SA[l],texte,q) );
				}
				
				return l;
			}

			template<typename saidx_t, typename text_iterator>
			static uint64_t suffixSearchTryInternal(
				saidx_t const * SA, 
				text_iterator texta, text_iterator texte, 
				uint64_t const n, uint64_t const o, uint64_t const q, 
				std::string const & filename,
				uint64_t const fs)
			{
				try
				{
					return suffixSearchInternal(SA, texta, texte, n, q, filename, fs);
				}
				catch(std::exception const & ex)
				{
					// std::cerr << ex.what() << std::endl;
					return suffixSearch(SA,n,o,q,filename,fs);
				}
			}
		};

		typedef CircularSuffixComparatorTemplate<CircularWrapperFactory> CircularSuffixComparator;
		typedef CircularSuffixComparatorTemplate<CompactDecoderWrapperFactory> CompactCircularSuffixComparator;
		typedef CircularSuffixComparatorTemplate<PacDecoderWrapperFactory> PacCircularSuffixComparator;
		typedef CircularSuffixComparatorTemplate<PacTermDecoderWrapperFactory> PacTermCircularSuffixComparator;
		typedef CircularSuffixComparatorTemplate<Utf8DecoderWrapperFactory> Utf8CircularSuffixComparator;
	}
}
#endif
