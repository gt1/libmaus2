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

#if ! defined(HUFFMANBASE_HPP)
#define HUFFMANBASE_HPP

#include <memory>
#include <deque>
#include <stack>
#include <algorithm>

#include <libmaus2/huffman/HuffmanTreeNode.hpp>
#include <libmaus2/huffman/HuffmanTreeLeaf.hpp>
#include <libmaus2/huffman/HuffmanTreeInnerNode.hpp>
#include <libmaus2/huffman/HuffmanTreeLeafComparator.hpp>

#include <map>

namespace libmaus2
{
	namespace huffman
	{
		struct HuffmanBase
		{
			template<typename iterator>
			static ::libmaus2::util::shared_ptr<HuffmanTreeNode>::type createTree( iterator a, iterator e )
			{
				::std::map<int64_t,uint64_t> probs;

				for ( ; a != e; ++a )
					probs[*a]++;

				return createTree ( probs );
			}

			template<typename probs_type>
			static HuffmanTreeNode::shared_ptr_type createTree(probs_type const & probs)
			{
				if ( probs.begin() == probs.end() )
					return HuffmanTreeNode::shared_ptr_type();

				::std::deque< HuffmanTreeLeaf * >	leafs;
				::std::deque< HuffmanTreeInnerNode * > nodes;
				HuffmanTreeNode * N0 = 0, * N1 = 0;

				try
				{
					for ( typename probs_type::const_iterator ita = probs.begin(); ita != probs.end(); ++ita )
					{
						HuffmanTreeLeaf * zp = 0;
						leafs.push_back(zp);
						leafs.back() = new HuffmanTreeLeaf(ita->first,ita->second);
					}

					if ( leafs.size() == 1 )
						return HuffmanTreeNode::shared_ptr_type(leafs[0]);

					::std::sort ( leafs.begin(), leafs.end(), HuffmanTreeLeafComparator() );

					::std::deque< HuffmanTreeLeaf * >::const_iterator Lita, Litb;
					::std::deque< HuffmanTreeInnerNode *>::const_iterator Nita, Nitb;
					uint64_t lpop = 0;
					uint64_t npop = 0;

					while ( (leafs.size() + nodes.size()) > 1 )
					{
						Lita = leafs.begin(), Litb = leafs.end(); Nita = nodes.begin(), Nitb = nodes.end();
						lpop = npop = 0;

						if ( Lita == Litb ) { N0 = *(Nita++); npop += 1; }
						else if ( Nita == Nitb ) { N0 = *(Lita++); lpop += 1; }
						else if ( (*Lita)->getFrequency() < (*Nita)->getFrequency() ) { N0 = *(Lita++); lpop += 1; }
						else { N0 = *(Nita++); npop += 1; }

						if ( Lita == Litb ) { N1 = *(Nita++); npop += 1; }
						else if ( Nita == Nitb ) { N1 = *(Lita++); lpop += 1; }
						else if ( (*Lita)->getFrequency() < (*Nita)->getFrequency() ) { N1 = *(Lita++); lpop += 1; }
						else { N1 = *(Nita++); npop += 1; }

						for ( uint64_t i = 0; i < lpop; ++i ) leafs[i] = 0;
						for ( uint64_t i = 0; i < npop; ++i ) nodes[i] = 0;

						HuffmanTreeInnerNode * zp = 0;
						nodes.push_back(zp);

						nodes.back() = new HuffmanTreeInnerNode(N0,N1,N0->getFrequency() + N1->getFrequency());

						N0 = 0, N1 = 0;

						for ( uint64_t i = 0; i < lpop; ++i ) leafs.pop_front();
						for ( uint64_t i = 0; i < npop; ++i ) nodes.pop_front();
					}

					return HuffmanTreeNode::shared_ptr_type(nodes[0]);
				}
				catch(...)
				{
					for ( unsigned int i = 0; i < leafs.size(); ++i )
						delete leafs[i];
					for ( unsigned int i = 0; i < nodes.size(); ++i )
						delete nodes[i];
					delete N0;
					delete N1;
					throw;
				}
			}
		};
	}
}
#endif
