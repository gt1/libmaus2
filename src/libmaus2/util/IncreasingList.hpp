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

#if ! defined(INCREASINGLIST_HPP)
#define INCREASINGLIST_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/bitio/putBit.hpp>

namespace libmaus2
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
                        typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                
                        private:
                        uint64_t const n;
                        uint64_t const b;
                        uint64_t const m;
                        // lower bits
                        ::libmaus2::bitio::CompactArray C;
                        // upper bitstream
                        ::libmaus2::autoarray::AutoArray < uint64_t > Bup;
                        // select dictionary for upper bitstream
                        ::libmaus2::rank::ERank222B::unique_ptr_type R;
                        
                        public:

                        uint64_t get(uint64_t const i) const
                        {
                                return ((R->select1(i)-i)<<b)  + C.get(i);
                        }

                        uint64_t operator[](uint64_t const i) const
                        {
                                return get(i);
                        }

                        void put(uint64_t const i, uint64_t const v)
                        {
                                C.set(i,v & m);
                                ::libmaus2::bitio::putBit(Bup.get(), i + (v >> b), true );
                        }

                        uint64_t byteSize() const;
                        void setup();

                        IncreasingList(uint64_t const rn, uint64_t const rb);

                        static void test(std::vector<uint64_t> const & W);
                        static void testRandom(uint64_t const n, uint64_t const k);
                };
        }
}
#endif
