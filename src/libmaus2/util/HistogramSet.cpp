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

#include <libmaus2/util/HistogramSet.hpp>

libmaus2::util::HistogramSet::HistogramSet(uint64_t const numhist, uint64_t const lowsize)
: H(numhist)
{
	for ( uint64_t i = 0; i < numhist; ++i )
	{
		Histogram::unique_ptr_type tHi( new Histogram(lowsize) );
		H [ i ] = UNIQUE_PTR_MOVE ( tHi );
	}
}

void libmaus2::util::HistogramSet::print(std::ostream & out) const
{
	for ( uint64_t i = 0; i < H.size(); ++i )
	{
		out << "--- hist " << i << " ---" << std::endl;
		H[i]->print(out);
	}
}

libmaus2::util::Histogram::unique_ptr_type libmaus2::util::HistogramSet::merge() const
{
	if ( H.size() )
	{
		Histogram::unique_ptr_type hist ( new Histogram(H[0]->getLowSize()) );
		for ( uint64_t i = 0; i < H.size(); ++i )
			hist->merge(*H[i]);
		return UNIQUE_PTR_MOVE(hist);
	}
	else
	{
		Histogram::unique_ptr_type thist;
		return UNIQUE_PTR_MOVE(thist);
	}
}
