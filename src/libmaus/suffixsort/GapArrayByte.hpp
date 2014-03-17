/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_SUFFIXSORT_GAPARRAYBYTE_HPP)
#define LIBMAUS_SUFFIXSORT_GAPARRAYBYTE_HPP

#include <libmaus/suffixsort/GapArrayByteDecoderBuffer.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/gamma/GammaGapEncoder.hpp>
#include <libmaus/huffman/GapEncoder.hpp>
#include <libmaus/sorting/MergingReadBack.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/util/Histogram.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct GapArrayByte
		{
			typedef GapArrayByte this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			libmaus::autoarray::AutoArray<uint8_t> G;
			libmaus::autoarray::AutoArray<uint64_t> O;
			std::vector<uint64_t *> Oa;
			std::vector<uint64_t *> Oc;
			std::vector<uint64_t *> Oe;
			
			std::string tmpfilename;
			libmaus::aio::CheckedOutputStream::unique_ptr_type tmpCOS;
			libmaus::parallel::OMPLock tmpfilelock;
			std::vector<uint64_t> blocksizes;

			void writeBlock(uint64_t * const pa, uint64_t * const pc)
			{
				std::sort(pa,pc);
				libmaus::parallel::ScopeLock llock(tmpfilelock);
				tmpCOS->write(reinterpret_cast<char const *>(pa),(pc-pa)*sizeof(uint64_t));
				blocksizes.push_back(pc-pa);
			}

			void flush(uint64_t const t)
			{
				if ( Oc[t] != Oa[t] )
				{
					writeBlock(Oa[t],Oc[t]);			
					Oc[t] = Oa[t];
				}
			}

			public:
			GapArrayByte(
				uint64_t const gsize, 
				uint64_t const osize, 
				uint64_t const threads, 
				std::string const rtmpfilename
			)
			: G(gsize), O(osize*threads,false), Oa(threads), Oc(threads), Oe(threads), tmpfilename(rtmpfilename)
			{
				uint64_t * p = O.begin();
				
				for ( uint64_t i = 0; i < threads; ++i )
				{
					Oa[i] = p;
					Oc[i] = p;
					p += osize;
					Oe[i] = p;
				}
				
				assert ( p == O.end() );

				libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilename);
				libmaus::aio::CheckedOutputStream::unique_ptr_type ttmpCOS(
					new libmaus::aio::CheckedOutputStream(tmpfilename)
				);
				
				tmpCOS = UNIQUE_PTR_MOVE(ttmpCOS);
			}
			~GapArrayByte()
			{
				remove(tmpfilename.c_str());
			}
				
			void flush()
			{
				// combine all blocks
				uint64_t * Op = O.begin();
			
				for ( uint64_t i = 0; i < Oa.size(); ++i )
				{
					for ( uint64_t const * Oi = Oa[i]; Oi != Oc[i]; ++Oi )
						*(Op++) = *Oi;
						
					Oc[i] = Oa[i];
				}
				
				// write to file if any data present
				if ( Op != O.begin() )
					writeBlock(O.begin(),Op);
					
				tmpCOS.reset();
				
				std::string const tmpfilesorted = tmpfilename + ".sorted";
				libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilesorted);
				libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type SGO(
					new libmaus::aio::SynchronousGenericOutput<uint64_t>(tmpfilesorted,1024));
				
				libmaus::sorting::MergingReadBack<uint64_t> MRB(tmpfilename,blocksizes);
				uint64_t prev;
				uint64_t prevcnt = 0;
				uint64_t overflowcnt = 0;
				
				if ( MRB.getNext(prev) )
				{
					prevcnt = 1;
					
					uint64_t next;
					
					while ( MRB.getNext(next) )
					{
						if ( next != prev )
						{
							assert ( prevcnt );
							
							SGO->put(prev);
							SGO->put(prevcnt);
							overflowcnt++;
						
							prev = next;				
							prevcnt = 1;
						}
						else
						{
							prevcnt += 1;
						}
					}
					
					assert ( prevcnt );

					SGO->put(prev);
					SGO->put(prevcnt);
					overflowcnt++;		
				}
				
				SGO->flush();
				SGO.reset();
				
				rename(tmpfilesorted.c_str(), tmpfilename.c_str());
			}

			/**
			 * increment value for rank r
			 *
			 * @param r rank
			 * @return true iff there is an overflow
			 **/
			bool operator()(uint64_t const r)
			{
				#if defined(_OPENMP) && defined(LIBMAUS_HAVE_SYNC_OPS)
				return __sync_add_and_fetch(G.begin()+r,1) == 0;
				#else
				return (++G[r]) == 0;
				#endif
			}
			
			/**
			 * push overflow for rank r on thread t
			 **/
			void operator()(uint64_t const r, uint64_t const t)
			{
				*(Oc[t]++) = r;
				if ( Oc[t] == Oe[t] )
					flush(t);
			}

			libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type getDecoder(uint64_t const offset = 0)
			{
				libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type tdec(
					new libmaus::suffixsort::GapArrayByteDecoder(
						G.begin(),G.size(),tmpfilename,offset
					)
				);
				
				return UNIQUE_PTR_MOVE(tdec);
			}
			
			void saveHufGapArray(std::string const & gapfile)
			{
				::libmaus::timing::RealTimeClock rtc;
			
				std::cerr << "[V] saving gap file...";
				rtc.start();

				libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type pdecoder;
				libmaus::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pdecbuf;
				
				libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type t0decoder = (getDecoder());
				pdecoder = UNIQUE_PTR_MOVE(t0decoder);
				libmaus::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type t0decbuf(
					new libmaus::suffixsort::GapArrayByteDecoderBuffer(*pdecoder,8192));
				pdecbuf = UNIQUE_PTR_MOVE(t0decbuf);
				libmaus::suffixsort::GapArrayByteDecoderBuffer::iterator it0 = t0decbuf->begin();
					
				::libmaus::util::Histogram gaphist;
				for ( uint64_t i = 0; i < G.size(); ++i )
					gaphist ( *(it0++) );

				libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type t1decoder = (getDecoder());
				pdecoder = UNIQUE_PTR_MOVE(t1decoder);
				libmaus::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type t1decbuf(
					new libmaus::suffixsort::GapArrayByteDecoderBuffer(*pdecoder,8192));
				pdecbuf = UNIQUE_PTR_MOVE(t1decbuf);
				libmaus::suffixsort::GapArrayByteDecoderBuffer::iterator it1 = t1decbuf->begin();

				::libmaus::huffman::GapEncoder GE(gapfile,gaphist,G.size());
				GE.encode(it1,G.size());
				GE.flush();
			}
			
			void saveGammaGapArray(std::string const & gapfile)
			{
				::libmaus::timing::RealTimeClock rtc;
			
				std::cerr << "[V] saving gap file...";
				rtc.start();

				libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type pdecoder;
				libmaus::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pdecbuf;
				
				libmaus::suffixsort::GapArrayByteDecoder::unique_ptr_type t0decoder = (getDecoder());
				pdecoder = UNIQUE_PTR_MOVE(t0decoder);
				libmaus::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type t0decbuf(
					new libmaus::suffixsort::GapArrayByteDecoderBuffer(*pdecoder,8192));
				pdecbuf = UNIQUE_PTR_MOVE(t0decbuf);
				libmaus::suffixsort::GapArrayByteDecoderBuffer::iterator it0 = t0decbuf->begin();

				::libmaus::gamma::GammaGapEncoder GE(gapfile);
				GE.encode(it0,G.size());
				std::cerr << "done in time " << rtc.getElapsedSeconds() << std::endl;
			}
		};
	}
}
#endif
