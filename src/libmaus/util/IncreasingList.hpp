/**
    libmaus
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
**/

#if ! defined(INCREASINGLIST_HPP)
#define INCREASINGLIST_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/CompactArray.hpp>
#include <libmaus/random/Random.hpp>
#include <libmaus/rank/ERank222B.hpp>

namespace libmaus
{
        namespace util
        {
                /**
                 * data structure for storing a sorted sequence
                 * of n unsigned integers over [0,U-1] in space
                 * 2n + n log(U/n) + o(n) bits
                 * insert the n elements using put in any order,
                 * then call setup
                 *
                 * (cf. Hon et al. Breaking a Time-and-Space barrier in Constructing Full-Text Indices)
                 **/
                struct IncreasingList
                {
                        public:
                        typedef IncreasingList this_type;
                        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
                
                        private:
                        uint64_t const n;
                        uint64_t const b;
                        uint64_t const m;
                        // lower bits
                        ::libmaus::bitio::CompactArray C;
                        // upper bitstream
                        ::libmaus::autoarray::AutoArray < uint64_t > Bup;
                        // select dictionary for upper bitstream
                        ::libmaus::rank::ERank222B::unique_ptr_type R;
                        
                        public:
                        uint64_t byteSize() const
                        {
                                return 6*sizeof(uint64_t) + Bup.byteSize() + C.byteSize();
                        }
                        
                        void put(uint64_t const i, uint64_t const v)
                        {
                                C.set(i,v & m);
                                ::libmaus::bitio::putBit(Bup.get(), i + (v >> b), true );
                        }
                        
                        void setup()
                        {
                                R = UNIQUE_PTR_MOVE(::libmaus::rank::ERank222B::unique_ptr_type(new ::libmaus::rank::ERank222B(Bup.get(),Bup.size()*64)));
                        }

                        uint64_t get(uint64_t const i) const
                        {
                                return ((R->select1(i)-i)<<b)  + C.get(i);
                        }

                        uint64_t operator[](uint64_t const i) const
                        {
                                return get(i);
                        }

                        #if 0
                        IncreasingList(uint64_t const rn, uint64_t const rU)
                        : n(rn), n2(2*n), n264( ((n2 + 63)/64)*64 ), Bup(n264/64), U(rU),
                          b ( ::libmaus::math::numbits( n ? ((U+n-1)/n) : 0 ) ), m(::libmaus::math::lowbits(b)), C(n,b)
                        {
                                if ( n >= 16*1024 )
                                        std::cerr << "U=" << U << " n=" << n << " b=" << b << " m=" << m << std::endl;
                        }
                        #endif

                        IncreasingList(uint64_t const rn, uint64_t const rb)
                        : n(rn), b(rb), m(::libmaus::math::lowbits(b)), C(n,b), 
                          Bup( (((n*m) >> b) + n + 63) / 64 )
                        {
                        }
                        
                        static void test(std::vector<uint64_t> const & W)
                        {
                                if ( W.size() )
                                {
                                        std::vector<uint64_t> V = W;
                                        std::sort(V.begin(),V.end());
                                        uint64_t const U = V.back() + 1;
                                        IncreasingList IL(V.size(),U);
                                        
                                        for ( uint64_t i = 0; i < V.size(); ++i )
                                                IL.put(i,V[i]);
                                        
                                        IL.setup();
                                        
                                        for ( uint64_t i = 0; i < V.size(); ++i )
                                        {
                                                assert ( V[i] == IL[i] );
                                                // std::cerr << V[i] << "\t" << IL[i] << std::endl;
                                        }
                                }
                        }
                        
                        static void testRandom(uint64_t const n, uint64_t const k)
                        {
                                std::vector < uint64_t > V(n);
                                for ( uint64_t i = 0; i < n; ++i )
                                        V[i] = ::libmaus::random::Random::rand64() % k;
                                test(V);
                        }
                };
        }
}
#endif
