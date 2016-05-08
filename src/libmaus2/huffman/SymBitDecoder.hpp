/*
    libmaus2
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
#if ! defined(LIBMAUS2_HUFFMAN_SYMBITDECODER_HPP)
#define LIBMAUS2_HUFFMAN_SYMBITDECODER_HPP

#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/bitio/BitIOInput.hpp>
#include <libmaus2/bitio/readElias.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/IndexDecoderDataArray.hpp>
#include <libmaus2/util/HistogramSet.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/huffman/IndexLoader.hpp>
#include <libmaus2/gamma/GammaEncoderBase.hpp>

#include <libmaus2/huffman/SymBit.hpp>
#include <libmaus2/huffman/SymBitRun.hpp>
#include <libmaus2/huffman/RLInitType.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct SymBitDecoder : public RLInitType
		{
			typedef SymBitDecoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef SymBit value_type;
			typedef SymBitRun run_type;

			::libmaus2::huffman::IndexDecoderDataArray::unique_ptr_type Pidda;
			::libmaus2::huffman::IndexDecoderDataArray const & idda;

			::libmaus2::autoarray::AutoArray < SymBitRun > rlbuffer;
			SymBitRun * ra;
			SymBitRun * rc;
			SymBitRun * re;

			::libmaus2::util::unique_ptr<libmaus2::aio::InputStreamInstance>::type istr;

			#if defined(SLOWDEC)
			::libmaus2::bitio::StreamBitInputStream::unique_ptr_type SBIS;
			#else
			typedef ::libmaus2::huffman::BitInputBuffer4 sbis_type;
			sbis_type::unique_ptr_type SBIS;
			#endif

			uint64_t fileptr;
			uint64_t blockptr;

			bool openNewFile()
			{
				if ( fileptr < idda.data.size() ) // file ptr valid, does file exist?
				{
					assert ( blockptr < idda.data[fileptr].numentries ); // check block pointer

					// open new input file stream
					::libmaus2::util::unique_ptr<libmaus2::aio::InputStreamInstance>::type tistr(
                                                new libmaus2::aio::InputStreamInstance(idda.data[fileptr].filename));
					istr = UNIQUE_PTR_MOVE(tistr);

					// seek to position and check if we succeeded
					istr->clear();
					istr->seekg(idda.data[fileptr].getPos(blockptr),std::ios::beg);

					if ( static_cast<int64_t>(istr->tellg()) != static_cast<int64_t>(idda.data[fileptr].getPos(blockptr)) )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "SymBitDecoder::openNewFile(): Failed to seek in file "
							<< idda.data[fileptr].filename << std::endl;
						se.finish();
						throw se;
					}

					// set up bit input
					sbis_type::raw_input_ptr_type ript(new sbis_type::raw_input_type(*istr));
					sbis_type::unique_ptr_type tSBIS(
                                                        new sbis_type(ript,static_cast<uint64_t>(64*1024))
                                                );
					SBIS = UNIQUE_PTR_MOVE(tSBIS);

					return true;
				}
				else
				{
					return false;
				}
			}

			uint64_t getN() const
			{
				if ( idda.vvec.size() )
					return idda.vvec[idda.vvec.size()-1];
				else
					return 0;
			}

			// init by position in stream
			void init(uint64_t offset)
			{
				if ( offset < idda.vvec[idda.vvec.size()-1] )
				{
					::libmaus2::huffman::FileBlockOffset const FBO = idda.findVBlock(offset);
					fileptr = FBO.fileptr;
					blockptr = FBO.blockptr;
					uint64_t restoffset = FBO.offset;

					openNewFile();

					SymBitRun SC;
					while ( restoffset )
					{
						decodeRun(SC);

						uint64_t const av = std::min(restoffset,SC.rlen);

						if ( av < SC.rlen )
							putBack(SymBitRun(SC.sym,SC.sbit,SC.rlen-av));

						restoffset -= av;
					}
				}
			}


			// init by position in stream
			void initK(uint64_t offset)
			{
				if ( offset < idda.kvec[idda.kvec.size()-1] )
				{
					::libmaus2::huffman::FileBlockOffset const FBO = idda.findKBlock(offset);
					fileptr = FBO.fileptr;
					blockptr = FBO.blockptr;

					openNewFile();
					fillBuffer();

					rc += FBO.offset;
				}
			}

			SymBitDecoder(
				std::vector<std::string> const & rfilenames, uint64_t offset /* offset = 0 */, uint64_t const numthreads
			)
			:
			  Pidda(::libmaus2::huffman::IndexDecoderDataArray::construct(rfilenames,numthreads)),
			  idda(*Pidda),
			  ra(rlbuffer.end()), rc(rlbuffer.end()), re(rlbuffer.end()),
			  fileptr(0), blockptr(0)
			{
				init(offset);
			}

			SymBitDecoder(
				::libmaus2::huffman::IndexDecoderDataArray const & ridda,
				uint64_t offset
			)
			:
			  Pidda(),
			  idda(ridda),
			  ra(rlbuffer.end()), rc(rlbuffer.end()), re(rlbuffer.end()),
			  fileptr(0), blockptr(0)
			{
				init(offset);
			}


			SymBitDecoder(::libmaus2::huffman::IndexDecoderDataArray const & ridda, uint64_t const offset, rl_init_type const & type)
			:
			  Pidda(),
			  idda(ridda),
			  ra(rlbuffer.end()), rc(rlbuffer.end()), re(rlbuffer.end()),
			  fileptr(0), blockptr(0)
			{
				switch ( type )
				{
					case rl_init_type_v:
						init(offset);
						break;
					case rl_init_type_k:
						initK(offset);
						break;
				}
				// init(offset);
			}

			SymBitDecoder(
				::libmaus2::huffman::IndexDecoderDataArray const & ridda,
				::libmaus2::huffman::IndexEntryContainerVector const * /* = 0 */,
				uint64_t offset /* = 0 */
			)
			:
			  Pidda(),
			  idda(ridda),
			  ra(rlbuffer.end()), rc(rlbuffer.end()), re(rlbuffer.end()),
			  fileptr(0), blockptr(0)
			{
				init(offset);
			}

			void decodeSymSeq(uint64_t const numsymruns)
			{
				::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(*SBIS);
				// construct decoder for symbols
				::libmaus2::huffman::CanonicalEncoder symdec(symmap);

				// decode symbols
				for ( uint64_t i = 0; i < numsymruns; ++i )
				{
					uint64_t const sym = symdec.fastDecode(*SBIS);
					rlbuffer[i].sym = sym;
				}
			}

			void decodeSymCntSeq(uint64_t const numsymruns)
			{
				// read huffman code maps
				bool const symcntescape = SBIS->readBit();
				::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symcntmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(*SBIS);

				// construct decoder for symbol runlengths
				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type escsymcntdec;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type symcntdec;
				if ( symcntescape )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tescsymcntdec(new ::libmaus2::huffman::EscapeCanonicalEncoder(symcntmap));
					escsymcntdec = UNIQUE_PTR_MOVE(tescsymcntdec);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tsymcntdec(new ::libmaus2::huffman::CanonicalEncoder(symcntmap));
					symcntdec = UNIQUE_PTR_MOVE(tsymcntdec);
				}

				// decode runlengths
				if ( symcntescape )
					for ( uint64_t i = 0; i < numsymruns; ++i )
					{
						uint64_t const cnt = escsymcntdec->fastDecode(*SBIS);
						rlbuffer[i].rlen = cnt;
					}
				else
					for ( uint64_t i = 0; i < numsymruns; ++i )
					{
						uint64_t const cnt = symcntdec->fastDecode(*SBIS);
						rlbuffer[i].rlen = cnt;
					}
			}

			void decodeSBits(uint64_t const numsymruns)
			{
				for ( uint64_t i = 0; i < numsymruns; ++i )
					rlbuffer[i].sbit = SBIS->readBit();
			}

			bool fillBuffer()
			{
				bool newfile = false;

				// check if we need to open a new file
				while ( fileptr < idda.data.size() && blockptr == idda.data[fileptr].numentries )
				{
					fileptr++;
					blockptr = 0;
					newfile = true;
				}
				// we have reached the end, no more blocks
				if ( fileptr == idda.data.size() )
					return false;
				// open new file if necessary
				if ( newfile )
					openNewFile();

				// byte align stream
				SBIS->flush();

				// read block size
				uint64_t const numsymruns = ::libmaus2::bitio::readElias2(*SBIS);
				rlbuffer.ensureSize(numsymruns);

				ra = rlbuffer.begin();
				rc = ra;
				re = ra + numsymruns;

				decodeSymSeq(numsymruns);
				decodeSymCntSeq(numsymruns);
				decodeSBits(numsymruns);

				SBIS->flush();

				// next block
				blockptr++;

				return true;
			}

			inline bool decode(SymBit & SC)
			{
				if ( rc == re )
				{
					fillBuffer();
					if ( rc == re )
						return false;
				}

				assert ( rc->rlen );

				SC = static_cast<SymBit const &>(*rc);

				if ( ! (--(rc->rlen)) )
					++rc;

				return true;
			}

			inline bool decodeRun(SymBitRun & SC)
			{
				if ( rc == re )
				{
					fillBuffer();
					if ( rc == re )
						return false;
				}

				assert ( rc->rlen );

				SC = *rc++;

				return true;
			}

			void putBack(SymBitRun const & SC)
			{
				assert ( rc != ra );
				*--rc = SC;
			}

			static uint64_t getLength(std::string const & filename)
			{
				libmaus2::huffman::IndexDecoderData IDD(filename);
				return IDD.vacc;
			};

			// get length of vector of files in symbols
			static uint64_t getLength(std::vector<std::string> const & filenames, uint64_t const numthreads)
			{
				libmaus2::parallel::PosixSpinLock lock;
				uint64_t s = 0;
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					uint64_t const ls = getLength(filenames[i]);
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					s += ls;
				}
				return s;
			}

			static uint64_t getKLength(std::string const & fn)
			{
				libmaus2::huffman::IndexDecoderData IDD(fn);
				return IDD.kacc;
			}

			static uint64_t getKLength(std::vector<std::string> const & fn, uint64_t const numthreads)
			{
				libmaus2::parallel::PosixSpinLock lock;
				uint64_t s = 0;
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < fn.size(); ++i )
				{
					uint64_t const ls = getKLength(fn[i]);
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					s += ls;
				}
				return s;
			}
		};
	}
}
#endif
