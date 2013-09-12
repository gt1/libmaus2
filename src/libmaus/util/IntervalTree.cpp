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

#include <libmaus/util/IntervalTree.hpp>
			
bool libmaus::util::IntervalTree::isLeaf() const
{
	return !leftchild;
}

libmaus::util::IntervalTree::IntervalTree(
	::libmaus::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > const & H,
	uint64_t const ileft,
	uint64_t const iright
)
{
	if ( ileft == 0 && iright == H.size() )
	{
		for ( uint64_t i = 1; i < H.size(); ++i )
		{
			if ( H[i-1].second != H[i].first )
			{
				::libmaus::exception::LibMausException se;
				se.getStream() << "IntervalTree::IntervalTree() defect sequence containing intervals "
					<< "[" << H[i-1].first << "," << H[i-1].second << ")"
					<< " and "
					<< "[" << H[i].first << "," << H[i].second << ")" << std::endl;
				se.finish();
				throw se;
			}
		}
		for ( uint64_t i = 0; i < H.size(); ++i )
		{
			if ( ! ( H[i].first < H[i].second ) )
			{
				::libmaus::exception::LibMausException se;
				se.getStream() << "IntervalTree::Interval: defect interval " << i << "=[" << H[i].first << "," << H[i].second << ")" << std::endl;
				se.finish();
				throw se;
			}
		}
	}

	if ( ileft == iright )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "IntervalTree::IntervalTree(): ileft=" << ileft << "==iright=" << iright << std::endl;
		se.finish();
		throw se;
	}
	assert ( iright > ileft );

	// reached singleton interval
	if ( ileft + 1 == iright )
	{
		split = ileft;
	}
	// otherwise two child intervals
	else
	{
		uint64_t const imiddle = (ileft + iright)/2;
		split = H[imiddle].first;
		unique_ptr_type tleftchild(new IntervalTree(H,ileft,imiddle));
		leftchild = UNIQUE_PTR_MOVE(tleftchild);
		unique_ptr_type trightchild(new IntervalTree(H,imiddle,iright));
		rightchild = UNIQUE_PTR_MOVE(trightchild);
	}
}

libmaus::util::IntervalTree::~IntervalTree() {}

uint64_t libmaus::util::IntervalTree::findTrace(std::vector < IntervalTree const * > & trace, uint64_t const v) const
{
	trace.push_back(this);

	if ( leftchild.get() )
	{
		if ( v < split )
			return leftchild->findTrace(trace,v);
		else
			return rightchild->findTrace(trace,v);
	}
	else
	{
		return split;
	}	
}

libmaus::util::IntervalTree const * libmaus::util::IntervalTree::lca(uint64_t const v, uint64_t const w) const
{
	std::vector < IntervalTree const * > tracev, tracew;
	findTrace(tracev,v);
	findTrace(tracew,w);
	assert ( tracev[0] == tracew[0] );
	
	IntervalTree const * lca = 0;
	for ( uint64_t i = tracev.size()-1; !lca; --i )
		for ( uint64_t j = 0; (!lca) && j < tracew.size(); ++j )
			if ( tracev[i] == tracew[j] )
				lca = tracev[i];

	assert ( lca );
	
	return lca;
}

uint64_t libmaus::util::IntervalTree::find(uint64_t const v) const
{
	if ( leftchild.get() )
	{
		if ( v < split )
			return leftchild->find(v);
		else
			return rightchild->find(v);
	}
	else
	{
		return split;
	}
}

uint64_t libmaus::util::IntervalTree::getNumLeafs() const
{
	if ( leftchild.get() )
		return leftchild->getNumLeafs() + rightchild->getNumLeafs();
	else
		return 1;
}

std::ostream & libmaus::util::IntervalTree::flatten(std::ostream & ostr, uint64_t depth) const
{
	ostr << std::string(depth,' ');
	
	if ( leftchild.get() )
	{
		ostr << "node(" << split << ")\n";
		leftchild->flatten(ostr,depth+1);
		rightchild->flatten(ostr,depth+1);
	}
	else
	{
		ostr << "leaf(" << split << ")\n";
	}
	
	return ostr;
}

std::ostream & libmaus::util::operator<<(std::ostream & out, libmaus::util::IntervalTree const & I)
{
	return I.flatten(out);
}
