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

#if ! defined(HASHCONTAINER_HPP)
#define HASHCONTAINER_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/BaseConstants.hpp>
#include <libmaus2/lcs/SuffixPrefixResult.hpp>
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

namespace libmaus2
{
	namespace lcs
	{
		struct HashContainer
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

			#if 0
			typedef uint16_t table_entry_type;
			typedef int16_t offset_type;
			#else
			typedef uint32_t table_entry_type;
			typedef int32_t offset_type;			
			#endif

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

			#if defined(HASHCONTAINER_PARTIALFUNCTION)
			// table hash value to control list position
			::libmaus2::autoarray::AutoArray<table_entry_type> T;
			// pairs q-value, listlen
			typedef std::pair<uint32_t,uint32_t> control_element;
			//
			::libmaus2::autoarray::AutoArray<control_element> C;
			//
			uint32_t clen;
			// lists (offsets)
			::libmaus2::autoarray::AutoArray<offset_type> O;
			
			void put(uint32_t const v, offset_type const o)
			{
				//index in control list
				uint32_t p = T[v];
				
				// value present?
				if ( p >= clen || C[p].first != v )
				{
					// insert new
					p = clen++;
					// set q value
					C[p].first = v;
					C[p].second = 0;
					// set table value
					T[v] = p;
				}
				
				assert ( p*mp + C[p].second < O.size() );
				
				// insert offset
				O [ p*mp + (C[p].second++) ] = o;
			}
			
			inline bool get(uint32_t const v) const
			{
				table_entry_type const p = T[v];
				
				if ( p >= clen || C[p].first != v )
					return false;
				
				return true;
			}
			
			uint32_t count(uint32_t const v) const
			{
				assert ( T[v] < clen && C[T[v]].first == v );
				return C[T[v]].second;
			}
			
			offset_type offset(uint32_t const v, unsigned int const j) const
			{
				assert ( T[v] < clen && C[T[v]].first == v );
				return O[T[v]*mp+j];
			}

			void print() const
			{
				for ( uint64_t i = 0; i < clen; ++i )
				{
					forw.buffer = C[i].first;
					
					std::cerr << "k-mer " << decodeBuffer(forw) << " #occ " << count(forw.buffer);
					std::cerr << " pos={";
					
					for ( uint64_t j = 0; j < count(forw.buffer); ++j )
						std::cerr << offset(forw.buffer,j) << ((j+1<count(forw.buffer))?",":"");

					std::cerr << "}";
					std::cerr << std::endl;
				}
			}
			#else
			typedef ::libmaus2::util::unordered_map < uint32_t, std::vector < offset_type > >::type o_type;
			o_type O;
			
			void put(uint32_t const v, offset_type const o) { O[v].push_back(o); }
			bool get(uint32_t const v) const { return O.find(v) != O.end() ; }
			uint32_t count(uint32_t const v) const
			{
				assert ( get(v) );
				return O.find(v)->second.size();
			}
			offset_type offset(uint32_t const v, unsigned int const j) const
			{
				assert ( get(v) );
				assert ( j < count(v) );
				return (O.find(v)->second)[j];
			}
			void print()
			{
				for ( o_type::const_iterator ita = O.begin();
					ita != O.end(); ++ita )
				{
					forw.buffer = ita->first;
					
					std::cerr << "k-mer " << decodeBuffer(forw) << " #occ " << count(forw.buffer);
					std::cerr << " pos={";
					
					for ( uint64_t j = 0; j < count(forw.buffer); ++j )
						std::cerr << offset(forw.buffer,j) << ((j+1<count(forw.buffer))?",":"");

					std::cerr << "}";
					std::cerr << std::endl;
				
				}
			}
			#endif

			//
			HashContainer(unsigned int const rk, unsigned int const rm)
			: k(rk), hashbits(k<<1), hashlen(1u<<hashbits), hashmask(hashlen-1), forw(k),
			  trace(),
			  m(rm), mp(m-k+1), 
			  #if defined(HASHCONTAINER_PARTIALFUNCTION)
			  T(hashlen,false), C(mp,false), clen(0), O(mp*mp,false)
			  #else
			  O()
			  #endif
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
				#if defined(HASHCONTAINER_PARTIALFUNCTION)
				clen = 0;
				#else
				O.clear();
				#endif
				
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
			}

			template<typename iterator>
			inline ::libmaus2::lcs::SuffixPrefixResult match(
				iterator pattern, 
				offset_type const n,
				iterator ref
				)
			{
				trace.resize(0);
				::libmaus2::lcs::SuffixPrefixResult SPR;

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
					 **/
					std::vector < ::libmaus2::lcs::OffsetElement > offsets;
					for ( offset_type z = 0; z < np; )
					{
						// if only determinate bases
						if ( e < 1 )
						{
							#if defined(HASHSPDEBUG)
							std::cerr << "Trying to match " << decodeBuffer(forw) << std::endl;
							#endif
							
							// is current k-mer in index?
							if ( get(forw.buffer) )
							{
								#if defined(HASHSPDEBUG)
								std::cerr << "Matching " << decodeBuffer(forw) << std::endl;
								#endif

								// iterate over occurences of k-mer in index
								if ( count(forw.buffer) == 1 )
								{
									for ( uint64_t j = 0; j < count(forw.buffer); ++j )
									{
										// push new pair
										offsets.push_back( ::libmaus2::lcs::OffsetElement(z,offset(forw.buffer,j)/*O[p*mp+j]*/ - z) );
										#if defined(HASHSPDEBUG)
										std::cerr << offsets.back() << std::endl;
										#endif
									}
								}
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
					
					std::sort(offsets.begin(),offsets.end());
					
					// #define HASHSPDEBUG
					
					#if defined(HASHSPDEBUG)
					for ( unsigned int i = 0; i < offsets.size(); ++i )
						std::cerr << "(" << offsets[i].querypos << "," << offsets[i].offset << ")";
					std::cerr << std::endl;
					std::cerr << "(position in query string, position in index string - position in query string)" << std::endl;
					#endif

					std::vector < uint64_t > LIS = 
						::libmaus2::util::LongestIncreasingSubsequence< 
							std::vector < ::libmaus2::lcs::OffsetElement >::const_iterator,
							::libmaus2::lcs::OffsetElementQueryposComparator 
						>::longestIncreasingSubsequence
						(
							offsets.begin(),offsets.size()
						);
					
					#if defined(HASHSPDEBUG)
					std::cerr << "LIS: ";
					for ( uint64_t i = 0; i < LIS.size(); ++i )
						std::cerr
							<< "(" << offsets[LIS[i]].querypos << "," << offsets[LIS[i]].offset << ")";
					std::cerr << std::endl;
					#endif

					std::vector < ::libmaus2::lcs::OffsetElement > noffsets;
					for ( uint64_t i = 0; i < LIS.size(); ++i )
						noffsets.push_back ( offsets [ LIS[i] ] );
					offsets = noffsets;

					// #define HASHSPDEBUG

					if ( offsets.size() && offsets.front().offset >= 0 )
					{
						// #define HASHSPDEBUG

						uint64_t aclip = 0;
						
						std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > > tracebi(trace);
					
						// fraction of indels permitted
						double const indelfrac = 0.25;

						if ( offsets[0].querypos > 0 )
						{
							// exclusive end position of alignment on b (query)
							int64_t const posb = offsets[0].querypos;
							// offset between positions on a (reference) and b (query)
							int64_t const off = offsets[0].offset;
							
							// exclusive end position of alignment on a (reference)
							int64_t const posa = posb+off;
							
							// length of a (reference) to be aligned
							int64_t alignlenb = posb;
							int64_t alignposb = 0;
							// 
							int64_t indellen = static_cast<int64_t>(std::ceil(alignlenb * indelfrac));
							int64_t alignlena = alignlenb + indellen;
							int64_t alignposa = posa-alignlena;
							
							if ( alignposa < 0 )
							{
								alignlena += alignposa;
								alignposa -= alignposa;
							}
	
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
								
								::libmaus2::lcs::GenericEditDistance lcsobj(alignlena,alignlenb,6);
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
						
						for ( uint64_t i = 0; i < k; ++i )
							*(tracebi++) = ::libmaus2::lcs::BaseConstants::STEP_MATCH;
						
						for ( uint64_t i = 0; i+1 < offsets.size(); ++i )
						{
							#if defined(HASHSPDEBUG)
							std::cerr << "[i]=" << offsets[i] << " [i+1]=" << offsets[i+1] << std::endl;
							#endif
						
							if ( 
								offsets[i+1].offset == offsets[i].offset
								&&
								offsets[i+1].querypos - offsets[i].querypos <= static_cast<int64_t>(k)
							)
							{
								for ( int64_t j = 0; j < (offsets[i+1].querypos - offsets[i].querypos); ++j )
									*(tracebi++) = ::libmaus2::lcs::BaseConstants::STEP_MATCH;
							}
							else
							{
								int64_t unmatchedpatstart = (offsets[i].querypos + k );
								int64_t unmatchedpatend = offsets[i+1].querypos;
								int64_t unmatchedpatlen = unmatchedpatend-unmatchedpatstart;
								
								int64_t unmatchedrefstart = (offsets[i].querypos+offsets[i].offset+k);
								int64_t unmatchedrefend = (offsets[i+1].querypos+offsets[i+1].offset);
								int64_t unmatchedreflen = unmatchedrefend - unmatchedrefstart;
								
								if ( 
									(unmatchedpatlen < 0 || unmatchedreflen < 0)
									&&
									unmatchedpatlen + k >= 0 
									&& 
									unmatchedreflen + k >= 0
									&&
									trace.size() >= k
								)
								{
									#if defined(HASHSPDEBUG)
									std::cerr << "Trying to handle first strange case, please check: " << std::endl;
									std::cerr << "a=" << std::string(ref,ref+m) << std::endl;
									std::cerr << "unmatchedrefstart=" << unmatchedrefstart << std::endl;
									std::cerr << "unmatchedrefend=" << unmatchedrefend << std::endl;
									std::cerr << "b=" << std::string(pattern,pattern+n) << std::endl;
									std::cerr << "unmatchedpatstart=" << unmatchedpatstart << std::endl;
									std::cerr << "unmatchedpatend=" << unmatchedpatend << std::endl;
									std::cerr << "[i]=" << offsets[i] << " [i+1]=" << offsets[i+1] << std::endl;
									std::cerr << "[i]=" << std::string(pattern+offsets[i].querypos,pattern+offsets[i].querypos+k) << std::endl;
									std::cerr << "[i+1]=" << std::string(pattern+offsets[i+1].querypos,pattern+offsets[i+1].querypos+k) << std::endl;
									#endif
									
									unmatchedpatstart = offsets[i].querypos;
									unmatchedpatlen = unmatchedpatend-unmatchedpatstart;
									
									unmatchedrefstart = offsets[i].querypos+offsets[i].offset;
									unmatchedreflen = unmatchedrefend - unmatchedrefstart;
									
									assert ( unmatchedpatlen >= 0 );
									assert ( unmatchedreflen >= 0 );
									
									for ( uint64_t j = 0; j < k; ++j )
									{
										assert ( trace.back() == ::libmaus2::lcs::BaseConstants::STEP_MATCH );
										trace.pop_back();
									}
									
									tracebi = std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > >(trace);
									
									#if defined(HASHSPDEBUG)
									std::cerr << "unmatchedrefstart=" << unmatchedrefstart << std::endl;
									std::cerr << "unmatchedrefend=" << unmatchedrefend << std::endl;
									std::cerr << std::string(ref+unmatchedrefstart,ref+unmatchedrefend) << std::endl;
									std::cerr << "unmatchedpatstart=" << unmatchedpatstart << std::endl;
									std::cerr << "unmatchedpatend=" << unmatchedpatend << std::endl;
									std::cerr << std::string(pattern+unmatchedpatstart,pattern+unmatchedpatend) << std::endl;
									#endif
								}
								
								if ( unmatchedpatlen >= 0 && unmatchedpatlen >= 0 )
								{
									#if defined(HASHSPDEBUG)
									std::cerr << offsets[i] << "*" << offsets[i+1] << std::endl;
									std::cerr << "Unmatched pattern " 
										<< unmatchedpatlen << " "
										<< std::string(pattern+unmatchedpatstart,pattern+unmatchedpatend)
										<< std::endl;
										
									std::cerr << "Unmatched reference "
										<< unmatchedreflen
										<< " "
										<< std::string ( ref + unmatchedrefstart, ref + unmatchedrefend )
										<< std::endl;
									#endif
									
									uint64_t const alignposb = unmatchedpatstart;
									uint64_t const alignlenb = unmatchedpatlen;
									
									uint64_t const alignposa = unmatchedrefstart;
									uint64_t const alignlena = unmatchedreflen;
									uint64_t const indellen = std::max(alignlena,alignlenb)-std::min(alignlena,alignlenb);

									if ( ::libmaus2::lcs::BandedEditDistance::validParameters(alignlena,alignlenb,indellen) )
									{
										#if defined(HASHSPDEBUG)
										std::cerr << "Using banded." << std::endl;
										#endif
										
										::libmaus2::lcs::BandedEditDistance lcsobj(alignlena,alignlenb,indellen);
										/* ::libmaus2::lcs::EditDistanceResult R = */
											lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,0,0);

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
											lcsobj.process(ref+alignposa,pattern+alignposb,alignlena,alignlenb,0,0);

										#if defined(HASHSPDEBUG)
										lcsobj.printAlignment(std::cerr,ref+alignposa,pattern+alignposb);
										#endif
										lcsobj.appendTrace(tracebi);
									}

									for ( uint64_t j = 0; j < k; ++j )
										*(tracebi++) = ::libmaus2::lcs::BaseConstants::STEP_MATCH;
								}
								else
								{
									std::cerr << "Unhandled strange case, please check input: " << std::endl;
									std::cerr << "a=" << std::string(ref,ref+m) << std::endl;
									std::cerr << "unmatchedrefstart=" << unmatchedrefstart << std::endl;
									std::cerr << "unmatchedrefend=" << unmatchedrefend << std::endl;
									std::cerr << "b=" << std::string(pattern,pattern+n) << std::endl;
									std::cerr << "unmatchedpatstart=" << unmatchedpatstart << std::endl;
									std::cerr << "unmatchedpatend=" << unmatchedpatend << std::endl;
									std::cerr << "[i]=" << offsets[i] << " [i+1]=" << offsets[i+1] << std::endl;
									std::cerr << "[i]=" << std::string(pattern+offsets[i].querypos,pattern+offsets[i].querypos+k) << std::endl;
									std::cerr << "[i+1]=" << std::string(pattern+offsets[i+1].querypos,pattern+offsets[i+1].querypos+k) << std::endl;
									trace.resize(0);
									return ::libmaus2::lcs::SuffixPrefixResult();
								}
							}
						}
						
						
						if ( 
							static_cast<int64_t>(offsets.back().querypos + offsets.back().offset + k) != static_cast<int64_t>(m) 
						)
						{
							#if defined(HASHSPDEBUG)
							std::cerr << "something missing, offsets.back().querypos + k = " << offsets.back().querypos + k << " n=" << n << std::endl;
							#endif
							
							uint64_t alignposb = offsets.back().querypos + k;
							uint64_t alignposa = offsets.back().querypos + offsets.back().offset + k;
							uint64_t alignlena = m - alignposa;
							
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

							tracebi = std::back_insert_iterator < std::deque < libmaus2::lcs::BaseConstants::step_type > >(trace);
							
							int64_t const indellen = static_cast<int64_t>(std::ceil(alignlena * indelfrac));
							uint64_t const alignlenb = std::min(alignlena + indellen, n-alignposb);
							
							#if defined(HASHSPDEBUG)
							// std::cerr << std::string(ref+alignposa,ref+alignposa+alignlena) << std::endl;
							// std::cerr << std::string(pattern+alignposb,pattern+alignposb+alignlenb) << std::endl;
							#endif
							
							// typedef typename ::std::iterator_traits<iterator>::value_type char_type;
							// typedef typename ::std::iterator_traits<iterator>::reference char_reference_type;
							// typedef typename ::std::iterator_traits<iterator>::pointer char_pointer_type;
							// typedef typename ::std::iterator_traits<iterator>::difference_type char_difference_type;
							typedef ::std::reverse_iterator<iterator> reverse_iterator;
							
							iterator refstart = ref+alignposa;
							iterator refend = refstart+alignlena;
							iterator patstart = pattern+alignposb;
							iterator patend = patstart+alignlenb;
							
							reverse_iterator revrefstart(refend), revrefend(refstart);
							reverse_iterator revpatstart(patend), revpatend(patstart);
							
							#if defined(HASHSPDEBUG)
							//std::cerr << "ra " << std::string(revrefstart,revrefend) << std::endl;
							//std::cerr << "rb " << std::string(revpatstart,revpatend) << std::endl;
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
						
						while ( trace.size() && trace.front() == ::libmaus2::lcs::BaseConstants::STEP_INS )
						{
							aclip++;
							trace.pop_front();
						}
						
						uint64_t numins = 0, numdel, nummat, nummis;
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
							}
						
						uint64_t const bclip = n+numins-trace.size();
						
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
