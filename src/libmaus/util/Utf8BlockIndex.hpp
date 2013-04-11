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
#if ! defined(LIBMAUS_UTIL_UTF8BLOCKINDEX_HPP)
#define LIBMAUS_UTIL_UTF8BLOCKINDEX_HPP

#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/math/numbits.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace util
	{
		struct Utf8BlockIndexDecoder
		{
			uint64_t blocksize;
			uint64_t lastblocksize;
			uint64_t maxblockbytes;
			uint64_t numblocks;
			
			::libmaus::aio::CheckedInputStream CIS;
			
			Utf8BlockIndexDecoder(std::string const & filename)
			: CIS(filename)
			{
				blocksize = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				lastblocksize = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				maxblockbytes = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				numblocks = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
			}
			
			uint64_t operator[](uint64_t const i)
			{
				CIS.clear();
				CIS.seekg((4+i)*sizeof(uint64_t));
				
				return ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
			}
		};

		struct Utf8BlockIndex
		{
			typedef Utf8BlockIndex this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t blocksize;
			uint64_t lastblocksize;
			uint64_t maxblockbytes;
			::libmaus::autoarray::AutoArray<uint64_t> blockstarts;
			
			private:
			Utf8BlockIndex()
			{
			
			}
			
			public:
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,blocksize);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,lastblocksize);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,maxblockbytes);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,blockstarts.size());
				for ( uint64_t i = 0; i < blockstarts.size(); ++i )
					::libmaus::util::NumberSerialisation::serialiseNumber(stream,blockstarts[i]);	
			}
			std::string serialise() const
			{
				::std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
			
			static unique_ptr_type constructFromSerialised(std::string const & fn)
			{
				::libmaus::aio::CheckedInputStream CIS(fn);

				unique_ptr_type UP(new Utf8BlockIndex);

				UP->blocksize = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				UP->lastblocksize = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				UP->maxblockbytes = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				uint64_t const numblocks = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				
				UP->blockstarts = ::libmaus::autoarray::AutoArray<uint64_t>(numblocks,false);
				
				for ( uint64_t i = 0; i < numblocks; ++i )
					UP->blockstarts[i] = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				
				
				return UNIQUE_PTR_MOVE(UP);
			}

			static unique_ptr_type constructFromUtf8File(std::string const & fn, uint64_t const rblocksize = 16ull*1024ull)
			{
				uint64_t const fs = ::libmaus::util::GetFileSize::getFileSize(fn);
				

				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif
				uint64_t const numparts = 1024*numthreads;
				uint64_t const bblocksize = (fs + numparts-1)/numparts;
				uint64_t const numbblocks = (fs + bblocksize-1)/bblocksize;

				uint64_t const blocksize = ::libmaus::math::nextTwoPow(rblocksize);
				unsigned int const blockshift = ::libmaus::math::numbits(blocksize)-1;
				assert ( (1ull << blockshift) == blocksize );
				uint64_t const blockmask = blocksize-1;

				if ( ! fs )
				{
					unique_ptr_type UP(new Utf8BlockIndex);
					UP->blocksize = blocksize;
					UP->lastblocksize = 0;
					return UNIQUE_PTR_MOVE(UP);
				}

				
				::libmaus::aio::CheckedInputStream::unique_ptr_type CIS(new ::libmaus::aio::CheckedInputStream(fn));
				std::vector<uint64_t> preblockstarts;
				
				for ( uint64_t b = 0; b < numbblocks; ++b )
				{
					uint64_t low = b*bblocksize;
					
					CIS->clear();
					CIS->seekg(low);

					int c = -1;

					uint64_t p = low;
					while ( ((c=CIS->get()) >= 0) && ((c & 0xc0) == (0x80)) )
						++p;
						
					preblockstarts.push_back(p);
					/*
					template<typename in_type>
					static uint32_t decodeUTF8(in_type & istr, uint64_t & codelen)
					*/	                        			
				}
				
				// CIS.reset();
				
				preblockstarts.push_back(fs);
				::libmaus::autoarray::AutoArray<uint64_t> bblocksyms(numbblocks+1,false);
				::libmaus::autoarray::AutoArray< ::libmaus::aio::CheckedInputStream::unique_ptr_type > thrstreams(numthreads,false);
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					thrstreams[t] = UNIQUE_PTR_MOVE(
						::libmaus::aio::CheckedInputStream::unique_ptr_type
						(new ::libmaus::aio::CheckedInputStream(fn))
					);
				}
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t b = 0; b < static_cast<int64_t>(numbblocks); ++b )
				{
					#if defined(_OPENMP)
					uint64_t const threadid = omp_get_thread_num();
					#else
					uint64_t const threadid = 0;
					#endif
					
					::libmaus::aio::CheckedInputStream * CIS = thrstreams[threadid].get();
					CIS->clear();
					CIS->seekg(preblockstarts[b]);
					
					uint64_t codelen = 0;
					uint64_t syms = 0;
					uint64_t const bblocksize = preblockstarts[b+1]-preblockstarts[b];
					
					while ( codelen != bblocksize )
					{
						::libmaus::util::UTF8::decodeUTF8(*CIS, codelen);
						syms++;
					}
					
					assert ( syms <= bblocksize );
					
					bblocksyms[b] = syms;			
				}
				
				bblocksyms.prefixSums();
				
				uint64_t const numsyms = bblocksyms[bblocksyms.size()-1];
				uint64_t const numblocks = (numsyms + blocksize-1)/blocksize;
				
				::libmaus::autoarray::AutoArray<uint64_t> blockstarts(numblocks,false);
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t b = 0; b < static_cast<int64_t>(numbblocks); ++b )
				{
					#if defined(_OPENMP)
					uint64_t const threadid = omp_get_thread_num();
					#else
					uint64_t const threadid = 0;
					#endif
					
					::libmaus::aio::CheckedInputStream * CIS = thrstreams[threadid].get();
					CIS->clear();
					CIS->seekg(preblockstarts[b]);
					
					uint64_t codelen = 0;
					uint64_t syms = bblocksyms[b];
					uint64_t const bblocksize = preblockstarts[b+1]-preblockstarts[b];
					
					while ( codelen != bblocksize )
					{
						if ( (syms & blockmask) == 0 )
							blockstarts[syms>>blockshift] = preblockstarts[b]+codelen;
					
						::libmaus::util::UTF8::decodeUTF8(*CIS, codelen);
						syms++;
					}			
				}
				
				uint64_t maxblockbytes = 0;
				for ( uint64_t i = 0; i+1 < numblocks; ++i )
					maxblockbytes = std::max(maxblockbytes,blockstarts[i+1]-blockstarts[i]);
				maxblockbytes = std::max(maxblockbytes,fs-blockstarts[blockstarts.size()-1]);

				#if defined(UTF8SPLITDEBUG)
				CIS->clear();
				CIS->seekg(0);
				
				int cc = -1;
				uint64_t symcnt = 0;
				uint64_t codelen = 0;
				while ( (cc=CIS->peek()) >= 0 )
				{
					if ( symcnt % blocksize == 0 )
					{
						assert ( blockstarts[symcnt/blocksize] == codelen );
					}
				
					::libmaus::util::UTF8::decodeUTF8(*CIS, codelen);		
					symcnt += 1;
				}
				#endif
			
				unique_ptr_type UP(new this_type);
				UP->blockstarts = blockstarts;
				UP->blocksize = blocksize;
				UP->maxblockbytes = maxblockbytes;

				CIS->clear();
				uint64_t codelen = UP->blockstarts[UP->blockstarts.size()-1];
				UP->lastblocksize = 0;
				CIS->seekg(codelen);
				while ( codelen != fs )
				{
					::libmaus::util::UTF8::decodeUTF8(*CIS, codelen);
					++(UP->lastblocksize);	
				}
				
				return UNIQUE_PTR_MOVE(UP);
			}
		};
	}
}
#endif
