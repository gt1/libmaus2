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
#if ! defined(LIBMAUS_UTIL_UTF8STRING_HPP)
#define LIBMAUS_UTIL_UTF8STRING_HPP

#include <libmaus/rank/ImpCacheLineRank.hpp>
#include <libmaus/select/ImpCacheLineSelectSupport.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/util/Histogram.hpp>

namespace libmaus
{
	namespace util
	{
		struct Utf8String
		{
			typedef Utf8String this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus::autoarray::AutoArray<uint8_t> A;
			::libmaus::rank::ImpCacheLineRank::unique_ptr_type I;
			::libmaus::select::ImpCacheLineSelectSupport::unique_ptr_type S;
			
			static shared_ptr_type constructRaw(
				std::string const & filename, 
				uint64_t const offset = 0, 
				uint64_t const blength = std::numeric_limits<uint64_t>::max()
			)
			{
				return shared_ptr_type(new this_type(filename, offset, blength));		
			}
			
			Utf8String(
				std::string const & filename, 
				uint64_t offset = 0, 
				uint64_t blength = std::numeric_limits<uint64_t>::max())
			{	
				::libmaus::aio::CheckedInputStream CIS(filename);
				uint64_t const fs = ::libmaus::util::GetFileSize::getFileSize(CIS);
				offset = std::min(offset,fs);
				blength = std::min(blength,fs-offset);
			
				CIS.seekg(offset);
				A = ::libmaus::autoarray::AutoArray<uint8_t>(blength,false);
				CIS.read(reinterpret_cast<char *>(A.begin()),blength);
				
				uint64_t const bitalign = 6*64;
				
				I = UNIQUE_PTR_MOVE(::libmaus::rank::ImpCacheLineRank::unique_ptr_type(new ::libmaus::rank::ImpCacheLineRank(((blength+(bitalign-1))/bitalign)*bitalign)));

				::libmaus::rank::ImpCacheLineRank::WriteContext WC = I->getWriteContext();
				for ( uint64_t i = 0; i < blength; ++i )
					if ( (!(A[i] & 0x80)) || ((A[i] & 0xc0) != 0x80) )
						WC.writeBit(1);
					else
						WC.writeBit(0);
						
				for ( uint64_t i = blength; i % bitalign; ++i )
					WC.writeBit(0);
						
				WC.flush();
				
				S = ::libmaus::select::ImpCacheLineSelectSupport::unique_ptr_type(new ::libmaus::select::ImpCacheLineSelectSupport(*I,8));
				
				#if 0
				std::cerr << "A.size()=" << A.size() << std::endl;
				std::cerr << "I.byteSize()=" << I->byteSize() << std::endl;
				#endif
			}
			
			uint64_t size() const
			{
				if ( A.size() )
					return I->rank1(A.size()-1);
				else
					return 0;
			}
			
			wchar_t operator[](uint64_t const i) const
			{
				uint64_t const p = I->select1(i);
				::libmaus::util::GetObject<uint8_t const *> G(A.begin()+p);
				return ::libmaus::util::UTF8::decodeUTF8(G);
			}
			wchar_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
			
			::libmaus::util::Histogram::unique_ptr_type getHistogram() const
			{
				::libmaus::util::Histogram::unique_ptr_type hist(new ::libmaus::util::Histogram);
				
				for ( uint64_t i = 0; i < A.size(); ++i )
					if ( (A[i] & 0xc0) != 0x80 )
					{
						::libmaus::util::GetObject<uint8_t const *> G(A.begin()+i);
						wchar_t const v = ::libmaus::util::UTF8::decodeUTF8(G);
						(*hist)(v);
					}
					
				return UNIQUE_PTR_MOVE(hist);
			}
			
			std::map<int64_t,uint64_t> getHistogramAsMap() const
			{
				::libmaus::util::Histogram::unique_ptr_type hist = UNIQUE_PTR_MOVE(getHistogram());
				return hist->getByType<int64_t>();
			}
		};

		struct Utf8StringPairAdapter
		{
			Utf8String::shared_ptr_type U;
			
			Utf8StringPairAdapter(Utf8String::shared_ptr_type rU) : U(rU) {}
			
			uint64_t size() const
			{
				return 2*U->size();
			}
			
			wchar_t operator[](uint64_t const i) const
			{
				static unsigned int const shift = 12;
				static wchar_t const mask = (1u << shift)-1;
				
				wchar_t const full = U->get(i>>1);
				
				if ( i & 1 )
					return full & mask;
				else
					return full >> shift;
			}
			wchar_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
		};
	}
}
#endif
