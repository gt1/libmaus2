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
#if ! defined(LIBMAUS_BAMBAM_READENDS_COMPARATOR_HPP)
#define LIBMAUS_BAMBAM_READENDS_COMPARATOR_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/CompactReadEndsBase.hpp>
#include <libmaus/bambam/CompactReadEndsComparator.hpp>
#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/SortedFragDecoder.hpp>
#include <libmaus/util/DigitTable.hpp>
#include <libmaus/util/CountGetObject.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ReadEndsContainer : public ::libmaus::bambam::CompactReadEndsBase 
		{
			typedef ReadEndsContainer this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			static ::libmaus::util::DigitTable const D;

			typedef uint32_t index_type;
			enum { index_type_size = sizeof(index_type) };
			::libmaus::autoarray::AutoArray<index_type> A;
			
			index_type * iptr;
			uint8_t * dptr;
			
			std::string const tempfilename;
			
			::libmaus::aio::CheckedOutputStream::unique_ptr_type tempfileout;
			
			typedef std::pair<uint64_t,uint64_t> upair;
			std::vector < upair > tmpoffsetintervals;
			std::vector < uint64_t > tmpoutcnts;
			
			bool const copyAlignments;

			::libmaus::aio::CheckedOutputStream & getTempFile()
			{
				if ( ! tempfileout.get() )
				{
					::libmaus::aio::CheckedOutputStream::unique_ptr_type rtmpfile(new ::libmaus::aio::CheckedOutputStream(tempfilename));
					tempfileout = UNIQUE_PTR_MOVE(rtmpfile);
				}
				return *tempfileout;
			}

			ReadEndsContainer(
				uint64_t const bytes, std::string const & rtempfilename,
				bool const rcopyAlignments = false
			)
			: A( (bytes + index_type_size - 1) / index_type_size, false ), 
			  iptr(A.end()), dptr(reinterpret_cast<uint8_t *>(A.begin())),
			  tempfilename(rtempfilename), copyAlignments(rcopyAlignments)
			{
			
			}

			::libmaus::bambam::SortedFragDecoder::unique_ptr_type getDecoder()
			{
				flush();
				
				return UNIQUE_PTR_MOVE(
					::libmaus::bambam::SortedFragDecoder::construct(tempfilename,tmpoffsetintervals,tmpoutcnts)
				);
			}
			
			uint64_t freeSpace() const
			{
				return reinterpret_cast<uint8_t const *>(iptr) - dptr;
			}
			
			void decodeEntry(uint64_t const ioff, ::libmaus::bambam::ReadEnds & RE) const
			{
				uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
				/* uint32_t const len = */ decodeLength(eptr);
						
				::libmaus::util::CountGetObject<uint8_t const *> G(eptr);
				RE.reset();
				RE.get(G);	
			}

			::libmaus::autoarray::AutoArray<uint8_t> decodeEntryArray(uint64_t const ioff) const
			{
				uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
				uint32_t const len = decodeLength(eptr);
				
				::libmaus::autoarray::AutoArray<uint8_t> A(len,false);
				std::copy(eptr,eptr+len,A.begin());
						
				return A;
			}
			
			::libmaus::bambam::ReadEnds decodeEntry(uint64_t const ioff) const
			{
				::libmaus::bambam::ReadEnds RE;
				decodeEntry(ioff,RE);
				return RE;
			}
			
			void flush()
			{
				if ( iptr != A.end() )
				{
					// sort entries in buffer
					::libmaus::bambam::CompactReadEndsComparator const comp(reinterpret_cast<uint8_t const *>(A.begin()));
					std::sort(iptr,A.end(),comp);
					
					#if 0
					// std::cerr << "Checking sorting...";
					for ( index_type * xptr = iptr; xptr+1 < A.end(); ++xptr )
					{
						::libmaus::bambam::ReadEnds const REa = decodeEntry(xptr[0]);
						::libmaus::bambam::ReadEnds const REb = decodeEntry(xptr[1]);
						bool const ok = REa < REb;

						if ( ! ok )
						{
							std::cerr << "failed:\n" <<
								REa << "\n" <<
								REb << std::endl;

							::libmaus::autoarray::AutoArray<uint8_t> A = decodeEntryArray(xptr[0]);
							::libmaus::autoarray::AutoArray<uint8_t> B = decodeEntryArray(xptr[1]);
							
							for ( uint64_t i = 0; i < std::min(A.size(),B.size()); ++i )
								std::cerr << std::hex << static_cast<int>(A[i]) << std::dec << ";";
							std::cerr << std::endl;
							for ( uint64_t i = 0; i < std::min(A.size(),B.size()); ++i )
								std::cerr << std::hex << static_cast<int>(B[i]) << std::dec << ";";						
							std::cerr << std::endl;
						}
						assert ( ok );
					}
					// std::cerr << "done." << std::endl;
					#endif
					
					::libmaus::aio::CheckedOutputStream & tempfile = getTempFile();
					uint64_t const prepos = tempfile.tellp();
					::libmaus::lz::SnappyOutputStream< ::libmaus::aio::CheckedOutputStream > SOS(tempfile);
							
					// write entries
					for ( index_type * xptr = iptr; xptr != A.end(); ++xptr )
					{
						index_type const ioff = *xptr;
						uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
						/* uint32_t const len = */ decodeLength(eptr);
						
						::libmaus::util::CountGetObject<uint8_t const *> G(eptr);
						::libmaus::bambam::ReadEnds RE;
						RE.get(G);
						RE.put(SOS);
						
						// std::cerr << RE << std::endl;
					}

					SOS.flush();
					uint64_t const postpos = tempfile.tellp();
					
					// file positions
					tmpoffsetintervals.push_back(upair(prepos,postpos));
					// number of elements
					tmpoutcnts.push_back(A.end()-iptr);

					// std::cerr << "Wrote snappy block [" << prepos << "," << postpos << ")" << ", " << (A.end()-iptr) << " entries." << std::endl;

					// reset buffer
					iptr = A.end();
					dptr = reinterpret_cast<uint8_t *>(A.begin());
					
					// std::cerr << "block." << std::endl;
				}
			}
			
			void put(::libmaus::bambam::ReadEnds const & R)
			{
				// assert ( R.recode() == R );
			
				// compute space required for adding R
				uint64_t const entryspace = getEntryLength(R);
				uint64_t const numlen = getNumberLength(entryspace);
				uint64_t const idexlen = sizeof(index_type);
				uint64_t const reqspace = entryspace+numlen+idexlen;
				
				assert ( reqspace <= A.size() * sizeof(index_type) );
				
				// flush buffer if needed
				if ( reqspace > freeSpace() )
					flush();
				
				// store current offset
				*(--iptr) = dptr - reinterpret_cast<uint8_t *>(A.begin());
				::libmaus::util::PutObject<uint8_t *> P(dptr);
				// put length
				::libmaus::util::UTF8::encodeUTF8(entryspace,P);
				assert ( (P.p - dptr) == static_cast<ptrdiff_t>(numlen) );
				// put entry data
				R.put(P);
				assert ( (P.p - dptr) == static_cast<ptrdiff_t>(numlen+entryspace) );
				
				dptr = P.p;		
			}
			
			void putFrag(::libmaus::bambam::BamAlignment const & p, ::libmaus::bambam::BamHeader const & header)
			{
				::libmaus::bambam::ReadEnds RE(p,header,RE, copyAlignments);
				// fillFrag(p,header,RE);
				put(RE);
			}

			void putPair(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamAlignment const & q, 
				::libmaus::bambam::BamHeader const & header
			)
			{
				::libmaus::bambam::ReadEnds RE(p,q,header,RE, copyAlignments);
				// fillFragPair(p,q,header,RE);
				put(RE);
			}
		};
	}
}
#endif
