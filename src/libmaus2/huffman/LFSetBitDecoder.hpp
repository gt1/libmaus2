/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_HUFFMAN_LFSETBITDECODER_HPP)
#define LIBMAUS2_HUFFMAN_LFSETBITDECODER_HPP

#include <libmaus2/huffman/LFSupportBitDecoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/gamma/GammaPDIndexDecoder.hpp>
#include <libmaus2/huffman/LFInfo.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/bitio/readElias.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/huffman/LFSetBit.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFSetBitDecoder : public libmaus2::gamma::GammaPDIndexDecoderBase
		{
			typedef LFSetBitDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef LFSetBit value_type;

			libmaus2::gamma::GammaPDIndexDecoder index;
			libmaus2::gamma::GammaPDIndexDecoder::FileBlockOffset FBO;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type PSGI;

			libmaus2::autoarray::AutoArray<LFSetBit> B;

			LFSetBit * pa;
			LFSetBit * pc;
			LFSetBit * pe;

			typedef std::pair<int64_t,uint64_t> rl_pair;
			::libmaus2::autoarray::AutoArray < rl_pair > rlbuffer;

			void openFile()
			{
				if ( FBO.file < index.Vfn.size() )
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type TISI(new libmaus2::aio::InputStreamInstance(index.Vfn[FBO.file]));
					PISI = UNIQUE_PTR_MOVE(TISI);
					PISI->clear();
					PISI->seekg(FBO.blockoffset);

					libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type TSGI(new libmaus2::aio::SynchronousGenericInput<uint64_t>(*PISI,4096,std::numeric_limits<uint64_t>::max(),false /* check mod */));
					PSGI = UNIQUE_PTR_MOVE(TSGI);
				}
				else
				{
					PSGI.reset();
					PISI.reset();
				}
			}

			uint64_t decodeRL()
			{
				LFSSupportBitDecoder SBIS(*PSGI);

				// read block size
				uint64_t const bs = ::libmaus2::bitio::readElias2(SBIS);

				bool const cntescape = SBIS.readBit();

				// read huffman code maps
				::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(SBIS);
				::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > cntmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(SBIS);

				// construct decoder for symbols
				::libmaus2::huffman::CanonicalEncoder symdec(symmap);

				// construct decoder for runlengths
				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntdec;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type cntdec;
				if ( cntescape )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntdec(new ::libmaus2::huffman::EscapeCanonicalEncoder(cntmap));
					esccntdec = UNIQUE_PTR_MOVE(tesccntdec);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tcntdec(new ::libmaus2::huffman::CanonicalEncoder(cntmap));
					cntdec = UNIQUE_PTR_MOVE(tcntdec);
				}

				// increase buffersize if necessary
				rlbuffer.ensureSize(bs);

				// byte align input stream
				//SBIS.flush();

				// decode symbols
				for ( uint64_t i = 0; i < bs; ++i )
				{
					uint64_t const sym = symdec.fastDecode(SBIS);
					rlbuffer[i].first = sym;
				}

				// byte align
				// SBIS.flush();

				// decode runlengths
				if ( cntescape )
					for ( uint64_t i = 0; i < bs; ++i )
					{
						uint64_t const cnt = esccntdec->fastDecode(SBIS);
						rlbuffer[i].second = cnt;
					}
				else
					for ( uint64_t i = 0; i < bs; ++i )
					{
						uint64_t const cnt = cntdec->fastDecode(SBIS);
						rlbuffer[i].second = cnt;
					}

				// byte align
				SBIS.flush();

				return bs;
			}

			void decodeSetBit(uint64_t const numobj)
			{
				LFSSupportBitDecoder SBIS(*PSGI);

				for ( uint64_t i = 0; i < numobj; ++i )
					B[i].sbit = SBIS.readBit();

				SBIS.flush();
			}

			bool decodeBlock()
			{
				while ( FBO.file < index.Vfn.size() && FBO.block >= index.blocksPerFile[FBO.file] )
				{
					FBO.file++;
					FBO.block = 0;
					FBO.blockoffset = 0; // check this if we change the file format
					FBO.offset = 0;
					openFile();
				}
				if ( FBO.file == index.Vfn.size() )
				{
					PSGI.reset();
					PISI.reset();
					return false;
				}

				uint64_t const numsymruns = decodeRL();

				uint64_t numobj = 0;
				for ( uint64_t i = 0; i < numsymruns; ++i )
					numobj += rlbuffer[i].second;

				B.ensureSize(numobj);
				uint64_t o = 0;
				for ( uint64_t i = 0; i < numsymruns; ++i )
					for ( uint64_t j = 0; j < rlbuffer[i].second; ++j )
						B[o++].sym = rlbuffer[i].first;

				decodeSetBit(numobj);

				pa = B.begin();
				pc = B.begin();
				pe = B.begin()+numobj;

				FBO.block += 1;

				return true;
			}

			void init(uint64_t const offset)
			{
				FBO = index.lookup(offset);

				if ( FBO.file < index.Vfn.size() )
				{
					openFile();
					decodeBlock();

					LFSetBit v;
					while ( FBO.offset )
					{
						bool const ok = decode(v);
						assert ( ok );
						FBO.offset--;
					}
				}
			}

			LFSetBitDecoder(std::vector<std::string> const & rVfn, uint64_t const offset)
			: index(rVfn), FBO(), PISI(), PSGI(), B(), pa(0), pc(0), pe(0), rlbuffer()
			{
				init(offset);
			}

			bool decode(LFSetBit & v)
			{
				while ( pc == pe )
				{
					bool const ok = decodeBlock();
					if ( ! ok )
						return false;
				}

				v = *(pc++);
				return true;
			}

			LFSetBit decode()
			{
				LFSetBit v;
				bool const ok = decode(v);
				assert ( ok );
				return v;
			}

			static uint64_t getLength(std::string const & fn, uint64_t const /* numthreads */)
			{
				return libmaus2::gamma::GammaPDIndexDecoderBase::getNumValues(fn);
			}

			static uint64_t getLength(
				std::vector<std::string> const & Vfn,
				uint64_t const
					#if defined(_OPENMP)
					numthreads
					#endif
			)
			{
				uint64_t volatile s = 0;
				libmaus2::parallel::PosixSpinLock lock;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					uint64_t const ls = getLength(Vfn[i],1);
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					s += ls;
				}
				return s;
			}
		};
	}
}

#endif
