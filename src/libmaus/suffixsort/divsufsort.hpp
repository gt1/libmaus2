#if ! defined(DIVSUFSORT_HPP)
#define DIVSUFSORT_HPP

/*
 * Copyright (c) 2003-2008 Yuta Mori All Rights Reserved.
 * Copyright (c) 2011-2013 German Tischler
 * Copyright (c) 2011-2013 Genome Research Limited
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <algorithm>
#include <cassert>
// #include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <sstream>

#if defined(_OPENMP)
# include <omp.h>
#endif

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		extern int const lg_table[256];
		extern int const sqq_table[256];

		template<unsigned int bitcount>
		struct DivSufSortTypes
		{
		};


		template<>
		struct DivSufSortTypes<32>
		{
			typedef int32_t saint_t;
			typedef int32_t saidx_t;

			static inline saint_t tr_ilg(saidx_t n) 
			{
				return (n & 0xffff0000) ?
					((n & 0xff000000) ?
					24 + lg_table[(n >> 24) & 0xff] :
					16 + lg_table[(n >> 16) & 0xff]) :
					((n & 0x0000ff00) ?
						8 + lg_table[(n >>  8) & 0xff] :
						0 + lg_table[(n >>  0) & 0xff]);
			}
		};

		template<>
		struct DivSufSortTypes<64>
		{
			typedef int32_t saint_t;
			typedef int64_t saidx_t;

			static inline saint_t tr_ilg(saidx_t n)
			{
				return (n >> 32) ?
					  ((n >> 48) ?
					    ((n >> 56) ?
					      56 + lg_table[(n >> 56) & 0xff] :
					      48 + lg_table[(n >> 48) & 0xff]) :
					    ((n >> 40) ?
					      40 + lg_table[(n >> 40) & 0xff] :
					      32 + lg_table[(n >> 32) & 0xff])) :
					  ((n & 0xffff0000) ?
					    ((n & 0xff000000) ?
					      24 + lg_table[(n >> 24) & 0xff] :
					      16 + lg_table[(n >> 16) & 0xff]) :
					    ((n & 0x0000ff00) ?
					       8 + lg_table[(n >>  8) & 0xff] :
					       0 + lg_table[(n >>  0) & 0xff]));
			}
		};

		template<unsigned int bitcount, typename index_const_iterator, int ss_blocksize>
		struct SssortDivSufSortTypes : DivSufSortTypes<bitcount>
		{
			typedef typename DivSufSortTypes<bitcount>::saidx_t saidx_t;
			typedef typename DivSufSortTypes<bitcount>::saint_t saint_t;
			typedef DivSufSortTypes<bitcount> base_type;

			static inline saint_t ss_ilg(saidx_t n) 
			{
				if ( ss_blocksize == 0 )
					return base_type::tr_ilg(n);
				else if ( ss_blocksize < 256 )
					return lg_table[n];
				else
					return (n & 0xff00) ?
					  8 + lg_table[(n >> 8) & 0xff] :
					  0 + lg_table[(n >> 0) & 0xff];
			}
		};

		template<
			unsigned int bitcount, 
			typename symbol_const_iterator,
			typename index_iterator,
			typename index_const_iterator,
			int SS_BLOCKSIZE = 1024
		>
		struct Sssort : SssortDivSufSortTypes<bitcount,index_const_iterator,SS_BLOCKSIZE>
		{
			typedef SssortDivSufSortTypes<bitcount,index_const_iterator,SS_BLOCKSIZE> base_type;
			typedef typename base_type::saint_t saint_t;
			typedef typename std::iterator_traits<symbol_const_iterator>::value_type sauchar_t;
			typedef typename DivSufSortTypes<bitcount>::saidx_t saidx_t;

			static int const SS_MISORT_STACKSIZE =
				(SS_BLOCKSIZE == 0) ?
					(
					((bitcount > 32)?96:64)
					)
					:
					(
					(SS_BLOCKSIZE <= 4096) ? 16 : 24
					);

			static int const SS_SMERGE_STACKSIZE = bitcount > 32 ? 64 : 32;

			static int const  SS_INSERTIONSORT_THRESHOLD = 8;

			template<typename typea, typename typeb, typename typec, typename typed, unsigned int k>
			struct FiniteStack4
			{
				struct Stack4Element
				{
					typea a;
					typeb b;
					typec c;
					typed d;
				};
				
				Stack4Element stack[k];
				unsigned int ssize;
				
				FiniteStack4()
				: ssize(0)
				{
				
				}
				
				inline void push(
					typea a,
					typeb b,
					typec c,
					typed d
					)
				{
					assert ( ssize < k );
					stack[ssize].a = a;
					stack[ssize].b = b;
					stack[ssize].c = c;
					stack[ssize].d = d;
					ssize += 1;
				}
				
				inline bool pop(
					typea & a,
					typeb & b,
					typec & c,
					typed & d)
				{
					if ( ssize )
					{
						ssize -= 1;
						a = stack[ssize].a;
						b = stack[ssize].b;
						c = stack[ssize].c;
						d = stack[ssize].d;
						return true;
					}
					else
					{
						return false;
					}
				}
			};

			static inline
			saidx_t
			ss_isqrt(saidx_t x) {
			  saidx_t y, e;

			  if(x >= (SS_BLOCKSIZE * SS_BLOCKSIZE)) { return SS_BLOCKSIZE; }
			  e = (x & 0xffff0000) ?
				((x & 0xff000000) ?
				  24 + lg_table[(x >> 24) & 0xff] :
				  16 + lg_table[(x >> 16) & 0xff]) :
				((x & 0x0000ff00) ?
				   8 + lg_table[(x >>  8) & 0xff] :
				   0 + lg_table[(x >>  0) & 0xff]);

			  if(e >= 16) {
			    y = sqq_table[x >> ((e - 6) - (e & 1))] << ((e >> 1) - 7);
			    if(e >= 24) { y = (y + 1 + x / y) >> 1; }
			    y = (y + 1 + x / y) >> 1;
			  } else if(e >= 8) {
			    y = (sqq_table[x >> ((e - 6) - (e & 1))] >> (7 - (e >> 1))) + 1;
			  } else {
			    return sqq_table[x] >> 4;
			  }

			  return (x < (y * y)) ? y - 1 : y;
			}



			/*---------------------------------------------------------------------------*/

			/* Compares two suffixes. */
			template<typename index_iterator_one, typename index_iterator_two>
			static inline
			saint_t
			ss_compare(symbol_const_iterator T,
				   index_iterator_one p1, index_iterator_two p2,
				   saidx_t depth) {
			  symbol_const_iterator U1, U2, U1n, U2n;

			  for(U1 = T + depth + *p1,
			      U2 = T + depth + *p2,
			      U1n = T + *(p1 + 1) + 2,
			      U2n = T + *(p2 + 1) + 2;
			      (U1 < U1n) && (U2 < U2n) && (*U1 == *U2);
			      ++U1, ++U2) {
			  }

			  return U1 < U1n ?
				(U2 < U2n ? *U1 - *U2 : 1) :
				(U2 < U2n ? -1 : 0);
			}


			/*---------------------------------------------------------------------------*/

			/* Insertionsort for small size groups */
			static
			void
			ss_insertionsort(symbol_const_iterator T, index_const_iterator PA,
					 index_iterator first, index_iterator last, saidx_t depth) {
			  index_iterator i, j;
			  saidx_t t;
			  saint_t r;

			  for(i = last - 2; first <= i; --i) {
			    for(t = *i, j = i + 1; 0 < (r = ss_compare(T, PA + t, PA + *j, depth));) {
			      do { *(j - 1) = *j; } while((++j < last) && (*j < 0));
			      if(last <= j) { break; }
			    }
			    if(r == 0) { *j = ~*j; }
			    *(j - 1) = t;
			  }
			}

			/*---------------------------------------------------------------------------*/

			static inline
			void
			ss_fixdown(symbol_const_iterator Td, index_const_iterator PA,
				   index_iterator SA, saidx_t i, saidx_t size) {
			  saidx_t j, k;
			  saidx_t v;
			  saint_t c, d, e;

			  for(v = SA[i], c = Td[PA[v]]; (j = 2 * i + 1) < size; SA[i] = SA[k], i = k) {
			    d = Td[PA[SA[k = j++]]];
			    if(d < (e = Td[PA[SA[j]]])) { k = j; d = e; }
			    if(d <= c) { break; }
			  }
			  SA[i] = v;
			}

			/* Simple top-down heapsort. */
			static
			void
			ss_heapsort(symbol_const_iterator Td, index_const_iterator PA, index_iterator SA, saidx_t size) {
			  saidx_t i, m;
			  saidx_t t;

			  m = size;
			  if((size % 2) == 0) {
			    m--;
			    if(Td[PA[SA[m / 2]]] < Td[PA[SA[m]]]) { indexSwapNonRef(SA+m, SA+(m/2)); }
			  }

			  for(i = m / 2 - 1; 0 <= i; --i) { ss_fixdown(Td, PA, SA, i, m); }
			  if((size % 2) == 0) { indexSwapNonRef(SA, SA+m); ss_fixdown(Td, PA, SA, 0, m); }
			  for(i = m - 1; 0 < i; --i) {
			    t = SA[0], SA[0] = SA[i];
			    ss_fixdown(Td, PA, SA, 0, i);
			    SA[i] = t;
			  }
			}


			/*---------------------------------------------------------------------------*/

			/* Returns the median of three elements. */
			static inline
			index_iterator
			ss_median3(symbol_const_iterator Td, index_const_iterator PA,
				   index_iterator v1, index_iterator v2, index_iterator v3) {
			  if(Td[PA[*v1]] > Td[PA[*v2]]) { ::std::swap(v1, v2); }
			  if(Td[PA[*v2]] > Td[PA[*v3]]) {
			    if(Td[PA[*v1]] > Td[PA[*v3]]) { return v1; }
			    else { return v3; }
			  }
			  return v2;
			}

			/* Returns the median of five elements. */
			static inline
			index_iterator
			ss_median5(symbol_const_iterator Td, index_const_iterator PA,
				   index_iterator v1, index_iterator v2, index_iterator v3, index_iterator v4, index_iterator v5) {
			  if(Td[PA[*v2]] > Td[PA[*v3]]) { ::std::swap(v2, v3); }
			  if(Td[PA[*v4]] > Td[PA[*v5]]) { ::std::swap(v4, v5); }
			  if(Td[PA[*v2]] > Td[PA[*v4]]) { ::std::swap(v2, v4); ::std::swap(v3, v5); }
			  if(Td[PA[*v1]] > Td[PA[*v3]]) { ::std::swap(v1, v3); }
			  if(Td[PA[*v1]] > Td[PA[*v4]]) { ::std::swap(v1, v4); ::std::swap(v3, v5); }
			  if(Td[PA[*v3]] > Td[PA[*v4]]) { return v4; }
			  return v3;
			}

			/* Returns the pivot element. */
			static inline
			index_iterator
			ss_pivot(symbol_const_iterator Td, index_const_iterator PA, index_iterator first, index_iterator last) {
			  index_iterator middle;
			  saidx_t t;

			  t = last - first;
			  middle = first + t / 2;

			  if(t <= 512) {
			    if(t <= 32) {
			      return ss_median3(Td, PA, first, middle, last - 1);
			    } else {
			      t >>= 2;
			      return ss_median5(Td, PA, first, first + t, middle, last - 1 - t, last - 1);
			    }
			  }
			  t >>= 3;
			  first  = ss_median3(Td, PA, first, first + t, first + (t << 1));
			  middle = ss_median3(Td, PA, middle - t, middle, middle + t);
			  last   = ss_median3(Td, PA, last - 1 - (t << 1), last - 1 - t, last - 1);
			  return ss_median3(Td, PA, first, middle, last);
			}


			/*---------------------------------------------------------------------------*/

			/* Binary partition for substrings. */
			static inline
			index_iterator
			ss_partition(index_const_iterator PA,
					    index_iterator first, index_iterator last, saidx_t depth) {
			  index_iterator a, b;
			  saidx_t t;
			  for(a = first - 1, b = last;;) {
			    for(; (++a < b) && ((PA[*a] + depth) >= (PA[*a + 1] + 1));) { *a = ~*a; }
			    for(; (a < --b) && ((PA[*b] + depth) <  (PA[*b + 1] + 1));) { }
			    if(b <= a) { break; }
			    t = ~*b;
			    *b = *a;
			    *a = t;
			  }
			  if(first < a) { *first = ~*first; }
			  return a;
			}
			
			inline static void indexSwap(index_iterator & i0, index_iterator & i1)
			{
				saidx_t const v0 = *i0;
				*i0 = *i1;
				*i1 = v0;
			}
			inline static void indexSwapNonRef(index_iterator i0, index_iterator i1)
			{
				saidx_t const v0 = *i0;
				*i0 = *i1;
				*i1 = v0;
			}

			/* Multikey introsort for medium size groups. */
			static
			void
			ss_mintrosort(symbol_const_iterator T, index_const_iterator PA,
				      index_iterator first, index_iterator last,
				      saidx_t depth) {
			  FiniteStack4<index_iterator, index_iterator, saidx_t, saint_t, SS_MISORT_STACKSIZE> stack;
			  symbol_const_iterator Td;
			  index_iterator a, b, c, d, e, f;
			  saidx_t s, t;
			  saint_t limit;
			  saint_t v, x = 0;
			  
			  for(limit = base_type::ss_ilg(last - first);;) {

			    if((last - first) <= SS_INSERTIONSORT_THRESHOLD) 
			    {
				if ( 1 < SS_INSERTIONSORT_THRESHOLD )
				{
					if(1 < (last - first)) { ss_insertionsort(T, PA, first, last, depth); }
				}
			      if ( ! stack.pop(first,last,depth,limit) )
				return;
			      continue;
			    }

			    Td = T + depth;
			    if(limit-- == 0) { ss_heapsort(Td, PA, first, last - first); }
			    if(limit < 0) {
			      for(a = first + 1, v = Td[PA[*first]]; a < last; ++a) {
				if((x = Td[PA[*a]]) != v) {
				  if(1 < (a - first)) { break; }
				  v = x;
				  first = a;
				}
			      }
			      if(static_cast<saint_t>(Td[PA[*first] - 1]) < v) {
				first = ss_partition(PA, first, a, depth);
			      }
			      if((a - first) <= (last - a)) {
				if(1 < (a - first)) {
				  stack.push(a, last, depth, -1);
				  last = a, depth += 1, limit = base_type::ss_ilg(a - first);
				} else {
				  first = a, limit = -1;
				}
			      } else {
				if(1 < (last - a)) {
				  stack.push(first, a, depth + 1, base_type::ss_ilg(a - first));
				  first = a, limit = -1;
				} else {
				  last = a, depth += 1, limit = base_type::ss_ilg(a - first);
				}
			      }
			      continue;
			    }

			    /* choose pivot */
			    a = ss_pivot(Td, PA, first, last);
			    v = Td[PA[*a]];
			    indexSwap(first, a);

			    /* partition */
			    for(b = first; (++b < last) && ((x = Td[PA[*b]]) == v);) { }
			    if(((a = b) < last) && (x < v)) {
			      for(; (++b < last) && ((x = Td[PA[*b]]) <= v);) {
				if(x == v) { indexSwap(b, a); ++a; }
			      }
			    }
			    for(c = last; (b < --c) && ((x = Td[PA[*c]]) == v);) { }
			    if((b < (d = c)) && (x > v)) {
			      for(; (b < --c) && ((x = Td[PA[*c]]) >= v);) {
				if(x == v) { indexSwap(c, d); --d; }
			      }
			    }
			    for(; b < c;) {
			      indexSwap(b, c);
			      for(; (++b < c) && ((x = Td[PA[*b]]) <= v);) {
				if(x == v) { indexSwap(b, a); ++a; }
			      }
			      for(; (b < --c) && ((x = Td[PA[*c]]) >= v);) {
				if(x == v) { indexSwap(c, d); --d; }
			      }
			    }

			    if(a <= d) {
			      c = b - 1;

			      if((s = a - first) > (t = b - a)) { s = t; }
			      for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { indexSwap(e, f); }
			      if((s = d - c) > (t = last - d - 1)) { s = t; }
			      for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { indexSwap(e, f); }

			      a = first + (b - a), c = last - (d - c);
			      b = (v <= static_cast<saint_t>(Td[PA[*a] - 1])) ? a : ss_partition(PA, a, c, depth);

			      if((a - first) <= (last - c)) {
				if((last - c) <= (c - b)) {
				  stack.push(b, c, depth + 1, base_type::ss_ilg(c - b));
				  stack.push(c, last, depth, limit);
				  last = a;
				} else if((a - first) <= (c - b)) {
				  stack.push(c, last, depth, limit);
				  stack.push(b, c, depth + 1, base_type::ss_ilg(c - b));
				  last = a;
				} else {
				  stack.push(c, last, depth, limit);
				  stack.push(first, a, depth, limit);
				  first = b, last = c, depth += 1, limit = base_type::ss_ilg(c - b);
				}
			      } else {
				if((a - first) <= (c - b)) {
				  stack.push(b, c, depth + 1, base_type::ss_ilg(c - b));
				  stack.push(first, a, depth, limit);
				  first = c;
				} else if((last - c) <= (c - b)) {
				  stack.push(first, a, depth, limit);
				  stack.push(b, c, depth + 1, base_type::ss_ilg(c - b));
				  first = c;
				} else {
				  stack.push(first, a, depth, limit);
				  stack.push(c, last, depth, limit);
				  first = b, last = c, depth += 1, limit = base_type::ss_ilg(c - b);
				}
			      }
			    } else {
			      limit += 1;
			      if(static_cast<saint_t>(Td[PA[*first] - 1]) < v) {
				first = ss_partition(PA, first, last, depth);
				limit = base_type::ss_ilg(last - first);
			      }
			      depth += 1;
			    }
			  }
			}

			/*---------------------------------------------------------------------------*/

			static inline
			void
			ss_blockswap(index_iterator a, index_iterator b, saidx_t n) {
			  saidx_t t;
			  for(; 0 < n; --n, ++a, ++b) {
			    t = *a, *a = *b, *b = t;
			  }
			}

			static inline
			void
			ss_rotate(index_iterator first, index_iterator middle, index_iterator last) {
			  index_iterator a, b;
			  saidx_t t;
			  saidx_t l, r;
			  l = middle - first, r = last - middle;
			  for(; (0 < l) && (0 < r);) {
			    if(l == r) { ss_blockswap(first, middle, l); break; }
			    if(l < r) {
			      a = last - 1, b = middle - 1;
			      t = *a;
			      do {
				*a-- = *b, *b-- = *a;
				if(b < first) {
				  *a = t;
				  last = a;
				  if((r -= l + 1) <= l) { break; }
				  a -= 1, b = middle - 1;
				  t = *a;
				}
			      } while(1);
			    } else {
			      a = first, b = middle;
			      t = *a;
			      do {
				*a++ = *b, *b++ = *a;
				if(last <= b) {
				  *a = t;
				  first = a + 1;
				  if((l -= r + 1) <= r) { break; }
				  a += 1, b = middle;
				  t = *a;
				}
			      } while(1);
			    }
			  }
			}


			/*---------------------------------------------------------------------------*/

			static
			void
			ss_inplacemerge(symbol_const_iterator T, index_const_iterator PA,
					index_iterator first, index_iterator middle, index_iterator last,
					saidx_t depth) {
			  index_const_iterator p;
			  index_iterator a, b;
			  saidx_t len, half;
			  saint_t q, r;
			  saint_t x;

			  for(;;) {
			    if(*(last - 1) < 0) { x = 1; p = PA + ~*(last - 1); }
			    else                { x = 0; p = PA +  *(last - 1); }
			    for(a = first, len = middle - first, half = len >> 1, r = -1;
				0 < len;
				len = half, half >>= 1) {
			      b = a + half;
			      q = ss_compare(T, PA + ((0 <= *b) ? *b : ~*b), p, depth);
			      if(q < 0) {
				a = b + 1;
				half -= (len & 1) ^ 1;
			      } else {
				r = q;
			      }
			    }
			    if(a < middle) {
			      if(r == 0) { *a = ~*a; }
			      ss_rotate(a, middle, last);
			      last -= middle - a;
			      middle = a;
			      if(first == middle) { break; }
			    }
			    --last;
			    if(x != 0) { while(*--last < 0) { } }
			    if(middle == last) { break; }
			  }
			}


			/*---------------------------------------------------------------------------*/

			/* Merge-forward with internal buffer. */
			static
			void
			ss_mergeforward(symbol_const_iterator T, index_const_iterator PA,
					index_iterator first, index_iterator middle, index_iterator last,
					index_iterator buf, saidx_t depth) {
			  index_iterator a, b, c, bufend;
			  saidx_t t;
			  saint_t r;

			  bufend = buf + (middle - first) - 1;
			  ss_blockswap(buf, first, middle - first);

			  for(t = *(a = first), b = buf, c = middle;;) {
			    r = ss_compare(T, PA + *b, PA + *c, depth);
			    if(r < 0) {
			      do {
				*a++ = *b;
				if(bufend <= b) { *bufend = t; return; }
				*b++ = *a;
			      } while(*b < 0);
			    } else if(r > 0) {
			      do {
				*a++ = *c, *c++ = *a;
				if(last <= c) {
				  while(b < bufend) { *a++ = *b, *b++ = *a; }
				  *a = *b, *b = t;
				  return;
				}
			      } while(*c < 0);
			    } else {
			      *c = ~*c;
			      do {
				*a++ = *b;
				if(bufend <= b) { *bufend = t; return; }
				*b++ = *a;
			      } while(*b < 0);

			      do {
				*a++ = *c, *c++ = *a;
				if(last <= c) {
				  while(b < bufend) { *a++ = *b, *b++ = *a; }
				  *a = *b, *b = t;
				  return;
				}
			      } while(*c < 0);
			    }
			  }
			}

			/* Merge-backward with internal buffer. */
			static
			void
			ss_mergebackward(symbol_const_iterator T, index_const_iterator PA,
					 index_iterator first, index_iterator middle, index_iterator last,
					 index_iterator buf, saidx_t depth) {
			  index_const_iterator p1, p2;
			  index_iterator a, b, c, bufend;
			  saidx_t t;
			  saint_t r;
			  saint_t x;

			  bufend = buf + (last - middle) - 1;
			  ss_blockswap(buf, middle, last - middle);

			  x = 0;
			  if(*bufend < 0)       { p1 = PA + ~*bufend; x |= 1; }
			  else                  { p1 = PA +  *bufend; }
			  if(*(middle - 1) < 0) { p2 = PA + ~*(middle - 1); x |= 2; }
			  else                  { p2 = PA +  *(middle - 1); }
			  for(t = *(a = last - 1), b = bufend, c = middle - 1;;) {
			    r = ss_compare(T, p1, p2, depth);
			    if(0 < r) {
			      if(x & 1) { do { *a-- = *b, *b-- = *a; } while(*b < 0); x ^= 1; }
			      *a-- = *b;
			      if(b <= buf) { *buf = t; break; }
			      *b-- = *a;
			      if(*b < 0) { p1 = PA + ~*b; x |= 1; }
			      else       { p1 = PA +  *b; }
			    } else if(r < 0) {
			      if(x & 2) { do { *a-- = *c, *c-- = *a; } while(*c < 0); x ^= 2; }
			      *a-- = *c, *c-- = *a;
			      if(c < first) {
				while(buf < b) { *a-- = *b, *b-- = *a; }
				*a = *b, *b = t;
				break;
			      }
			      if(*c < 0) { p2 = PA + ~*c; x |= 2; }
			      else       { p2 = PA +  *c; }
			    } else {
			      if(x & 1) { do { *a-- = *b, *b-- = *a; } while(*b < 0); x ^= 1; }
			      *a-- = ~*b;
			      if(b <= buf) { *buf = t; break; }
			      *b-- = *a;
			      if(x & 2) { do { *a-- = *c, *c-- = *a; } while(*c < 0); x ^= 2; }
			      *a-- = *c, *c-- = *a;
			      if(c < first) {
				while(buf < b) { *a-- = *b, *b-- = *a; }
				*a = *b, *b = t;
				break;
			      }
			      if(*b < 0) { p1 = PA + ~*b; x |= 1; }
			      else       { p1 = PA +  *b; }
			      if(*c < 0) { p2 = PA + ~*c; x |= 2; }
			      else       { p2 = PA +  *c; }
			    }
			  }
			}

			/* D&C based merge. */
			static
			void
			ss_swapmerge(symbol_const_iterator T, index_const_iterator PA,
				     index_iterator first, index_iterator middle, index_iterator last,
				     index_iterator buf, saidx_t bufsize, saidx_t depth) {
			#define GETIDX(a) ((0 <= (a)) ? (a) : (~(a)))
			#define MERGE_CHECK(a, b, c)\
			  do {\
			    if(((c) & 1) ||\
			       (((c) & 2) && (ss_compare(T, PA + GETIDX(*((a) - 1)), PA + *(a), depth) == 0))) {\
			      *(a) = ~*(a);\
			    }\
			    if(((c) & 4) && ((ss_compare(T, PA + GETIDX(*((b) - 1)), PA + *(b), depth) == 0))) {\
			      *(b) = ~*(b);\
			    }\
			  } while(0)
			  FiniteStack4<index_iterator, index_iterator, index_iterator, saint_t, SS_SMERGE_STACKSIZE> stack;

			  index_iterator l, r, lm, rm;
			  saidx_t m, len, half;
			  saint_t check, next;

			  for(check = 0;;) {
			    if((last - middle) <= bufsize) {
			      if((first < middle) && (middle < last)) {
				ss_mergebackward(T, PA, first, middle, last, buf, depth);
			      }
			      MERGE_CHECK(first, last, check);
			      if ( ! stack.pop(first, middle, last, check) )
				return;
			      continue;
			    }

			    if((middle - first) <= bufsize) {
			      if(first < middle) {
				ss_mergeforward(T, PA, first, middle, last, buf, depth);
			      }
			      MERGE_CHECK(first, last, check);
			      if ( ! stack.pop(first, middle, last, check) )
				return;
			      continue;
			    }

			    for(m = 0, len = ::std::min(middle - first, last - middle), half = len >> 1;
				0 < len;
				len = half, half >>= 1) {
			      if(ss_compare(T, PA + GETIDX(*(middle + m + half)),
					       PA + GETIDX(*(middle - m - half - 1)), depth) < 0) {
				m += half + 1;
				half -= (len & 1) ^ 1;
			      }
			    }

			    if(0 < m) {
			      lm = middle - m, rm = middle + m;
			      ss_blockswap(lm, middle, m);
			      l = r = middle, next = 0;
			      if(rm < last) {
				if(*rm < 0) {
				  *rm = ~*rm;
				  if(first < lm) { for(; *--l < 0;) { } next |= 4; }
				  next |= 1;
				} else if(first < lm) {
				  for(; *r < 0; ++r) { }
				  next |= 2;
				}
			      }

			      if((l - first) <= (last - r)) {
				stack.push(r, rm, last, (next & 3) | (check & 4));
				middle = lm, last = l, check = (check & 3) | (next & 4);
			      } else {
				if((next & 2) && (r == middle)) { next ^= 6; }
				stack.push(first, lm, l, (check & 3) | (next & 4));
				first = r, middle = rm, check = (next & 3) | (check & 4);
			      }
			    } else {
			      if(ss_compare(T, PA + GETIDX(*(middle - 1)), PA + *middle, depth) == 0) {
				*middle = ~*middle;
			      }
			      MERGE_CHECK(first, last, check);
			      if ( ! stack.pop(first, middle, last, check) )
				return;
			    }
			  }
			}


			/*---------------------------------------------------------------------------*/

			/*- Function -*/

			/* Substring sort */
			static void
			sssort(symbol_const_iterator T, index_const_iterator PA,
			       index_iterator first, index_iterator last,
			       index_iterator buf, saidx_t bufsize,
			       saidx_t depth, saidx_t n, saint_t lastsuffix) {
			  index_iterator a;

			  index_iterator b, middle, curbuf;
			  saidx_t j, k, curbufsize, limit;

			  saidx_t i;

			  if(lastsuffix != 0) { ++first; }

			if ( SS_BLOCKSIZE == 0 )
			  ss_mintrosort(T, PA, first, last, depth);

			else
			{
			  if((bufsize < SS_BLOCKSIZE) &&
			      (bufsize < (last - first)) &&
			      (bufsize < (limit = ss_isqrt(last - first)))) {
			    if(SS_BLOCKSIZE < limit) { limit = SS_BLOCKSIZE; }
			    buf = middle = last - limit, bufsize = limit;
			  } else {
			    middle = last, limit = 0;
			  }
			  for(a = first, i = 0; SS_BLOCKSIZE < (middle - a); a += SS_BLOCKSIZE, ++i) {


			    if (  SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE )
				ss_mintrosort(T, PA, a, a + SS_BLOCKSIZE, depth);
			    else if ( 1 < SS_BLOCKSIZE )
				ss_insertionsort(T, PA, a, a + SS_BLOCKSIZE, depth);
			
			    curbufsize = last - (a + SS_BLOCKSIZE);
			    curbuf = a + SS_BLOCKSIZE;
			    if(curbufsize <= bufsize) { curbufsize = bufsize, curbuf = buf; }
			    for(b = a, k = SS_BLOCKSIZE, j = i; j & 1; b -= k, k <<= 1, j >>= 1) {
			      ss_swapmerge(T, PA, b - k, b, b + k, curbuf, curbufsize, depth);
			    }
			  }
			  
			  if ( SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE )
			    ss_mintrosort(T, PA, a, middle, depth);
			  else if ( 1 < SS_BLOCKSIZE )
			    ss_insertionsort(T, PA, a, middle, depth);
			
			  for(k = SS_BLOCKSIZE; i != 0; k <<= 1, i >>= 1) {
			    if(i & 1) {
			      ss_swapmerge(T, PA, a - k, a, middle, buf, bufsize, depth);
			      a -= k;
			    }
			  }
			  if(limit != 0) {
			    if ( SS_INSERTIONSORT_THRESHOLD < SS_BLOCKSIZE )
			      ss_mintrosort(T, PA, middle, last, depth);
			    else if ( 1 < SS_BLOCKSIZE )
			      ss_insertionsort(T, PA, middle, last, depth);
			
			    ss_inplacemerge(T, PA, first, middle, last, depth);
			  }


			}

			  if(lastsuffix != 0) {
			    /* Insert last type B* suffix. */
			    saidx_t PAi[2]; PAi[0] = PA[*(first - 1)], PAi[1] = n - 2;
			    for(a = first, i = *(first - 1);
				(a < last) && ((*a < 0) || (0 < ss_compare(T, &(PAi[0]), PA + *a, depth)));
				++a) {
			      *(a - 1) = *a;
			    }
			    *(a - 1) = i;
			  }
			}
		};


		template<unsigned int bitcount, typename index_iterator, typename index_const_iterator>
		struct TrSort : DivSufSortTypes<bitcount>
		{
			typedef DivSufSortTypes<bitcount> base_type;
			typedef typename DivSufSortTypes<bitcount>::saidx_t saidx_t;
			
			template<typename typea, typename typeb, typename typec, typename typed, typename typee, unsigned int k>
			struct FiniteStack5
			{
				struct Stack5Element
				{
					typea a;
					typeb b;
					typec c;
					typed d;
					typee e;
				};
				
				Stack5Element const & operator[](uint64_t const i) const
				{
					return stack[i];
				}
				Stack5Element       & operator[](uint64_t const i)
				{
					return stack[i];
				}
				
				Stack5Element stack[k];
				unsigned int ssize;
				
				FiniteStack5()
				: ssize(0)
				{
				
				}
				
				inline void push(
					typea a,
					typeb b,
					typec c,
					typed d,
					typee e
					)
				{
					assert ( ssize < k );
					stack[ssize].a = a;
					stack[ssize].b = b;
					stack[ssize].c = c;
					stack[ssize].d = d;
					stack[ssize].e = e;
					ssize += 1;
				}
				
				inline bool pop(
					typea & a,
					typeb & b,
					typec & c,
					typed & d,
					typee & e)
				{
					if ( ssize )
					{
						ssize -= 1;
						a = stack[ssize].a;
						b = stack[ssize].b;
						c = stack[ssize].c;
						d = stack[ssize].d;
						e = stack[ssize].e;
						return true;
					}
					else
					{
						return false;
					}
				}
			};

			public:
			typedef typename DivSufSortTypes<bitcount>::saint_t saint_t;

			static saidx_t const TR_INSERTIONSORT_THRESHOLD = 8;
			static unsigned int const TR_STACKSIZE = (bitcount > 32) ? 96 : 64;

			private:
			/*---------------------------------------------------------------------------*/

			/* Simple insertionsort for small size groups. */
			static
			void
			tr_insertionsort(index_const_iterator ISAd, index_iterator first, index_iterator last) {
			  index_iterator a, b;
			  saidx_t t, r;

			  for(a = first + 1; a < last; ++a) {
			    for(t = *a, b = a - 1; 0 > (r = ISAd[t] - ISAd[*b]);) {
			      do { *(b + 1) = *b; } while((first <= --b) && (*b < 0));
			      if(b < first) { break; }
			    }
			    if(r == 0) { *b = ~*b; }
			    *(b + 1) = t;
			  }
			}


			/*---------------------------------------------------------------------------*/

			static inline
			void
			tr_fixdown(index_const_iterator ISAd, index_iterator SA, saidx_t i, saidx_t size) {
			  saidx_t j, k;
			  saidx_t v;
			  saidx_t c, d, e;

			  for(v = SA[i], c = ISAd[v]; (j = 2 * i + 1) < size; SA[i] = SA[k], i = k) {
			    d = ISAd[SA[k = j++]];
			    if(d < (e = ISAd[SA[j]])) { k = j; d = e; }
			    if(d <= c) { break; }
			  }
			  SA[i] = v;
			}

			inline static void indexSwapNonRef(index_iterator i0, index_iterator i1)
			{
				saidx_t const v0 = *i0;
				*i0 = *i1;
				*i1 = v0;
			}

			/* Simple top-down heapsort. */
			static
			void
			tr_heapsort(index_const_iterator ISAd, index_iterator SA, saidx_t size) {
			  saidx_t i, m;
			  saidx_t t;

			  m = size;
			  if((size % 2) == 0) {
			    m--;
			    if(ISAd[SA[m / 2]] < ISAd[SA[m]]) { indexSwapNonRef(SA+m, SA+(m / 2)); }
			  }

			  for(i = m / 2 - 1; 0 <= i; --i) { tr_fixdown(ISAd, SA, i, m); }
			  if((size % 2) == 0) { indexSwapNonRef(SA, SA+m); tr_fixdown(ISAd, SA, 0, m); }
			  for(i = m - 1; 0 < i; --i) {
			    t = SA[0], SA[0] = SA[i];
			    tr_fixdown(ISAd, SA, 0, i);
			    SA[i] = t;
			  }
			}


			/*---------------------------------------------------------------------------*/

			/* Returns the median of three elements. */
			static inline
			index_iterator
			tr_median3(index_const_iterator ISAd, index_iterator v1, index_iterator v2, index_iterator v3) {
			  if(ISAd[*v1] > ISAd[*v2]) { ::std::swap(v1, v2); }
			  if(ISAd[*v2] > ISAd[*v3]) {
			    if(ISAd[*v1] > ISAd[*v3]) { return v1; }
			    else { return v3; }
			  }
			  return v2;
			}

			/* Returns the median of five elements. */
			static inline
			index_iterator
			tr_median5(index_const_iterator ISAd,
				   index_iterator v1, index_iterator v2, index_iterator v3, index_iterator v4, index_iterator v5) {
			  if(ISAd[*v2] > ISAd[*v3]) { ::std::swap(v2, v3); }
			  if(ISAd[*v4] > ISAd[*v5]) { ::std::swap(v4, v5); }
			  if(ISAd[*v2] > ISAd[*v4]) { ::std::swap(v2, v4); ::std::swap(v3, v5); }
			  if(ISAd[*v1] > ISAd[*v3]) { ::std::swap(v1, v3); }
			  if(ISAd[*v1] > ISAd[*v4]) { ::std::swap(v1, v4); ::std::swap(v3, v5); }
			  if(ISAd[*v3] > ISAd[*v4]) { return v4; }
			  return v3;
			}

			/* Returns the pivot element. */
			static inline
			index_iterator 
			tr_pivot(index_const_iterator ISAd, index_iterator first, index_iterator last) {
			  index_iterator middle;
			  saidx_t t;

			  t = last - first;
			  middle = first + t / 2;

			  if(t <= 512) {
			    if(t <= 32) {
			      return tr_median3(ISAd, first, middle, last - 1);
			    } else {
			      t >>= 2;
			      return tr_median5(ISAd, first, first + t, middle, last - 1 - t, last - 1);
			    }
			  }
			  t >>= 3;
			  first  = tr_median3(ISAd, first, first + t, first + (t << 1));
			  middle = tr_median3(ISAd, middle - t, middle, middle + t);
			  last   = tr_median3(ISAd, last - 1 - (t << 1), last - 1 - t, last - 1);
			  return tr_median3(ISAd, first, middle, last);
			}


			/*---------------------------------------------------------------------------*/

			struct trbudget_t 
			{
			  saidx_t chance;
			  saidx_t remain;
			  saidx_t incval;
			  saidx_t count;
			};

			static inline
			void
			trbudget_init(trbudget_t *budget, saidx_t chance, saidx_t incval) {
			  budget->chance = chance;
			  budget->remain = budget->incval = incval;
			}

			static inline
			saint_t
			trbudget_check(trbudget_t *budget, saidx_t size) {
			  if(size <= budget->remain) { budget->remain -= size; return 1; }
			  if(budget->chance == 0) { budget->count += size; return 0; }
			  budget->remain += budget->incval - size;
			  budget->chance -= 1;
			  return 1;
			}


			/*---------------------------------------------------------------------------*/

			static inline
			void
			tr_partition(index_const_iterator ISAd,
				     index_iterator first, index_iterator middle, index_iterator last,
				     index_iterator *pa, index_iterator *pb, saidx_t v) {
			  index_iterator a, b, c, d, e, f;
			  saidx_t t, s;
			  saidx_t x = 0;

			  for(b = middle - 1; (++b < last) && ((x = ISAd[*b]) == v);) { }
			  if(((a = b) < last) && (x < v)) {
			    for(; (++b < last) && ((x = ISAd[*b]) <= v);) {
			      if(x == v) { indexSwap(b, a); ++a; }
			    }
			  }
			  for(c = last; (b < --c) && ((x = ISAd[*c]) == v);) { }
			  if((b < (d = c)) && (x > v)) {
			    for(; (b < --c) && ((x = ISAd[*c]) >= v);) {
			      if(x == v) { indexSwap(c, d); --d; }
			    }
			  }
			  for(; b < c;) {
			    indexSwap(b, c);
			    for(; (++b < c) && ((x = ISAd[*b]) <= v);) {
			      if(x == v) { indexSwap(b, a); ++a; }
			    }
			    for(; (b < --c) && ((x = ISAd[*c]) >= v);) {
			      if(x == v) { indexSwap(c, d); --d; }
			    }
			  }

			  if(a <= d) {
			    c = b - 1;
			    if((s = a - first) > (t = b - a)) { s = t; }
			    for(e = first, f = b - s; 0 < s; --s, ++e, ++f) { indexSwap(e, f); }
			    if((s = d - c) > (t = last - d - 1)) { s = t; }
			    for(e = b, f = last - s; 0 < s; --s, ++e, ++f) { indexSwap(e, f); }
			    first += (b - a), last -= (d - c);
			  }
			  *pa = first, *pb = last;
			}

			static
			void
			tr_copy(index_iterator ISA, index_const_iterator SA,
				index_iterator first, index_iterator a, index_iterator b, index_iterator last,
				saidx_t depth) {
			  /* sort suffixes of middle partition
			     by using sorted order of suffixes of left and right partition. */
			  index_iterator c, d, e;
			  saidx_t s, v;

			  v = b - SA - 1;
			  for(c = first, d = a - 1; c <= d; ++c) {
			    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
			      *++d = s;
			      ISA[s] = d - SA;
			    }
			  }
			  for(c = last - 1, e = d + 1, d = b; e < d; --c) {
			    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
			      *--d = s;
			      ISA[s] = d - SA;
			    }
			  }
			}

			static
			void
			tr_partialcopy(index_iterator ISA, index_const_iterator SA,
				       index_iterator first, index_iterator a, index_iterator b, index_iterator last,
				       saidx_t depth) {
			  index_iterator c, d, e;
			  saidx_t s, v;
			  saidx_t rank, lastrank, newrank = -1;

			  v = b - SA - 1;
			  lastrank = -1;
			  for(c = first, d = a - 1; c <= d; ++c) {
			    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
			      *++d = s;
			      rank = ISA[s + depth];
			      if(lastrank != rank) { lastrank = rank; newrank = d - SA; }
			      ISA[s] = newrank;
			    }
			  }

			  lastrank = -1;
			  for(e = d; first <= e; --e) {
			    rank = ISA[*e];
			    if(lastrank != rank) { lastrank = rank; newrank = e - SA; }
			    if(newrank != rank) { ISA[*e] = newrank; }
			  }

			  lastrank = -1;
			  for(c = last - 1, e = d + 1, d = b; e < d; --c) {
			    if((0 <= (s = *c - depth)) && (ISA[s] == v)) {
			      *--d = s;
			      rank = ISA[s + depth];
			      if(lastrank != rank) { lastrank = rank; newrank = d - SA; }
			      ISA[s] = newrank;
			    }
			  }
			}

			inline static void indexSwap(index_iterator & i0, index_iterator & i1)
			{
				saidx_t const v0 = *i0;
				*i0 = *i1;
				*i1 = v0;
			}

			static
			void
			tr_introsort(index_iterator ISA, index_const_iterator ISAd,
				     index_iterator SA, index_iterator first, index_iterator last,
				     trbudget_t *budget) {
			  FiniteStack5<index_const_iterator, index_iterator , index_iterator , saint_t, saint_t, TR_STACKSIZE> stack;
			  index_iterator a, b, c;
			  saidx_t v, x = 0;
			  saidx_t incr = ISAd - ISA;
			  saint_t limit, next;
			  saint_t trlink = -1;

			  for(limit = base_type::tr_ilg(last - first);;) {

			    if(limit < 0) {
			      if(limit == -1) {
				/* tandem repeat partition */
				tr_partition(ISAd - incr, first, first, last, &a, &b, last - SA - 1);

				/* update ranks */
				if(a < last) {
				  for(c = first, v = a - SA - 1; c < a; ++c) { ISA[*c] = v; }
				}
				if(b < last) {
				  for(c = a, v = b - SA - 1; c < b; ++c) { ISA[*c] = v; }
				}

				/* push */
				if(1 < (b - a)) {
				  stack.push(index_const_iterator(), a, b, 0, 0);
				  stack.push(ISAd - incr, first, last, -2, trlink);
				  trlink = stack.ssize - 2;
				}
				if((a - first) <= (last - b)) {
				  if(1 < (a - first)) {
				    stack.push(ISAd, b, last, base_type::tr_ilg(last - b), trlink);
				    last = a, limit = base_type::tr_ilg(a - first);
				  } else if(1 < (last - b)) {
				    first = b, limit = base_type::tr_ilg(last - b);
				  } else {
				    if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
				  }
				} else {
				  if(1 < (last - b)) {
				    stack.push(ISAd, first, a, base_type::tr_ilg(a - first), trlink);
				    first = b, limit = base_type::tr_ilg(last - b);
				  } else if(1 < (a - first)) {
				    last = a, limit = base_type::tr_ilg(a - first);
				  } else {
				    if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
				  }
				}
			      } else if(limit == -2) {
				/* tandem repeat copy */
				a = stack[--stack.ssize].b;
				b = stack[stack.ssize].c;
				if(stack[stack.ssize].d == 0) {
				  tr_copy(ISA, SA, first, a, b, last, ISAd - ISA);
				} else {
				  if(0 <= trlink) { stack[trlink].d = -1; }
				  tr_partialcopy(ISA, SA, first, a, b, last, ISAd - ISA);
				}
				if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
			      } else {
				/* sorted partition */
				if(0 <= *first) {
				  a = first;
				  do { ISA[*a] = a - SA; } while((++a < last) && (0 <= *a));
				  first = a;
				}
				if(first < last) {
				  a = first; do { *a = ~*a; } while(*++a < 0);
				  next = (ISA[*a] != ISAd[*a]) ? base_type::tr_ilg(a - first + 1) : -1;
				  if(++a < last) { for(b = first, v = a - SA - 1; b < a; ++b) { ISA[*b] = v; } }

				  /* push */
				  if(trbudget_check(budget, a - first)) {
				    if((a - first) <= (last - a)) {
				      stack.push(ISAd, a, last, -3, trlink);
				      ISAd += incr, last = a, limit = next;
				    } else {
				      if(1 < (last - a)) {
					stack.push(ISAd + incr, first, a, next, trlink);
					first = a, limit = -3;
				      } else {
					ISAd += incr, last = a, limit = next;
				      }
				    }
				  } else {
				    if(0 <= trlink) { stack[trlink].d = -1; }
				    if(1 < (last - a)) {
				      first = a, limit = -3;
				    } else {
				      if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
				    }
				  }
				} else {
				  if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
				}
			      }
			      continue;
			    }

			    if((last - first) <= TR_INSERTIONSORT_THRESHOLD) {
			      tr_insertionsort(ISAd, first, last);
			      limit = -3;
			      continue;
			    }

			    if(limit-- == 0) {
			      tr_heapsort(ISAd, first, last - first);
			      for(a = last - 1; first < a; a = b) {
				for(x = ISAd[*a], b = a - 1; (first <= b) && (ISAd[*b] == x); --b) { *b = ~*b; }
			      }
			      limit = -3;
			      continue;
			    }

			    /* choose pivot */
			    a = tr_pivot(ISAd, first, last);
			    indexSwap(first, a);
			    v = ISAd[*first];

			    /* partition */
			    tr_partition(ISAd, first, first + 1, last, &a, &b, v);
			    if((last - first) != (b - a)) {
			      next = (ISA[*a] != v) ? base_type::tr_ilg(b - a) : -1;

			      /* update ranks */
			      for(c = first, v = a - SA - 1; c < a; ++c) { ISA[*c] = v; }
			      if(b < last) { for(c = a, v = b - SA - 1; c < b; ++c) { ISA[*c] = v; } }

			      /* push */
			      if((1 < (b - a)) && (trbudget_check(budget, b - a))) {
				if((a - first) <= (last - b)) {
				  if((last - b) <= (b - a)) {
				    if(1 < (a - first)) {
				      stack.push(ISAd + incr, a, b, next, trlink);
				      stack.push(ISAd, b, last, limit, trlink);
				      last = a;
				    } else if(1 < (last - b)) {
				      stack.push(ISAd + incr, a, b, next, trlink);
				      first = b;
				    } else {
				      ISAd += incr, first = a, last = b, limit = next;
				    }
				  } else if((a - first) <= (b - a)) {
				    if(1 < (a - first)) {
				      stack.push(ISAd, b, last, limit, trlink);
				      stack.push(ISAd + incr, a, b, next, trlink);
				      last = a;
				    } else {
				      stack.push(ISAd, b, last, limit, trlink);
				      ISAd += incr, first = a, last = b, limit = next;
				    }
				  } else {
				    stack.push(ISAd, b, last, limit, trlink);
				    stack.push(ISAd, first, a, limit, trlink);
				    ISAd += incr, first = a, last = b, limit = next;
				  }
				} else {
				  if((a - first) <= (b - a)) {
				    if(1 < (last - b)) {
				      stack.push(ISAd + incr, a, b, next, trlink);
				      stack.push(ISAd, first, a, limit, trlink);
				      first = b;
				    } else if(1 < (a - first)) {
				      stack.push(ISAd + incr, a, b, next, trlink);
				      last = a;
				    } else {
				      ISAd += incr, first = a, last = b, limit = next;
				    }
				  } else if((last - b) <= (b - a)) {
				    if(1 < (last - b)) {
				      stack.push(ISAd, first, a, limit, trlink);
				      stack.push(ISAd + incr, a, b, next, trlink);
				      first = b;
				    } else {
				      stack.push(ISAd, first, a, limit, trlink);
				      ISAd += incr, first = a, last = b, limit = next;
				    }
				  } else {
				    stack.push(ISAd, first, a, limit, trlink);
				    stack.push(ISAd, b, last, limit, trlink);
				    ISAd += incr, first = a, last = b, limit = next;
				  }
				}
			      } else {
				if((1 < (b - a)) && (0 <= trlink)) { stack[trlink].d = -1; }
				if((a - first) <= (last - b)) {
				  if(1 < (a - first)) {
				    stack.push(ISAd, b, last, limit, trlink);
				    last = a;
				  } else if(1 < (last - b)) {
				    first = b;
				  } else {
				    if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
				  }
				} else {
				  if(1 < (last - b)) {
				    stack.push(ISAd, first, a, limit, trlink);
				    first = b;
				  } else if(1 < (a - first)) {
				    last = a;
				  } else {
				    if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
				  }
				}
			      }
			    } else {
			      if(trbudget_check(budget, last - first)) {
				limit = base_type::tr_ilg(last - first), ISAd += incr;
			      } else {
				if(0 <= trlink) { stack[trlink].d = -1; }
				if ( ! stack.pop(ISAd, first, last, limit, trlink) )
					return;
			      }
			    }
			  }
			}



			/*---------------------------------------------------------------------------*/

			/*- Function -*/

			/* Tandem repeat sort */
			public:
			static void
			trsort(index_iterator ISA, index_iterator SA, saidx_t n, saidx_t depth) {
			  index_iterator ISAd;
			  index_iterator first, last;
			  trbudget_t budget;
			  saidx_t t, skip, unsorted;

			  trbudget_init(&budget, base_type::tr_ilg(n) * 2 / 3, n);
			/*  trbudget_init(&budget, base_type::tr_ilg(n) * 3 / 4, n); */
			  for(ISAd = ISA + depth; -n < *SA; ISAd += ISAd - ISA) {
			    first = SA;
			    skip = 0;
			    unsorted = 0;
			    do {
			      if((t = *first) < 0) { first -= t; skip += t; }
			      else {
				if(skip != 0) { *(first + skip) = skip; skip = 0; }
				last = SA + ISA[t] + 1;
				if(1 < (last - first)) {
				  budget.count = 0;
				  tr_introsort(ISA, ISAd, SA, first, last, &budget);
				  if(budget.count != 0) { unsorted += budget.count; }
				  else { skip = first - last; }
				} else if((last - first) == 1) {
				  skip = -1;
				}
				first = last;
			      }
			    } while(first < (SA + n));
			    if(skip != 0) { *(first + skip) = skip; }
			    if(unsorted == 0) { break; }
			  }
			}
		};


		template<
			unsigned int bitcount, 
			typename symbol_iterator, 
			typename symbol_const_iterator, 
			typename index_iterator,
			typename index_const_iterator,
			int _ALPHABET_SIZE=256,
			bool _ompparallel = true>
		struct DivSufSort : public DivSufSortTypes<bitcount>
		{
		        enum { ALPHABET_SIZE = _ALPHABET_SIZE };
		        enum { ompparallel = _ompparallel };
                
			typedef typename DivSufSortTypes<bitcount>::saint_t saint_t;
			typedef typename std::iterator_traits<symbol_const_iterator>::value_type sauchar_t;
			typedef typename DivSufSortTypes<bitcount>::saidx_t saidx_t;
			
			static saidx_t const BUCKET_A_SIZE = ALPHABET_SIZE;
			static saidx_t const BUCKET_B_SIZE = ALPHABET_SIZE * ALPHABET_SIZE;

			static saidx_t const & BUCKET_A(
				saidx_t const * bucket_A,
				saint_t const c0
				)
			{
				return bucket_A[c0];
			}
			static saidx_t & BUCKET_A(
				saidx_t * bucket_A,
				saint_t const c0
				)
			{
				return bucket_A[c0];
			}

			static saidx_t & BUCKET_B(
				saidx_t * bucket_B,
				saint_t const _c0,
				saint_t const _c1)
			{
				if ( ALPHABET_SIZE == 256 )
					return (bucket_B[((_c1) << 8) | (_c0)]);
				else
					return (bucket_B[(_c1) * ALPHABET_SIZE + (_c0)]);
			}
			static saidx_t const & BUCKET_B(
				saidx_t const * bucket_B,
				saint_t const _c0,
				saint_t const _c1)
			{
				if ( ALPHABET_SIZE == 256 )
					return (bucket_B[((_c1) << 8) | (_c0)]);
				else
					return (bucket_B[(_c1) * ALPHABET_SIZE + (_c0)]);
			}

			static saidx_t & BUCKET_BSTAR(
				saidx_t * bucket_B,
				saint_t const _c0,
				saint_t const _c1)
			{
				if ( ALPHABET_SIZE == 256 )
					return (bucket_B[((_c0) << 8) | (_c1)]);
				else
					return (bucket_B[(_c0) * ALPHABET_SIZE + (_c1)]);
			}

			static saidx_t const & BUCKET_BSTAR(
				saidx_t const * bucket_B,
				saint_t const _c0,
				saint_t const _c1)
			{
				if ( ALPHABET_SIZE == 256 )
					return (bucket_B[((_c0) << 8) | (_c1)]);
				else
					return (bucket_B[(_c0) * ALPHABET_SIZE + (_c1)]);
			}

			/*- Private Functions -*/

			/* Sorts suffixes of type B*. */
			static
			saidx_t
			sort_typeBstar(symbol_const_iterator T, index_iterator SA,
				       saidx_t *bucket_A, saidx_t *bucket_B,
				       saidx_t n) {
			index_iterator PAb, ISAb, buf;

			#ifdef _OPENMP
			  index_iterator curbuf;
			  saidx_t l;
			  saint_t d0, d1;
			#endif

			  saidx_t i, j, k, t, m, bufsize;
			  saint_t c0, c1;

			  /* Initialize bucket arrays. */
			  for(i = 0; i < BUCKET_A_SIZE; ++i) { bucket_A[i] = 0; }
			  for(i = 0; i < BUCKET_B_SIZE; ++i) { bucket_B[i] = 0; }

			  /* Count the number of occurrences of the first one or two characters of each
			     type A, B and B* suffix. Moreover, store the beginning position of all
			     type B* suffixes into the array SA. */
			  for(i = n - 1, m = n, c0 = T[n - 1]; 0 <= i;) {
			    /* type A suffix. */
			    do { ++BUCKET_A(bucket_A, c1 = c0); } while((0 <= --i) && ((c0 = T[i]) >= c1));
			    if(0 <= i) {
			      /* type B* suffix. */
			      ++BUCKET_BSTAR(bucket_B,c0, c1);
			      SA[--m] = i;
			      /* type B suffix. */
			      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) {
				++BUCKET_B(bucket_B, c0, c1);
			      }
			    }
			  }
			  m = n - m;
			/*
			note:
			  A type B* suffix is lexicographically smaller than a type B suffix that
			  begins with the same first two characters.
			*/

			  /* Calculate the index of start/end point of each bucket. */
			  for(c0 = 0, i = 0, j = 0; c0 < ALPHABET_SIZE; ++c0) {
			    t = i + BUCKET_A(bucket_A,c0);
			    BUCKET_A(bucket_A, c0) = i + j; /* start point */
			    i = t + BUCKET_B(bucket_B, c0, c0);
			    for(c1 = c0 + 1; c1 < ALPHABET_SIZE; ++c1) {
			      j += BUCKET_BSTAR(bucket_B,c0, c1);
			      BUCKET_BSTAR(bucket_B,c0, c1) = j; /* end point */
			      i += BUCKET_B(bucket_B, c0, c1);
			    }
			  }

			  if(0 < m) {
			    /* Sort the type B* suffixes by their first two characters. */
			    PAb = SA + n - m; ISAb = SA + m;
			    for(i = m - 2; 0 <= i; --i) {
			      t = PAb[i], c0 = T[t], c1 = T[t + 1];
			      SA[--BUCKET_BSTAR(bucket_B,c0, c1)] = i;
			    }
			    t = PAb[m - 1], c0 = T[t], c1 = T[t + 1];
			    SA[--BUCKET_BSTAR(bucket_B,c0, c1)] = m - 1;

			    /* Sort the type B* substrings using sssort. */
			    
			#if defined(_OPENMP)
			  if ( ompparallel )
			  {
                              buf = SA + m, bufsize = (n - (2 * m)) / omp_get_max_threads();
                              c0 = ALPHABET_SIZE - 2, c1 = ALPHABET_SIZE - 1, j = m;
                              
                              #pragma omp parallel default(shared) private(curbuf, k, l, d0, d1)
                              {
                                curbuf = buf + omp_get_thread_num() * bufsize;
                                k = 0;
                                for(;;) {
                                  #pragma omp critical(sssort_lock)
                                  {
                                    if(0 < (l = j)) {
                                      d0 = c0, d1 = c1;
                                      do {
                                        k = BUCKET_BSTAR(bucket_B,d0, d1);
                                        if(--d1 <= d0) {
                                          d1 = ALPHABET_SIZE - 1;
                                          if(--d0 < 0) { break; }
                                        }
                                      } while(((l - k) <= 1) && (0 < (l = k)));
                                      c0 = d0, c1 = d1, j = k;
                                    }
                                  }
                                  if(l == 0) { break; }
                                  Sssort<bitcount,symbol_const_iterator,index_iterator, index_const_iterator>::sssort(T, PAb, SA + k, SA + l,
                                         curbuf, bufsize, 2, n, *(SA + k) == (m - 1));
                                }
                              }
                              
                          }
                          else
			#endif
			    {
                              buf = SA + m, bufsize = n - (2 * m);
                              for(c0 = ALPHABET_SIZE - 2, j = m; 0 < j; --c0) 
                              {
                                for(c1 = ALPHABET_SIZE - 1; c0 < c1; j = i, --c1) 
                                {
                                  i = BUCKET_BSTAR(bucket_B,c0, c1);
                                  if(1 < (j - i)) 
                                  {
                                    Sssort<bitcount,symbol_const_iterator,index_iterator,index_const_iterator>::sssort(T, PAb, SA + i, SA + j,
                                           buf, bufsize, 2, n, *(SA + i) == (m - 1));
                                  }
                                }
                              }
			    }

			    /* Compute ranks of type B* substrings. */
			    for(i = m - 1; 0 <= i; --i) {
			      if(0 <= SA[i]) {
				j = i;
				do { ISAb[SA[i]] = i; } while((0 <= --i) && (0 <= SA[i]));
				SA[i + 1] = i - j;
				if(i <= 0) { break; }
			      }
			      j = i;
			      do { ISAb[SA[i] = ~SA[i]] = j; } while(SA[--i] < 0);
			      ISAb[SA[i]] = j;
			    }

			    /* Construct the inverse suffix array of type B* suffixes using trsort. */
			    TrSort<bitcount,index_iterator,index_const_iterator>::trsort(ISAb, SA, m, 1);

			    /* Set the sorted order of tyoe B* suffixes. */
			    for(i = n - 1, j = m, c0 = T[n - 1]; 0 <= i;) {
			      for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) >= c1); --i, c1 = c0) { }
			      if(0 <= i) {
				t = i;
				for(--i, c1 = c0; (0 <= i) && ((c0 = T[i]) <= c1); --i, c1 = c0) { }
				SA[ISAb[--j]] = ((t == 0) || (1 < (t - i))) ? t : ~t;
			      }
			    }

			    /* Calculate the index of start/end point of each bucket. */
			    BUCKET_B(bucket_B, ALPHABET_SIZE - 1, ALPHABET_SIZE - 1) = n; /* end point */
			    for(c0 = ALPHABET_SIZE - 2, k = m - 1; 0 <= c0; --c0) {
			      i = BUCKET_A(bucket_A, c0 + 1) - 1;
			      for(c1 = ALPHABET_SIZE - 1; c0 < c1; --c1) {
				t = i - BUCKET_B(bucket_B, c0, c1);
				BUCKET_B(bucket_B, c0, c1) = i; /* end point */

				/* Move all type B* suffixes to the correct position. */
				for(i = t, j = BUCKET_BSTAR(bucket_B,c0, c1);
				    j <= k;
				    --i, --k) { SA[i] = SA[k]; }
			      }
			      BUCKET_BSTAR(bucket_B,c0, c0 + 1) = i - BUCKET_B(bucket_B, c0, c0) + 1; /* start point */
			      BUCKET_B(bucket_B, c0, c0) = i; /* end point */
			    }
			  }

			  return m;
			}

			/* Constructs the suffix array by using the sorted order of type B* suffixes. */
			static void construct_SA(symbol_const_iterator T, index_iterator SA,
				     saidx_t *bucket_A, saidx_t *bucket_B,
				     saidx_t n, saidx_t m) {
			  index_iterator i, j, k;
			  saidx_t s;
			  saint_t c0, c1, c2;

			  if(0 < m) {
			    /* Construct the sorted order of type B suffixes by using
			       the sorted order of type B* suffixes. */
			    for(c1 = ALPHABET_SIZE - 2; 0 <= c1; --c1) {
			      /* Scan the suffix array from right to left. */
			      for(i = SA + BUCKET_BSTAR(bucket_B,c1, c1 + 1),
				  j = SA + BUCKET_A(bucket_A, c1 + 1) - 1, k = index_iterator(), c2 = -1;
				  i <= j;
				  --j) {
				if(0 < (s = *j)) {
				  assert(static_cast<saint_t>(T[s]) == c1);
				  assert(((s + 1) < n) && (T[s] <= T[s + 1]));
				  assert(T[s - 1] <= T[s]);
				  *j = ~s;
				  c0 = T[--s];
				  if((0 < s) && (static_cast<saint_t>(T[s - 1]) > c0)) { s = ~s; }
				  if(c0 != c2) {
				    if(0 <= c2) { BUCKET_B(bucket_B, c2, c1) = k - SA; }
				    k = SA + BUCKET_B(bucket_B, c2 = c0, c1);
				  }
				  assert(k < j);
				  *k-- = s;
				} else {
				  assert(((s == 0) && (static_cast<saint_t>(T[s]) == c1)) || (s < 0));
				  *j = ~s;
				}
			      }
			    }
			  }

			  /* Construct the suffix array by using
			     the sorted order of type B suffixes. */
			  k = SA + BUCKET_A(bucket_A, c2 = T[n - 1]);
			  *k++ = (static_cast<saint_t>(T[n - 2]) < c2) ? ~(n - 1) : (n - 1);
			  /* Scan the suffix array from left to right. */
			  for(i = SA, j = SA + n; i < j; ++i) {
			    if(0 < (s = *i)) {
			      assert(T[s - 1] >= T[s]);
			      c0 = T[--s];
			      if((s == 0) || (static_cast<saint_t>(T[s - 1]) < c0)) { s = ~s; }
			      if(c0 != c2) {
				BUCKET_A(bucket_A, c2) = k - SA;
				k = SA + BUCKET_A(bucket_A, c2 = c0);
			      }
			      assert(i < k);
			      *k++ = s;
			    } else {
			      assert(s < 0);
			      *i = ~s;
			    }
			  }
			}

			/* Constructs the burrows-wheeler transformed string directly
			   by using the sorted order of type B* suffixes. */
			static
			saidx_t
			construct_BWT(symbol_const_iterator T, index_iterator SA,
				      saidx_t *bucket_A, saidx_t *bucket_B,
				      saidx_t n, saidx_t m) {
			  index_iterator i, j, k, orig;
			  saidx_t s;
			  saint_t c0, c1, c2;

			  if(0 < m) {
			    /* Construct the sorted order of type B suffixes by using
			       the sorted order of type B* suffixes. */
			    for(c1 = ALPHABET_SIZE - 2; 0 <= c1; --c1) {
			      /* Scan the suffix array from right to left. */
			      for(i = SA + BUCKET_BSTAR(bucket_B, c1, c1 + 1),
				  j = SA + BUCKET_A(bucket_A, c1 + 1) - 1, k = NULL, c2 = -1;
				  i <= j;
				  --j) {
				if(0 < (s = *j)) {
				  assert(T[s] == c1);
				  assert(((s + 1) < n) && (T[s] <= T[s + 1]));
				  assert(T[s - 1] <= T[s]);
				  c0 = T[--s];
				  *j = ~((saidx_t)c0);
				  if((0 < s) && (T[s - 1] > c0)) { s = ~s; }
				  if(c0 != c2) {
				    if(0 <= c2) { BUCKET_B(bucket_B, c2, c1) = k - SA; }
				    k = SA + BUCKET_B(bucket_B, c2 = c0, c1);
				  }
				  assert(k < j);
				  *k-- = s;
				} else if(s != 0) {
				  *j = ~s;
			#ifndef NDEBUG
				} else {
				  assert(T[s] == c1);
			#endif
				}
			      }
			    }
			  }

			  /* Construct the BWTed string by using
			     the sorted order of type B suffixes. */
			  k = SA + BUCKET_A(bucket_A, c2 = T[n - 1]);
			  *k++ = (T[n - 2] < c2) ? ~((saidx_t)T[n - 2]) : (n - 1);
			  /* Scan the suffix array from left to right. */
			  for(i = SA, j = SA + n, orig = SA; i < j; ++i) {
			    if(0 < (s = *i)) {
			      assert(T[s - 1] >= T[s]);
			      c0 = T[--s];
			      *i = c0;
			      if((0 < s) && (T[s - 1] < c0)) { s = ~((saidx_t)T[s - 1]); }
			      if(c0 != c2) {
				BUCKET_A(bucket_A, c2) = k - SA;
				k = SA + BUCKET_A(bucket_A, c2 = c0);
			      }
			      assert(i < k);
			      *k++ = s;
			    } else if(s != 0) {
			      *i = ~s;
			    } else {
			      orig = i;
			    }
			  }

			  return orig - SA;
			}


			/*---------------------------------------------------------------------------*/

			/*- Function -*/

			static
			saint_t
			divsufsort(symbol_const_iterator T, index_iterator SA, saidx_t n) {
			  saidx_t m;
			  saint_t err = 0;

			  /* Check arguments. */
			  if((T == symbol_const_iterator()) || (SA == index_iterator()) || (n < 0)) { return -1; }
			  else if(n == 0) { return 0; }
			  else if(n == 1) { SA[0] = 0; return 0; }
			  else if(n == 2) { m = (T[0] < T[1]); SA[m ^ 1] = 0, SA[m] = 1; return 0; }

			  ::libmaus::autoarray::AutoArray < saidx_t > Abucket_A(BUCKET_A_SIZE,false);
			  ::libmaus::autoarray::AutoArray < saidx_t > Abucket_B(BUCKET_B_SIZE,false);
			  saidx_t * bucket_A = Abucket_A.get();
			  saidx_t * bucket_B = Abucket_B.get();;

			  /* Suffixsort. */
			  if((bucket_A != NULL) && (bucket_B != NULL)) {
			    m = sort_typeBstar(T, SA, bucket_A, bucket_B, n);
			    construct_SA(T, SA, bucket_A, bucket_B, n, m);
			  } else {
			    err = -2;
			  }

			  return err;
			}

			static
			saidx_t
			divbwt(symbol_const_iterator T, symbol_iterator U, saidx_t * const A, saidx_t n) {
			  ::libmaus::autoarray::AutoArray < saidx_t > BB;
			  saidx_t *B;
			  saidx_t m, pidx, i;

			  /* Check arguments. */
			  if((T == 0) || (U == 0) || (n < 0)) { return -1; }
			  else if(n <= 1) { if(n == 1) { U[0] = T[0]; } return n; }

			  if ( A )
				B = A;
			  else
			  {
				BB = ::libmaus::autoarray::AutoArray < saidx_t >(n+1);
				B = BB.get();
			  }

			  ::libmaus::autoarray::AutoArray < saidx_t > Abucket_A(BUCKET_A_SIZE);
			  ::libmaus::autoarray::AutoArray < saidx_t > Abucket_B(BUCKET_B_SIZE);
			  saidx_t * bucket_A = Abucket_A.get();
			  saidx_t * bucket_B = Abucket_B.get();;

			  /* Burrows-Wheeler Transform. */
			  if((B != NULL) && (bucket_A != NULL) && (bucket_B != NULL)) {
			    m = sort_typeBstar(T, B, bucket_A, bucket_B, n);
			    pidx = construct_BWT(T, B, bucket_A, bucket_B, n, m);

			    /* Copy to output string. */
			    U[0] = T[n - 1];
			    for(i = 0; i < pidx; ++i) { U[i + 1] = (sauchar_t)B[i]; }
			    for(i += 1; i < n; ++i) { U[i] = (sauchar_t)B[i]; }
			    pidx += 1;
			  } else {
			    pidx = -2;
			  }

			  return pidx;
			}

			static const char *
			divsufsort_version(void) 
			{
			  return "2.0.1";
			}
		};

		template<
			unsigned int bitcount, 
			typename symbol_iterator, 
			typename symbol_const_iterator,
			typename index_iterator,
			typename index_const_iterator,
			int ALPHABET_SIZE=256>
		struct DivSufSortUtils
		{
			typedef DivSufSort<bitcount, 
				symbol_iterator, symbol_const_iterator, 
				index_iterator, index_const_iterator,
				ALPHABET_SIZE> sort_type;

			typedef typename DivSufSortTypes<bitcount>::saint_t saint_t;
			typedef typename std::iterator_traits<symbol_const_iterator>::value_type sauchar_t;
			typedef typename DivSufSortTypes<bitcount>::saidx_t saidx_t;

			/* Binary search for inverse bwt. */
			static
			saidx_t
			binarysearch_lower(index_const_iterator A, saidx_t size, saidx_t value) {
			  saidx_t half, i;
			  for(i = 0, half = size >> 1;
			      0 < size;
			      size = half, half >>= 1) {
			    if(A[i + half] < value) {
			      i += half + 1;
			      half -= (size & 1) ^ 1;
			    }
			  }
			  return i;
			}


			/*- Functions -*/

			#if 0
			/* Burrows-Wheeler transform. */
			static saint_t
			bw_transform(symbol_const_iterator T, symbol_iterator U, index_iterator SA,
				     saidx_t n, index_iterator idx) {
			  index_iterator A;
			  saidx_t i, j, p, t;
			  saint_t c;

			  /* Check arguments. */
			  if((T == NULL) || (U == NULL) || (n < 0) || (idx == NULL)) { return -1; }
			  if(n <= 1) {
			    if(n == 1) { U[0] = T[0]; }
			    *idx = n;
			    return 0;
			  }

			  if((A = SA) == NULL) {
			    i = sort_type::divbwt(T, U, NULL, n);
			    if(0 <= i) { *idx = i; i = 0; }
			    return (saint_t)i;
			  }

			  /* BW transform. */
			  if(T == U) {
			    t = n;
			    for(i = 0, j = 0; i < n; ++i) {
			      p = t - 1;
			      t = A[i];
			      if(0 <= p) {
				c = T[j];
				U[j] = (j <= p) ? T[p] : (sauchar_t)A[p];
				A[j] = c;
				j++;
			      } else {
				*idx = i;
			      }
			    }
			    p = t - 1;
			    if(0 <= p) {
			      c = T[j];
			      U[j] = (j <= p) ? T[p] : (sauchar_t)A[p];
			      A[j] = c;
			    } else {
			      *idx = i;
			    }
			  } else {
			    U[0] = T[n - 1];
			    for(i = 0; A[i] != 0; ++i) { U[i + 1] = T[A[i] - 1]; }
			    *idx = i + 1;
			    for(++i; i < n; ++i) { U[i] = T[A[i] - 1]; }
			  }

			  return 0;
			}
			#endif

			#if 0
			/* Inverse Burrows-Wheeler transform. */
			static saint_t
			inverse_bw_transform(symbol_const_iterator T, symbol_iterator U, saidx_t * const A,
					     saidx_t n, saidx_t idx) {
			  saidx_t C[ALPHABET_SIZE];
			  sauchar_t D[ALPHABET_SIZE];
			  saidx_t *B;
			  ::libmaus::autoarray::AutoArray<saidx_t> BB;
			  saidx_t i, p;
			  saint_t c, d;

			  /* Check arguments. */
			  if((T == NULL) || (U == NULL) || (n < 0) || (idx < 0) ||
			     (n < idx) || ((0 < n) && (idx == 0))) {
			    return -1;
			  }
			  if(n <= 1) { return 0; }

			  if ( A )
				B = A;
			  else
			  {
				BB = ::libmaus::autoarray::AutoArray<saidx_t>(n);
				B = BB.get();
			  }

			  /* Inverse BW transform. */
			  for(c = 0; c < ALPHABET_SIZE; ++c) { C[c] = 0; }
			  for(i = 0; i < n; ++i) { ++C[T[i]]; }
			  for(c = 0, d = 0, i = 0; c < ALPHABET_SIZE; ++c) {
			    p = C[c];
			    if(0 < p) {
			      C[c] = i;
			      D[d++] = (sauchar_t)c;
			      i += p;
			    }
			  }
			  for(i = 0; i < idx; ++i) { B[C[T[i]]++] = i; }
			  for( ; i < n; ++i)       { B[C[T[i]]++] = i + 1; }
			  for(c = 0; c < d; ++c) { C[c] = C[D[c]]; }
			  for(i = 0, p = idx; i < n; ++i) {
			    U[i] = D[binarysearch_lower(C, d, p)];
			    p = B[p - 1];
			  }

			  return 0;
			}
			#endif

			/* Checks the suffix array SA of the string T. */
			static saint_t
			sufcheck(symbol_const_iterator T, index_const_iterator SA,
				 saidx_t n, saint_t verbose) {
			  saidx_t C[ALPHABET_SIZE];
			  saidx_t i, p, q, t;
			  saint_t c;

			  if(verbose) { fprintf(stderr, "sufcheck: "); }

			  /* Check arguments. */
			  if((T == NULL) || (SA == index_const_iterator()) || (static_cast<int64_t>(n) < 0)) {
			    if(verbose) { fprintf(stderr, "Invalid arguments.\n"); }
			    return -1;
			  }
			  if(n == 0) {
			    if(verbose) { fprintf(stderr, "Done.\n"); }
			    return 0;
			  }

			  /* check range: [0..n-1] */
			  for(i = 0; i < n; ++i) {
			    if((static_cast<int64_t>(SA[i]) < 0) || (n <= SA[i])) {
			      if(verbose) {
				std::ostringstream ostr;
				ostr << "Out of the range [0,"<< (n-1) <<"].\n"
					<< "  SA[" << i << "]="<<SA[i]<<"\n";
				fprintf(stderr,"%s",ostr.str().c_str());
			      }
			      return -2;
			    }
			  }

			  /* check first characters. */
			  for(i = 1; i < n; ++i) {
			    if(T[SA[i - 1]] > T[SA[i]]) {
			      if(verbose) {
				std::ostringstream ostr;
				ostr << "Suffixes in wrong order.\n"
					<< "T[SA[" << i-1 << "]=" << SA[i-1] << "]=" << T[SA[i-1]]
					<< " > "
					<< "T[SA[" << i << "]=" << SA[i] << "]=" << T[SA[i]]
					<< "\n";
				fprintf(stderr, "%s", ostr.str().c_str());
			      }
			      return -3;
			    }
			  }

			  /* check suffixes. */
			  for(i = 0; i < ALPHABET_SIZE; ++i) { C[i] = 0; }
			  for(i = 0; i < n; ++i) { ++C[T[i]]; }
			  for(i = 0, p = 0; i < ALPHABET_SIZE; ++i) {
			    t = C[i];
			    C[i] = p;
			    p += t;
			  }

			  q = C[T[n - 1]];
			  C[T[n - 1]] += 1;
			  for(i = 0; i < n; ++i) {
			    p = SA[i];
			    if(0 < p) {
			      c = T[--p];
			      t = C[c];
			    } else {
			      c = T[p = n - 1];
			      t = q;
			    }
			    if((t < 0) || (p != SA[t])) {
			      if(verbose) {
				std::ostringstream ostr;
				ostr << "Suffix in wrong position.\n" 
					<< " SA[" << t << "]=" << ((0 <= t) ? SA[t] : -1) << " or\n"
					<< " SA[" << i << "]=" << SA[i] << "\n";
				fprintf(stderr,"%s",ostr.str().c_str());
			      }
			      return -4;
			    }
			    if(t != q) {
			      ++C[c];
			      if((n <= C[c]) || (static_cast<saint_t>(T[SA[C[c]]]) != c)) { C[c] = -1; }
			    }
			  }

			  if(1 <= verbose) { fprintf(stderr, "Done.\n"); }
			  return 0;
			}


			static
			int
			_compare(symbol_const_iterator T, saidx_t Tsize,
				 symbol_const_iterator P, saidx_t Psize,
				 saidx_t suf, saidx_t *match) {
			  saidx_t i, j;
			  saint_t r;
			  for(i = suf + *match, j = *match, r = 0;
			      (i < Tsize) && (j < Psize) && ((r = T[i] - P[j]) == 0); ++i, ++j) { }
			  *match = j;
			  return (r == 0) ? -(j != Psize) : r;
			}

			/* Search for the pattern P in the string T. */
			saidx_t
			sa_search(symbol_const_iterator T, saidx_t Tsize,
				  symbol_const_iterator P, saidx_t Psize,
				  const saidx_t *SA, saidx_t SAsize,
				  saidx_t *idx) {
			  saidx_t size, lsize, rsize, half;
			  saidx_t match, lmatch, rmatch;
			  saidx_t llmatch, lrmatch, rlmatch, rrmatch;
			  saidx_t i, j, k;
			  saint_t r;

			  if(idx != NULL) { *idx = -1; }
			  if((T == NULL) || (P == NULL) || (SA == NULL) ||
			     (Tsize < 0) || (Psize < 0) || (SAsize < 0)) { return -1; }
			  if((Tsize == 0) || (SAsize == 0)) { return 0; }
			  if(Psize == 0) { if(idx != NULL) { *idx = 0; } return SAsize; }

			  for(i = j = k = 0, lmatch = rmatch = 0, size = SAsize, half = size >> 1;
			      0 < size;
			      size = half, half >>= 1) {
			    match = ::std::min(lmatch, rmatch);
			    r = _compare(T, Tsize, P, Psize, SA[i + half], &match);
			    if(r < 0) {
			      i += half + 1;
			      half -= (size & 1) ^ 1;
			      lmatch = match;
			    } else if(r > 0) {
			      rmatch = match;
			    } else {
			      lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

			      /* left part */
			      for(llmatch = lmatch, lrmatch = match, half = lsize >> 1;
				  0 < lsize;
				  lsize = half, half >>= 1) {
				lmatch = ::std::min(llmatch, lrmatch);
				r = _compare(T, Tsize, P, Psize, SA[j + half], &lmatch);
				if(r < 0) {
				  j += half + 1;
				  half -= (lsize & 1) ^ 1;
				  llmatch = lmatch;
				} else {
				  lrmatch = lmatch;
				}
			      }

			      /* right part */
			      for(rlmatch = match, rrmatch = rmatch, half = rsize >> 1;
				  0 < rsize;
				  rsize = half, half >>= 1) {
				rmatch = ::std::min(rlmatch, rrmatch);
				r = _compare(T, Tsize, P, Psize, SA[k + half], &rmatch);
				if(r <= 0) {
				  k += half + 1;
				  half -= (rsize & 1) ^ 1;
				  rlmatch = rmatch;
				} else {
				  rrmatch = rmatch;
				}
			      }

			      break;
			    }
			  }

			  if(idx != NULL) { *idx = (0 < (k - j)) ? j : i; }
			  return k - j;
			}

			/* Search for the character c in the string T. */
			static saidx_t
			sa_simplesearch(symbol_const_iterator T, saidx_t Tsize,
					const saidx_t *SA, saidx_t SAsize,
					saint_t c, saidx_t *idx) {
			  saidx_t size, lsize, rsize, half;
			  saidx_t i, j, k, p;
			  saint_t r;

			  if(idx != NULL) { *idx = -1; }
			  if((T == NULL) || (SA == NULL) || (Tsize < 0) || (SAsize < 0)) { return -1; }
			  if((Tsize == 0) || (SAsize == 0)) { return 0; }

			  for(i = j = k = 0, size = SAsize, half = size >> 1;
			      0 < size;
			      size = half, half >>= 1) {
			    p = SA[i + half];
			    r = (p < Tsize) ? T[p] - c : -1;
			    if(r < 0) {
			      i += half + 1;
			      half -= (size & 1) ^ 1;
			    } else if(r == 0) {
			      lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

			      /* left part */
			      for(half = lsize >> 1;
				  0 < lsize;
				  lsize = half, half >>= 1) {
				p = SA[j + half];
				r = (p < Tsize) ? T[p] - c : -1;
				if(r < 0) {
				  j += half + 1;
				  half -= (lsize & 1) ^ 1;
				}
			      }

			      /* right part */
			      for(half = rsize >> 1;
				  0 < rsize;
				  rsize = half, half >>= 1) {
				p = SA[k + half];
				r = (p < Tsize) ? T[p] - c : -1;
				if(r <= 0) {
				  k += half + 1;
				  half -= (rsize & 1) ^ 1;
				}
			      }

			      break;
			    }
			  }

			  if(idx != NULL) { *idx = (0 < (k - j)) ? j : i; }
			  return k - j;
			}
		};
	}
}
#endif

