/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_READENDSCONTAINER_HPP)
#define LIBMAUS2_BAMBAM_READENDSCONTAINER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/CompactReadEndsBase.hpp>
#include <libmaus2/bambam/CompactReadEndsComparator.hpp>
#include <libmaus2/bambam/ReadEnds.hpp>
#include <libmaus2/bambam/ReadEndsContainerBase.hpp>
#include <libmaus2/bambam/SortedFragDecoder.hpp>
#include <libmaus2/util/DigitTable.hpp>
#include <libmaus2/util/CountGetObject.hpp>
#include <libmaus2/sorting/SerialRadixSort64.hpp>
#include <libmaus2/lz/SnappyOutputStream.hpp>

#include <libmaus2/bambam/ReadEndsBlockDecoderBase.hpp>
#include <libmaus2/bambam/ReadEndsBlockDecoderBaseCollection.hpp>
#include <libmaus2/bambam/ReadEndsStreamDecoderBase.hpp>
#include <libmaus2/bambam/ReadEndsStreamDecoderFileBase.hpp>
#include <libmaus2/bambam/ReadEndsStreamDecoder.hpp>		

#include <libmaus2/index/ExternalMemoryIndexGenerator.hpp>

#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

namespace libmaus2
{
	namespace bambam
	{		
		/**
		 * container class for ReadEnds objects
		 **/
		struct ReadEndsContainer : public ::libmaus2::bambam::CompactReadEndsBase, public ::libmaus2::bambam::ReadEndsContainerBase
		{
			//! this type
			typedef ReadEndsContainer this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
					
			private:
			//! digit table
			static ::libmaus2::util::DigitTable const D;

			//! in table index type
			typedef uint32_t index_type;
			//! index size
			enum { index_type_size = sizeof(index_type) };
			//! buffer array
			::libmaus2::autoarray::AutoArray<index_type> A;
			
			//! index insertion pointer (from back of A)
			index_type * iptr;
			//! data insert pointer (from front of A)
			uint8_t * dptr;
			
			//! temporary file name
			std::string const tempfilename;
			//! temporary file name of index
			std::string const tempfilenameindex;
			
			//! temporary output file stream
			::libmaus2::aio::OutputStream::unique_ptr_type tempfileout;
			//! compressed output stream
			::libmaus2::lz::SnappyOutputStream< ::libmaus2::aio::OutputStream >::unique_ptr_type pSOS;
			
			//! index generator type
			typedef libmaus2::index::ExternalMemoryIndexGenerator<
				libmaus2::bambam::ReadEndsBase,
				ReadEndsContainerBase::baseIndexShift,
				ReadEndsContainerBase::innerIndexShift
				> index_generator_type;
			//! index generator pointer type
			typedef index_generator_type::unique_ptr_type index_generator_pointer_type;
			//! index generator
			index_generator_pointer_type Pindexer;
			uint64_t indexerpos;
			
			std::vector<uint64_t> indexblockstart;
			
			//! uint64_t pair
			typedef std::pair<uint64_t,uint64_t> upair;
			//! index for blocks in temporary file
			std::vector < upair > tmpoffsetintervals;
			//! number of objects stored in each block of temporary file
			std::vector < uint64_t > tmpoutcnts;
			
			//! if true then copy alignments
			bool const copyAlignments;
			
			//! minimum length of stored objcts in bytes (for determining how many runs of radix sort are possible)
			uint64_t minlen;
			
			bool decodingPrepared;

			/**
			 * @return temporary file output stream
			 **/
			::libmaus2::aio::OutputStream & getTempFile()
			{
				if ( ! tempfileout.get() )
				{
					::libmaus2::aio::OutputStream::unique_ptr_type rtmpfile(libmaus2::aio::OutputStreamFactoryContainer::constructUnique(tempfilename));
					tempfileout = UNIQUE_PTR_MOVE(rtmpfile);
				}
				return *tempfileout;
			}

			/**
			 * @return temporary file output stream for index
			 **/
			index_generator_type * getIndexer()
			{
				if ( tempfilenameindex.size() )
				{
					if ( ! Pindexer )
					{
						index_generator_pointer_type Tindexer(new index_generator_type(tempfilenameindex));
						Pindexer = UNIQUE_PTR_MOVE(Tindexer);
					}
					return Pindexer.get();
				}
				else
				{
					return 0;
				}
			}
			
			/**
			 * @return compressed temporary file output stream
			 **/
			::libmaus2::lz::SnappyOutputStream< ::libmaus2::aio::OutputStream > & getCompressedTempFile()
			{
				if ( ! pSOS )
				{
					::libmaus2::lz::SnappyOutputStream< ::libmaus2::aio::OutputStream >::unique_ptr_type tSOS(
						new ::libmaus2::lz::SnappyOutputStream< ::libmaus2::aio::OutputStream >(getTempFile())
					);
					pSOS = UNIQUE_PTR_MOVE(tSOS);
				}
				return *pSOS;
			}

			#define READENDSRADIXSORT
			
			/**
			 * @return free space in buffer in bytes
			 **/
			uint64_t freeSpace() const
			{
				uint64_t const textdata = dptr-reinterpret_cast<uint8_t const *>(A.begin());
				uint64_t const ptrdata = (A.end() - iptr)*sizeof(index_type);
				
				#if defined(READENDSRADIXSORT) && defined(LIBMAUS2_HAVE_x86_64)
				return A.size() * sizeof(index_type) - (2*ptrdata + textdata);
				#else
				uint64_t const exfreespace = A.size() * sizeof(index_type) - (ptrdata + textdata);
				assert ( static_cast<ptrdiff_t>(exfreespace) == (reinterpret_cast<uint8_t const *>(iptr) - dptr) );
				return reinterpret_cast<uint8_t const *>(iptr) - dptr;
				#endif
			}
			
			/**
			 * decode entry at offset ioff
			 *
			 * @param ioff offset in buffer
			 * @param RE reference to object to be filled
			 **/
			void decodeEntry(uint64_t const ioff, ::libmaus2::bambam::ReadEnds & RE) const
			{
				uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
				/* uint32_t const len = */ decodeLength(eptr);
						
				::libmaus2::util::CountGetObject<uint8_t const *> G(eptr);
				RE.reset();
				RE.get(G);	
			}

			/**
			 * store compact read ends object at offset ioff in array and return it
			 *
			 * @param ioff offset in buffer
			 * @param compact read ends object at offset ioff in buffer
			 **/
			::libmaus2::autoarray::AutoArray<uint8_t> decodeEntryArray(uint64_t const ioff) const
			{
				uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
				uint32_t const len = decodeLength(eptr);
				
				::libmaus2::autoarray::AutoArray<uint8_t> A(len,false);
				std::copy(eptr,eptr+len,A.begin());
						
				return A;
			}
			
			/**
			 * decode and return ReadEnds object at offset ioff
			 *
			 * @param ioff offset in buffer
			 * @return decoded ReadEnds object
			 **/
			::libmaus2::bambam::ReadEnds decodeEntry(uint64_t const ioff) const
			{
				::libmaus2::bambam::ReadEnds RE;
				decodeEntry(ioff,RE);
				return RE;
			}
			
			/**
			 * projector type for radix sort
			 **/
			struct RadixProjectorType
			{
				private:
				//! buffer array
				uint8_t const * const A;
				
				public:
				/**
				 * constructor
				 *
				 * @param rA buffer
				 **/
				RadixProjectorType(uint8_t const * const rA) : A(rA) {}
				
				/**
				 * project object at offset i to its first 8 bytes
				 *
				 * @param i offset in buffer
				 * @return first eight bytes of compact object
				 **/
				uint64_t operator()(uint64_t const i) const
				{
					uint8_t const * p = A+i; 
					decodeLength(p);
					return *(reinterpret_cast<uint64_t const *>(p));
				}
			};

			/**
			 * projector type for radix sort including offset
			 **/
			struct RadixProjectorTypeOffset
			{
				private:
				//! buffer
				uint8_t const * const A;
				//! offset
				uint64_t const offset;
				
				public:
				/**
				 * construct
				 *
				 * @param rA buffer
				 * @param roffset offset
				 **/
				RadixProjectorTypeOffset(uint8_t const * const rA, uint64_t const roffset) : A(rA), offset(roffset) {}
				
				/**
				 * @param i offset in buffer
				 * @return eight bytes from compact read ends representation starting from offset of object
				 **/
				uint64_t operator()(uint64_t const i) const
				{
					uint8_t const * p = A+i; 
					decodeLength(p);
					return *(reinterpret_cast<uint64_t const *>(p+offset));
				}
			};

			public:
			/**
			 * constructor
			 *
			 * @param bytes size of buffer in bytes
			 * @param rtempfilename name of temporary file
			 * @param rcopyAlignments copy alignments along with read ends objects
			 **/ 
			ReadEndsContainer(uint64_t const bytes, std::string const & rtempfilename, bool const rcopyAlignments = false)
			: A( (bytes + index_type_size - 1) / index_type_size, false ), 
			  iptr(A.end()), dptr(reinterpret_cast<uint8_t *>(A.begin())),
			  tempfilename(rtempfilename), 
			  tempfilenameindex(),
			  indexerpos(0),
			  copyAlignments(rcopyAlignments),
			  minlen(std::numeric_limits<uint64_t>::max()),
			  decodingPrepared(false)
			{
			
			}

			/**
			 * constructor
			 *
			 * @param bytes size of buffer in bytes
			 * @param rtempfilename name of temporary file
			 * @param rtempfilenameindex name of temporary file for index
			 * @param rcopyAlignments copy alignments along with read ends objects
			 **/ 
			ReadEndsContainer(
				uint64_t const bytes, 
				std::string const & rtempfilename, 
				std::string const & rtempfilenameindex,
				bool const rcopyAlignments = false
			)
			: A( (bytes + index_type_size - 1) / index_type_size, false ), 
			  iptr(A.end()), dptr(reinterpret_cast<uint8_t *>(A.begin())),
			  tempfilename(rtempfilename), 
			  tempfilenameindex(rtempfilenameindex),
			  indexerpos(0),
			  copyAlignments(rcopyAlignments),
			  minlen(std::numeric_limits<uint64_t>::max()),
			  decodingPrepared(false)
			{
			
			}
			
			/**
			 * release buffer array
			 **/
			void releaseArray()
			{
				A.release();				
				iptr = A.end();
			}

			/**
			 * remove the temporary files
			 **/
			void removeTmpFiles()
			{
				if ( tempfilename.size() )
					libmaus2::aio::FileRemoval::removeFile(tempfilename);
				if ( tempfilenameindex.size() )
					libmaus2::aio::FileRemoval::removeFile(tempfilenameindex);
			}
			
			/**
			 * flush and close output files, deallocate memory
			 **/
			void prepareDecoding()
			{
				if ( ! decodingPrepared )
				{
					flush();
					
					if ( pSOS )
					{
						assert ( pSOS->getOffset().second == 0 );
						pSOS.reset();
					}

					getTempFile().flush();
					tempfileout.reset();
					
					if ( getIndexer() )
						Pindexer.reset();

					releaseArray();
					
					decodingPrepared = true;
				}
			}

			/**
			 * get decoder for reading back sorted stored objects
			 *
			 * @return decoder for reading back sorted stored objects
			 **/
			::libmaus2::bambam::SortedFragDecoder::unique_ptr_type getDecoder()
			{
				prepareDecoding();
				
				::libmaus2::bambam::SortedFragDecoder::unique_ptr_type ptr(
					::libmaus2::bambam::SortedFragDecoder::construct(tempfilename,tmpoffsetintervals,tmpoutcnts)
				);
				return UNIQUE_PTR_MOVE(ptr);
			}

			/**
			 * get decoder for unmerged stream
			 *
			 * @return decoder for unmerged stream
			 **/
			::libmaus2::bambam::ReadEndsStreamDecoder::unique_ptr_type getUnmergedDecoder()
			{
				prepareDecoding();
				
				::libmaus2::bambam::ReadEndsStreamDecoder::unique_ptr_type tptr(
					new ::libmaus2::bambam::ReadEndsStreamDecoder(tempfilename)
				);
				
				return UNIQUE_PTR_MOVE(tptr);
			}

			/**
			 * flush buffer
			 **/
			void flush()
			{
				if ( iptr != A.end() )
				{
					// std::cerr << "[V] minlen=" << minlen << std::endl;
				
					// sort entries in buffer
					::libmaus2::bambam::CompactReadEndsComparator const comp(reinterpret_cast<uint8_t const *>(A.begin()));
					::libmaus2::bambam::CompactReadEndsComparator::prepare(reinterpret_cast<uint8_t *>(A.begin()),A.end()-iptr);
					
					#if defined(READENDSRADIXSORT) && defined(LIBMAUS2_HAVE_x86_64)
					unsigned int const maxradruns = 1;
					unsigned int const posradruns = minlen >> 3;
					unsigned int const radruns = std::min(maxradruns,posradruns);
					// std::cerr << "radruns: " << radruns << std::endl;
					
					if ( radruns )
					{
						uint64_t const radixn = A.end()-iptr;
						
						for ( unsigned int r = 1; r < radruns; ++r )
						{
							uint64_t const offset = (radruns-r)<<3;
							RadixProjectorTypeOffset RP(reinterpret_cast<uint8_t const *>(A.begin()),offset);
							std::cerr << "offset " << offset << std::endl;
							libmaus2::sorting::SerialRadixSort64<index_type,RadixProjectorTypeOffset>::radixSort(iptr,iptr-radixn,radixn,RP);
						}
					
						RadixProjectorType RP(reinterpret_cast<uint8_t const *>(A.begin()));
						libmaus2::sorting::SerialRadixSort64<index_type,RadixProjectorType>::radixSort(
							iptr,iptr-radixn,radixn,RP
						);
						
						#if 1
						uint64_t low = 0;
						
						while ( low != radixn )
						{
							uint64_t high = std::min(low+16*1024,radixn);
							while ( high < radixn &&  RP(iptr[high]) == RP(iptr[high-1]) )
								++high;

							std::sort(iptr+low,iptr+high,comp);
						
							low = high;
						}
						#else
						std::sort(iptr,A.end(),comp);						
						#endif
					}
					else
					#endif
					{
						std::sort(iptr,A.end(),comp);
					}
					::libmaus2::bambam::CompactReadEndsComparator::prepare(reinterpret_cast<uint8_t *>(A.begin()),A.end()-iptr);
					
					#if 0
					// std::cerr << "Checking sorting...";
					for ( index_type * xptr = iptr; xptr+1 < A.end(); ++xptr )
					{
						::libmaus2::bambam::ReadEnds const REa = decodeEntry(xptr[0]);
						::libmaus2::bambam::ReadEnds const REb = decodeEntry(xptr[1]);
						bool const ok = REa < REb;

						if ( ! ok )
						{
							std::cerr << "failed:\n" <<
								REa << "\n" <<
								REb << std::endl;

							::libmaus2::autoarray::AutoArray<uint8_t> A = decodeEntryArray(xptr[0]);
							::libmaus2::autoarray::AutoArray<uint8_t> B = decodeEntryArray(xptr[1]);
							
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
					
					index_generator_type * indexer = getIndexer();
					if ( indexer )
						indexblockstart.push_back(indexer->setup());
						
					::libmaus2::lz::SnappyOutputStream< ::libmaus2::aio::OutputStream > & SOS = getCompressedTempFile();
					uint64_t const prepos = SOS.getOffset().first;
					assert ( SOS.getOffset().second == 0 );
							
					// write entries
					for ( index_type * xptr = iptr; xptr != A.end(); ++xptr )
					{						
						if ( indexer && ((xptr - iptr) & index_generator_type::base_index_mask) == 0 )
						{
							index_type const ioff = *xptr;
							uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
							
							// decode
							/* uint32_t const len = */ decodeLength(eptr);
							::libmaus2::util::CountGetObject<uint8_t const *> G(eptr);
							
							::libmaus2::bambam::ReadEnds RE;
							RE.get(G);
							// put object
							indexer->put(RE,SOS.getOffset());							
						}

						index_type const ioff = *xptr;
						uint8_t const * eptr = reinterpret_cast<uint8_t const *>(A.begin()) + ioff;
					
						#if 1
						uint32_t const len = decodeLength(eptr);
						SOS.write(reinterpret_cast<char const *>(eptr),len);
						#else
						/* uint32_t const len = */ decodeLength(eptr);
						
						::libmaus2::util::CountGetObject<uint8_t const *> G(eptr);
						::libmaus2::bambam::ReadEnds RE;
						RE.get(G);
						RE.put(SOS);
						
						// std::cerr << RE << std::endl;
						#endif
					}

					SOS.flush();
					uint64_t const postpos = SOS.getOffset().first;
					assert ( SOS.getOffset().second == 0 );

					if ( indexer )
						indexer->flush();
					
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
				
				minlen = std::numeric_limits<uint64_t>::max();
			}
			
			ReadEndsBlockDecoderBaseCollectionInfoBase getMergeInfo()
			{
				prepareDecoding();
				return ReadEndsBlockDecoderBaseCollectionInfoBase(tempfilename,tempfilenameindex,tmpoutcnts,indexblockstart);
			}
			
			ReadEndsBlockDecoderBaseCollection<true>::unique_ptr_type getBaseDecoderCollectionWithProxy()
			{
				prepareDecoding();
				
				ReadEndsBlockDecoderBaseCollection<true>::unique_ptr_type tptr(
					new ReadEndsBlockDecoderBaseCollection<true>(
						std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase>(1,getMergeInfo())
					)
				);
				
				return UNIQUE_PTR_MOVE(tptr);
			}

			ReadEndsBlockDecoderBaseCollection<false>::unique_ptr_type getBaseDecoderCollectionWithoutProxy()
			{
				prepareDecoding();
				
				ReadEndsBlockDecoderBaseCollection<false>::unique_ptr_type tptr(
					new ReadEndsBlockDecoderBaseCollection<false>(
						std::vector<ReadEndsBlockDecoderBaseCollectionInfoBase>(1,getMergeInfo())					
					)
				);
				
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			/**
			 * construct fragment ReadEnds object from p and put it in the buffer
			 *
			 * @param p alignment
			 * @param header BAM header
			 **/
			template<typename header_type>
			void putFrag(::libmaus2::bambam::BamAlignment const & p, header_type const & header, uint64_t const tagid = 0)
			{
				::libmaus2::bambam::ReadEnds RE(p,header, /* RE, */ copyAlignments,tagid);
				// fillFrag(p,header,RE);
				put(RE);
			}

			/**
			 * construct fragment ReadEnds object from p and put it in the buffer
			 *
			 * @param p alignment
			 * @param header BAM header
			 **/
			template<typename header_type>
			void putFrag(
				uint8_t const * p, 
				uint64_t const pblocksize,
				header_type const & header, 
				uint64_t const tagid = 0
			)
			{
				::libmaus2::bambam::ReadEnds RE(p,pblocksize,header, /* RE, */ copyAlignments,tagid);
				// fillFrag(p,header,RE);
				put(RE);
			}

			/**
			 * construct pair ReadEnds object from p and q and put it in the buffer
			 *
			 * @param p alignment
			 * @param q alignment
			 * @param header BAM header
			 **/
			template<typename header_type>
			void putPair(
				::libmaus2::bambam::BamAlignment const & p, 
				::libmaus2::bambam::BamAlignment const & q, 
				header_type const & header,
				uint64_t tagid = 0
			)
			{
				::libmaus2::bambam::ReadEnds RE(p,q,header, /* RE, */ copyAlignments,tagid);
				// fillFragPair(p,q,header,RE);
				put(RE);				
			}

			/**
			 * construct pair ReadEnds object from p and q and put it in the buffer
			 *
			 * @param p alignment
			 * @param q alignment
			 * @param header BAM header
			 **/
			template<typename header_type>
			void putPair(
				uint8_t const * p,
				uint64_t const pblocksize,
				uint8_t const * q,
				uint64_t const qblocksize,
				header_type const & header,
				uint64_t tagid = 0
			)
			{
				::libmaus2::bambam::ReadEnds RE(p,pblocksize,q,qblocksize,header, /* RE, */ copyAlignments,tagid);
				// fillFragPair(p,q,header,RE);
				put(RE);				
			}

			/**
			 * put R in buffer
			 *
			 * @param R object to be put in buffer
			 **/
			void put(::libmaus2::bambam::ReadEnds const & R)
			{
				// assert ( R.recode() == R );
			
				// compute space required for adding R
				uint64_t const entryspace = getEntryLength(R);
				uint64_t const numlen = getNumberLength(entryspace);
				uint64_t const idexlen = sizeof(index_type);
				#if defined(READENDSRADIXSORT) && defined(LIBMAUS2_HAVE_x86_64)
				uint64_t const reqspace = entryspace+numlen+2*idexlen;
				#else
				uint64_t const reqspace = entryspace+numlen+idexlen;
				#endif
				
				assert ( reqspace <= A.size() * sizeof(index_type) );
				
				// flush buffer if needed
				if ( reqspace > freeSpace() )
					flush();
					
				minlen = std::min(entryspace,minlen);
				
				// store current offset
				*(--iptr) = dptr - reinterpret_cast<uint8_t *>(A.begin());
				
				// put entry
				::libmaus2::util::PutObject<uint8_t *> P(dptr);
				// put length
				::libmaus2::util::UTF8::encodeUTF8(entryspace,P);
				assert ( (P.p - dptr) == static_cast<ptrdiff_t>(numlen) );
				// put entry data
				R.put(P);
				assert ( (P.p - dptr) == static_cast<ptrdiff_t>(numlen+entryspace) );
				
				dptr = P.p;		
			}

			size_t byteSize() const
			{
				return
					A.byteSize() +
					sizeof(iptr) +
					sizeof(dptr) +
					tempfilename.size() +
					tempfilenameindex.size() + 
					sizeof(pSOS) +
					(pSOS ? pSOS->byteSize() : 0) +
					sizeof(Pindexer) +
					(Pindexer ? Pindexer->byteSize() : 0) +
					sizeof(indexerpos) +
					(indexblockstart.size() * sizeof(uint64_t)) +
					tmpoffsetintervals.size() * sizeof(upair) +
					tmpoutcnts.size() * sizeof(uint64_t) +
					sizeof(copyAlignments) +
					sizeof(minlen) +
					sizeof(decodingPrepared);
			}
		};
	}
}
#endif
