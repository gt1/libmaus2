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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_GRAPHENCODER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_GRAPHENCODER_HPP

#include <libmaus2/dazzler/align/OverlapIndexer.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/OutputAdapter.hpp>
#include <libmaus2/sorting/SortingBufferedOutputFile.hpp>
#include <libmaus2/util/Histogram.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct GraphEncoder
			{
				struct ASort
				{
					int64_t abpos;
					int64_t aepos;
					uint64_t index;

					ASort() {}
					ASort(int64_t const rabpos, int64_t const raepos, uint64_t const rindex)
					: abpos(rabpos), aepos(raepos), index(rindex) {}

					int64_t getRange() const
					{
						return aepos-abpos;
					}

					bool operator<(ASort const & A) const
					{
						if ( abpos != A.abpos )
							return abpos < A.abpos;
						if ( aepos != A.aepos )
							return aepos < A.aepos;
						if ( index != A.index )
							return index < A.index;
						return false;
					}
				};

				struct ASortRange
				{
					bool operator()(ASort const & A, ASort const & B) const
					{
						int64_t const a = A.getRange();
						int64_t const b = B.getRange();
						return a < b;
					}
				};

				template<typename ge_type>
				static void encodeSignedValue(ge_type & GE, int64_t const af)
				{
					GE.encodeWord((af < 0) ? 1 : 0, 1);
					GE.encode((af < 0) ? (-af) : af);
				}

				static void writeNumber(libmaus2::huffman::HuffmanEncoderFileStd & HEFS, uint64_t const v)
				{
					HEFS.write(v >> 32,32);
					HEFS.write(v&0xFFFFFFFFULL,32);
				}

				static void writeMap(libmaus2::huffman::HuffmanEncoderFileStd & HEFS, std::map<uint64_t,uint64_t> const & M)
				{
					writeNumber(HEFS,M.size());
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					{
						writeNumber(HEFS,ita->first);
						writeNumber(HEFS,ita->second);
					}
				}

				static void writePairVector(libmaus2::huffman::HuffmanEncoderFileStd & HEFS, std::vector < std::pair<uint64_t,uint64_t > > const & V)
				{
					writeNumber(HEFS,V.size());
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						writeNumber(HEFS,V[i].first);
						writeNumber(HEFS,V[i].second);
					}
				}

				struct PointerEntry
				{
					uint64_t index;
					uint64_t pointer;

					PointerEntry()
					{

					}

					PointerEntry(std::istream & in)
					: index(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
					  pointer(libmaus2::util::NumberSerialisation::deserialiseNumber(in)) {}

					PointerEntry(double const rindex, uint64_t const rpointer) : index(rindex), pointer(rpointer) {}

					std::ostream & serialise(std::ostream & out) const
					{
						libmaus2::util::NumberSerialisation::serialiseNumber(out,index);
						libmaus2::util::NumberSerialisation::serialiseNumber(out,pointer);
						return out;
					}

					std::istream & deserialise(std::istream & in)
					{
						*this = PointerEntry(in);
						return in;
					}

					bool operator<(PointerEntry const & E) const
					{
						if ( index != E.index )
							return index < E.index;
						else
							return pointer < E.pointer;
					}

					bool operator>(PointerEntry const & E) const
					{
						if ( index != E.index )
							return index > E.index;
						else
							return pointer > E.pointer;
					}

				};

				static void encodegraph(
					std::string const & out,
					std::vector<std::string> const & arg,
					std::string const & tmpfilebase,
					int const verbose
				)
				{
					libmaus2::aio::OutputStreamInstance OSI(out);
					encodegraph(OSI,arg,tmpfilebase,verbose);
				}

				static void encodegraph(
					std::ostream & out,
					std::vector<std::string> const & arg,
					std::string const & tmpfilebase,
					int const verbose
				)
				{
					libmaus2::util::Histogram linkcnthist;
					std::map<uint64_t,uint64_t> bdif;
					std::map<uint64_t,uint64_t> bdifcnt;
					std::map<uint64_t,uint64_t> bfirst;
					std::map<uint64_t,uint64_t> bfirstcnt;
					std::map<uint64_t,uint64_t> abfirst;
					std::map<uint64_t,uint64_t> abfirstcnt;
					std::map<uint64_t,uint64_t> abdif;
					std::map<uint64_t,uint64_t> abdifcnt;
					std::map<uint64_t,uint64_t> bbfirst;
					std::map<uint64_t,uint64_t> bbfirstcnt;
					std::map<uint64_t,uint64_t> bbdif;
					std::map<uint64_t,uint64_t> bbdifcnt;
					std::map<uint64_t,uint64_t> abfirstrange;
					std::map<uint64_t,uint64_t> abfirstrangecnt;
					std::map<uint64_t,uint64_t> abdifrange;
					std::map<uint64_t,uint64_t> abdifrangecnt;
					std::map<uint64_t,uint64_t> bbfirstrange;
					std::map<uint64_t,uint64_t> bbfirstrangecnt;
					std::map<uint64_t,uint64_t> bbdifrange;
					std::map<uint64_t,uint64_t> bbdifrangecnt;
					std::map<uint64_t,uint64_t> emap;

					std::string const tmpptr = tmpfilebase + "_pointers";

					for ( uint64_t i = 0; i < arg.size(); ++i )
					{
						libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type AF(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(arg[i]));
						libmaus2::dazzler::align::Overlap OVL;

						while ( AF->peekNextOverlap(OVL) )
						{
							int64_t const aid = OVL.aread;
							std::vector < libmaus2::dazzler::align::Overlap > V;

							while ( AF->peekNextOverlap(OVL) && OVL.aread == aid )
							{
								AF->getNextOverlap(OVL);
								V.push_back(OVL);
							}

							assert ( V.size() );

							for ( uint64_t i = 1; i < V.size(); ++i )
							{
								assert ( V[i-1].aread == V[i].aread );
								assert ( V[i-1].bread <= V[i].bread );
							}

							for ( uint64_t i = 1; i < V.size(); ++i )
								bdif[V.size()] += (V[i].bread - V[i-1].bread);
							bdifcnt[V.size()] += V.size()-1;

							bfirst[V.size()] += V[0].bread;
							bfirstcnt[V.size()] += 1;

							std::vector<ASort> VA;
							for ( uint64_t i = 0; i < V.size(); ++i )
								VA.push_back(ASort(V[i].path.abpos,V[i].path.aepos,i));
							std::sort(VA.begin(),VA.end());

							abfirst[V.size()] += VA[0].abpos;
							abfirstcnt[V.size()] += 1;

							for ( uint64_t i = 1; i < VA.size(); ++i )
								abdif[V.size()] += (VA[i].abpos - VA[i-1].abpos);
							abdifcnt[V.size()] += VA.size()-1;

							std::vector<ASort> VB;
							for ( uint64_t i = 0; i < V.size(); ++i )
								VB.push_back(ASort(V[i].path.bbpos,V[i].path.bepos,i));
							std::sort(VB.begin(),VB.end());

							bbfirst[V.size()] += VB[0].abpos;
							bbfirstcnt[V.size()] += 1;

							for ( uint64_t i = 1; i < VB.size(); ++i )
								bbdif[V.size()] += (VB[i].abpos - VB[i-1].abpos);
							bbdifcnt[V.size()] += VB.size()-1;

							std::sort(VA.begin(),VA.end(),ASortRange());
							abfirstrange[V.size()] += VA[0].getRange();
							for ( uint64_t i = 1; i < VA.size(); ++i )
								abdifrange[V.size()] += VA[i].getRange() - VA[i-1].getRange();
							abfirstrangecnt[V.size()] += 1;
							abdifrangecnt[V.size()] += VA.size()-1;

							std::sort(VB.begin(),VB.end(),ASortRange());
							bbfirstrange[V.size()] += VB[0].getRange();
							for ( uint64_t i = 1; i < VB.size(); ++i )
								bbdifrange[V.size()] += VB[i].getRange() - VB[i-1].getRange();
							bbfirstrangecnt[V.size()] += 1;
							bbdifrangecnt[V.size()] += VA.size()-1;

							for ( uint64_t i = 0; i < V.size(); ++i )
								emap[V[i].path.diffs]++;

							linkcnthist(V.size());

							if ( verbose && ( (aid % (16*1024) == 0) || (!(AF->peekNextOverlap(OVL))) ) )
								std::cerr << "[V] " << aid << std::endl;
						}
					}

					std::map<uint64_t,uint64_t> Mbfirst;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = bfirst.begin(); ita != bfirst.end(); ++ita )
						Mbfirst [ ita -> first ] = ( ita->second + bfirstcnt.find(ita->first)->second - 1 ) / bfirstcnt.find(ita->first)->second;
					std::map<uint64_t,uint64_t> Mbdif;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = bdif.begin(); ita != bdif.end(); ++ita )
						Mbdif [ ita -> first ] = ( ita->second + bdifcnt.find(ita->first)->second - 1 ) / bdifcnt.find(ita->first)->second;

					std::map<uint64_t,uint64_t> Mabfirst;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = abfirst.begin(); ita != abfirst.end(); ++ita )
						Mabfirst [ ita -> first ] = ( ita->second + abfirstcnt.find(ita->first)->second - 1 ) / abfirstcnt.find(ita->first)->second;
					std::map<uint64_t,uint64_t> Mabdif;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = abdif.begin(); ita != abdif.end(); ++ita )
						Mabdif [ ita -> first ] = ( ita->second + abdifcnt.find(ita->first)->second - 1 ) / abdifcnt.find(ita->first)->second;

					std::map<uint64_t,uint64_t> Mbbfirst;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = bbfirst.begin(); ita != bbfirst.end(); ++ita )
						Mbbfirst [ ita -> first ] = ( ita->second + bbfirstcnt.find(ita->first)->second - 1 ) / bbfirstcnt.find(ita->first)->second;
					std::map<uint64_t,uint64_t> Mbbdif;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = bbdif.begin(); ita != bbdif.end(); ++ita )
						Mbbdif [ ita -> first ] = ( ita->second + bbdifcnt.find(ita->first)->second - 1 ) / bbdifcnt.find(ita->first)->second;

					std::map<uint64_t,uint64_t> Mabfirstrange;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = abfirstrange.begin(); ita != abfirstrange.end(); ++ita )
						Mabfirstrange [ ita -> first ] = ( ita->second + abfirstrangecnt.find(ita->first)->second - 1 ) / abfirstrangecnt.find(ita->first)->second;
					std::map<uint64_t,uint64_t> Mabdifrange;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = abdifrange.begin(); ita != abdifrange.end(); ++ita )
						Mabdifrange [ ita -> first ] = ( ita->second + abdifrangecnt.find(ita->first)->second - 1 ) / abdifrangecnt.find(ita->first)->second;

					std::map<uint64_t,uint64_t> Mbbfirstrange;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = bbfirstrange.begin(); ita != bbfirstrange.end(); ++ita )
						Mbbfirstrange [ ita -> first ] = ( ita->second + bbfirstrangecnt.find(ita->first)->second - 1 ) / bbfirstrangecnt.find(ita->first)->second;
					std::map<uint64_t,uint64_t> Mbbdifrange;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = bbdifrange.begin(); ita != bbdifrange.end(); ++ita )
						Mbbdifrange [ ita -> first ] = ( ita->second + bbdifrangecnt.find(ita->first)->second - 1 ) / bbdifrangecnt.find(ita->first)->second;

					std::vector < std::pair<uint64_t,uint64_t > > const linkcntfreqs = linkcnthist.getFreqSymVector();

					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esclinkcntenc;
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type linkcntenc;

					bool const linkcntesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(linkcntfreqs);
					if ( linkcntesc )
					{
						::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesclinkcntenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(linkcntfreqs));
						esclinkcntenc = UNIQUE_PTR_MOVE(tesclinkcntenc);
					}
					else
					{
						::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tlinkcntenc(new ::libmaus2::huffman::CanonicalEncoder(linkcnthist.getByType<int64_t>()));
						linkcntenc = UNIQUE_PTR_MOVE(tlinkcntenc);
					}

					std::map<int64_t,uint64_t> iemap;
					for ( std::map<uint64_t,uint64_t>::const_iterator it = emap.begin(); it != emap.end(); ++it )
						iemap[it->first] = it->second;

					bool const emapesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(iemap);
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type escemapenc;
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type emapenc;
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

					libmaus2::util::TempFileRemovalContainer::addTempFile(tmpptr);
					libmaus2::aio::OutputStreamInstance::unique_ptr_type Tptr(new libmaus2::aio::OutputStreamInstance(tmpptr));

					libmaus2::sorting::SerialisingSortingBufferedOutputFile<PointerEntry,std::less<PointerEntry> >::unique_ptr_type SSBOF(
						new libmaus2::sorting::SerialisingSortingBufferedOutputFile<PointerEntry,std::less<PointerEntry>>(tmpptr));

					libmaus2::huffman::HuffmanEncoderFileStd HEFS(out);

					writePairVector(HEFS,linkcntfreqs);
					writeMap(HEFS,emap);
					writeMap(HEFS,Mbfirst);
					writeMap(HEFS,Mbdif);
					writeMap(HEFS,Mabfirst);
					writeMap(HEFS,Mabdif);
					writeMap(HEFS,Mbbfirst);
					writeMap(HEFS,Mbbdif);
					writeMap(HEFS,Mabfirstrange);
					writeMap(HEFS,Mabdifrange);
					writeMap(HEFS,Mbbfirstrange);
					writeMap(HEFS,Mbbdifrange);

					for ( uint64_t i = 0; i < arg.size(); ++i )
					{
						libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type AF(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(arg[i]));
						libmaus2::dazzler::align::Overlap OVL;

						while ( AF->peekNextOverlap(OVL) )
						{
							int64_t const aid = OVL.aread;
							std::vector < libmaus2::dazzler::align::Overlap > V;

							while ( AF->peekNextOverlap(OVL) && OVL.aread == aid )
							{
								AF->getNextOverlap(OVL);
								V.push_back(OVL);
							}

							assert ( V.size() );

							uint64_t const p = HEFS.tellp();

							SSBOF->put(PointerEntry(aid,p));

							uint64_t const nb = libmaus2::math::numbits(V.size()-1);

							std::vector<ASort> VA;
							for ( uint64_t i = 0; i < V.size(); ++i )
								VA.push_back(ASort(V[i].path.abpos,V[i].path.aepos,i));
							std::sort(VA.begin(),VA.end());

							std::vector<ASort> VB;
							for ( uint64_t i = 0; i < V.size(); ++i )
								VB.push_back(ASort(V[i].path.bbpos,V[i].path.bepos,i));
							std::sort(VB.begin(),VB.end());

							// length of vector
							if ( linkcntesc )
								esclinkcntenc->encode(HEFS,V.size());
							else
								linkcntenc->encode(HEFS,V.size());

							for ( uint64_t i = 0; i < V.size(); ++i )
							{
								int64_t const d = V[i].path.diffs;
								if ( emapesc )
									escemapenc->encode(HEFS,d);
								else
									emapenc->encode(HEFS,d);
							}

							// inverse bit
							for ( uint64_t i = 0; i < V.size(); ++i )
								HEFS.write(V[i].isInverse(),1);

							// sorting for ab and bb
							for ( uint64_t i = 0; i < VA.size(); ++i )
								HEFS.write(VA[i].index,nb);
							for ( uint64_t i = 0; i < VB.size(); ++i )
								HEFS.write(VB[i].index,nb);

							// sorting for arange and brange
							std::sort(VA.begin(),VA.end(),ASortRange());
							std::sort(VB.begin(),VB.end(),ASortRange());
							for ( uint64_t i = 0; i < VA.size(); ++i )
								HEFS.write(VA[i].index,nb);
							for ( uint64_t i = 0; i < VB.size(); ++i )
								HEFS.write(VB[i].index,nb);

							// restore sorting
							std::sort(VA.begin(),VA.end());
							std::sort(VB.begin(),VB.end());

							libmaus2::huffman::OutputAdapter OA(HEFS);
							libmaus2::gamma::GammaEncoder<libmaus2::huffman::OutputAdapter> GE(OA);

							// b
							int64_t const bfirstavg = Mbfirst.find(V.size())->second;
							int64_t const bdifavg = Mbdif.find(V.size())->second;
							encodeSignedValue(GE,V[0].bread - bfirstavg);
							for ( uint64_t i = 1; i < V.size(); ++i )
								encodeSignedValue(GE,(V[i].bread-V[i-1].bread) - bdifavg);

							// abpos
							int64_t const abfirstavg = Mabfirst.find(V.size())->second;
							int64_t const abdifavg = Mabdif.find(V.size())->second;
							encodeSignedValue(GE,VA[0].abpos - abfirstavg);
							for ( uint64_t i = 1; i < VA.size(); ++i )
								encodeSignedValue(GE,(VA[i].abpos - VA[i-1].abpos)-abdifavg);

							// bbpos
							int64_t const bbfirstavg = Mbbfirst.find(V.size())->second;
							int64_t const bbdifavg = Mbbdif.find(V.size())->second;
							encodeSignedValue(GE,VB[0].abpos - bbfirstavg);
							for ( uint64_t i = 1; i < VB.size(); ++i )
								encodeSignedValue(GE,(VB[i].abpos - VB[i-1].abpos)-bbdifavg);

							// arange
							std::sort(VA.begin(),VA.end(),ASortRange());
							int64_t const abfirstrangeavg = Mabfirstrange.find(V.size())->second;
							int64_t const abdifrangeavg = Mabdifrange.find(V.size())->second;
							encodeSignedValue(GE,VA[0].getRange() - abfirstrangeavg);
							for ( uint64_t i = 1; i < VA.size(); ++i )
								encodeSignedValue(GE,(VA[i].getRange() - VA[i-1].getRange()) - abdifrangeavg);

							// brange
							std::sort(VB.begin(),VB.end(),ASortRange());
							int64_t const bbfirstrangeavg = Mbbfirstrange.find(V.size())->second;
							int64_t const bbdifrangeavg = Mbbdifrange.find(V.size())->second;
							encodeSignedValue(GE,VB[0].getRange() - bbfirstrangeavg);
							for ( uint64_t i = 1; i < VB.size(); ++i )
								encodeSignedValue(GE,(VB[i].getRange() - VB[i-1].getRange()) - bbdifrangeavg);

							GE.flush();
							HEFS.flushBitStream();

							if ( verbose && ( (aid % (16*1024) == 0) || (!(AF->peekNextOverlap(OVL))) ) )
								std::cerr << "[V] " << aid << std::endl;
						}
					}

					HEFS.flush();

					uint64_t const ipos = HEFS.tellp();
					libmaus2::sorting::SerialisingSortingBufferedOutputFile< PointerEntry,std::less<PointerEntry> >::merger_ptr_type Pmerger(SSBOF->getMerger());
					PointerEntry PE;
					uint64_t next = 0;
					while ( Pmerger->getNext(PE) )
					{
						assert ( next <= PE.index );

						while ( next < PE.index )
						{
							libmaus2::util::NumberSerialisation::serialiseNumber(out,std::numeric_limits<uint64_t>::max());
							++next;
						}
						assert ( next == PE.index );
						libmaus2::util::NumberSerialisation::serialiseNumber(out,PE.pointer);
						++next;
					}
					libmaus2::util::NumberSerialisation::serialiseNumber(out,ipos);
					out.flush();
				}
			};
		}
	}
}
#endif
