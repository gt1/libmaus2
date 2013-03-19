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

#if ! defined(COMPRESSTABLE_HPP)
#define COMPRESSTABLE_HPP

#include <vector>
#include <map>

#include <libmaus/uint/uint.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/huffman/HuffmanTreeNode.hpp>
#include <libmaus/huffman/HuffmanTreeInnerNode.hpp>
#include <libmaus/huffman/HuffmanTreeLeaf.hpp>

namespace libmaus
{
	namespace huffman
	{
		template<unsigned int words>
		struct CompressTableEntry
		{
			::std::vector<int> symbols;
			unsigned int nexttable;
			libmaus::uint::UInt<words> compressed;
			::std::vector<unsigned int> thresholds;
			unsigned int bitswritten;
		};

		template<unsigned int words>
		struct CompressTable
		{
			enum Visit { First, Second, Third };
			
			unsigned int const lookupbits;
			uint64_t const entriespernode;
			::libmaus::autoarray::AutoArray < CompressTableEntry<words> > entries;
			
			CompressTableEntry<words> const & operator()(unsigned int const tableid, uint64_t const mask) const
			{
				return entries [ tableid * entriespernode + mask ];
			}

			CompressTable(HuffmanTreeNode const * root, unsigned int const rlookupbits)
			: lookupbits(rlookupbits), entriespernode(1ull << lookupbits)
			{
				::std::stack < ::std::pair < HuffmanTreeNode const *, Visit > > S;

				S.push( ::std::pair < HuffmanTreeNode const *, Visit > (root,First) );
				
				::std::map < HuffmanTreeInnerNode const *, unsigned int > nodeToId;
				::std::vector < HuffmanTreeInnerNode const * > idToNode;

				while ( ! S.empty() )
				{
					HuffmanTreeNode const * node = S.top().first; 
					Visit const firstvisit = S.top().second;
					S.pop();
					
					if ( node->isLeaf() )
					{
						#if 0
						HuffmanTreeLeaf const * lnode = dynamic_cast<HuffmanTreeLeaf const *>(node);
						#endif
					}
					else 
					{
						HuffmanTreeInnerNode const * inode = dynamic_cast<HuffmanTreeInnerNode const *>(node);
						
						if ( firstvisit == First )
						{
							unsigned int const id = idToNode.size();
							nodeToId[inode] = id;
							idToNode.push_back(inode);
							
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Second) );
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->left,First) );
						}
						else if ( firstvisit == Second )
						{
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Third) );
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->right,First) );
						}
						else
						{
						}
					}
				}
				
				entries = ::libmaus::autoarray::AutoArray < CompressTableEntry<words> >( entriespernode * idToNode.size() );
				
				// ::std::cerr << "Number of entries is " << entriespernode * idToNode.size() << ::std::endl;
				
				for ( unsigned int id = 0; id < idToNode.size(); ++id )
				{
					HuffmanTreeInnerNode const * inode = idToNode[id];
					uint64_t const offset = id * entriespernode;
					
					for ( uint64_t m = 0; m != entriespernode; ++m )
					{
						CompressTableEntry<words> & entry = entries [ offset + m ];
						HuffmanTreeNode const * cur = inode;
						bool eraseNextBit = (cur == root);
						unsigned int bitswritten = 0;
						libmaus::uint::UInt<words> compressed;
						
						for ( unsigned int b = 0; b != lookupbits; ++b )
						{
							HuffmanTreeInnerNode const * icur = dynamic_cast<HuffmanTreeInnerNode const *>(cur);
							
							if ( m & (1ull << (lookupbits-b-1) ) )
							{
								if ( ! eraseNextBit )
								{
									bitswritten++;
									compressed <<= 1;
									compressed |= libmaus::uint::UInt<words>(1ull);
								}
								cur = icur->right;
							}
							else
							{
								if ( ! eraseNextBit )
								{
									bitswritten++;
									compressed <<= 1;
									compressed |= libmaus::uint::UInt<words>(0ull);
								}
								cur = icur->left;
							}
							
							eraseNextBit = false;
							
							if ( cur->isLeaf() )
							{
								entry.symbols.push_back ( dynamic_cast<HuffmanTreeLeaf const *>(cur)->symbol );
								entry.thresholds.push_back ( bitswritten );
								cur = root;
								eraseNextBit = true;	
							}
						}
						
						entry.compressed = compressed;
						entry.bitswritten = bitswritten;
						entry.nexttable = nodeToId.find( dynamic_cast<HuffmanTreeInnerNode const *>(cur))->second;
					}
				}
			}
		};
	}
}
#endif
