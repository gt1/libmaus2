/*
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
*/

#include <libmaus/util/GenericIntervalTree.hpp>
		
libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > libmaus::util::GenericIntervalTree::computeNonEmpty(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & V)
{
	uint64_t nonempty = 0;
	for ( uint64_t i = 0; i < V.size(); ++i )
		if ( V[i].first != V[i].second )
			nonempty++;

	if ( nonempty == 0 )
		std::cerr << "all of the " << V.size() << " intervals are empty." << std::endl;

	::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > R(nonempty);
	nonempty = 0;
	for ( uint64_t i = 0; i < V.size(); ++i )
		if ( V[i].first != V[i].second )
			R [ nonempty++ ] = V[i];
	return R;
}
libmaus::bitio::IndexedBitVector::unique_ptr_type libmaus::util::GenericIntervalTree::computeNonEmptyBV(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & V)
{
	::libmaus::bitio::IndexedBitVector::unique_ptr_type BV(new ::libmaus::bitio::IndexedBitVector(V.size()));
	for ( uint64_t i = 0; i < V.size(); ++i )
		(*BV)[i] = (V[i].first != V[i].second);
	BV->setupIndex();
	return UNIQUE_PTR_MOVE(BV);
}

libmaus::util::GenericIntervalTree::GenericIntervalTree(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & H)
: nonempty(computeNonEmpty(H)), BV(computeNonEmptyBV(H)), 
  IT(nonempty.size() ? (new IntervalTree(nonempty,0,nonempty.size())) : 0)
{

}

uint64_t libmaus::util::GenericIntervalTree::find(uint64_t const v) const
{
	uint64_t const r = IT->find(v);
	return BV->select1(r);
}
