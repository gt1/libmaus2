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
#if ! defined(LIBMAUS2_FASTX_KMERFREQINWINDOWSTATS_HPP)
#define LIBMAUS2_FASTX_KMERFREQINWINDOWSTATS_HPP

#include <map>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <cassert>

namespace libmaus2
{
	namespace fastx
	{
		struct KmerFreqInWindowStats
		{
			static uint64_t getKmer(char const * t, unsigned int const k)
			{
				uint64_t v = 0;
				for ( uint64_t i = 0; i < k; ++i )
				{
					v <<= 2;
					v |= libmaus2::fastx::mapChar(t[i]);
				}

				return v;
			}

			static std::map < uint64_t, uint64_t > kmerCopiesInWindowFreq(
				std::string const & text,
				uint64_t const windowsize,
				uint64_t const it,
				uint64_t const k
			)
			{
				uint64_t const n = text.size();

				uint64_t const numwindowleftpos = n - windowsize + 1;
				unsigned int const numk = windowsize-k+1;

				assert ( n >= windowsize );

				std::map < uint64_t,uint64_t> C;
				uint64_t csum = 0;

				for ( uint64_t i = 0; i < it; ++i )
				{
					uint64_t const windowleft = libmaus2::random::Random::rand64() % (numwindowleftpos);
					uint64_t const inwindow = libmaus2::random::Random::rand64() % numk;

					char const * window = text.c_str() + windowleft;

					bool ok = true;
					for ( uint64_t j = 0; j < windowsize; ++j )
						if ( libmaus2::fastx::mapChar(window[j]) > 3 )
							ok = false;

					if ( ! ok )
						continue;

					uint64_t const refkmer = getKmer(window+inwindow,k);
					uint64_t c = 0;
					for ( uint64_t j = 0; j < numk; ++j )
						if ( getKmer(window+j,k) == refkmer )
							++c;

					assert ( c );

					C[c]++;
					csum += 1;

					// std::cerr << "windowleft=" << windowleft << " inwindow=" << inwindow << " c=" << c << std::endl;
				}

				return C;
			}

			static std::map < uint64_t, double > kmerCopiesInWindow(
				std::string const & text,
				uint64_t const windowsize,
				uint64_t const it,
				uint64_t const k
			)
			{
				std::map < uint64_t, uint64_t > const M = kmerCopiesInWindowFreq(text,windowsize,it,k);
				uint64_t c = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator itc = M.begin(); itc != M.end(); ++itc )
					c += itc->second;
				double const dc = c;
				std::map < uint64_t, double > DM;
				for ( std::map<uint64_t,uint64_t>::const_iterator itc = M.begin(); itc != M.end(); ++itc )
					DM[itc->first] = itc->second / dc;
				return DM;
			}
		};
	}
}
#endif
