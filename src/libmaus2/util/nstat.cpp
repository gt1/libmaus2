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

#include <libmaus2/util/nstat.hpp>

/**
 * compute n staticic use ta = 50, b = 100 for n50
 *
 * @param ta numerator
 * @param b denominator
 * @param nval reference for storing n value
 * @param avg reference for storing average value
 **/
void libmaus2::util::NStat::nstat(
	std::vector<uint64_t> const & clens,
	uint64_t const ta,
	uint64_t const b,
	double & nval,
	double & avg
	)
{
	// no data?
	if ( clens.size() == 0 || (std::accumulate(clens.begin(),clens.end(),0ull) == 0) )
	{
		nval = avg = 0;
	}
	// only one contig?
	else if ( clens.size() == 1 )
	{
		nval = avg = clens[0];
	}
	else
	{
		uint64_t const a = b-ta;
		if ( a > b )
		{
			::libmaus2::exception::LibMausException se;
			se.getStream() << "Parameter error.";
			se.finish();
			throw se;
		}

		// total bases in contigs
		uint64_t total = 0;
		for ( uint64_t i = 0; i < clens.size(); ++i )
			total += clens[i];

		uint64_t nl = 0, nr = 0;

		if (
			(((total-1) * a) % b == 0)
			||
			(a==b)
		)
		{
			uint64_t const medidx = (a!=b)? ((total-1)*a/b):(total-1);

			uint64_t low = 0;
			for ( uint64_t i = 0; i < clens.size(); ++i )
			{
				uint64_t const high = low + clens[i];

				if ( (low <= medidx) && (medidx < high) )
					nl = nr = clens[i];

				low = high;
			}
		}
		else
		{
			uint64_t const i0 = (total*a)/b-1;
			uint64_t const i1 = (total*a)/b;

			uint64_t low = 0;

			for ( uint64_t i = 0; i < clens.size(); ++i )
			{
				uint64_t const high = low + clens[i];

				if ( (low <= i0) && (i0 < high) )
					nl = clens[i];
				if ( (low <= i1) && (i1 < high) )
					nr = clens[i];

				low = high;
			}
		}

		uint64_t const acc = std::accumulate(clens.begin(),clens.end(),0ull);
		avg = static_cast<double>(acc)/static_cast<double>(clens.size());

		if ( nl == nr )
			nval = nl;
		else
			nval = static_cast<double>(nl+nr)/2.0;
	}
}
