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

#if ! defined(HASHCONTAINER2_HPP)
#define HASHCONTAINER2_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/BaseConstants.hpp>
#include <libmaus2/lcs/SuffixPrefixResult.hpp>
#include <libmaus2/lcs/SuffixPrefix.hpp>
#include <libmaus2/lcs/OffsetElement.hpp>
#include <libmaus2/lcs/GenericEditDistance.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/lcs/SuffixPrefixAlignmentPrint.hpp>
#include <libmaus2/util/LongestIncreasingSubsequence.hpp>
#include <iostream>
#include <cmath>
#include <deque>
#include <libmaus2/util/unordered_map.hpp>
#include <libmaus2/util/SimpleHashMap.hpp>
#include <libmaus2/math/ilog.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct HashContainer2
		{
			static ::libmaus2::autoarray::AutoArray<uint8_t> createSymMap()
			{
				::libmaus2::autoarray::AutoArray<uint8_t> S(256);

				S['a'] = S['A'] = ::libmaus2::fastx::mapChar('A');
				S['c'] = S['C'] = ::libmaus2::fastx::mapChar('C');
				S['g'] = S['G'] = ::libmaus2::fastx::mapChar('G');
				S['t'] = S['T'] = ::libmaus2::fastx::mapChar('T');

				return S;
			}

			static ::libmaus2::autoarray::AutoArray<unsigned int> createErrorMap()
			{
				::libmaus2::autoarray::AutoArray<unsigned int> S(256);
				std::fill ( S.get(), S.get()+256, 1 );

				S['a'] = S['A'] = 0;
				S['c'] = S['C'] = 0;
				S['g'] = S['G'] = 0;
				S['t'] = S['T'] = 0;

				return S;
			}

			static ::libmaus2::autoarray::AutoArray<uint8_t> const S;
			static ::libmaus2::autoarray::AutoArray<unsigned int> const E;

			typedef int32_t offset_type;

			// k-gram
			uint32_t const k;
			// k-bits (k-gram*2)
			uint32_t const hashbits;
			// 2^k-bits
			uint32_t const hashlen;
			// (2^k-bits)-1
			uint32_t const hashmask;

			// buffer
			::libmaus2::fastx::SingleWordDNABitBuffer forw;
			// trace of alignment
			std::deque < libmaus2::lcs::BaseConstants::step_type > trace;

			uint64_t const m;
			uint64_t const mp;

			typedef ::libmaus2::util::SimpleHashMap<uint32_t,offset_type> o_type;
			o_type O;

			bool get(uint32_t const v) const
			{
				return O.contains(v);
			}
			void put(uint32_t const v, offset_type const o)
			{
				if ( get(v) )
					O.insert(v,std::numeric_limits<offset_type>::min());
				else
					O.insert(v,o);
			}
			void cleanHash()
			{
				for ( o_type::pair_type * ita = O.begin(); ita != O.end(); ++ita )
					if ( ita->first != O.unused() && ita->second == std::numeric_limits<offset_type>::min() )
						ita->first = O.unused();
			}
			offset_type offset(uint32_t const v) const
			{
				assert ( get(v) );
				return O.get(v);
			}
			void print()
			{
				for ( o_type::pair_type const * ita = O.begin(); ita != O.end(); ++ita )
					if ( ita->first != O.unused() )
					{
						forw.buffer = ita->first;

						std::cerr << "k-mer " << decodeBuffer(forw);
						std::cerr << " pos=" << ita->second;
						std::cerr << std::endl;

					}
			}

			//
			HashContainer2(unsigned int const rk, unsigned int const rm)
			: k(rk), hashbits(k<<1), hashlen(1u<<hashbits), hashmask(hashlen-1), forw(k),
			  trace(),
			  m(rm), mp(m-k+1),
			  O(::libmaus2::math::ilog(2*m))
			{}


			template<typename buffer_type>
			static std::string decodeBuffer(buffer_type const & reve)
			{
				std::string rstring = reve.toString();
				for ( unsigned int i = 0; i < rstring.size(); ++i )
				{
					switch ( rstring[i] )
					{
						case '0': rstring[i] = 'A'; break;
						case '1': rstring[i] = 'C'; break;
						case '2': rstring[i] = 'G'; break;
						case '3': rstring[i] = 'T'; break;
					}
				}
				return rstring;
			}

			template<typename iterator>
			inline void process(iterator pattern)
			{
				forw.reset();
				// reset hash
				O.clear();

				iterator sequence = pattern;
				iterator fsequence = sequence;
				typedef typename std::iterator_traits<iterator>::value_type value_type;

				unsigned int forwe = 0;
				for ( unsigned int i = 0; i < k; ++i )
				{
					value_type const base = *(sequence++);
					forwe += E [ base ];
					forw.pushBackUnmasked( S [ base ]  );
				}

				unsigned int e = forwe;

				for ( unsigned int z = 0; z < mp; )
				{
					if ( e < 1 )
					{
						#if 0
						std::cerr << "Putting " << decodeBuffer(forw) << " value " << forw.buffer << " offset " << z << std::endl;
						#endif
						put(forw.buffer,z);
					}

					if ( ++z < mp )
					{
						e -= E[*(fsequence++)];

						value_type const base = *(sequence++);
						forw.pushBackMasked( S[base] );

						e += E[base];
					}
				}

				cleanHash();
			}

			template<typename iterator>
			inline ::libmaus2::lcs::SuffixPrefixResult match(
				iterator pattern,
				offset_type const n,
				iterator ref,
				double const indelfrac = 0.05
				)
			{
				trace.resize(0);
				::libmaus2::lcs::SuffixPrefixResult SPR;

				#define HASHSPDEBUG

				#if defined(HASHSPDEBUG)
				print();
				#endif

				// #define HASHSPDEBUG

				/**
				 * process if pattern is at least of length k, skip otherwise
				 **/
				if ( n >= static_cast<offset_type>(k) )
				{
					forw.reset();
					iterator sequence = pattern;
					iterator fsequence = sequence;
					typedef typename std::iterator_traits<iterator>::value_type value_type;

					/*
					 * extract mask of first k-mer
					 */
					unsigned int forwe = 0;
					for ( unsigned int i = 0; i < k; ++i )
					{
						value_type const base = *(sequence++);
						forwe += E [ base ];
						forw.pushBackUnmasked( S [ base ]  );
					}

					unsigned int e = forwe;
					offset_type const np = n-k+1;

					/**
					 * search for matching kmers,
					 * store match positions
					 * as pairs (position on b, position on a)
					 **/
					std::vector < ::libmaus2::lcs::OffsetElement > offsets;
					for ( offset_type z = 0; z < np; )
					{
						// if only determinate bases
						if ( e < 1 )
						{
							#define HASHSPDEBUG

							#if defined(HASHSPDEBUG)
							// std::cerr << "Trying to match " << decodeBuffer(forw) << std::endl;
							#endif

							// is current k-mer in index?
							if ( get(forw.buffer) )
							{

								// iterate over occurences of k-mer in index
								// push new pair
								offsets.push_back(::libmaus2::lcs::OffsetElement(z,offset(forw.buffer)));

								#if defined(HASHSPDEBUG)
								std::cerr << "Matching " << decodeBuffer(forw)
									<< " position on A=" << offset(forw.buffer)
									<< " position on B=" << z << std::endl;
								#endif
							}
							// put(forw.buffer,z);
						}

						// update k-mer
						if ( ++z < np )
						{
							e -= E[*(fsequence++)];

							value_type const base = *(sequence++);
							forw.pushBackMasked( S[base] );

							e += E[base];
						}
					}

					std::vector < uint64_t > LIS = ::libmaus2::util::LongestIncreasingSubsequence<
						std::vector < ::libmaus2::lcs::OffsetElement >::const_iterator,
						::libmaus2::lcs::OffsetElementOffsetComparator
					>::longestIncreasingSubsequence(
						offsets.begin(),offsets.size());

					#if 1
					std::cerr << "LIS: ";
					for ( uint64_t i = 0; i < LIS.size(); ++i )
						std::cerr
							<< "(" << offsets[LIS[i]].querypos << "," << offsets[LIS[i]].offset << ")";
					std::cerr << std::endl;
					#endif

					std::vector < ::libmaus2::lcs::OffsetElement > noffsets;
					for ( uint64_t i = 0; i < LIS.size(); ++i )
						noffsets.push_back(offsets[LIS[i]]);
					offsets = noffsets;

					if ( offsets.size() )
					{
						std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > > tracebi(trace);
						uint64_t aclip = 0;

						uint64_t const nmmax = std::max(static_cast<uint64_t>(n),static_cast<uint64_t>(m));
						int64_t  const indellen = static_cast<int64_t>(std::ceil(nmmax * indelfrac));

						/* is there an unmatched part in front? */
						if ( offsets[0] . querypos )
						{
							// unmatched symbols in query (b)
							int64_t missing = offsets[0].querypos;

							// std::cerr << "Something missing." << std::endl;

							int64_t const bstart = 0;
							int64_t const bend = missing;
							int64_t alignposb = bstart;
							int64_t alignlenb = bend-bstart;

							int64_t const aend = offsets[0].offset;
							int64_t const astart = std::max( aend - static_cast<int64_t>( missing + indellen ), static_cast<int64_t>(0) );
							int64_t alignlena = aend-astart;
							int64_t alignposa = astart;

							#if 0
							std::cerr << "Align " <<
								std::string(ref+astart,ref+aend) << " to " <<
								std::string(pattern+bstart,pattern+bend)
								<< std::endl;
							#endif

							if ( ::libmaus2::lcs::BandedEditDistance::validParameters(alignlena,alignlenb,indellen) )
							{
								#if defined(HASHSPDEBUG)
								std::cerr << "Using banded." << std::endl;
								#endif

								::libmaus2::lcs::BandedEditDistance lcsobj(alignlena,alignlenb,indellen);
								::libmaus2::lcs::EditDistanceResult R =
									lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,0,0);

								uint64_t const frontinserts = lcsobj.getFrontInserts();

								#if 1
								alignposa += frontinserts;
								alignlena -= frontinserts;
								lcsobj.ta += frontinserts;
								R.numins -= frontinserts;
								#endif

								#if defined(HASHSPDEBUG)
								lcsobj.printAlignment(std::cerr,ref+alignposa,pattern);
								#endif
								lcsobj.appendTrace(tracebi);
								aclip = alignposa;
							}
							else
							{
								#if defined(HASHSPDEBUG)
								std::cerr << "Using generic." << std::endl;
								#endif

								::libmaus2::lcs::GenericEditDistance lcsobj(alignlena,alignlenb,indellen);
								::libmaus2::lcs::EditDistanceResult R =
									lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,0,0);

								uint64_t const frontinserts = lcsobj.getFrontInserts();

								#if 1
								alignposa += frontinserts;
								alignlena -= frontinserts;
								lcsobj.ta += frontinserts;
								R.numins -= frontinserts;
								#endif

								#if defined(HASHSPDEBUG)
								lcsobj.printAlignment(std::cerr,ref+alignposa,pattern);
								#endif
								lcsobj.appendTrace(tracebi);
								aclip = alignposa;
							}
						}
						else
						{
							aclip = offsets[0].offset;
						}

						int64_t oleft = 0;
						while ( oleft != static_cast<int64_t>(offsets.size()) )
						{
							int64_t const odif =
								static_cast<int64_t>(offsets[oleft].offset) -
								static_cast<int64_t>(offsets[oleft].querypos);
							int64_t oright = oleft;

							while ( oright != static_cast<int64_t>(offsets.size()) &&
								(
									static_cast<int64_t>(offsets[oright].offset) -
									static_cast<int64_t>(offsets[oright].querypos)
									== odif
								) &&
								(offsets[oright].querypos-offsets[oleft].querypos == oright-oleft)
							)
								oright++;

							#if 0
							std::cerr << "oleft=" << offsets[oleft] << ","
								<< "oright=" << offsets[oright-1] << std::endl;
							#endif

							for ( int64_t i = 0; i < k+(oright-oleft-1); ++i )
								*(tracebi++) = ::libmaus2::lcs::BaseConstants::STEP_MATCH;

							if ( oright != static_cast<int64_t>(offsets.size()) )
							{
								#if 0
								std::cerr << " oright=" << offsets[oright] << std::endl;
								#endif

								uint64_t astart = offsets[oright-1].offset + k;
								uint64_t aend = offsets[oright].offset;
								uint64_t bstart = offsets[oright-1].querypos + k;
								uint64_t bend = offsets[oright].querypos;
								uint64_t pcnt = 0;

								// m length of a

								while (
									(astart > aend || bstart > bend)
									&&
									trace.size()
									&&
									(trace.back() == ::libmaus2::lcs::BaseConstants::STEP_MATCH
									||
									trace.back() == ::libmaus2::lcs::BaseConstants::STEP_MISMATCH)
								)
								{
									astart--;
									bstart--;
									trace.pop_back();
									pcnt++;
								}

								assert ( astart <= aend );
								assert ( bstart <= bend );

								while (
									pcnt < k
									&&
									trace.size()
									&&
									trace.back() == ::libmaus2::lcs::BaseConstants::STEP_MATCH
								)
								{
									trace.pop_back();
									assert ( astart );
									astart--;
									assert ( bstart );
									bstart--;
									pcnt++;
								}

								tracebi = std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > >(trace);

								uint64_t const alignposa = astart;
								uint64_t const alignlena = aend-astart;
								uint64_t const alignposb = bstart;
								uint64_t const alignlenb = bend-bstart;

								#if 0
								std::cerr << "astart=" << astart << " aend=" << aend << " alen=" << alignlena << std::endl;
								std::cerr << "bstart=" << bstart << " bend=" << bend << " blen=" << alignlenb << std::endl;
								#endif

								#if 0
								std::cerr << "Align " <<
									std::string(ref+astart,ref+aend) << " to " <<
									std::string(pattern+bstart,pattern+bend)
									<< std::endl;
								#endif

								// uint64_t const indellen = std::max(alignlena,alignlenb)-std::min(alignlena,alignlenb);

								if ( ::libmaus2::lcs::BandedEditDistance::validParameters(alignlena,alignlenb,indellen) )
								{
									#if defined(HASHSPDEBUG)
									std::cerr << "Using banded." << std::endl;
									#endif

									::libmaus2::lcs::BandedEditDistance lcsobj(alignlena,alignlenb,indellen);
									/* ::libmaus2::lcs::EditDistanceResult R = */
										lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,1,1);

									#if defined(HASHSPDEBUG)
									lcsobj.printAlignment(std::cerr,ref+alignposa,pattern+alignposb);
									#endif
									lcsobj.appendTrace(tracebi);
								}
								else
								{
									#if defined(HASHSPDEBUG)
									std::cerr << "Using generic." << std::endl;
									#endif

									#if defined(HASHSPDEBUG)
									std::cerr << "alignlena=" << alignlena << " alignlenb=" << alignlenb << std::endl;
									#endif

									::libmaus2::lcs::GenericEditDistance lcsobj(alignlena,alignlenb,indellen);
									/* ::libmaus2::lcs::EditDistanceResult R = */
										lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,1,1);

									#if defined(HASHSPDEBUG)
									lcsobj.printAlignment(std::cerr,ref+alignposa,pattern+alignposb);
									#endif
									lcsobj.appendTrace(tracebi);
								}
							}

							oleft = oright;
						}

						if (
							static_cast<int64_t>(offsets.back().offset + k) != static_cast<int64_t>(m)
						)
						{
							uint64_t alignposa = offsets.back().offset + k;
							uint64_t alignlena = m - alignposa;
							uint64_t alignposb = offsets.back().querypos + k;
							uint64_t alignlenb = std::min(alignlena + indellen, /* rest of b */ n-alignposb);

							#if 0
							/* make length of alignment k */
							while (
								alignlena < k &&
								alignposa &&
								alignposb &&
								trace.size()
							)
							{
								alignposb--;
								alignposa--;
								alignlena++;
								trace.pop_back();
							}
							#endif

							#if defined(HASHSPDEBUG)
							std::cerr << std::string(ref+alignposa,ref+alignposa+alignlena) << std::endl;
							std::cerr << std::string(pattern+alignposb,pattern+alignposb+alignlenb) << std::endl;
							#endif

							// typedef typename ::std::iterator_traits<iterator>::value_type char_type;
							// typedef typename ::std::iterator_traits<iterator>::reference char_reference_type;
							// typedef typename ::std::iterator_traits<iterator>::pointer char_pointer_type;
							// typedef typename ::std::iterator_traits<iterator>::difference_type char_difference_type;
							typedef ::std::reverse_iterator<iterator> reverse_iterator;

							iterator refstart = ref+alignposa;
							iterator refend   = refstart+alignlena;
							iterator patstart = pattern+alignposb;
							iterator patend   = patstart+alignlenb;

							reverse_iterator revrefstart(refend), revrefend(refstart);
							reverse_iterator revpatstart(patend), revpatend(patstart);

							#if defined(HASHSPDEBUG)
							std::cerr << "ra " << std::string(revrefstart,revrefend) << std::endl;
							std::cerr << "rb " << std::string(revpatstart,revpatend) << std::endl;
							#endif

							/* rev */
							if ( ::libmaus2::lcs::BandedEditDistance::validParameters(alignlenb,alignlena,indellen) )
							{
								#if defined(HASHSPDEBUG)
								std::cerr << "Using banded." << std::endl;
								#endif

								::libmaus2::lcs::BandedEditDistance lcsobj(alignlenb,alignlena,indellen);
								/* ::libmaus2::lcs::EditDistanceResult R = */
									lcsobj.process(revpatstart,revrefstart,alignlenb,alignlena,0,0);

								lcsobj.removeFrontInserts();
								lcsobj.invert();

								#if defined(HASHSPDEBUG)
								lcsobj.printAlignment(std::cerr,refstart,patstart);
								#endif
								lcsobj.appendTrace(tracebi);
							}
							else
							{
								#if defined(HASHSPDEBUG)
								std::cerr << "Using generic." << std::endl;
								#endif

								::libmaus2::lcs::GenericEditDistance lcsobj(alignlenb,alignlena,indellen);
								/* ::libmaus2::lcs::EditDistanceResult R = */
									lcsobj.process(revpatstart,revrefstart,alignlenb,alignlena,0,0);

								lcsobj.removeFrontInserts();
								lcsobj.invert();

								#if defined(HASHSPDEBUG)
								lcsobj.printAlignment(std::cerr,refstart,patstart);
								#endif
								lcsobj.appendTrace(tracebi);
							}
						}

						while ( trace.size() && trace.back() == ::libmaus2::lcs::BaseConstants::STEP_INS )
							trace.pop_back();

						bool traceupdated = true;

						while ( traceupdated )
						{
							traceupdated = false;

							uint64_t idcnt = 0;
							uint64_t icnt = 0;
							uint64_t dcnt = 0;

							for ( uint64_t i = 0; i < trace.size(); ++i )
							{
								if (
									trace[i] == ::libmaus2::lcs::BaseConstants::STEP_INS
									||
									trace[i] == ::libmaus2::lcs::BaseConstants::STEP_DEL
								)
								{
									idcnt++;
									if ( trace[i] == ::libmaus2::lcs::BaseConstants::STEP_INS )
										icnt++;
									else
										dcnt++;
								}
								else
								{
									if ( idcnt > 1 && icnt && dcnt )
									{
										uint64_t const tracestart = i - idcnt;
										uint64_t const traceend = i;

										#if defined(HASHSPDEBUG)
										std::cerr << "idcnt=" << idcnt << ",";
										for ( uint64_t j = tracestart; j < traceend; ++j )
											std::cerr << trace[j];
										std::cerr << std::endl;
										#endif

										uint64_t apos = 0, bpos = 0;
										for ( uint64_t j = 0; j < tracestart; ++j )
											switch ( trace[j] )
											{
												case ::libmaus2::lcs::BaseConstants::STEP_MATCH:
												case ::libmaus2::lcs::BaseConstants::STEP_MISMATCH:
													apos++;
													bpos++;
													break;
												case ::libmaus2::lcs::BaseConstants::STEP_INS:
													apos++;
													break;
												case ::libmaus2::lcs::BaseConstants::STEP_DEL:
													bpos++;
													break;
												case ::libmaus2::lcs::BaseConstants::STEP_RESET:
													break;
											}
										uint64_t alen = 0, blen = 0;
										for ( uint64_t j = tracestart; j < traceend; ++j )
											if ( trace[j] == ::libmaus2::lcs::BaseConstants::STEP_INS )
												alen++;
											else
												blen++;

										uint64_t const alignlena = alen;
										uint64_t const alignlenb = blen;
										uint64_t const alignposa = apos+aclip;
										uint64_t const alignposb = bpos;

										#if defined(HASHSPDEBUG)
										std::cerr << "Realign\n" <<
											std::string(ref+alignposa,ref+alignposa+alignlena) << "\n" <<
											std::string(pattern+alignposb,pattern+alignposb+alignlenb) << "\n";
										#endif

										std::deque < libmaus2::lcs::BaseConstants::step_type > newtrace;
										std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > > newtracebi(newtrace);
										std::copy ( trace.begin(), trace.begin() + tracestart, newtracebi );

										if ( ::libmaus2::lcs::BandedEditDistance::validParameters(alignlena,alignlenb,indellen) )
										{
											#if defined(HASHSPDEBUG)
											std::cerr << "Using banded." << std::endl;
											#endif

											::libmaus2::lcs::BandedEditDistance lcsobj(alignlena,alignlenb,indellen);
											/* ::libmaus2::lcs::EditDistanceResult R = */
												lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,1,1);

											#if defined(HASHSPDEBUG)
											lcsobj.printAlignment(std::cerr,ref+alignposa,pattern+alignposb);
											#endif
											lcsobj.appendTrace(newtracebi);
										}
										else
										{
											#if defined(HASHSPDEBUG)
											std::cerr << "Using generic." << std::endl;
											#endif

											#if defined(HASHSPDEBUG)
											std::cerr << "alignlena=" << alignlena << " alignlenb=" << alignlenb << std::endl;
											#endif

											::libmaus2::lcs::GenericEditDistance lcsobj(alignlena,alignlenb,indellen);
											/* ::libmaus2::lcs::EditDistanceResult R = */
												lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,1,1);

											#if defined(HASHSPDEBUG)
											lcsobj.printAlignment(std::cerr,ref+alignposa,pattern+alignposb);
											#endif
											lcsobj.appendTrace(newtracebi);
										}

										std::copy ( trace.begin()+traceend, trace.end(), newtracebi );
										trace = newtrace;
										traceupdated = true;

										break;
									}

									idcnt = 0;
									icnt = 0;
									dcnt = 0;
								}
							}
						}

						uint64_t numins = 0, numdel = 0, nummat = 0, nummis = 0;
						for ( uint64_t i = 0; i < trace.size(); ++i )
							switch ( trace[i] )
							{
								case ::libmaus2::lcs::BaseConstants::STEP_INS:
									numins++;
									break;
								case ::libmaus2::lcs::BaseConstants::STEP_DEL:
									numdel++;
									break;
								case ::libmaus2::lcs::BaseConstants::STEP_MATCH:
									nummat++;
									break;
								case ::libmaus2::lcs::BaseConstants::STEP_MISMATCH:
									nummis++;
									break;
								case ::libmaus2::lcs::BaseConstants::STEP_RESET:
									break;
							}

						uint64_t const bclip = n+numins-trace.size();

						#if 0
						while ( trace.size() && trace.back() == ::libmaus2::lcs::BaseConstants::STEP_INS )
						{
							trace.pop_back();
							numins--;
							bclip++;
						}
						#endif

						// std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > > tracebi(trace);

						SPR = ::libmaus2::lcs::SuffixPrefixResult(aclip,bclip,numins,numdel,nummat,nummis);
					}
				}

				return SPR;
			}

			template<typename iterator_a, typename iterator_b>
			std::ostream & printAlignmentLines(
				std::ostream & out,
				iterator_a ref,
				iterator_b pattern,
				uint64_t const n,
				::libmaus2::lcs::SuffixPrefixResult const & SPR,
				uint64_t const linelen
				) const
			{
				::libmaus2::lcs::SuffixPrefixAlignmentPrint::printAlignmentLines(
					out,
					std::string(ref,ref+m),
					std::string(pattern,pattern+n),
					SPR,
					linelen,
					trace.begin(),
					trace.end()
				);

				return out;
			}
		};
	}
}
#endif
