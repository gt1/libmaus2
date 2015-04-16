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
#if ! defined(LIBMAUS2_UTIL_KMP_HPP)
#define LIBMAUS2_UTIL_KMP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/UnsignedCharVariant.hpp>
#include <map>

/*
 * this file contains some Knuth, Morris, Pratt type algorithms as outlined
 * in Crochemore et al.: Algorithms on Strings
 */
namespace libmaus2
{
	namespace util
	{
		struct KMP
		{
			template<typename stream_type>
			struct BestPrefix
			{
				typedef typename stream_type::char_type char_type;
				typedef typename UnsignedCharVariant<char_type>::type unsigned_char_type;
			
				std::vector<int64_t> best_prefix;
				int64_t i;
				uint64_t j;
				std::vector<unsigned_char_type> x;
				stream_type & stream;
				uint64_t const m;

				struct BestPrefixXAdapter
				{
					BestPrefix<stream_type> * owner;
					
					BestPrefixXAdapter(BestPrefix<stream_type> * rowner = 0) : owner(rowner) {}
					
					unsigned_char_type operator[](uint64_t const i)
					{
						if ( i < owner->x.size() )
							return owner->x[i];
							
						// std::cerr << "accessing " << i << std::endl;
						
						(*owner)[i];

						assert ( i < owner->x.size() );
						return owner->x[i];
					}
				};
				
				BestPrefixXAdapter getXAdapter()
				{
					return BestPrefixXAdapter(this);
				}
				
				BestPrefix(stream_type & rstream, uint64_t const rm)
				: best_prefix(), i(0), j(1), stream(rstream), m(rm)
				{
					best_prefix.push_back(-1);
					
					if ( m )
					{
						typename stream_type::int_type const c = stream.get();
						assert ( c != stream_type::traits_type::eof() );
						x.push_back(c);
					}
				}
				
				int64_t operator[](uint64_t const k)
				{
					assert ( k <= m );
					
					if ( k < best_prefix.size() )
						return best_prefix[k];
						
					// std::cerr << "k=" << k << std::endl;
					
					// std::cerr << ::libmaus2::util::StackTrace().toString() << std::endl;
					
					assert ( stream && (! stream.eof()) );
					
					for ( ; j < m && j <= k; ++j, ++i )
					{
						assert ( j == x.size() );
						typename stream_type::int_type const c = stream.get();
						assert ( c != stream_type::traits_type::eof() );
						x.push_back(c);
						
						if ( x[j] == x[i] )
							best_prefix.push_back(best_prefix[i]);
						else
						{
							best_prefix.push_back(i);
							// std::cerr << "set best_prefix[" << (best_prefix.size()-1) << " to " << best_prefix.back() << std::endl;
							do
							{
								i = best_prefix[i];
							} while ( (i >= 0) && (x[j] != x[i]) );
						}
					}
					
					if ( k == m )
					{
						best_prefix.push_back(i);
					}
					
					return best_prefix[k];
				}
			};
		
			// computes the best prefix table for the pattern x of length m
			template<typename const_iterator>
			static inline ::libmaus2::autoarray::AutoArray<int64_t> BEST_PREFIX(const_iterator x, uint64_t const m)
			{
				::libmaus2::autoarray::AutoArray<int64_t> best_prefix(m+1,false);
				best_prefix[0] = -1;
				int64_t i = 0;
				
				for ( uint64_t j = 1; j < m; ++j, ++i )
					if ( x[j] == x[i] )
						best_prefix[j] = best_prefix[i];
					else
					{
						best_prefix[j] = i;
						do
						{
							i = best_prefix[i];
						} while ( (i >= 0) && (x[j] != x[i]) );
					}

				best_prefix[m] = i;
				
				return best_prefix;
			}

			// compute the first position of the longest prefix of x matching in y
			// returns pair (position,matchlength)
			template<typename const_iterator_x, typename const_iterator_y, typename pi_type>
			static inline std::pair<uint64_t, uint64_t> PREFIX_SEARCH_INTERNAL(
				const_iterator_x x, uint64_t const m, pi_type & pi, 
				const_iterator_y y, uint64_t const n, bool first = true)
			{
				typedef typename std::iterator_traits<const_iterator_y>::value_type key_type;
				
				int64_t i = 0;
				
				uint64_t maxj = 0;
				uint64_t maxi = 0;

				// iterate over input	
				for ( uint64_t j = 0; j < n; ++j )
				{
					// read input
					key_type const a = *(y++);
					
					// use failure function if we have a full match
					if ( i == static_cast<int>(m) )
						i = pi[m];
					
					// follow failure function while we have no match for next symbol
					while ( (i >= 0) && (a != x[i]) )
						i = pi[i];

					i += 1;
					
					// i is now the match length
					
					// we have a new maximal match
					if ( i > static_cast<int>(maxi) )
						maxi = i, maxj = j;
					
					// if we want the first match and have a full match, return result
					if ( first && (i == static_cast<int>(m)) )
						return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);
				}

				return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);
			}

			// compute the first position of the longest prefix of x matching in y
			// returns pair (position,matchlength)
			template<typename const_iterator_x, typename const_iterator_y, typename pi_type>
			static inline std::pair<uint64_t, uint64_t> PREFIX_SEARCH_INTERNAL_RESTRICTED(
				const_iterator_x x, uint64_t const m, pi_type & pi, 
				const_iterator_y & y, uint64_t const n, 
				uint64_t const restrict,
				bool first = true
			)
			{
				typedef int64_t key_type;
				
				int64_t i = 0;
				
				uint64_t maxj = 0;
				uint64_t maxi = 0;

				// iterate over input until position of match surely is at least restrict
				// the formula is (j+1)-(i+1), where we have (i+1) as i may be incremented
				// by one before we check (j+1)-i < restirct below
				for ( uint64_t j = 0; j < n && (((j+1)-(i+1)) < restrict); ++j )
				{
					// std::cerr << "j=" << j << " i=" << i << std::endl;
				
					// read input
					key_type const a = y.get();
					
					// use failure function if we have a full match
					if ( i == static_cast<int>(m) )
						i = pi[m];
					
					// follow failure function while we have no match for next symbol
					while ( (i >= 0) && (a != x[i]) )
						i = pi[i];

					i += 1;
					
					// i is now the match length
					
					// we have a new maximal match
					if ( i > static_cast<int>(maxi) && ((j+1)-i) < restrict )
					{
						maxi = i, maxj = j;
						// std::cerr << "set maxi to " << i << std::endl;
					}
					
					// if we want the first match and have a full match, return result
					if ( first && (i == static_cast<int>(m)) )
						return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);
				}

				return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);
			}

			// compute the first position of the longest prefix of x matching in y
			// returns pair (position,matchlength)
			template<typename const_iterator_x, typename const_iterator_y, typename pi_type>
			static inline std::pair<uint64_t, uint64_t> PREFIX_SEARCH_INTERNAL_RESTRICTED_BOUNDED(
				const_iterator_x x, uint64_t const m, pi_type & pi, 
				const_iterator_y & y, uint64_t const n, 
				uint64_t const restrict,
				uint64_t const bound,
				bool first = true
			)
			{
				typedef int64_t key_type;
				
				int64_t i = 0;
				
				uint64_t maxj = 0;
				uint64_t maxi = 0;

				// iterate over input until position of match surely is at least restrict
				// the formula is (j+1)-(i+1), where we have (i+1) as i may be incremented
				// by one before we check (j+1)-i < restirct below
				for ( uint64_t j = 0; j < n && (((j+1)-(i+1)) < restrict); ++j )
				{
					// std::cerr << "j=" << j << " i=" << i << std::endl;
				
					// read input
					key_type const a = y.get();
					
					// use failure function if we have a full match
					if ( i == static_cast<int>(m) )
						i = pi[m];
					
					// follow failure function while we have no match for next symbol
					while ( (i >= 0) && (a != x[i]) )
						i = pi[i];

					i += 1;
					
					// i is now the match length
					
					// we have a new maximal match
					if ( 
						i > static_cast<int>(maxi) && ((j+1)-i) < restrict 
					)
					{
						maxi = i, maxj = j;
						// std::cerr << "set maxi to " << i << std::endl;
						
						if ( i >= static_cast<int64_t>(bound) )
							return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);					
					}
					
					// if we want the first match and have a full match, return result
					if ( first && (i == static_cast<int>(m)) )
						return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);
				}

				return std::pair<uint64_t, uint64_t>((maxj+1)-maxi,maxi);
			}

			// compute the first position of the longest prefix of x matching in y
			// returns pair (position,matchlength)
			template<typename const_iterator_x, typename const_iterator_y>
			static inline std::pair<uint64_t, uint64_t> PREFIX_SEARCH(
				const_iterator_x x, uint64_t const m, 
				const_iterator_y y, uint64_t const n, 
				bool first = true
			)
			{
				::libmaus2::autoarray::AutoArray<int64_t> pi = BEST_PREFIX(x,m);
				return PREFIX_SEARCH_INTERNAL(x,m,pi,y,n,first);
			}
		};
	}
}
#endif
