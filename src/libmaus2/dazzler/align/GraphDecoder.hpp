/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_GRAPHDECODER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_GRAPHDECODER_HPP

#include <libmaus2/dazzler/align/GraphDecoderContext.hpp>
#include <libmaus2/dazzler/align/GraphDecoderContextAllocator.hpp>
#include <libmaus2/dazzler/align/GraphDecoderContextTypeInfo.hpp>
#include <libmaus2/huffman/InputAdapter.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct GraphDecoder
			{
				typedef GraphDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector < std::pair<uint64_t,uint64_t > > linkcntfreqs;
				std::map<uint64_t,uint64_t> emap;
				std::map<uint64_t,uint64_t> Mbfirst;
				std::map<uint64_t,uint64_t> Mbdif;
				std::map<uint64_t,uint64_t> Mabfirst;
				std::map<uint64_t,uint64_t> Mabdif;
				std::map<uint64_t,uint64_t> Mbbfirst;
				std::map<uint64_t,uint64_t> Mbbdif;
				std::map<uint64_t,uint64_t> Mabfirstrange;
				std::map<uint64_t,uint64_t> Mabdifrange;
				std::map<uint64_t,uint64_t> Mbbfirstrange;
				std::map<uint64_t,uint64_t> Mbbdifrange;

				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esclinkcntenc;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type linkcntenc;

				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type escemapenc;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type emapenc;

				bool linkcntesc;
				bool emapesc;

				uint64_t ipos;
				uint64_t n;

				libmaus2::parallel::LockedGrowingFreeList<libmaus2::dazzler::align::GraphDecoderContext,libmaus2::dazzler::align::GraphDecoderContextAllocator,libmaus2::dazzler::align::GraphDecoderContextTypeInfo> contextFreeList;

				libmaus2::dazzler::align::GraphDecoderContext::shared_ptr_type getContext()
				{
					return contextFreeList.get();
				}

				void returnContext(libmaus2::dazzler::align::GraphDecoderContext::shared_ptr_type context)
				{
					contextFreeList.put(context);
				}

				uint64_t size() const
				{
					return n;
				}

				static void loadMap(libmaus2::huffman::InputAdapter & IA, std::map<uint64_t,uint64_t> & M)
				{
					M.clear();
					uint64_t const c = IA.read(64);

					for ( uint64_t i = 0; i < c; ++i )
					{
						uint64_t const k = IA.read(64);
						uint64_t const v = IA.read(64);
						M[k] = v;
					}
				}

				uint64_t getPointer(std::istream & in, uint64_t const i) const
				{
					in.clear();
					in.seekg(ipos + i * sizeof(uint64_t));
					return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				}

				void init(std::istream & in)
				{
					libmaus2::huffman::InputAdapter IA(in);

					uint64_t const c = IA.read(64);
					linkcntfreqs.resize(c);
					for ( uint64_t i = 0; i < c; ++i )
					{
						linkcntfreqs[i].first = IA.read(64);
						linkcntfreqs[i].second = IA.read(64);
					}

					loadMap(IA,emap);
					loadMap(IA,Mbfirst);
					loadMap(IA,Mbdif);
					loadMap(IA,Mabfirst);
					loadMap(IA,Mabdif);
					loadMap(IA,Mbbfirst);
					loadMap(IA,Mbbdif);
					loadMap(IA,Mabfirstrange);
					loadMap(IA,Mabdifrange);
					loadMap(IA,Mbbfirstrange);
					loadMap(IA,Mbbdifrange);

					linkcntesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(linkcntfreqs);
					if ( linkcntesc )
					{
						::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesclinkcntenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(linkcntfreqs));
						esclinkcntenc = UNIQUE_PTR_MOVE(tesclinkcntenc);
					}
					else
					{
						std::map<int64_t,uint64_t> M;
						for ( uint64_t i = 0; i < linkcntfreqs.size(); ++i )
							M [ linkcntfreqs[i].second ] = linkcntfreqs[i].first;
						::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tlinkcntenc(new ::libmaus2::huffman::CanonicalEncoder(M));
						linkcntenc = UNIQUE_PTR_MOVE(tlinkcntenc);
					}

					std::map<int64_t,uint64_t> iemap;
					for ( std::map<uint64_t,uint64_t>::const_iterator it = emap.begin(); it != emap.end(); ++it )
						iemap[it->first] = it->second;

					emapesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(iemap);
					if ( emapesc )
					{
						::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(iemap));
						escemapenc = UNIQUE_PTR_MOVE(tenc);
					}
					else
					{
						::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tenc(new ::libmaus2::huffman::CanonicalEncoder(iemap));
						emapenc = UNIQUE_PTR_MOVE(tenc);
					}


					in.clear();
					in.seekg(
						- static_cast<int64_t>(sizeof(uint64_t)),
						std::ios::end
					);

					uint64_t const epos = in.tellg();

					ipos = libmaus2::util::NumberSerialisation::deserialiseNumber(in);

					assert ( (epos - ipos) % sizeof(uint64_t) == 0 );

					n = (epos - ipos)/sizeof(uint64_t);
				}

				GraphDecoder(std::istream & in)
				{
					init(in);
				}

				GraphDecoder(std::string const & fn)
				{
					libmaus2::aio::InputStreamInstance ISI(fn);
					init(ISI);
				}

				template<typename decoder_type>
				static int64_t decodeSignedValue(decoder_type & decoder)
				{
					bool const sign = decoder.decodeWord(1);
					int64_t val = decoder.decode();
					return sign ? -val : val;
				}

				void decode(std::istream & in, uint64_t const i, uint64_t const j, libmaus2::dazzler::align::GraphDecoderContext & GDC) const
				{
					decode(in,i,GDC);

					uint64_t o = 0;
					for ( uint64_t i = 0; i < GDC.size(); ++i )
						if ( GDC[i].bread == static_cast<int64_t>(j) )
							GDC.A[o++] = GDC[i];
					GDC.n = o;
				}

				void decode(std::istream & in, uint64_t const i, libmaus2::dazzler::align::GraphDecoderContext & GDC) const
				{
					uint64_t const p = getPointer(in,i);

					if ( p != std::numeric_limits<uint64_t>::max() )
					{
						in.clear();
						in.seekg(p,std::ios::beg);

						libmaus2::huffman::InputAdapter IA(in);

						uint64_t const c = linkcntesc ? esclinkcntenc->fastDecode(*(IA.PBIB)) : linkcntenc->fastDecode(*(IA.PBIB));

						GDC.setup(c);


						libmaus2::autoarray::AutoArray<uint64_t> & absort = GDC.absort;
						libmaus2::autoarray::AutoArray<uint64_t> & bbsort = GDC.bbsort;
						libmaus2::autoarray::AutoArray<uint64_t> & arangesort = GDC.arangesort;
						libmaus2::autoarray::AutoArray<uint64_t> & brangesort = GDC.brangesort;
						libmaus2::autoarray::AutoArray<uint64_t> & O = GDC.O;
						libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::OverlapHeader> & A = GDC.A;

						uint64_t const nb = libmaus2::math::numbits(c-1);

						for ( uint64_t i = 0; i < c; ++i )
						{
							if ( emapesc )
							{
								A[i].diffs = escemapenc->fastDecode(*(IA.PBIB));
							}
							else
							{
								A[i].diffs = emapenc->fastDecode(*(IA.PBIB));
							}
						}

						for ( uint64_t j = 0; j < c; ++j )
							A[j].aread = i;
						for ( uint64_t i = 0; i < c; ++i )
							A[i].inv = IA.read(1);
						for ( uint64_t i = 0; i < c; ++i )
							absort[i] = IA.read(nb);
						for ( uint64_t i = 0; i < c; ++i )
							bbsort[i] = IA.read(nb);
						for ( uint64_t i = 0; i < c; ++i )
							arangesort[i] = IA.read(nb);
						for ( uint64_t i = 0; i < c; ++i )
							brangesort[i] = IA.read(nb);

						libmaus2::gamma::GammaDecoder<libmaus2::huffman::InputAdapter> GD(IA);

						int64_t const bfirstavg = Mbfirst.find(c)->second;
						int64_t const bdifavg = Mbdif.find(c)->second;

						A[0].bread = decodeSignedValue(GD) + bfirstavg;
						for ( uint64_t i = 1; i < c; ++i )
							A[i].bread = A[i-1].bread + decodeSignedValue(GD) + bdifavg;

						int64_t const abfirstavg = Mabfirst.find(c)->second;
						int64_t const abdifavg = Mabdif.find(c)->second;
						O[0] = decodeSignedValue(GD) + abfirstavg;
						for ( uint64_t i = 1; i < c; ++i )
							O[i] = O[i-1] + decodeSignedValue(GD) + abdifavg;

						for ( uint64_t i = 0; i < c; ++i )
							A[ absort[i] ].abpos = O[i];

						int64_t const bbfirstavg = Mbbfirst.find(c)->second;
						int64_t const bbdifavg = Mbbdif.find(c)->second;
						O[0] = decodeSignedValue(GD) + bbfirstavg;
						for ( uint64_t i = 1; i < c; ++i )
							O[i] = O[i-1] + decodeSignedValue(GD) + bbdifavg;

						for ( uint64_t i = 0; i < c; ++i )
							A[ bbsort[i] ].bbpos = O[i];

						int64_t const abfirstrangeavg = Mabfirstrange.find(c)->second;
						int64_t const abdifrangeavg = Mabdifrange.find(c)->second;
						O[0] = decodeSignedValue(GD) + abfirstrangeavg;
						for ( uint64_t i = 1; i < c; ++i )
							O[i] = O[i-1] + decodeSignedValue(GD) + abdifrangeavg;

						for ( uint64_t i = 0; i < c; ++i )
							A[ arangesort[i] ].aepos = O[i] + A[ arangesort[i] ].abpos;

						int64_t const bbfirstrangeavg = Mbbfirstrange.find(c)->second;
						int64_t const bbdifrangeavg = Mbbdifrange.find(c)->second;
						O[0] = decodeSignedValue(GD) + bbfirstrangeavg;
						for ( uint64_t i = 1; i < c; ++i )
							O[i] = O[i-1] + decodeSignedValue(GD) + bbdifrangeavg;

						for ( uint64_t i = 0; i < c; ++i )
							A[ brangesort[i] ].bepos = O[i] + A[ brangesort[i] ].bbpos;
					}
					else
					{
						GDC.setup(0);
					}
				}
			};
		}
	}
}
#endif
