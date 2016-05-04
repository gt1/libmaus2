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
#if ! defined(LIBMAUS2_GAMMA_LFRANKLCPENCODER_HPP)
#define LIBMAUS2_GAMMA_LFRANKLCPENCODER_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/LFInfo.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/huffman/LFSupportBitDecoder.hpp>
#include <libmaus2/huffman/LFRankLCP.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFRankLCPEncoder
		{
			typedef LFRankLCPEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string fn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;

			typedef libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO64_type;
			SGO64_type::unique_ptr_type PSGO64;

			std::string metafn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PMETA;

			libmaus2::autoarray::AutoArray<LFRankLCP> B;
			LFRankLCP * const pa;
			LFRankLCP * pc;
			LFRankLCP * const pe;

			typedef std::pair<int64_t,uint64_t> symrun;
			libmaus2::autoarray::AutoArray<symrun> Asymrun;

			uint64_t valueswritten;

			bool flushed;

			uint64_t offset;

			LFRankLCPEncoder(std::string const & rfn, uint64_t const blocksize = 4096)
			: fn(rfn), POSI(new libmaus2::aio::OutputStreamInstance(fn)),
			  PSGO64(new libmaus2::aio::SynchronousGenericOutput<uint64_t>(*POSI,4096)),
			  metafn(fn + ".meta"),
			  PMETA(new libmaus2::aio::OutputStreamInstance(metafn)),
			  B(blocksize,false),
			  pa(B.begin()),
			  pc(B.begin()),
			  pe(B.end()),
			  valueswritten(0),
			  flushed(false),
			  offset(0)
			{
			}

			void encode(LFRankLCP const & L)
			{
				*pc++ = L;

				if ( pc == pe )
					implicitFlush();
			}

			template<typename projector>
			void encodeSequence()
			{
				uint64_t const sgowritten_bef = PSGO64->getWrittenBytes();

				uint64_t maxval = 0;
				for ( LFRankLCP * p = pa; p != pc; ++p )
					if ( projector::project(*p) > maxval )
							maxval = projector::project(*p);
				unsigned int const numbits = ::libmaus2::math::numbits(maxval);

				::libmaus2::bitio::FastWriteBitWriterStream64Std writer(*PSGO64);
				writer.writeElias2(pc-pa);
				writer.writeElias2(numbits);
				for ( LFRankLCP * p = pa; p != pc; ++p )
					writer.write(projector::project(*p),numbits);

				writer.flush();

				uint64_t const sgo_aft = PSGO64->getWrittenBytes();

				offset += (sgo_aft-sgowritten_bef);
			}

			struct RProjector { static uint64_t project(LFRankLCP const & P) { return P.r; } };
			struct LCPProjector { static uint64_t project(LFRankLCP const & P) { return P.lcp; } };

			void implicitFlush()
			{

				if ( pc != pa )
				{
					#if 0
					std::cerr << "implicitFlush()" << std::endl;
					#endif

					assert ( ! flushed );

					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,offset);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,valueswritten);

					uint64_t const bs = (pc-pa);

					encodeSequence<RProjector>();
					encodeSequence<LCPProjector>();

					valueswritten += bs;

					pc = pa;
				}
			}

			void flush()
			{
				if ( ! flushed )
				{
					implicitFlush();
					flushed = true;

					PSGO64->flush();

					//uint64_t const offset = PSGO64->getWrittenBytes();
					assert ( offset == PSGO64->getWrittenBytes() );

					PSGO64.reset();

					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,offset);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,valueswritten);

					PMETA->flush();
					PMETA.reset();

					assert ( static_cast<int64_t>(POSI->tellp()) == static_cast<int64_t>(offset) );

					libmaus2::aio::InputStreamInstance::unique_ptr_type METAin(new libmaus2::aio::InputStreamInstance(metafn));
					while ( METAin->peek() != std::istream::traits_type::eof() )
						libmaus2::util::NumberSerialisation::serialiseNumber(*POSI,libmaus2::util::NumberSerialisation::deserialiseNumber(*METAin));
					METAin.reset();

					libmaus2::aio::FileRemoval::removeFile(metafn);

					// write index offset
					libmaus2::util::NumberSerialisation::serialiseNumber(*POSI,offset);

					POSI->flush();
					POSI.reset();
				}
			}

			~LFRankLCPEncoder()
			{
				flush();
			}
		};
	}
}

#endif
