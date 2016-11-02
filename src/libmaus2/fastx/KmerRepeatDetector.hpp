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
#if ! defined(LIBMAUS2_FASTX_KMERREPEATDETECTOR_HPP)
#define LIBMAUS2_FASTX_KMERREPEATDETECTOR_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/math/lowbits.hpp>

namespace libmaus2
{
	namespace fastx
	{
		/**
		 * detector for repeating kmers in a string
		 **/
		struct KmerRepeatDetector
		{
			unsigned int const k;
			uint64_t const kmask;
			libmaus2::autoarray::AutoArray<uint16_t> A;
			libmaus2::autoarray::AutoArray<uint16_t> B;
			libmaus2::autoarray::AutoArray<uint64_t> R;
			libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > P;
			uint64_t ro;
			uint64_t po;

			// constructor by kmer length
			KmerRepeatDetector(unsigned int const rk) : k(rk), kmask(libmaus2::math::lowbits(2*k)), A(1ull << (2*k),false), B(1ull << (2*k),false), ro(0), po(0)
			{
				std::fill(A.begin(),A.end(),0ull);
				std::fill(B.begin(),B.end(),0ull);
			}

			void printRepeats() const
			{
				uint64_t plow = 0;
				while ( plow < po )
				{
					uint64_t phigh = plow+1;
					while ( phigh < po && P[plow].first == P[phigh].first )
						++phigh;

					uint64_t const v = P[plow].first;

					std::cerr << "KmerRepeatDetector repeating ";
					for ( unsigned int i = 0; i < k; ++i )
						std::cerr << libmaus2::fastx::remapChar((v >> (k-i-1)*2) & 3);
					std::cerr << " " << phigh-plow << " ";
					for ( uint64_t i = plow; i < phigh; ++i )
						std::cerr << P[i].second << ";";
					std::cerr << std::endl;

					plow = phigh;
				}
			}

			template<typename iterator>
			bool detect(iterator const it, uint64_t const n)
			{
				ro = 0;
				po = 0;

				if ( n >= k )
				{
					// iterator after k-1 symbols
					iterator const it_first = it + (k-1);
					// iterator after n symbols
					iterator const it_end = it + n;

					// current kmer value
					uint64_t v;
					// current iterator
					iterator it_current;

					// collect first k-1 symbols
					it_current = it;
					v = 0;
					while ( it_current != it_first )
					{
						v <<= 2;
						v |= libmaus2::fastx::mapChar(*(it_current++));
					}
					// compute kmers
					while ( it_current != it_end )
					{
						v <<= 2;
						v &= kmask;
						v |= libmaus2::fastx::mapChar(*(it_current++));
						// add to frequency of v
						A [ v ]++;
					}

					// reset
					it_current = it;
					// recompute first k-1 symbols
					v = 0;
					while ( it_current != it_first )
					{
						v <<= 2;
						v |= libmaus2::fastx::mapChar(*(it_current++));
					}
					// reiterate over k-mers
					while ( it_current != it_end )
					{
						v <<= 2;
						v &= kmask;
						v |= libmaus2::fastx::mapChar(*(it_current++));

						// note every kmer occuring more than once
						if ( A[v] > 1 )
						{
							// push value
							R.push(ro,v);
							// copy frequency to B
							B[v] = A[v];
							// erase frequency in A
							A[v] = 0;
						}
					}

					// if any repeating kmers were found
					if ( ro )
					{
						// for repeating kmers
						std::sort(R.begin(),R.begin()+ro);

						// recompute first k-1
						it_current = it;
						v = 0;
						while ( it_current != it_first )
						{
							v <<= 2;
							v |= libmaus2::fastx::mapChar(*(it_current++));
						}
						// compute positions of repeating kmers
						while ( it_current != it_end )
						{
							v <<= 2;
							v &= kmask;
							v |= libmaus2::fastx::mapChar(*(it_current++));
							if ( B[v] )
								P.push(po,std::pair<uint64_t,uint64_t>(v,(it_current-it)-k));
						}

						// sort repeating kmers by (value,position)
						std::sort(P.begin(),P.begin()+po);

						// erase B array
						it_current = it;
						v = 0;
						while ( it_current != it_first )
						{
							v <<= 2;
							v |= libmaus2::fastx::mapChar(*(it_current++));
						}
						while ( it_current != it_end )
						{
							v <<= 2;
							v &= kmask;
							v |= libmaus2::fastx::mapChar(*(it_current++));
							B[v] = 0;
						}
					}

					// erase A array
					it_current = it;
					v = 0;
					while ( it_current != it_first )
					{
						v <<= 2;
						v |= libmaus2::fastx::mapChar(*(it_current++));
					}
					while ( it_current != it_end )
					{
						v <<= 2;
						v &= kmask;
						v |= libmaus2::fastx::mapChar(*(it_current++));
						A [ v ] = 0;
					}
				}

				// return true if string contains repeating kmers
				return po != 0;
			}
		};
	}
}
#endif
