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

#include <deque>
#include <libmaus2/wavelet/toWaveletTreeBits.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/bitio/CompactSparseArray.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/bitio/getBit.hpp>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <libmaus2/parallel/OMPLock.hpp>

namespace libmaus2
{
	namespace wavelet
	{
		struct BitLevelComparator
		{
			uint64_t const mask;

			BitLevelComparator(unsigned int rb)
			: mask ( 1ull << rb ) {}

			bool operator()(uint64_t i, uint64_t j)
			{
				return (i&mask) < (j&mask);
			}
		};

		::libmaus2::autoarray::AutoArray<uint64_t> toWaveletTreeBits(::libmaus2::bitio::CompactArray * C, bool const verbose)
		{
			uint64_t const pn = ((C->n + 63) / 64)*64;
			::libmaus2::autoarray::AutoArray<uint64_t> B( pn/64 , false );

			typedef std::pair<uint64_t, uint64_t> qtype;
			std::deque < qtype > Q;
			Q.push_back( qtype(0,C->n) );

			if ( verbose )
				std::cerr << "(Sorting bits...";

			for ( int ib = (C->getB())-1; ib>=0; --ib )
			{
				std::deque < qtype > Q2;
				uint64_t const sb = (C->getB()-ib-1);

				uint64_t const mask = (1ull << ib);
				if ( verbose )
					std::cerr << "(l=" << ib << ")";

				::libmaus2::bitio::CompactSparseArray S(C->D,C->n, C->getB() - sb , sb , C->getB());

				while ( Q.size() )
				{
					uint64_t l = Q.front().first, r = Q.front().second;
					Q.pop_front();

					// std::cerr << "[" << l << "," << r << "]" << std::endl;

					uint64_t ones = 0;
					for ( uint64_t i = l; i < r; ++i )
					{
						uint64_t v = (C->get(i)&mask)>>ib;
						ones += v;
						// std::cerr << v;
						::libmaus2::bitio::putBits(B.get(), i, 1, v);
					}

					uint64_t zeros = (r-l)-ones;

					#if 1
					::libmaus2::bitio::CompactArray CZ(zeros, C->getB() - sb);
					::libmaus2::bitio::CompactArray CO(ones, C->getB() - sb);

					uint64_t zc = 0, oc = 0;
					for ( uint64_t i = l; i < r; ++i )
					{
						uint64_t const v = S.get(i);

						if ( v & mask )
							CO.set ( oc++, v);
						else
							CZ.set ( zc++, v);
					}

					assert ( zc == zeros );
					assert ( oc == ones );

					uint64_t ac = l;
					for ( zc = 0; zc < zeros; ++zc )
						S.set(ac++, CZ.get(zc) );
					for ( oc = 0; oc < ones; ++oc )
						S.set(ac++, CO.get(oc) );

					#else
					::libmaus2::bitio::CompactSparseArrayIterator itA(&S); itA += l;
					::libmaus2::bitio::CompactSparseArrayIterator itB(&S); itB += r;
					if ( verbose )
						std::cerr << "stable sorting...";
					// std::stable_sort ( itA, itB, BitLevelComparator(ib) );
					std::__inplace_stable_sort ( itA, itB, BitLevelComparator(ib) );
					if ( verbose )
						std::cerr << "done." << std::endl;
					#endif

					if ( zeros )
						Q2.push_back ( qtype(l,l+zeros) );
					if ( ones )
						Q2.push_back ( qtype(r-ones,r) );
				}
				// std::cerr << std::endl;

				::libmaus2::bitio::CompactSparseArray W(C->D,C->n, 1, sb , C->getB());
				for ( uint64_t i = 0; i < C->n; ++i )
					W.set ( i , ::libmaus2::bitio::getBits(B.get(), i, 1) );

				Q = Q2;
			}
			if ( verbose )
				std::cerr << "done)";

			B.release();

			#if 0
			for ( unsigned int i = 1; i < C->n; ++i )
				assert ( C->get(i-1) <= C->get(i) );
			#endif

			uint64_t const bb = (C->n * C->getB());
			uint64_t const bb64 = ((bb+63)/64)*64;

			typedef ::libmaus2::rank::ERank222B::writer_type writer_type;
			::libmaus2::autoarray::AutoArray< uint64_t > A(bb64/64,false);
			std::fill(A.get(), A.get()+(bb64/64), 0);
			writer_type AW(A.get());

			if ( verbose )
				std::cerr << "(resorting...";
			for ( int ib = (C->getB())-1; ib>=0; --ib )
			{
				uint64_t const sb = (C->getB()-ib-1);
				::libmaus2::bitio::CompactSparseArray W(C->D,C->n, 1, sb , C->getB());
				for ( uint64_t i = 0; i < C->n; ++i )
				{
					AW.writeBit(W.get(i));
					// std::cerr << W.get(i);
				}
				// std::cerr << std::endl;
			}
			AW.flush();
			if ( verbose )
				std::cerr << "done)";

			return A;
		}


		static unsigned int getMaxThreads()
		{
#if defined(_OPENMP)
			uint64_t const numthreads = omp_get_max_threads();
#else
			uint64_t const numthreads = 1;
#endif
			return numthreads;
		}

		struct CopyBackPacket
		{
			uint64_t h;
			uint64_t low;
			uint64_t high;

			CopyBackPacket() {}
			CopyBackPacket(uint64_t const rh, uint64_t const rlow, uint64_t const rhigh)
			: h(rh), low(rlow), high(rhigh) {}

			uint64_t getHighBitIndex(uint64_t const b) const
			{
				return high * b - 1;
			}
			uint64_t getLowBitIndex(uint64_t const b) const
			{
				return low * b;
			}
			static bool overlap(CopyBackPacket const & A, CopyBackPacket const & B, uint64_t const b)
			{
				uint64_t const highA = A.getHighBitIndex(b);
				uint64_t const lowB = B.getLowBitIndex(b);
				return (highA/64) >= (lowB)/64;
			}
		};

		::libmaus2::autoarray::AutoArray<uint64_t> toWaveletTreeBitsParallel(::libmaus2::bitio::CompactArray * C)
		{
			uint64_t const pn = ((C->n + 63) / 64)*64;
			::libmaus2::autoarray::AutoArray<uint64_t> B( pn/64 , false );
			::libmaus2::parallel::OMPLock block;

			typedef std::pair<uint64_t, uint64_t> qtype;
			std::deque < qtype > Q;
			Q.push_back( qtype(0,C->n) );

			std::cerr << "(Sorting bits...";
			for ( int ib = (C->getB())-1; ib>=0; --ib )
			{
				std::deque < qtype > Q2;
				uint64_t const sb = (C->getB()-ib-1);

				uint64_t const mask = (1ull << ib);
				std::cerr << "(l=" << ib << ")";

				::libmaus2::bitio::CompactSparseArray S(C->D,C->n, C->getB() - sb , sb , C->getB());

				while ( Q.size() )
				{
					uint64_t l = Q.front().first, r = Q.front().second;
					Q.pop_front();

					// std::cerr << "[" << l << "," << r << "]" << std::endl;

					uint64_t const numpackets = getMaxThreads() * 2;
					::libmaus2::autoarray::AutoArray < uint64_t > aones(numpackets+1);
					::libmaus2::autoarray::AutoArray < uint64_t > azeroes(numpackets+1);
					uint64_t const intervalsize = r-l;
					uint64_t const packetsize = ( intervalsize + numpackets - 1 ) / numpackets;

					std::cerr << "(c01/b";
#if defined(_OPENMP)
#pragma omp parallel for schedule(dynamic,1)
#endif
					for ( int64_t h = 0; h < static_cast<int64_t>(numpackets); ++h )
					{
						uint64_t ones = 0;
						uint64_t low = std::min ( l + h * packetsize, r );
						uint64_t const rlow = low;
						uint64_t const high = std::min ( low + packetsize, r );
						uint64_t const low64 = std::min ( ((low+63)/64)*64, high );
						uint64_t const high64 = high & (~(63ull));

						// std::cerr << "low=" << low << " low64=" << low64 << std::endl;

						/**
						 * align low to 64
						 **/
						block.lock();
						for ( ;  low != low64 ; ++low )
						{
							uint64_t const v = (C->get(low)&mask)>>ib;
							ones += v;
							::libmaus2::bitio::putBit(B.get(), low, v);
						}
						block.unlock();

						/**
						 * handle full blocks of 64 values
						 **/
						if ( low != high )
						{
							assert ( low % 64 == 0 );
							assert ( high64 >= low );

							uint64_t * Bptr = B.get() + (low/64);

							while ( low != high64 )
							{
								uint64_t vb = 0;
								uint64_t const lh = low+64;

								for ( ; low != lh ; ++low )
								{
									uint64_t const v = (C->get(low)&mask)>>ib;
									ones += v;
									vb <<= 1;
									vb |= v;
								}

								(*Bptr++) = vb;
							}
						}

						/**
						 * handle rest
						 **/
						block.lock();
						for ( ; (low != high) ; ++low )
						{
							uint64_t const v = (C->get(low)&mask)>>ib;
							ones += v;
							::libmaus2::bitio::putBit(B.get(), low, v);
						}
						block.unlock();

						uint64_t const zeroes = (high-rlow)-ones;

						aones [ h ] = ones;
						azeroes [ h ] = zeroes;
					}

					std::cerr << ")";

					/**
					 * compute prefix sums for zeroes and ones
					 **/
					{
						uint64_t c = 0;

						for ( uint64_t i = 0; i < numpackets + 1; ++i )
						{
							uint64_t const t = aones[i];
							aones[i] = c;
							c += t;
						}
					}
					{
						uint64_t c = 0;

						for ( uint64_t i = 0; i < numpackets + 1; ++i )
						{
							uint64_t const t = azeroes[i];
							azeroes[i] = c;
							c += t;
						}
					}

					uint64_t const ones = aones[numpackets];
					uint64_t const zeros = (r-l)-ones;

					::libmaus2::autoarray::AutoArray < ::libmaus2::bitio::CompactArray::unique_ptr_type > ACZ(numpackets);
					::libmaus2::autoarray::AutoArray < ::libmaus2::bitio::CompactArray::unique_ptr_type > ACO(numpackets);

					std::cerr << "(a";
					for ( uint64_t h = 0; h < numpackets; ++h )
					{
						::libmaus2::bitio::CompactArray::unique_ptr_type tACZ(
							new ::libmaus2::bitio::CompactArray( azeroes [ h+1 ] - azeroes[ h ], C->getB() - sb )
						);
						ACZ[h] = UNIQUE_PTR_MOVE(tACZ);
						::libmaus2::bitio::CompactArray::unique_ptr_type tACO(
							new ::libmaus2::bitio::CompactArray( aones [ h+1 ] - aones[ h ], C->getB() - sb )
						);
						ACO[h] = UNIQUE_PTR_MOVE(tACO);
					}
					std::cerr << ")";

					std::cerr << "(d";
#if defined(_OPENMP)
#pragma omp parallel for schedule(dynamic,1)
#endif
					for ( int64_t h = 0; h < static_cast<int64_t>(numpackets); ++h )
					{
						uint64_t const low = std::min ( l + h * packetsize, r );
						uint64_t const high = std::min ( low + packetsize, r );
						uint64_t zp = 0;
						uint64_t op = 0;

						::libmaus2::bitio::CompactArray & CO = *ACO[h];
						::libmaus2::bitio::CompactArray & CZ = *ACZ[h];

						for ( uint64_t i = low; i != high; ++i )
						{
							uint64_t const v = S.get(i);

							if ( v & mask )
								CO.set ( op++, v);
							else
								CZ.set ( zp++, v);
						}

						assert ( zp == azeroes[h+1]-azeroes[h] );
						assert ( op == aones[h+1]-aones[h] );
					}
					std::cerr << ")";

					std::vector < CopyBackPacket > zpacketstodo;
					for ( int64_t h = 0; h < static_cast<int64_t>(numpackets); ++h )
					{
						uint64_t const low = l + azeroes[h];
						uint64_t const high = low + (azeroes[h+1]-azeroes[h]);

						if ( high-low )
							zpacketstodo.push_back ( CopyBackPacket(h,low,high) );
					}
					std::vector < CopyBackPacket > opacketstodo;
					for ( int64_t h = 0; h < static_cast<int64_t>(numpackets); ++h )
					{
						uint64_t const low = l + azeroes[numpackets ] + aones[h];
						uint64_t const high = low + (aones[h+1]-aones[h]);

						if ( high-low )
							opacketstodo.push_back ( CopyBackPacket(h,low,high) );
					}

					std::vector < std::vector < CopyBackPacket > > zpackets;
					while ( zpacketstodo.size() )
					{
						std::vector < CopyBackPacket > zpacketsnewtodo;

						std::vector < CopyBackPacket > nlist;
						nlist.push_back(zpacketstodo.front());

						for ( uint64_t i = 1; i < zpacketstodo.size(); ++i )
							if ( CopyBackPacket::overlap(nlist.back(), zpacketstodo[i], C->getB()) )
								zpacketsnewtodo.push_back(zpacketstodo[i]);
							else
								nlist.push_back(zpacketstodo[i]);

						zpackets.push_back(nlist);

						zpacketstodo = zpacketsnewtodo;
					}

					std::vector < std::vector < CopyBackPacket > > opackets;
					while ( opacketstodo.size() )
					{
						std::vector < CopyBackPacket > opacketsnewtodo;

						std::vector < CopyBackPacket > nlist;
						nlist.push_back(opacketstodo.front());

						for ( uint64_t i = 1; i < opacketstodo.size(); ++i )
							if ( CopyBackPacket::overlap(nlist.back(), opacketstodo[i], C->getB()) )
								opacketsnewtodo.push_back(opacketstodo[i]);
							else
								nlist.push_back(opacketstodo[i]);

						opackets.push_back(nlist);

						opacketstodo = opacketsnewtodo;
					}

					// std::cerr << "zpackets: " << zpackets.size() << " opackets: " << opackets.size() << std::endl;

					std::cerr << "(cb";
					for ( uint64_t q = 0; q < zpackets.size(); ++q )
#if defined(_OPENMP)
#pragma omp parallel for schedule(dynamic,1)
#endif
						for ( int64_t j = 0; j < static_cast<int64_t>(zpackets[q].size()); ++j )
						{
							CopyBackPacket const CBP = zpackets[q][j];
							uint64_t ac = CBP.low;
							::libmaus2::bitio::CompactArray & CZ = *ACZ[CBP.h];

							for ( uint64_t zc = 0 ; zc != CBP.high-CBP.low; ++zc )
								S.set ( ac++ , CZ.get(zc) );
						}

					for ( uint64_t q = 0; q < opackets.size(); ++q )
#if defined(_OPENMP)
#pragma omp parallel for schedule(dynamic,1)
#endif
						for ( int64_t j = 0; j < static_cast<int64_t>(opackets[q].size()); ++j )
						{
							CopyBackPacket const CBP = opackets[q][j];
							uint64_t ac = CBP.low;
							::libmaus2::bitio::CompactArray & CO = *ACO[CBP.h];

							for ( uint64_t oc = 0 ; oc != CBP.high-CBP.low; ++oc )
								S.set ( ac++ , CO.get(oc) );
						}
					std::cerr << ")";

					if ( zeros )
						Q2.push_back ( qtype(l,l+zeros) );
					if ( ones )
						Q2.push_back ( qtype(r-ones,r) );
				}
				// std::cerr << std::endl;

				uint64_t const numpackets = getMaxThreads() * 2;
				uint64_t const intervalsize = C->n;
				uint64_t const packetsize = ( intervalsize + numpackets - 1 ) / numpackets;

				std::vector < CopyBackPacket > packetstodo;
				for ( int64_t h = 0; h < static_cast<int64_t>(numpackets); ++h )
				{
					uint64_t const low = std::min(h*packetsize,C->n);
					uint64_t const high = std::min(low+packetsize,C->n);

					if ( high-low )
						packetstodo.push_back ( CopyBackPacket(h,low,high) );
				}
				std::vector < std::vector < CopyBackPacket > > packets;
				while ( packetstodo.size() )
				{
					std::vector < CopyBackPacket > packetsnewtodo;

					std::vector < CopyBackPacket > nlist;
					nlist.push_back(packetstodo.front());

					for ( uint64_t i = 1; i < packetstodo.size(); ++i )
						if ( CopyBackPacket::overlap(nlist.back(), packetstodo[i], C->getB()) )
							packetsnewtodo.push_back(packetstodo[i]);
						else
							nlist.push_back(packetstodo[i]);

					packets.push_back(nlist);

					packetstodo = packetsnewtodo;
				}

				for ( uint64_t q = 0; q < packets.size(); ++q )
#if defined(_OPENMP)
#pragma omp parallel for schedule(dynamic,1)
#endif
					for ( int64_t h = 0; h < static_cast<int64_t>(packets[q].size()); ++h )
					{
						CopyBackPacket const CBP = packets[q][h];

						for ( uint64_t i = CBP.low; i < CBP.high; ++i )
							::libmaus2::bitio::putBit ( C->D , i*C->getB() + sb , ::libmaus2::bitio::getBit(B.get(), i) );
					}

				Q = Q2;
			}
			std::cerr << "done)";

			B.release();

			uint64_t const bb = (C->n * C->getB());
			uint64_t const bb64 = ((bb+63)/64)*64;

			typedef ::libmaus2::rank::ERank222B::writer_type writer_type;
			::libmaus2::autoarray::AutoArray< uint64_t > A(bb64/64,false);
			std::fill(A.get(), A.get()+(bb64/64), 0);
			writer_type AW(A.get());

			std::cerr << "(resorting...";
			for ( int ib = (C->getB())-1; ib>=0; --ib )
			{
				uint64_t const sb = (C->getB()-ib-1);
				::libmaus2::bitio::CompactSparseArray W(C->D,C->n, 1, sb , C->getB());
				for ( uint64_t i = 0; i < C->n; ++i )
				{
					AW.writeBit(W.get(i));
					// std::cerr << W.get(i);
				}
				// std::cerr << std::endl;
			}
			AW.flush();
			std::cerr << "done)";

			return A;
		}
	}
}
