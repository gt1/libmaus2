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
#if ! defined(LIBMAUS2_GAMMA_GAMMAPDENCODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAPDENCODER_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaPDEncoder
		{
			typedef GammaPDEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string fn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;
			typedef libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO_type;
			SGO_type::unique_ptr_type PSGO;

			std::string metafn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PMETA;

			libmaus2::autoarray::AutoArray<uint64_t> B;
			uint64_t * const pa;
			uint64_t * pc;
			uint64_t * const pe;

			uint64_t valueswritten;

			bool flushed;

			uint64_t offset;

			GammaPDEncoder(std::string const & rfn, uint64_t const blocksize = 4096)
			: fn(rfn), POSI(new libmaus2::aio::OutputStreamInstance(fn)),
			  PSGO(new libmaus2::aio::SynchronousGenericOutput<uint64_t>(*POSI,4096)),
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

			void encode(uint64_t const v)
			{
				*(pc++) = v;
				if ( pc == pe )
					implicitFlush();
			}

			void implicitFlush()
			{
				if ( pc != pa )
				{
					assert ( ! flushed );

					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,offset);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,valueswritten);

					uint64_t const bs = (pc-pa);

					uint64_t const sgo_bef = PSGO->getWrittenBytes();
					libmaus2::gamma::GammaEncoder<SGO_type> GE(*PSGO);
					GE.encodeSlow(bs-1);
					for ( uint64_t const * pp = pa; pp != pc; ++pp )
						GE.encodeSlow(*pp);
					GE.flush();
					uint64_t const sgo_aft = PSGO->getWrittenBytes();

					offset += (sgo_aft-sgo_bef);
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

					PSGO->flush();

					//uint64_t const offset = PSGO->getWrittenBytes();
					assert ( offset == PSGO->getWrittenBytes() );

					PSGO.reset();

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

			~GammaPDEncoder()
			{
				flush();
			}
		};
	}
}

#endif
