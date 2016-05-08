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
#if ! defined(LIBMAUS2_GAMMA_LFVALUEENCODER_HPP)
#define LIBMAUS2_GAMMA_LFVALUEENCODER_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/LFInfo.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/huffman/LFSupportBitDecoder.hpp>
#include <libmaus2/huffman/LFValue.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFValueEncoder
		{
			typedef LFValueEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string fn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;

			typedef libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO64_type;
			SGO64_type::unique_ptr_type PSGO64;

			std::string metafn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PMETA;

			libmaus2::autoarray::AutoArray<LFValue> B;
			LFValue * const pa;
			LFValue * pc;
			LFValue * const pe;

			typedef std::pair<int64_t,uint64_t> symrun;
			libmaus2::autoarray::AutoArray<symrun> Asymrun;

			uint64_t valueswritten;

			bool flushed;

			uint64_t offset;

			LFValueEncoder(std::string const & rfn, uint64_t const blocksize = 4096)
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

			void encode(LFValue const & L)
			{
				*pc++ = L;

				if ( pc == pe )
					implicitFlush();
			}

			void encodeRL(
				uint64_t const numruns,
				::libmaus2::util::Histogram & symhist,
				::libmaus2::util::Histogram & cnthist
			)
			{
				std::vector < std::pair<uint64_t,uint64_t > > const symfreqs = symhist.getFreqSymVector();
				std::vector < std::pair<uint64_t,uint64_t > > const cntfreqs = cnthist.getFreqSymVector();

				assert ( ! ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(symfreqs) );

				::libmaus2::huffman::CanonicalEncoder symenc(symhist.getByType<int64_t>());
				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntenc;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type cntenc;

				bool const cntesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(cntfreqs);
				if ( cntesc )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(cntfreqs));
					esccntenc = UNIQUE_PTR_MOVE(tesccntenc);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tcntenc(new ::libmaus2::huffman::CanonicalEncoder(cnthist.getByType<int64_t>()));
					cntenc = UNIQUE_PTR_MOVE(tcntenc);
				}

				uint64_t const sgowritten_bef = PSGO64->getWrittenBytes();

				::libmaus2::bitio::FastWriteBitWriterStream64Std writer(*PSGO64);

				// encode number of sym runs
				writer.writeElias2(numruns);
				// encode escape bit
				writer.writeBit(cntesc);

				symenc.serialise(writer);
				if ( esccntenc )
					esccntenc->serialise(writer);
				else
					cntenc->serialise(writer);

				//writer.flush();

				for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
					symenc.encode(writer,pi->first);

				//writer.flush();

				if ( cntesc )
					for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
						esccntenc->encode(writer,pi->second);
				else
					for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
						cntenc->encode(writer,pi->second);

				writer.flush();

				uint64_t const sgowritten_aft = PSGO64->getWrittenBytes();

				offset += sgowritten_aft-sgowritten_bef;
			}

			void encodeSyms()
			{
				#if 0
				std::cerr << "encodeSyms()" << std::endl;
				#endif

				LFValue * low = pa;
				uint64_t numruns = 0;

				::libmaus2::util::Histogram symhist;
				::libmaus2::util::Histogram cnthist;

				while ( low != pc )
				{
					LFValue * high = low;
					int64_t ref = (high++)->sym;
					while ( high != pc && high->sym == ref )
						++high;

					ptrdiff_t const cnt = high-low;
					Asymrun.push(numruns,symrun(ref,cnt));
					symhist(ref);
					cnthist(cnt);

					low = high;
				}

				encodeRL(numruns,symhist,cnthist);
			}

			void encodeSetBit()
			{
				uint64_t const sgowritten_bef = PSGO64->getWrittenBytes();

				::libmaus2::bitio::FastWriteBitWriterStream64Std writer(*PSGO64);
				for ( LFValue * p = pa; p != pc; ++p )
					writer.writeBit(p->sbit);

				writer.flush();

				uint64_t const sgo_aft = PSGO64->getWrittenBytes();

				offset += (sgo_aft-sgowritten_bef);
			}

			void encodeValue()
			{
				uint64_t const sgowritten_bef = PSGO64->getWrittenBytes();

				uint64_t maxval = 0;
				for ( LFValue * p = pa; p != pc; ++p )
					if ( p->sbit )
						if ( p->value > maxval )
							maxval = p->value;
				unsigned int const numbits = ::libmaus2::math::numbits(maxval);

				::libmaus2::bitio::FastWriteBitWriterStream64Std writer(*PSGO64);
				writer.writeElias2(numbits);
				for ( LFValue * p = pa; p != pc; ++p )
					if ( p->sbit )
						writer.write(p->value,numbits);

				writer.flush();

				uint64_t const sgo_aft = PSGO64->getWrittenBytes();

				offset += (sgo_aft-sgowritten_bef);
			}

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

					// encode symbol sequence (RL+huffman)
					encodeSyms();
					// encode setbits
					encodeSetBit();
					//
					encodeValue();

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

			~LFValueEncoder()
			{
				flush();
			}
		};
	}
}

#endif
