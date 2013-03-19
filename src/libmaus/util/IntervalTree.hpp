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

#if ! defined(INTERVALTREE_HPP)
#define INTERVALTREE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/bitio/BitVector.hpp>
#include <vector>
#include <cassert>

namespace libmaus
{
	namespace util
	{
		struct IntervalTree
		{
			typedef IntervalTree this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			unique_ptr_type leftchild;
			unique_ptr_type rightchild;

			uint64_t split;
			
			bool isLeaf() const
			{
				return !leftchild;
			}

			IntervalTree(
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
					leftchild = UNIQUE_PTR_MOVE(unique_ptr_type(new IntervalTree(H,ileft,imiddle)));
					rightchild = UNIQUE_PTR_MOVE(unique_ptr_type(new IntervalTree(H,imiddle,iright)));
				}
			}

			~IntervalTree() {}
			
			uint64_t findTrace(std::vector < IntervalTree const * > & trace, uint64_t const v) const
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
			
			IntervalTree const * lca(uint64_t const v, uint64_t const w) const
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

			uint64_t find(uint64_t const v) const
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
			
			uint64_t getNumLeafs() const
			{
				if ( leftchild.get() )
					return leftchild->getNumLeafs() + rightchild->getNumLeafs();
				else
					return 1;
			}
			
			std::ostream & flatten(std::ostream & ostr, uint64_t depth = 0) const
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
		};
		
		inline std::ostream & operator<<(std::ostream & out, IntervalTree const & I)
		{
			return I.flatten(out);
		}
		
		struct GenericIntervalTree
		{
			typedef GenericIntervalTree this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			static ::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > computeNonEmpty(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & V)
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
			static ::libmaus::bitio::IndexedBitVector::unique_ptr_type computeNonEmptyBV(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & V)
			{
				::libmaus::bitio::IndexedBitVector::unique_ptr_type BV(new ::libmaus::bitio::IndexedBitVector(V.size()));
				for ( uint64_t i = 0; i < V.size(); ++i )
					(*BV)[i] = (V[i].first != V[i].second);
				BV->setupIndex();
				return UNIQUE_PTR_MOVE(BV);
			}
			
			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > nonempty;
			::libmaus::bitio::IndexedBitVector::unique_ptr_type BV;
			IntervalTree::unique_ptr_type IT;
			
			GenericIntervalTree(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & H)
			: nonempty(computeNonEmpty(H)), BV(computeNonEmptyBV(H)), 
			  IT(nonempty.size() ? (new IntervalTree(nonempty,0,nonempty.size())) : 0)
			{
			
			}
			
			uint64_t find(uint64_t const v) const
			{
				uint64_t const r = IT->find(v);
				return BV->select1(r);
			}
		};
	}
}
#endif
