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

#if ! defined(DECODETABLE_HPP)
#define DECODETABLE_HPP

#include <libmaus2/huffman/HuffmanTreeNode.hpp>
#include <libmaus2/huffman/HuffmanTreeLeaf.hpp>
#include <libmaus2/huffman/HuffmanTreeInnerNode.hpp>

#include <map>

namespace libmaus2
{
	namespace huffman
	{
		struct DecodeTableEntry
		{
			::std::vector<int> symbols;
			unsigned int nexttable;
		};


		struct DecodeTable
		{
			typedef DecodeTable this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			enum Visit { First, Second, Third };
			
			unsigned int const lookupbits;
			uint64_t const entriespernode;
			::libmaus2::autoarray::AutoArray < DecodeTableEntry > entries;
			unsigned int innernodes;

			void printSingle() const
			{
				for ( unsigned int i = 0; i < innernodes; ++i )
				{
					
					::std::cerr << "\\begin{tabular}{|l|l|l|l|}" << ::std::endl;
					
					::std::cerr << "\\hline" << ::std::endl;
					::std::cerr << "\\multicolumn{4}{|c|}{Table " << i << "}\\\\" << ::std::endl;

					::std::cerr << "\\hline" << ::std::endl;
					::std::cerr << "C&N&\\#&D\\\\" << ::std::endl;
					::std::cerr << "\\hline" << ::std::endl;
					
					for ( unsigned int j = 0; j < entriespernode; ++j )
					{
						::std::cerr << "\\hline" << ::std::endl;
						// MSB first
						for ( unsigned int k = 0; k < lookupbits; ++k )
							::std::cerr << ((j >> (lookupbits-k-1))&1);
						
						::std::cerr << "&" << (*this)(i,j).nexttable << "&" <<
							(*this)(i,j).symbols.size() << "&";
						for ( unsigned int k = 0; k < (*this)(i,j).symbols.size(); ++k )
						{
							::std::cerr << static_cast<char>(((*this)(i,j).symbols[k]));
							if ( k+1 < (*this)(i,j).symbols.size() )
								::std::cerr << "";
						}
						::std::cerr << "\\\\" << ::std::endl;
					}
					::std::cerr << "\\hline" << ::std::endl;
					::std::cerr << "\\end{tabular}" << ::std::endl;
				}
			}

			
			void print() const
			{
				::std::cerr << "\\begin{tabular}{|l|";
				

				for ( unsigned int i = 0; i < innernodes; ++i )
					::std::cerr << "l|l|l|";
				::std::cerr << "}" << ::std::endl;

				::std::cerr << "\\hline" << ::std::endl;

				for ( unsigned int i = 0; i < innernodes; ++i )
					::std::cerr << "&\\multicolumn{3}{|c|}{Table " << i << "}";
				::std::cerr << "\\\\" << ::std::endl;

				::std::cerr << "\\hline" << ::std::endl;

				::std::cerr << "C";
				for ( unsigned int i = 0; i < innernodes; ++i )
				{
					::std::cerr << "&N&\\#&D";
				}
				::std::cerr << "\\\\" << ::std::endl;

				for ( unsigned int j = 0; j < entriespernode; ++j )
				{
					::std::cerr << "\\hline" << ::std::endl;
				
					for ( unsigned int k = 0; k < lookupbits; ++k )
						::std::cerr << ((j >> (lookupbits-k-1))&1);

					for ( unsigned int i = 0; i < innernodes; ++i )
					{
						::std::cerr << "&" << (*this)(i,j).nexttable << "&" <<
							(*this)(i,j).symbols.size() << "&";

						for ( unsigned int k = 0; k < (*this)(i,j).symbols.size(); ++k )
						{
							::std::cerr << static_cast<char>(((*this)(i,j).symbols[k]));
							if ( k+1 < (*this)(i,j).symbols.size() )
								::std::cerr << "";
						}				
					}
					
					::std::cerr << "\\\\" << ::std::endl;
				}
				
				::std::cerr << "\\hline" << ::std::endl;
				::std::cerr << "\\end{tabular}" << ::std::endl;
			}
			
			DecodeTableEntry const & operator()(unsigned int const tableid, uint64_t const mask) const
			{
				return entries [ tableid * entriespernode + mask ];
			}

			DecodeTable(HuffmanTreeNode const * root, unsigned int const rlookupbits)
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
						// HuffmanTreeLeaf const * lnode = dynamic_cast<HuffmanTreeLeaf const *>(node);
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
				
				innernodes = idToNode.size();
				entries = ::libmaus2::autoarray::AutoArray < DecodeTableEntry >( entriespernode * idToNode.size() );
				
				// ::std::cerr << "Number of entries is " << entriespernode * idToNode.size() << ::std::endl;
				
				for ( unsigned int id = 0; id < idToNode.size(); ++id )
				{
					HuffmanTreeInnerNode const * inode = idToNode[id];
					uint64_t const offset = id * entriespernode;
					
					for ( uint64_t m = 0; m != entriespernode; ++m )
					{
						DecodeTableEntry & entry = entries [ offset + m ];
						HuffmanTreeNode const * cur = inode;
						
						for ( unsigned int b = 0; b != lookupbits; ++b )
						{
							
							HuffmanTreeInnerNode const * icur = dynamic_cast<HuffmanTreeInnerNode const *>(cur);
							
							if ( m & (1ull << (lookupbits-b-1) ) )
							{
								cur = icur->right;
							}
							else
							{
								cur = icur->left;
							}
							
							if ( cur->isLeaf() )
							{
								entry.symbols.push_back ( dynamic_cast<HuffmanTreeLeaf const *>(cur)->symbol );
								cur = root;	
							}
						}
						
						entry.nexttable = nodeToId.find( dynamic_cast<HuffmanTreeInnerNode const *>(cur))->second;
					}
				}
			}
		};
	}
}
#endif
