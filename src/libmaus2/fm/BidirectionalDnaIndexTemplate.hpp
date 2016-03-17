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
#if ! defined(LIBMAUS2_FM_BIDIRECTIONALDNAINDEXTEMPLATE_HPP)
#define LIBMAUS2_FM_BIDIRECTIONALDNAINDEXTEMPLATE_HPP

#include <iostream>

#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/fm/BidirectionalIndexInterval.hpp>
#include <libmaus2/fm/BidirectionalIndexIntervalSymbol.hpp>
#include <libmaus2/fm/DictionaryInfoImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/fm/SampledSA.hpp>
#include <libmaus2/lf/LF.hpp>
#include <libmaus2/lf/ImpCompactHuffmanWaveletLF.hpp>
#include <libmaus2/util/OutputFileNameTools.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>

namespace libmaus2
{
	namespace fm
	{
		template<typename _lf_type, typename _sa_type>
		struct BidirectionalDnaIndexTemplate
		{
			typedef _lf_type lf_type;
			typedef _sa_type sa_type;
			typedef typename lf_type::unique_ptr_type lf_ptr_type;

			typedef typename lf_type::wt_type rank_dictionary_type;
			typedef typename rank_dictionary_type::unique_ptr_type rank_dictionary_ptr_type;

			typedef ::libmaus2::fm::DictionaryInfo<rank_dictionary_type> rank_dictionary_info_type;

			typedef typename sa_type::unique_ptr_type sa_ptr_type;

			std::string const dictname;
			std::string const basename;
			std::string const saname;
			lf_ptr_type LF;
			sa_ptr_type SA;
			::libmaus2::autoarray::AutoArray<int64_t> const symbols;

			static ::libmaus2::autoarray::AutoArray<int64_t> computeSymbolVector()
			{
				::libmaus2::autoarray::AutoArray<int64_t> S(6);
				for ( uint64_t i = 0; i < S.size(); ++i )
					S[i] = i;
				return S;
			}

			BidirectionalDnaIndexTemplate(std::string const & rdictname)
			: dictname(rdictname),
			  basename(libmaus2::util::OutputFileNameTools::clipOff(dictname,rank_dictionary_info_type::getDictionaryFileSuffix())),
			  saname(basename+rank_dictionary_info_type::getSampledSuffixArraySuffix()),
			  LF(lf_type::load(dictname)),
			  SA(sa_type::load(LF.get(),saname)),
			  // symbols(LF->getSymbols())
			  symbols(computeSymbolVector())
			{
				if ( LF->D.size() < 6 )
					LF->recomputeD(5);
			}

			libmaus2::fm::BidirectionalIndexInterval epsilon() const
			{
				return libmaus2::fm::BidirectionalIndexInterval(0,0,LF->n);
			}

			template<typename iterator>
			libmaus2::fm::BidirectionalIndexInterval biSearchBackward(iterator A, uint64_t const m) const
			{
				libmaus2::fm::BidirectionalIndexInterval BI = epsilon();

				for ( uint64_t i = 0; i < m; ++i )
					BI = backwardExtend(BI,A[m-i-1]);

				return BI;
			}

			template<typename iterator>
			libmaus2::fm::BidirectionalIndexInterval biSearchForward(iterator A, uint64_t const m) const
			{
				libmaus2::fm::BidirectionalIndexInterval BI = epsilon();

				for ( uint64_t i = 0; i < m; ++i )
					BI = forwardExtend(BI,A[i]);

				return BI;
			}

			typedef std::pair<uint64_t,uint64_t> extend_elem_type;

			uint64_t backwardExtendMulti(libmaus2::fm::BidirectionalIndexInterval const & BI, libmaus2::fm::BidirectionalIndexIntervalSymbol * const P) const
			{
				#if defined(_MSC_VER) || defined(__MINGW32__)
				extend_elem_type * E = reinterpret_cast<extend_elem_type *>(_alloca( symbols.size() * sizeof(extend_elem_type) ));
				#else
				extend_elem_type * E = reinterpret_cast<extend_elem_type *>( alloca( symbols.size() * sizeof(extend_elem_type) ));
				#endif

				memset(E,0,symbols.size() * sizeof(extend_elem_type));
				LF->W->stepDivArray(BI.spf, BI.spf+BI.siz, E);

				libmaus2::fm::BidirectionalIndexIntervalSymbol * PP = P;
				for ( unsigned int s = 1; s < symbols.size(); ++s )
					if ( E[s].second )
						switch ( s )
						{
							case 1: // straight A, rc T
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[1] + E[1].first /* bw for sym 1=A, rc(A)=T=4 */,
									BI.spr +
										E[0].second /* zero < T */ +
										E[2].second /* 2=C, rc(C)=G < T */ +
										E[3].second /* 3=G, rc(G)=C < T */ +
										E[4].second /* 4=T, rc(T)=A < T */ ,
									E[1].second /* number of 1=A symbols */));
								break;
							case 2: // straight C, rc G
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[2] + E[2].first /* bw for sym 1=C, rc(C)=G=3 */,
									BI.spr +
										E[0].second /* zero < G */ +
										E[3].second /* 3=G, rc(G)=C < G */ +
										E[4].second /* 4=T, rc(T)=A < G */ ,
									E[2].second /* number of 2=C symbols */));
								break;
							case 3: // straight G, rc C
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[3] + E[3].first /* bw for sym 3=G, rc(G)=C=2 */,
									BI.spr +
										E[0].second /* zero < C */ +
										E[4].second /* 4=T, rc(T)=A < C */ ,
									E[3].second /* number of 3=G symbols */));
								break;
							case 4: // straight T, rc A
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[4] + E[4].first /* bw for sym 4=T, rc(T)=A=1 */,
									BI.spr +
										E[0].second /* zero < A */,
									E[4].second /* number of 4=T symbols */));
								break;
							case 5:
							default:
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[5] + E[5].first /* bw for sym 5=N, rc(N)=N=5 */,
									BI.spr + E[0].second + E[1].second + E[2].second + E[3].second + E[4].second,
									E[5].second));
								break;
						}

				return PP-P;
			}

			uint64_t backwardExtendMultiZero(libmaus2::fm::BidirectionalIndexInterval const & BI, libmaus2::fm::BidirectionalIndexIntervalSymbol * const P) const
			{
				#if defined(_MSC_VER) || defined(__MINGW32__)
				extend_elem_type * E = reinterpret_cast<extend_elem_type *>(_alloca( symbols.size() * sizeof(extend_elem_type) ));
				#else
				extend_elem_type * E = reinterpret_cast<extend_elem_type *>( alloca( symbols.size() * sizeof(extend_elem_type) ));
				#endif

				memset(E,0,symbols.size() * sizeof(extend_elem_type));
				LF->W->stepDivArray(BI.spf, BI.spf+BI.siz, E);

				libmaus2::fm::BidirectionalIndexIntervalSymbol * PP = P;
				for ( unsigned int s = 0; s < symbols.size(); ++s )
					if ( E[s].second )
						switch ( s )
						{
							case 0: // term
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[0] + E[0].first /* bw for sym 0 */,
									BI.spr /* nothing smaller than 0 */,
									E[0].second /* number of 0 symbols */
								));
								break;
							case 1: // straight A, rc T
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[1] + E[1].first /* bw for sym 1=A, rc(A)=T=4 */,
									BI.spr +
										E[0].second /* zero < T */ +
										E[2].second /* 2=C, rc(C)=G < T */ +
										E[3].second /* 3=G, rc(G)=C < T */ +
										E[4].second /* 4=T, rc(T)=A < T */ ,
									E[1].second /* number of 1=A symbols */));
								break;
							case 2: // straight C, rc G
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[2] + E[2].first /* bw for sym 1=C, rc(C)=G=3 */,
									BI.spr +
										E[0].second /* zero < G */ +
										E[3].second /* 3=G, rc(G)=C < G */ +
										E[4].second /* 4=T, rc(T)=A < G */ ,
									E[2].second /* number of 2=C symbols */));
								break;
							case 3: // straight G, rc C
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[3] + E[3].first /* bw for sym 3=G, rc(G)=C=2 */,
									BI.spr +
										E[0].second /* zero < C */ +
										E[4].second /* 4=T, rc(T)=A < C */ ,
									E[3].second /* number of 3=G symbols */));
								break;
							case 4: // straight T, rc A
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[4] + E[4].first /* bw for sym 4=T, rc(T)=A=1 */,
									BI.spr +
										E[0].second /* zero < A */,
									E[4].second /* number of 4=T symbols */));
								break;
							case 5:
							default:
								*(PP++) = libmaus2::fm::BidirectionalIndexIntervalSymbol(s,libmaus2::fm::BidirectionalIndexInterval(
									LF->D[5] + E[5].first /* bw for sym 5=N, rc(N)=N=5 */,
									BI.spr + E[0].second + E[1].second + E[2].second + E[3].second + E[4].second,
									E[5].second));
								break;
						}

				return PP-P;
			}

			libmaus2::fm::BidirectionalIndexInterval backwardExtend(libmaus2::fm::BidirectionalIndexInterval const & BI, int64_t const sym) const
			{
				#if defined(_MSC_VER) || defined(__MINGW32__)
				extend_elem_type * E = reinterpret_cast<extend_elem_type *>(_alloca( symbols.size() * sizeof(extend_elem_type) ));
				#else
				extend_elem_type * E = reinterpret_cast<extend_elem_type *>( alloca( symbols.size() * sizeof(extend_elem_type) ));
				#endif

				memset(E,0,symbols.size() * sizeof(extend_elem_type));

				LF->W->stepDivArray(BI.spf, BI.spf+BI.siz, E);

				switch ( sym )
				{
					case 0: // term
						return libmaus2::fm::BidirectionalIndexInterval(
							LF->D[0] + E[0].first /* bw for sym 0 */,
							BI.spr /* nothing smaller than 0 */,
							E[0].second /* number of 0 symbols */
						);
					case 1: // straight A, rc T
						return libmaus2::fm::BidirectionalIndexInterval(
							LF->D[1] + E[1].first /* bw for sym 1=A, rc(A)=T=4 */,
							BI.spr +
								E[0].second /* zero < T */ +
								E[2].second /* 2=C, rc(C)=G < T */ +
								E[3].second /* 3=G, rc(G)=C < T */ +
								E[4].second /* 4=T, rc(T)=A < T */ ,
							E[1].second /* number of 1=A symbols */);
					case 2: // straight C, rc G
						return libmaus2::fm::BidirectionalIndexInterval(
							LF->D[2] + E[2].first /* bw for sym 1=C, rc(C)=G=3 */,
							BI.spr +
								E[0].second /* zero < G */ +
								E[3].second /* 3=G, rc(G)=C < G */ +
								E[4].second /* 4=T, rc(T)=A < G */ ,
							E[2].second /* number of 2=C symbols */);
					case 3: // straight G, rc C
						return libmaus2::fm::BidirectionalIndexInterval(
							LF->D[3] + E[3].first /* bw for sym 3=G, rc(G)=C=2 */,
							BI.spr +
								E[0].second /* zero < C */ +
								E[4].second /* 4=T, rc(T)=A < C */ ,
							E[3].second /* number of 3=G symbols */);
					case 4: // straight T, rc A
						return libmaus2::fm::BidirectionalIndexInterval(
							LF->D[4] + E[4].first /* bw for sym 4=T, rc(T)=A=1 */,
							BI.spr +
								E[0].second /* zero < A */,
							E[4].second /* number of 4=T symbols */);
					case 5:
					default:
						return libmaus2::fm::BidirectionalIndexInterval(
							LF->D[5] + E[5].first /* bw for sym 5=N, rc(N)=N=5 */,
							BI.spr + E[0].second + E[1].second + E[2].second + E[3].second + E[4].second,
							E[5].second);
				};
			}

			static int64_t rc(int64_t const v)
			{
				switch ( v )
				{
					case 1: return 4;
					case 2: return 3;
					case 3: return 2;
					case 4: return 1;
					default: return v;
				}
			}

			uint64_t forwardExtendMulti(libmaus2::fm::BidirectionalIndexInterval const & BI, libmaus2::fm::BidirectionalIndexIntervalSymbol * const P) const
			{
				uint64_t const num = backwardExtendMulti(BI.swap(),P);
				for ( uint64_t i = 0; i < num; ++i )
					P[i].swapInPlace();
				return num;
			}

			uint64_t forwardExtendMultiZero(libmaus2::fm::BidirectionalIndexInterval const & BI, libmaus2::fm::BidirectionalIndexIntervalSymbol * const P) const
			{
				uint64_t const num = backwardExtendMultiZero(BI.swap(),P);
				for ( uint64_t i = 0; i < num; ++i )
					P[i].swapInPlace();
				return num;
			}

			template<typename iterator>
			uint64_t getRightExtensions(libmaus2::fm::BidirectionalIndexInterval const & BI, iterator P) const
			{
				if ( BI.siz )
				{
					lf_type const & RLF = *LF;
					rank_dictionary_type const & RW = *(RLF.W);

					uint64_t const numex = RW.enumerateSymbolsInRangeUnsorted(BI.spr,BI.spr+BI.siz,P);

					for ( uint64_t i = 0; i < numex; ++i )
						P[i].first = rc(P[i].first);

					std::sort(P,P+numex);

					return numex;
				}
				else
					return 0;
			}

			template<typename iterator>
			uint64_t getLeftExtensions(libmaus2::fm::BidirectionalIndexInterval const & BI, iterator P) const
			{
				if ( BI.siz )
				{
					lf_type const & RLF = *LF;
					rank_dictionary_type const & RW = *(RLF.W);
					return RW.enumerateSymbolsInRangeSorted(BI.spf,BI.spf+BI.siz,P);
				}
				else
					return 0;
			}

			uint64_t countLeftExtensions(libmaus2::fm::BidirectionalIndexInterval const & BI, int64_t const sym) const
			{
				if ( BI.siz )
				{
					return LF->W->count(sym,BI.spf,BI.spf+BI.siz);
				}
				else
				{
					return 0;
				}
			}

			uint64_t countRightExtensions(libmaus2::fm::BidirectionalIndexInterval const & BI, int64_t const sym) const
			{
				if ( BI.siz )
				{
					return LF->W->count(rc(sym),BI.spr,BI.spr+BI.siz);
				}
				else
				{
					return 0;
				}
			}

			// forward extend query interval by one symbol
			libmaus2::fm::BidirectionalIndexInterval forwardExtend(libmaus2::fm::BidirectionalIndexInterval const & BI, int64_t const sym) const
			{
				return backwardExtend(BI.swap(),rc(sym)).swap();
			}

			// get coded text at rankr of length len
			std::string getText(uint64_t r, uint64_t len) const
			{
				std::string s(len,' ');
				std::string::iterator ita = s.begin();

				while ( len-- )
				{
					r = LF->phi(r);
					*(ita++) = (*LF)[r];
				}

				return s;
			}

			// get clear text for rank r of length len
			std::string getTextUnmapped(uint64_t r, uint64_t len) const
			{
				std::string s(len,' ');
				std::string::iterator ita = s.begin();

				while ( len-- )
				{
					r = LF->phi(r);
					*(ita++) = libmaus2::fastx::remapChar((*LF)[r]-1);
				}

				return s;
			}

			// get clear text for rank r until we find a terminator
			std::string getTextUnmapped(uint64_t r) const
			{
				std::vector<char> V;

				while ( true )
				{
					r = LF->phi(r);

					char const u = (*LF)[r];

					if ( u )
						V.push_back(libmaus2::fastx::remapChar(u-1));
					else
						break;
				}

				return std::string(V.begin(),V.end());
			}

			// get start positions of sequences (positions cyclically before terminators
			std::vector<uint64_t> getSeqStartPositions() const
			{
				std::vector<uint64_t> V;
				uint64_t const nz = LF->n ? LF->W->rank(0,LF->n-1) : 0;
				for ( uint64_t i = 0; i < nz; ++i )
					V.push_back((*SA)[(LF->W->select(0,i))]);
				std::sort(V.begin(),V.end());
				return V;
			}

			/*
			 * recursive backward search under hamming distance
			 * only up to half of the mismatches is allowed in the first half
			 * of the pattern, so the result has to be merged with
			 * a query for the reverse complement to find all matches
			 * within a distance of maxdif
			 */
			uint64_t hammingSearchRec(
				std::string const & s, // query string
				libmaus2::fm::BidirectionalIndexInterval const & BI, // current interval
				uint64_t const backoffset, // offset from back of query
				uint64_t const curdif, // current number of mismatches
				uint64_t const maxdif, // maximum number of mismatches
				libmaus2::fm::BidirectionalIndexIntervalSymbol * E, // space for query intervals
				std::vector<libmaus2::fm::BidirectionalIndexInterval> & VBI, // result vector
				uint64_t & totalmatches, // current total number of matches
				uint64_t const maxtotalmatches // maximum number of matches
			) const
			{
				// std::cerr << "backoffset=" << backoffset << std::endl;
				if ( totalmatches > maxtotalmatches )
				{
					return 0;
				}
				else if ( backoffset == s.size() )
				{
					#if 0
					std::string const remapped = remapString(s);
					std::cerr << std::string(30,'*');
					std::cerr << " mismatches=" << curdif << " ";
					std::cerr << std::string(30,'*') << std::endl;
					std::cerr << "original=" << remapped << std::endl;
					for ( uint64_t i = 0; i < BI.siz; ++i )
					{
						std::string const matched = getTextUnmapped(BI.spf+i,s.size());
						std::cerr << "matched =" << matched  << std::endl;
						std::cerr << "         ";
						for ( uint64_t j = 0; j < s.size(); ++j )
							if ( remapped[j] == matched[j] )
								std::cerr.put('+');
							else
								std::cerr.put('-');
						std::cerr << std::endl;
					}
					#endif

					VBI.push_back(BI);

					totalmatches += BI.siz;

					return BI.siz;
				}
				else
				{
					// query symbol
					int64_t const qsym = s[s.size()-backoffset-1];
					// number of symbols we can extend by
					uint64_t const nsym = backwardExtendMulti(BI,E);
					// number of matches
					uint64_t nump = 0;

					for ( uint64_t i = 0; i < nsym; ++i )
					{
						assert ( E[i].siz );

						if ( E[i].sym == qsym && qsym != 5 )
						{
							nump += hammingSearchRec(s,E[i],backoffset+1,curdif,maxdif,E+symbols.size(),VBI,totalmatches,maxtotalmatches);
						}
						// else if ( curdif+1 <= maxdif )
						else if (
							(backoffset <= s.size()/2 && (curdif+1) <= (maxdif+1)/2)
							||
							(backoffset  > s.size()/2 && (curdif+1) <= maxdif      )
						)
						{
							nump += hammingSearchRec(s,E[i],backoffset+1,curdif+1,maxdif,E+symbols.size(),VBI,totalmatches,maxtotalmatches);
						}
					}

					return nump;
				}
			}

			/*
			 * recursive backward search under hamming distance
			 * only up to half of the mismatches is allowed in the first half
			 * of the pattern, so the result has to be merged with
			 * a query for the reverse complement to find all matches
			 * within a distance of maxdif
			 */
			uint64_t hammingSearchRec(
				std::string const & s, // query string
				libmaus2::fm::BidirectionalIndexInterval const & BI, // current interval
				uint64_t const backoffset, // offset from back of query
				uint64_t const curdif, // current number of mismatches
				uint64_t const maxdif, // maximum number of mismatches
				libmaus2::fm::BidirectionalIndexIntervalSymbol * E, // space for query intervals
				std::vector< std::pair<uint64_t, libmaus2::fm::BidirectionalIndexInterval > > & VBI, // result vector
				uint64_t & totalmatches, // current total number of matches
				uint64_t const maxtotalmatches // maximum number of matches
			) const
			{
				// std::cerr << "backoffset=" << backoffset << std::endl;
				if ( totalmatches > maxtotalmatches )
				{
					return 0;
				}
				else if ( backoffset == s.size() )
				{
					#if 0
					std::string const remapped = remapString(s);
					std::cerr << std::string(30,'*');
					std::cerr << " mismatches=" << curdif << " ";
					std::cerr << std::string(30,'*') << std::endl;
					std::cerr << "original=" << remapped << std::endl;
					for ( uint64_t i = 0; i < BI.siz; ++i )
					{
						std::string const matched = getTextUnmapped(BI.spf+i,s.size());
						std::cerr << "matched =" << matched  << std::endl;
						std::cerr << "         ";
						for ( uint64_t j = 0; j < s.size(); ++j )
							if ( remapped[j] == matched[j] )
								std::cerr.put('+');
							else
								std::cerr.put('-');
						std::cerr << std::endl;
					}
					#endif

					VBI.push_back(std::pair<uint64_t, libmaus2::fm::BidirectionalIndexInterval>(curdif,BI));

					totalmatches += BI.siz;

					return BI.siz;
				}
				else
				{
					// query symbol
					int64_t const qsym = s[s.size()-backoffset-1];
					// number of symbols we can extend by
					uint64_t const nsym = backwardExtendMulti(BI,E);
					// number of matches
					uint64_t nump = 0;

					for ( uint64_t i = 0; i < nsym; ++i )
					{
						assert ( E[i].siz );

						if ( E[i].sym == qsym && qsym != 5 )
						{
							nump += hammingSearchRec(s,E[i],backoffset+1,curdif,maxdif,E+symbols.size(),VBI,totalmatches,maxtotalmatches);
						}
						// else if ( curdif+1 <= maxdif )
						else if (
							(backoffset <= s.size()/2 && (curdif+1) <= (maxdif+1)/2)
							||
							(backoffset  > s.size()/2 && (curdif+1) <= maxdif      )
						)
						{
							nump += hammingSearchRec(s,E[i],backoffset+1,curdif+1,maxdif,E+symbols.size(),VBI,totalmatches,maxtotalmatches);
						}
					}

					return nump;
				}
			}

			static void mapStringInPlace(std::string & s)
			{
				for ( uint64_t i = 0; i < s.size(); ++i )
					s[i] = libmaus2::fastx::mapChar(s[i])+1;
			}

			static void remapStringInPlace(std::string & s)
			{
				for ( uint64_t i = 0; i < s.size(); ++i )
					s[i] = libmaus2::fastx::remapChar(s[i]-1);
			}

			static std::string mapString(std::string const & s)
			{
				std::string t = s;
				mapStringInPlace(t);
				return t;
			}
			static std::string remapString(std::string const & s)
			{
				std::string t = s;
				remapStringInPlace(t);
				return t;
			}


			/*
			 * search a pattern with up to maxdif mismatches
			 */
			uint64_t hammingSearchRecUnmapped(std::string query, uint64_t const maxdif, uint64_t const maxtotalmatches, std::vector<libmaus2::fm::BidirectionalIndexInterval> & VBI)
			{
				// compute reverse complement
				std::string rquery = libmaus2::fastx::reverseComplementUnmapped(query);

				// map clear text to codes used in index
				mapStringInPlace(query);
				mapStringInPlace(rquery);

				// stack
				libmaus2::autoarray::AutoArray<libmaus2::fm::BidirectionalIndexIntervalSymbol> E(query.size()*symbols.size(),false);

				// total matches
				uint64_t totalmatches = 0;

				// search reverse complement
				hammingSearchRec(rquery,epsilon(),0,0,maxdif,E.begin(),VBI,totalmatches,maxtotalmatches);
				// turn into intervals for forward
				for ( uint64_t i = 0; i < VBI.size(); ++i )
					VBI[i].swapInPlace();

				// reset number of matches
				totalmatches = 0;
				// search straight/non rc pattern
				hammingSearchRec(query ,epsilon(),0,0,maxdif,E.begin(),VBI,totalmatches,maxtotalmatches);

				// sort intervals
				std::sort(VBI.begin(),VBI.end());

				// drop duplicates
				VBI.resize(std::unique(VBI.begin(),VBI.end())-VBI.begin());

				// compute number of matches
				uint64_t nump = 0;
				for ( uint64_t i = 0; i < VBI.size(); ++i )
					nump += VBI[i].siz;

				return nump;
			}

			/*
			 * search a pattern with up to maxdif mismatches
			 */
			uint64_t hammingSearchRecUnmapped(std::string query, uint64_t const maxdif, uint64_t const maxtotalmatches)
			{
				// vector of result intervals
				std::vector<libmaus2::fm::BidirectionalIndexInterval> VBI;
				//
				return hammingSearchRecUnmapped(query,maxdif,maxtotalmatches,VBI);
			}

			/*
			 * search a pattern with up to maxdif mismatches
			 */
			uint64_t hammingSearchRecUnmapped(std::string query, uint64_t const maxdif, uint64_t const maxtotalmatches, std::vector< std::pair<uint64_t, libmaus2::fm::BidirectionalIndexInterval> > & VBI)
			{
				// compute reverse complement
				std::string rquery = libmaus2::fastx::reverseComplementUnmapped(query);

				// map clear text to codes used in index
				mapStringInPlace(query);
				mapStringInPlace(rquery);

				// stack
				libmaus2::autoarray::AutoArray<libmaus2::fm::BidirectionalIndexIntervalSymbol> E(query.size()*symbols.size(),false);

				// total matches
				uint64_t totalmatches = 0;

				// search reverse complement
				hammingSearchRec(rquery,epsilon(),0,0,maxdif,E.begin(),VBI,totalmatches,maxtotalmatches);
				// turn into intervals for forward
				for ( uint64_t i = 0; i < VBI.size(); ++i )
					VBI[i].second.swapInPlace();

				// reset number of matches
				totalmatches = 0;
				// search straight/non rc pattern
				hammingSearchRec(query ,epsilon(),0,0,maxdif,E.begin(),VBI,totalmatches,maxtotalmatches);

				// sort intervals
				std::sort(VBI.begin(),VBI.end());

				// drop duplicates
				VBI.resize(std::unique(VBI.begin(),VBI.end())-VBI.begin());

				// compute number of matches
				uint64_t nump = 0;
				for ( uint64_t i = 0; i < VBI.size(); ++i )
					nump += VBI[i].second.siz;

				return nump;
			}

			template<typename iterator>
			void extractText(iterator const it, uint64_t const low, uint64_t const high, uint64_t r) const
			{
				iterator ita = it + low;
				iterator ite = it + high;

				while ( ite != ita )
				{
					std::pair<int64_t,uint64_t> const P = LF->extendedLF(r);
					*(--ite) = P.first;
					r = P.second;
				}
			}

			template<typename iterator>
			void extractText(iterator const it)
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				std::vector<upair> V;
				uint64_t const n = LF->n;
				uint64_t const nz = n ? LF->W->rank(0,n-1) : 0;
				for ( uint64_t i = 0; i < nz; ++i )
				{
					uint64_t const r = (LF->W->select(0,i));
					uint64_t const p = (*SA)[r];
					V.push_back(upair(p,r));
				}
				std::sort(V.begin(),V.end());

				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					uint64_t const i0 = i;
					uint64_t const i1 = (i+1) % V.size();
					extractText(it, V[i0].first, V[i1].first ? V[i1].first : n, V[i1].second);
				}
			}
			template<typename iterator>
			void extractTextParallel(iterator const it, uint64_t const numthreads)
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				std::vector<upair> V;
				uint64_t const n = LF->n;
				uint64_t const nz = n ? LF->W->rank(0,n-1) : 0;
				for ( uint64_t i = 0; i < nz; ++i )
				{
					uint64_t const r = (LF->W->select(0,i));
					uint64_t const p = (*SA)[r];
					V.push_back(upair(p,r));
				}
				std::sort(V.begin(),V.end());

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					uint64_t const i0 = i;
					uint64_t const i1 = (i+1) % V.size();
					extractText(it, V[i0].first, V[i1].first ? V[i1].first : n, V[i1].second);
				}
			}
		};

		typedef BidirectionalDnaIndexTemplate<libmaus2::lf::ImpCompactHuffmanWaveletLF,libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactHuffmanWaveletLF> > BidirectionalDnaIndexImpCompactHuffmanWaveletTree;
		typedef BidirectionalDnaIndexTemplate<libmaus2::lf::ImpCompactRLHuffmanWaveletLF,libmaus2::fm::SimpleSampledSA<libmaus2::lf::ImpCompactRLHuffmanWaveletLF> > BidirectionalDnaIndexImpCompactRLHuffmanWaveletTree;
	}
}
#endif
