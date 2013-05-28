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

#if ! defined(DYNAMICHUFFMANWAVELTTREE_HPP)
#define DYNAMICHUFFMANWAVELTTREE_HPP

#include <libmaus/bitbtree/bitbtree.hpp>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/wavelet/HuffmanWaveletTree.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <memory>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace wavelet
	{
		template<unsigned int k, unsigned int w>
		struct DynamicHuffmanWaveletTree
		{
			// private:
			static unsigned int const lookupwords = 1;
			typedef typename ::libmaus::bitbtree::BitBTree<k,w> bitbtreetype;
			typedef typename bitbtreetype::inner_node_allocator_type inner_node_allocator_type;
			typedef typename bitbtreetype::leaf_node_allocator_type leaf_node_allocator_type;

			struct DynamicHuffmanWaveletTreeNode
			{
				typedef typename ::libmaus::util::unique_ptr<
					DynamicHuffmanWaveletTreeNode
					>::type unique_ptr_type;

				bitbtreetype btree;
				unique_ptr_type left;
				unique_ptr_type right;
				
				DynamicHuffmanWaveletTreeNode(
					::libmaus::huffman::HuffmanTreeInnerNode const * const node,
					typename ::libmaus::util::shared_ptr < inner_node_allocator_type >::type & inner_node_allocator,
					typename ::libmaus::util::shared_ptr < leaf_node_allocator_type >::type & leaf_node_allocator
				) : btree(inner_node_allocator,leaf_node_allocator)
				{
					if ( ! (node->left->isLeaf()) )
					{
						::libmaus::huffman::HuffmanTreeInnerNode const * const hleft = dynamic_cast< ::libmaus::huffman::HuffmanTreeInnerNode const *>(node->left);
						left = UNIQUE_PTR_MOVE(unique_ptr_type ( new DynamicHuffmanWaveletTreeNode(hleft,inner_node_allocator,leaf_node_allocator) ) );
					}
					#if 0
					else
					{
						::libmaus::huffman::HuffmanTreeLeaf const * const hleft = dynamic_cast< ::libmaus::huffman::HuffmanTreeLeaf const *>(node->left);
						std::cerr << "reached left leaf " << hleft->symbol << std::endl;
					}
					#endif
					if ( ! (node->right->isLeaf()) )
					{
						::libmaus::huffman::HuffmanTreeInnerNode const * const hright = dynamic_cast< ::libmaus::huffman::HuffmanTreeInnerNode const *>(node->right);
						right = UNIQUE_PTR_MOVE(unique_ptr_type(new DynamicHuffmanWaveletTreeNode(hright,inner_node_allocator,leaf_node_allocator)));
					}
					#if 0
					else
					{
						::libmaus::huffman::HuffmanTreeLeaf const * const hright = dynamic_cast< ::libmaus::huffman::HuffmanTreeLeaf const *>(node->right);
						std::cerr << "reached right leaf " << hright->symbol << std::endl;			
					}
					#endif	
				}
				
				template<typename writer_type>
				::libmaus::wavelet::HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type serialize(writer_type & writer, uint64_t & offset) const
				{
					HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type Hnode =
						HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type ( new HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode (offset) );

					btree.serialize(writer);
					offset += btree.root_cnt;
					
					if ( left.get() )
						Hnode->left = left->serialize(writer,offset);
					if ( right.get() )
						Hnode->right = right->serialize(writer,offset);
						
					return Hnode;
				}
				template<typename writer_type>
				HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type serialize(writer_type & writer) const
				{
					uint64_t offset = 0;
					return serialize(writer,offset);
				}
				
				uint64_t countBits() const
				{
					uint64_t b = btree.root_cnt;
					
					if ( left.get() )
						b += left->countBits();
					if ( right.get() )
						b += right->countBits();
						
					return b;
				}
				
				void print(std::ostream & out) const
				{
					for ( uint64_t i = 0; i < btree.root_cnt; ++i )
						out << btree[i];

					if ( left.get() )
						left->print(out);
					if ( right.get() )
						right->print(out);
				}
			};
			
			typename ::libmaus::util::shared_ptr < inner_node_allocator_type >::type inner_node_allocator;
			typename ::libmaus::util::shared_ptr < leaf_node_allocator_type >::type leaf_node_allocator;
			typename ::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type hroot;

			::libmaus::util::unique_ptr< ::libmaus::huffman::EncodeTable<lookupwords> >::type enctable;
			typename DynamicHuffmanWaveletTreeNode::unique_ptr_type data;
			uint64_t n;
			
			public:
			void print(std::ostream & out) const
			{
				if ( data )
					data->print(out);
			}
			void serializeAsStatic(std::ostream & out) const
			{
				/**
				 * length of sequence
				 **/
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				
				/**
				 * huffman tree
				 **/
				hroot->serialize(out);

				/**
				 * serialize data bits
				 **/		
				uint64_t const totalbits = data.get() ? data->countBits() : 0;
				uint64_t const totalwords = (totalbits + 63)/64;
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,totalwords);

				HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type navroot;
				bitio::OutputBuffer<uint64_t> ob(16*1024,out);
				bitio::FastWriteBitWriterBuffer64 writer(ob);
				if ( data )
					navroot = data->serialize ( writer );
				writer.flush();
				ob.flush();
				out.flush();

				if ( navroot )
					HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::serialize(out,navroot.get());
					
				out.flush();
			}

			DynamicHuffmanWaveletTree(
				::libmaus::util::shared_ptr<huffman::HuffmanTreeNode>::type & rhroot
			)
			: 
				inner_node_allocator(new inner_node_allocator_type()),
				leaf_node_allocator(new leaf_node_allocator_type()),
				hroot(rhroot),
				enctable( new huffman::EncodeTable<lookupwords> (hroot.get()) ),
				n(0)
			{
				if ( ! hroot->isLeaf() )	
				{
					data = UNIQUE_PTR_MOVE(typename DynamicHuffmanWaveletTreeNode::unique_ptr_type (
						new DynamicHuffmanWaveletTreeNode(dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hroot.get()),inner_node_allocator,leaf_node_allocator)
					));
				}
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			void insert(int const key, uint64_t p)
			{
				assert ( p <= n );
				
				std::pair < ::libmaus::uint::UInt < lookupwords >, unsigned int > const & codeandlen = (*enctable)[key];
				::libmaus::uint::UInt < lookupwords > const & code = codeandlen.first;
				unsigned int const codelen = codeandlen.second;
				DynamicHuffmanWaveletTreeNode * node = data.get();
				
				for ( unsigned int d = 0; d < codelen; ++d )
				{
					bool const b = code.getBit(codelen-d-1);
					node->btree.insertBit(p,b);

					if ( b )
					{
						p = node->btree.rank1(p)-1;
						node = node->right.get();
					}
					else
					{
						p = node->btree.rank0(p)-1;
						node = node->left.get();			
					}
				}
				
				n += 1;
			}

			int access(uint64_t p) const
			{
				assert ( p < n );
				
				huffman::HuffmanTreeNode const * hufnode = hroot.get();
				DynamicHuffmanWaveletTreeNode const * node = data.get();
				
				while ( !hufnode->isLeaf() )
				{
					bool const b = node->btree[p];
					huffman::HuffmanTreeInnerNode const * innernode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hufnode);
					
					if ( b )
					{
						p = node->btree.rank1(p)-1;
						hufnode = innernode->right;
						node = node->right.get();
					}
					else
					{
						p = node->btree.rank0(p)-1;
						hufnode = innernode->left;
						node = node->left.get();
					}
				}

				huffman::HuffmanTreeLeaf const * leaf = dynamic_cast<huffman::HuffmanTreeLeaf const *>(hufnode);

				return leaf->symbol;
			}

			uint64_t rank(int const key, uint64_t p) const
			{
				assert ( p < n );

				std::pair < ::libmaus::uint::UInt < lookupwords >, unsigned int > const & codeandlen = (*enctable)[key];
				::libmaus::uint::UInt < lookupwords > const & code = codeandlen.first;
				unsigned int const codelen = codeandlen.second;
				DynamicHuffmanWaveletTreeNode * node = data.get();
				
				for ( unsigned int d = 0; d < codelen; ++d )
				{
					bool const b = code.getBit(codelen-d-1);

					if ( b )
					{
						uint64_t r = node->btree.rank1(p);
						
						if  ( ! r )
							return 0;
						
						p = r-1;
						node = node->right.get();
					}
					else
					{
						uint64_t r = node->btree.rank0(p);
						
						if ( ! r )
							return 0;
						
						p = r-1;
						node = node->left.get();			
					}
				}
				
				return p+1;
			}

		};
	}
}
#endif
