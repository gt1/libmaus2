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
#if ! defined(LIBMAUS2_GAMMA_LFRANKPOSENCODER_HPP)
#define LIBMAUS2_GAMMA_LFRANKPOSENCODER_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/LFRankPos.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/huffman/LFSupportBitDecoder.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct LFRankPosEncoder
		{
			typedef LFRankPosEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string fn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;

			typedef libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO64_type;
			SGO64_type::unique_ptr_type PSGO64;

			std::string metafn;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PMETA;

			libmaus2::autoarray::AutoArray<LFRankPos> B;
			LFRankPos * const pa;
			LFRankPos * pc;
			LFRankPos * const pe;

			libmaus2::autoarray::AutoArray<uint64_t> V;
			uint64_t vpo;

			typedef std::pair<int64_t,uint64_t> symrun;
			libmaus2::autoarray::AutoArray<symrun> Asymrun;

			uint64_t valueswritten;

			bool flushed;

			uint64_t offset;

			LFRankPosEncoder(std::string const & rfn, uint64_t const blocksize = 4096)
			: fn(rfn), POSI(new libmaus2::aio::OutputStreamInstance(fn)),
			  PSGO64(new libmaus2::aio::SynchronousGenericOutput<uint64_t>(*POSI,4096)),
			  metafn(fn + ".meta"),
			  PMETA(new libmaus2::aio::OutputStreamInstance(metafn)),
			  B(blocksize,false),
			  pa(B.begin()),
			  pc(B.begin()),
			  pe(B.end()),
			  V(),
			  vpo(0),
			  valueswritten(0),
			  flushed(false),
			  offset(0)
			{
			}

			void encode(LFRankPos const & L)
			{
				*pc = L;

				for ( uint64_t i = 0; i < L.n; ++i )
					V.push(vpo,L.v[i]);

				if ( ++pc == pe )
					implicitFlush();
			}

			void encode(LFRankPos const & L, uint64_t const v)
			{
				*pc = L;

				V.push(vpo,v);
				for ( uint64_t i = 0; i < L.n; ++i )
					V.push(vpo,L.v[i]);
				pc->n += 1;

				if ( ++pc == pe )
					implicitFlush();
			}

			void encodeRL(
				uint64_t const numruns,
				::libmaus2::util::Histogram & symhist,
				::libmaus2::util::Histogram & cnthist
			)
			{
				#if 0
				std::cerr << "encode RL ";
				for ( uint64_t i = 0; i < numruns; ++i )
					std::cerr << "(" << Asymrun[i].first << "," << Asymrun[i].second << ")";
				std::cerr << std::endl;

				std::cerr << std::string(80,'-') << std::endl;
				symhist.print(std::cerr);
				std::cerr << std::string(80,'-') << std::endl;
				cnthist.print(std::cerr);
				#endif


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

				#if 0
				{
					std::string const tmp = "mem://tmp_debug";

					std::vector < std::pair<uint64_t,uint64_t > > const symfreqs = symhist.getFreqSymVector();

					libmaus2::autoarray::AutoArray < std::pair<int64_t, uint64_t> > symclone;

					{
						::libmaus2::aio::OutputStreamInstance OSI(tmp);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(OSI,4096);
						::libmaus2::huffman::CanonicalEncoder symenc(symhist.getByType<int64_t>());
						symclone = symenc.syms.clone();

						::libmaus2::bitio::FastWriteBitWriterStream64Std writer(SGO);
						symenc.serialise(writer);
						for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
							symenc.encode(writer,pi->first);

						writer.flush();
						SGO.flush();
						OSI.flush();
					}

					{
						libmaus2::aio::InputStreamInstance ISI(tmp);
						libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(ISI,4096);
						libmaus2::huffman::LFSSupportBitDecoder bitdec(SGI);
						::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(bitdec);

						std::cerr << "symmap.size()=" << symmap.size() << " symclone.size()=" << symclone.size() << std::endl;

						assert ( symmap.size() == symclone.size() );
						for ( uint64_t i = 0; i < symmap.size(); ++i )
						{
							std::cerr << symmap[i].first << "," << symmap[i].second << "\t" << symclone[i].first << "," << symclone[i].second << std::endl;
							assert ( symmap[i].first == symclone[i].first );
							assert ( symmap[i].second == symclone[i].second );
						}

						::libmaus2::huffman::CanonicalEncoder symdec(symmap);
						for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
							assert ( pi->first == symdec.fastDecode(bitdec) );
					}
				}

				{
					std::string const tmp = "mem://tmp_debug";

					std::vector < std::pair<uint64_t,uint64_t > > const symfreqs = cnthist.getFreqSymVector();

					libmaus2::autoarray::AutoArray < std::pair<int64_t, uint64_t> > symclone;

					{
						::libmaus2::aio::OutputStreamInstance OSI(tmp);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(OSI,4096);
						::libmaus2::huffman::CanonicalEncoder symenc(cnthist.getByType<int64_t>());
						symclone = symenc.syms.clone();
						::libmaus2::bitio::FastWriteBitWriterStream64Std writer(SGO);
						symenc.serialise(writer);
						for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
							symenc.encode(writer,pi->second);
						writer.flush();
						SGO.flush();
						OSI.flush();
					}

					{
						libmaus2::aio::InputStreamInstance ISI(tmp);
						libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(ISI,4096);
						libmaus2::huffman::LFSSupportBitDecoder bitdec(SGI);
						::libmaus2::autoarray::AutoArray< std::pair<int64_t, uint64_t> > symmap = ::libmaus2::huffman::CanonicalEncoder::deserialise(bitdec);

						assert ( symmap.size() == symclone.size() );
						for ( uint64_t i = 0; i < symmap.size(); ++i )
						{
							std::cerr << symmap[i].first << "," << symmap[i].second << "\t" << symclone[i].first << "," << symclone[i].second << std::endl;
							assert ( symmap[i].first == symclone[i].first );
							assert ( symmap[i].second == symclone[i].second );
						}

						::libmaus2::huffman::CanonicalEncoder symdec(symmap);
						for ( symrun * pi = Asymrun.begin(); pi != Asymrun.begin()+numruns; ++pi )
							assert ( pi->second == symdec.fastDecode(bitdec) );
					}
				}
				#endif
			}

			void encodeR()
			{
				#if 0
				std::cerr << "encodeP(";

				for ( LFRankPos * p = pa; p != pc; ++p )
					std::cerr << p->p << ";";

				std::cerr << ")" << std::endl;
				#endif

				uint64_t const sgo_bef = PSGO64->getWrittenBytes();

				for ( LFRankPos * p = pa; p != pc; ++p )
					PSGO64->put(p->r);

				#if 0
				libmaus2::gamma::GammaEncoder<SGO64_type> GE(*PSGO64);

				for ( LFRankPos * p = pa; p != pc; ++p )
					GE.encodeSlow(p->p);
				GE.flush();
				#endif

				uint64_t const sgo_aft = PSGO64->getWrittenBytes();

				offset += sgo_aft - sgo_bef;
			}

			void encodeP()
			{
				#if 0
				std::cerr << "encodeP(";

				for ( LFRankPos * p = pa; p != pc; ++p )
					std::cerr << p->p << ";";

				std::cerr << ")" << std::endl;
				#endif

				uint64_t const sgo_bef = PSGO64->getWrittenBytes();

				for ( LFRankPos * p = pa; p != pc; ++p )
					PSGO64->put(p->p);

				#if 0
				libmaus2::gamma::GammaEncoder<SGO64_type> GE(*PSGO64);

				for ( LFRankPos * p = pa; p != pc; ++p )
					GE.encodeSlow(p->p);
				GE.flush();
				#endif

				uint64_t const sgo_aft = PSGO64->getWrittenBytes();

				offset += sgo_aft - sgo_bef;
			}

			void encodeN()
			{
				LFRankPos * low = pa;
				uint64_t numruns = 0;

				::libmaus2::util::Histogram symhist;
				::libmaus2::util::Histogram cnthist;

				while ( low != pc )
				{
					LFRankPos * high = low;
					uint64_t ref = (high++)->n;
					while ( high != pc && high->n == ref )
						++high;

					Asymrun.push(numruns,symrun(ref,high-low));
					symhist(ref);
					cnthist(high-low);

					low = high;
				}

				encodeRL(numruns,symhist,cnthist);
			}

			void encodeV()
			{
				#if 0
				std::cerr << "encodeV(";
				for ( uint64_t i = 0; i < vpo; ++i )
					std::cerr << V[i] << ";";
				std::cerr << ")" << std::endl;
				#endif

				uint64_t const sgo_bef = PSGO64->getWrittenBytes();

				libmaus2::gamma::GammaEncoder<SGO64_type> GE(*PSGO64);
				for ( uint64_t i = 0; i < vpo; ++i )
					GE.encodeSlow(V[i]);
				GE.flush();

				uint64_t const sgo_aft = PSGO64->getWrittenBytes();

				offset += sgo_aft - sgo_bef;
			}

			void encodeActive()
			{
				uint64_t const sgowritten_bef = PSGO64->getWrittenBytes();

				::libmaus2::bitio::FastWriteBitWriterStream64Std writer(*PSGO64);
				for ( LFRankPos * p = pa; p != pc; ++p )
					writer.writeBit(p->active);

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

					PSGO64->put(bs);
					offset += sizeof(uint64_t);

					// encode ranks (direct words)
					encodeR();
					// encode positions (direct words)
					encodeP();
					// encode number of samples stored
					encodeN();
					// encode values (gamma)
					encodeV();
					// encode active
					encodeActive();

					valueswritten += bs;

					pc = pa;
					vpo = 0;
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

			~LFRankPosEncoder()
			{
				flush();
			}
		};
	}
}

#endif
