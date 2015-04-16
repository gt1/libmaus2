/**
    libmaus
    Copyright (C) 2010 Johannes Fischer
    Copyright (C) 2010-2013 German Tischler
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
**/

/* This file contains a slighly modified version of Johannes Fischer's
 * succinct RMQ code. Original code by Johannes Fischer, added bugs
 * by German Tischler
 */

#if ! defined(FISCHER_SYSTEMATIC_SUCCINCT_RMQ_HPP)
#define FISCHER_SYSTEMATIC_SUCCINCT_RMQ_HPP

#include <cstdlib>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace rmq
	{
		struct FischerSystematicSuccinctRMQBase
		{
			public:
			typedef uint32_t DTidx;     // for indexing in arrays
			typedef uint8_t DTsucc;
			typedef uint16_t DTsucc2;

			protected:
			// stuff for clearing the least significant x bits (change for 64-bit computing)
			static const DTsucc HighestBitsSet[8];

			// Least Significant Bits for 8-bit-numbers:
			static const char LSBTable256[256];

			// the following stuff is for fast base 2 logarithms:
			// (currently only implemented for 32 bit numbers)
			static const char LogTable256[256];

			static DTidx lsb(DTsucc v) {
				return LSBTable256[v];
			}

			static DTidx log2fast(DTidx v) {
				DTidx c = 0;          // c will be lg(v)
				DTidx t, tt; // temporaries

				if ( (tt = (v >> 16)) )
					c = (t = v >> 24) ? 24 + LogTable256[t] : 16 + LogTable256[tt & 0xFF];
				else 
					c = (t = v >> 8) ? 8 + LogTable256[t] : LogTable256[v];
				return c;
			}
			static DTsucc clearbits(DTsucc n, DTidx x)
			{
				return n & HighestBitsSet[x];
			}

			// precomputed Catalan triangle (17 is enough for 64bit computing):
			static const DTidx Catalan[17][17];
		};

		template<typename array_iterator>
		struct FischerSystematicSuccinctRMQ : public FischerSystematicSuccinctRMQBase
		{
			public:
			typedef FischerSystematicSuccinctRMQ<array_iterator> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			typedef typename std::iterator_traits<array_iterator>::value_type DT;
			
			// microblock size
			static const unsigned int sbits = 3;
			static const DTidx s = 1<<sbits;

			// block size
			static const unsigned int sprimebits = 4;
			static const DTidx sprime = 1 << sprimebits;

			// superblock size
			static const unsigned int sprimeprimebits = 8;
			static const DTidx sprimeprime = 1 << sprimeprimebits;

			// depth of table M:
			static const DTidx M_depth = sprimeprimebits - sprimebits;

			// array
			array_iterator const a;

			// size of array a
			DTidx const n;

			// number of blocks (always n/sprime)
			DTidx const nb;

			// number of superblocks (always n/sprimeprime)
			DTidx const nsb;

			// number of microblocks (always n/s)
			DTidx const nmb;

			// type of blocks
			::libmaus::autoarray::AutoArray<DTsucc2> type;

			// precomputed in-block queries
			::libmaus::autoarray::AutoArray<DTsucc> APrec;
			::libmaus::autoarray::AutoArray<DTsucc *> Prec;
			
			// table M for the out-of-block queries (contains indices of block-minima)
			::libmaus::autoarray::AutoArray<DTsucc> AM;
			::libmaus::autoarray::AutoArray<DTsucc *> M;

			// depth of table M':
			DTidx const Mprime_depth;

			// table M' for superblock-queries (contains indices of block-minima)
			::libmaus::autoarray::AutoArray<DTidx> AMprime;
			::libmaus::autoarray::AutoArray<DTidx*> Mprime;
			
			inline static DTidx checkNb(DTidx nb, DTidx n)
			{
				// The following is necessary because we've fixed s, s' and s'' according to the computer's
				// word size and NOT according to the input size. This may cause the (super-)block-size
				// to be too big, or, in other words, the array too small. If this code is compiled on
				// a 32-bit computer, this happens iff n < 113. For such small instances it isn't 
				// advisable anyway to use this data structure, because simpler methods are faster and 
				// less space consuming.
				if (nb<sprimeprime/(2*sprime)) 
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Array of size " << n << " too small in FischerSystematicSuccinctRMQ.";
					se.finish();
					throw se;
				}
				else
				{
					return nb;			
				}
			}
			
			inline static ::libmaus::autoarray::AutoArray<DTsucc *> allocatePrec(::libmaus::autoarray::AutoArray<DTsucc> & APrec)
			{
				::libmaus::autoarray::AutoArray<DTsucc *> Prec(Catalan[s][s]);

				for (DTidx i = 0; i < Catalan[s][s]; i++) {
					Prec[i] = APrec.get() + i*s;
					Prec[i][0] = 1; // init with impossible value
				}

				return Prec;
			}

			inline static ::libmaus::autoarray::AutoArray<DTsucc *> allocateM(::libmaus::autoarray::AutoArray<DTsucc> & AM, DTidx nb)
			{
				::libmaus::autoarray::AutoArray<DTsucc *> M(M_depth);

				for (DTidx i = 0; i < M_depth; i++) {
					M[i] = AM.get() + i*nb;
				}

				return M;
			}

			inline static ::libmaus::autoarray::AutoArray<DTidx *> allocateMprime(::libmaus::autoarray::AutoArray<DTidx> & AMprime, DTidx Mprime_depth, DTidx nsb)
			{
				::libmaus::autoarray::AutoArray<DTidx *> Mprime(Mprime_depth);

				for (DTidx i = 0; i < Mprime_depth; i++) {
					Mprime[i] = AMprime.get() + i*nsb;
				}

				return Mprime;
			}
			
			inline static int ilog2floor(DTidx n)
			{
				int c = 0;
				
				while ( n )
				{
					n >>= 1;
					c++;
				}
				
				return c-1;
			}
			
			// return microblock-number of entry i:
			inline static DTidx microblock(DTidx i) { return i/s; }

			// return block-number of entry i:
			inline static DTidx block(DTidx i) { return i/sprime; }

			// return superblock-number of entry i:
			inline static DTidx superblock(DTidx i) { return i/sprimeprime; }
			
			// because M just stores offsets (rel. to start of block), this method
			// re-calculates the true index:
			inline DTidx m(DTidx k, DTidx block) const { return M[k][block]+(block*sprime); }

			public:
			/**
			 * Standard Constructor. a is the array to be prepared for RMQ.
			 * n is the size of the array.
			 */
			FischerSystematicSuccinctRMQ(array_iterator const ra, DTidx rn) 
			: a(ra), n(rn), 
				nb( checkNb(std::max(((n+sprime-1)/sprime),static_cast<DTidx>(1)),n) ), 
				nsb( std::max((n+sprimeprime-1)/sprimeprime,static_cast<DTidx>(1)) ), 
				nmb( std::max( (n+s-1)/s,static_cast<DTidx>(1)) ),
				type ( nmb ),
				APrec ( s * Catalan[s][s] ),
				Prec ( allocatePrec(APrec) ),
				AM ( M_depth * nb ),
				M ( allocateM(AM,nb) ),
				Mprime_depth ( ilog2floor(nsb) + 1 ),
				AMprime ( Mprime_depth * nsb ),
				Mprime ( allocateMprime(AMprime,Mprime_depth,nsb) )
			{
				// Type-calculation for the microblocks and pre-computation of in-microblock-queries:
				DT rp[s+1];   		// rp: rightmost path in Cart. tree
				DTidx z = 0;            // index in array a
				DTidx start;            // start of current block
				DTidx end;              // end of current block
				DTidx q;                // position in Catalan triangle
				DTidx p;                // --------- " ----------------

				// prec[i]: the jth bit is 1 iff j is 1. pos. to the left of i where a[j] < a[i] 
				DTidx gstack[s];
				DTidx gstacksize;
				DTidx g; // first position to the left of i where a[g[i]] < a[i]

				for (DTidx i = 0; i < nmb; i++) { // step through microblocks
					start = z;            // init start
					end = start + s;      // end of block (not inclusive!)
					if (end > n) end = n; // last block could be smaller than s!

					// compute block type as in Fischer/Heun CPM'06:
					q = s;        // init q
					p = s-1;      // init p
					type[i] = 0;  // init type (will be increased!)
					rp[1] = a[z]; // init rightmost path

					while (++z < end) {   // step through current block:
						p--;
						while ( (q-p-1>0) && (rp[q-p-1] > a[z]) ) {
							type[i] += Catalan[p][q]; // update type
							q--;
						}
						rp[q-p] = a[z]; // add last element to rightmost path
					}

					// precompute in-block-queries for this microblock (if necessary)
					// as in Alstrup et al. SPAA'02:
					if (Prec[type[i]][0] == 1) {
						Prec[type[i]][0] = 0;
						gstacksize = 0;
						for (DTidx j = start; j < end; j++) {
							while(gstacksize > 0 && (a[j] < a[gstack[gstacksize-1]])) {
								gstacksize--;
							}
							if(gstacksize > 0) {
								g = gstack[gstacksize-1];
								Prec[type[i]][j-start] = Prec[type[i]][g-start] | (1 << (g % s));
							}
							else Prec[type[i]][j-start] = 0;
							gstack[gstacksize++] = j;
						}
					}
				}

				// out-of-block- and out-of-superblock-queries:
				
				// fill 0'th rows of M and Mprime:
				z = 0; // minimum in current block
				q = 0; // pos. of min in current superblock
				g = 0; // number of current superblock
				for (DTidx i = 0; i < nb; i++) { // step through blocks
					start = z;              // init start
					p = start;              // init minimum
					end = start + sprime;   // end of block (not inclusive!)
					if (end > n) end = n;   // last block could be smaller than sprime!
					if (a[z] < a[q]) q = z; // update minimum in superblock

					while (++z < end) { // step through current block:
						if (a[z] < a[p]) p = z; // update minimum in block
						if (a[z] < a[q]) q = z; // update minimum in superblock
					}
					M[0][i] = p-start;                     // store index of block-minimum (offset!)
					if (z % sprimeprime == 0 || z == n) {  // reached end of superblock?
						Mprime[0][g++] = q;               // store index of superblock-minimum
						q = z;
					}
				}

				// fill M:
				DTidx dist = 1; // always 2^(j-1)
				for (DTidx j = 1; j < M_depth; j++) {
					for (DTidx i = 0; i < nb - dist; i++) { // be careful: loop may go too far
						M[j][i] = a[m(j-1, i)] <= a[m(j-1,i+dist)] ?
							M[j-1][i] : M[j-1][i+dist] + (dist*sprime); // add 'skipped' elements in a
					}
					for (DTidx i = nb - dist; i < nb; i++) M[j][i] = M[j-1][i]; // fill overhang
					dist *= 2;
				}

				// fill M':
				dist = 1; // always 2^(j-1)
				for (DTidx j = 1; j < Mprime_depth; j++) {
					for (DTidx i = 0; i < nsb - dist; i++) {
						Mprime[j][i] = a[Mprime[j-1][i]] <= a[Mprime[j-1][i+dist]] ?
							Mprime[j-1][i] : Mprime[j-1][i+dist];
					}
					for (DTidx i = nsb - dist; i < nsb; i++) Mprime[j][i] = Mprime[j-1][i]; // overhang
					dist *= 2;
				}
			}

			// liefert RMQ[i,j]
			DTidx operator()(DTidx i, DTidx j) const {
				DTidx mb_i = microblock(i);     // i's microblock
				DTidx mb_j = microblock(j);     // j's microblock
				DTidx min, min_i, min_j; 		// min: to be returned
				DTidx s_mi = mb_i * s;           // start of i's microblock
				DTidx i_pos = i - s_mi;          // pos. of i in its microblock

				if (mb_i == mb_j) { // only one microblock-query
					min_i = clearbits(Prec[type[mb_i]][j-s_mi], i_pos);
					min = min_i == 0 ? j : s_mi + lsb(min_i);
				}
				else {
					DTidx b_i = block(i);      // i's block
					DTidx b_j = block(j);      // j's block
					DTidx s_mj = mb_j * s;     // start of j's microblock
					DTidx j_pos = j - s_mj;    // position of j in its microblock
					min_i = clearbits(Prec[type[mb_i]][s-1], i_pos);
					min = min_i == 0 ? s_mi + s - 1 : s_mi + lsb(min_i); // left in-microblock-query
					min_j = Prec[type[mb_j]][j_pos] == 0 ?
						j : s_mj + lsb(Prec[type[mb_j]][j_pos]);         // right in-microblock-query
					if (a[min_j] < a[min]) min = min_j;

					if (mb_j > mb_i + 1) { // otherwise we're done!
						DTidx s_bi = b_i * sprime;      // start of block i
						DTidx s_bj = b_j * sprime;      // start of block j
						if (s_bi+s > i) { // do another microblock-query!
							mb_i++; // go one microblock to the right
							min_i = Prec[type[mb_i]][s-1] == 0 ?
								s_bi + sprime - 1 : s_mi + s + lsb(Prec[type[mb_i]][s-1]); // right in-block-query
							if (a[min_i] < a[min]) min = min_i;
						}
						if (j >= s_bj+s) { // and yet another microblock-query!
							mb_j--; // go one microblock to the left
							min_j = Prec[type[mb_j]][s-1] == 0 ?
								s_mj - 1 : s_bj + lsb(Prec[type[mb_j]][s-1]); // right in-block-query
							if (a[min_j] < a[min]) min = min_j;
						}

						DTidx block_difference = b_j - b_i;
						if (block_difference > 1) { // otherwise we're done!
							DTidx k, twotothek, block_tmp;  // for index calculations in M and M'
							b_i++; // block where out-of-block-query starts
							if (s_bj - s_bi - sprime <= sprimeprime) { // just one out-of-block-query
								k = log2fast(block_difference - 2);
								twotothek = 1 << k; // 2^k
								i = m(k, b_i); j = m(k, b_j-twotothek);
								min_i = a[i] <= a[j] ? i : j;
							}
							else { // here we have to answer a superblock-query:
								DTidx sb_i = superblock(i); // i's superblock
								DTidx sb_j = superblock(j); // j's superblock

								block_tmp = block((sb_i+1)*sprimeprime); // end of left out-of-block-query
								k = log2fast(block_tmp - b_i);
								twotothek = 1 << k; // 2^k
								i = m(k, b_i); j = m(k, block_tmp+1-twotothek);
								min_i = a[i] <= a[j] ? i : j;

								block_tmp = block(sb_j*sprimeprime); // start of right out-of-block-query
								k = log2fast(b_j - block_tmp);
								twotothek = 1 << k; // 2^k
								block_tmp--; // going one block to the left doesn't harm and saves some tests
								i = m(k, block_tmp); j = m(k, b_j-twotothek);
								min_j = a[i] <= a[j] ? i : j;

								if (a[min_j] < a[min_i]) min_i = min_j;

								if (sb_j > sb_i + 1) { // finally, the superblock-query:
									k = log2fast(sb_j - sb_i - 2);
									twotothek = 1 << k;
									i = Mprime[k][sb_i+1]; j = Mprime[k][sb_j-twotothek];
									min_j = a[i] <= a[j] ? i : j;
									if (a[min_j] < a[min_i]) min_i = min_j; // does NOT always return leftmost min!!!
								}

							}
							if (a[min_i] < a[min]) min = min_i; // does NOT always return leftmost min!!!
						}
					}
				}

				return min;
			}
		};
	}
}
#endif

