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
#if ! defined(LIBMAUS2_GAMMA_GAMMAPDDECODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAPDDECODER_HPP

#include <libmaus2/gamma/GammaPDIndexDecoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>


namespace libmaus2
{
	namespace gamma
	{
		struct GammaPDDecoder : public GammaPDIndexDecoderBase
		{
			typedef GammaPDDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			GammaPDIndexDecoder index;

			GammaPDIndexDecoder::FileBlockOffset FBO;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type PSGI;
			libmaus2::autoarray::AutoArray<uint64_t> B;

			uint64_t * pa;
			uint64_t * pc;
			uint64_t * pe;

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

				libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<uint64_t> > GD(*PSGI);
				uint64_t const bs = GD.decode() + 1;
				B.ensureSize(bs);
				for ( uint64_t i = 0; i < bs; ++i )
					B[i] = GD.decode();

				pa = B.begin();
				pc = B.begin();
				pe = B.begin() + bs;

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

					uint64_t v;
					while ( FBO.offset )
					{
						bool const ok = decode(v);
						assert ( ok );
						FBO.offset--;
					}
				}
			}

			GammaPDDecoder(std::vector<std::string> const & rVfn, uint64_t const offset)
			: index(rVfn)
			{
				init(offset);
			}

			bool decode(uint64_t & v)
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

			uint64_t decode()
			{
				uint64_t v;
				bool const ok = decode(v);
				assert ( ok );
				return v;
			}

			#if 0
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

			uint64_t const headerlength;
			uint64_t valueswritten;

			bool flushed;

			GammaPDDecoder(std::string const & rfn, uint64_t const blocksize = 4096)
			: fn(rfn), POSI(new libmaus2::aio::OutputStreamInstance(fn)),
			  PSGO(new libmaus2::aio::SynchronousGenericOutput<uint64_t>(*POSI,4096)),
			  metafn(fn + ".meta"),
			  PMETA(new libmaus2::aio::OutputStreamInstance(metafn)),
			  B(blocksize,false),
			  pa(B.begin()),
			  pc(B.begin()),
			  pe(B.end()),
			  headerlength(sizeof(uint64_t)),
			  valueswritten(0),
			  flushed(false)
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

					uint64_t const offset = headerlength + PSGO->getWrittenBytes();

					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,offset);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,valueswritten);

					uint64_t const bs = (pc-pa);

					libmaus2::gamma::GammaDecoder<SGO_type> GE(*PSGO);
					GE.encodeSlow(bs-1);
					for ( uint64_t const * pp = pa; pp != pc; ++pp )
						GE.encodeSlow(*pp);
					GE.flush();

					valueswritten += bs;

					pc = pa;
				}
			}

			void flush()
			{
				if ( ! flushed )
				{
					flushed = true;

					implicitFlush();
					PSGO->flush();
					PSGO.reset();

					uint64_t const offset = headerlength + PSGO->getWrittenBytes();

					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,offset);
					libmaus2::util::NumberSerialisation::serialiseNumber(*PMETA,valueswritten);

					PMETA->flush();
					PMETA.reset();

					assert ( POSI->tellp() == static_cast<int64_t>(offset) );

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

			~GammaPDDecoder()
			{
				flush();
			}
			#endif
		};
	}
}

#endif
