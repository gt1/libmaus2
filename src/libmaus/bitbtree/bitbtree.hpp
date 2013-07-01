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

#if ! defined(BITBTREE_HPP)
#define BITBTREE_HPP

#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <stack>
#include <sys/types.h>

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/uint/uint.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/bitio/getBit.hpp>

namespace libmaus
{
	namespace bitbtree
	{
		template< typename node_ptr_type, typename bit_count_type, typename one_count_type>
		struct BitBTreeInnerNodeData
		{
			node_ptr_type ptr;
			bit_count_type cnt;
			one_count_type bcnt;
			
			BitBTreeInnerNodeData() : ptr(0), cnt(0), bcnt(0) {}
			BitBTreeInnerNodeData(BitBTreeInnerNodeData<node_ptr_type,bit_count_type,one_count_type> const & o)
			: ptr(o.ptr), cnt(o.cnt), bcnt(o.bcnt) {}
		};

		template< typename node_ptr_type, typename bit_count_type, typename one_count_type, unsigned int k>
		struct BitBTreeInnerNode
		{
			typedef BitBTreeInnerNodeData<node_ptr_type,bit_count_type,one_count_type> data_type;
			
			data_type data[2*k];
			unsigned int dataFilled;
			
			BitBTreeInnerNode() : dataFilled(0) {}
		};

		template<unsigned int w>
		struct BitBTreeLeaf
		{
			::libmaus::uint::UInt<w> data;
		};

		template<unsigned int k, unsigned int w>
		struct BitBTree
		{
			typedef uint32_t node_ptr_type;
			typedef uint64_t bit_count_type;
			typedef bit_count_type one_count_type;
			typedef BitBTreeLeaf<w> leaf_type;
			typedef BitBTreeInnerNode<node_ptr_type,bit_count_type,one_count_type,k> inner_node_type;
			
			typedef BitBTree<k,w> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			#if defined(__GNUC__) && ((__GNUC__ >= 5) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1)))
			static node_ptr_type const bitbtree_leaf_mask = (1ull << (8*sizeof(node_ptr_type)-1));
			static node_ptr_type const bitbtree_node_base_mask = ~bitbtree_leaf_mask;
			#else
			#define bitbtree_leaf_mask 0x80000000UL
			#define bitbtree_node_base_mask 0x7FFFFFFFUL
			#endif
			
			template<typename writer_type> 
			void serialize(writer_type & writer, node_ptr_type ptr, bit_count_type cnt) const
			{
				if ( isLeaf(ptr) )
				{
					leaf_type const & lnode = getLeaf(ptr);
					lnode.data.serializeReverse(writer,cnt);
				}
				else
				{
					inner_node_type const & inode = getNode(ptr);
					for ( unsigned int i = 0; i < inode.dataFilled; ++i )
						serialize ( writer, inode.data[i].ptr, inode.data[i].cnt );
				}
			}
			
			template<typename writer_type>
			void serialize(writer_type & writer) const
			{
				if ( ! empty )
					serialize(writer, root, root_cnt);
			}
			
			void serializeAsAutoArray(std::ostream & out) const
			{
				uint64_t const n = root_cnt;
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);

				bitio::OutputBuffer<uint64_t> ob(16*1024,out);
				bitio::FastWriteBitWriterBuffer64 writer(ob);
				serialize ( writer );
				writer.flush();
				ob.flush();
				out.flush();
			}

			template<typename node_type, typename node_ptr_type, node_ptr_type mask, node_ptr_type bitbtree_node_base_mask_value>
			struct NodeAllocator
			{
				typedef NodeAllocator<node_type, node_ptr_type, mask, bitbtree_node_base_mask_value> this_type;
				typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				autoarray::AutoArray< node_type > Anodes;
				node_type * nodes;
				uint64_t allocatedNodes;
				std::stack < node_ptr_type > nodeFreeList;
				
				NodeAllocator()
				: nodes(0), allocatedNodes(0)
				{} 
				
				node_type const & operator[](node_ptr_type const & i) const
				{
					return nodes[i & bitbtree_node_base_mask_value];
				}
				node_type & operator[](node_ptr_type & i) const
				{
					return nodes[i & bitbtree_node_base_mask_value];
				}
				
				node_ptr_type allocate()
				{
					if ( nodeFreeList.empty() )
					{
						uint64_t newAllocatedNodes = std::max(allocatedNodes+1, (allocatedNodes * 9)/8 );
						autoarray::AutoArray< node_type > NAnodes(newAllocatedNodes,false);
						std::copy(nodes, nodes+allocatedNodes, NAnodes.get());
						Anodes = NAnodes;
						nodes = Anodes.get();
						
						for ( uint64_t i = allocatedNodes; i < newAllocatedNodes; ++i )
							nodeFreeList.push(i);
							
						allocatedNodes = newAllocatedNodes;	
					}
					
					node_ptr_type node = nodeFreeList.top();
					nodeFreeList.pop();
					return (node | mask);
				}
				void free(node_ptr_type node)
				{
					nodeFreeList.push(node & bitbtree_node_base_mask);
				}
				
				shared_ptr_type clone() const
				{
					shared_ptr_type cloned (
						new NodeAllocator<node_type, node_ptr_type, mask, bitbtree_node_base_mask>()
						);
					
					cloned->Anodes = Anodes.clone();
					cloned->nodes = cloned->Anodes.get();
					cloned->allocatedNodes = allocatedNodes;
					cloned->nodeFreeList = nodeFreeList;
					
					return cloned;
				}
				
				uint64_t bitSize() const
				{
					return
						Anodes.byteSize()*8+
						sizeof(node_type *)*8+
						sizeof(uint64_t)*8+
						nodeFreeList.size() * sizeof(node_ptr_type) * 8;
				} 
			};

			template<typename node_type, typename node_ptr_type, node_ptr_type mask, node_ptr_type bitbtree_node_base_mask_value>
			struct BlockNodeAllocator
			{
				typedef BlockNodeAllocator<node_type,node_ptr_type,mask,bitbtree_node_base_mask_value> this_type;
				typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				static unsigned int const blockshift = 16;
				static uint64_t const blocksize = 1ull<<blockshift;
				static uint64_t const blockmask = blocksize-1;
				typedef autoarray::AutoArray<node_type> block_type;
				typedef typename block_type::shared_ptr_type block_ptr_type;
				
				std::vector < block_ptr_type  > blocks;
				std::stack < node_ptr_type > nodeFreeList;
				
				BlockNodeAllocator() : blocks(), nodeFreeList() {}
				
				node_type const & operator[](node_ptr_type const & i) const
				{
					return (*blocks[(i & bitbtree_node_base_mask_value)>>blockshift])[(i & bitbtree_node_base_mask_value)&blockmask];
				}
				node_type & operator[](node_ptr_type & i) const
				{
					return (*blocks[(i & bitbtree_node_base_mask_value)>>blockshift])[(i & bitbtree_node_base_mask_value)&blockmask];
				}
				
				node_ptr_type allocate()
				{
					if ( nodeFreeList.empty() )
					{
						uint64_t newblockid = blocks.size();
						blocks.push_back( typename autoarray::AutoArray<node_type>::shared_ptr_type() );
						blocks.back() = 
							typename autoarray::AutoArray<node_type>::shared_ptr_type(
								new autoarray::AutoArray<node_type>(blocksize)
							);
						
						for ( uint64_t i = 0; i < blocksize; ++i )
							nodeFreeList.push( newblockid*blocksize + i );
					}
					
					node_ptr_type node = nodeFreeList.top();
					nodeFreeList.pop();
					return (node | mask);
				}
				void free(node_ptr_type node)
				{
					nodeFreeList.push(node & bitbtree_node_base_mask_value);
				}
				
				shared_ptr_type clone() const
				{
					shared_ptr_type cloned (
						new BlockNodeAllocator<node_type, node_ptr_type, mask, bitbtree_node_base_mask_value>()
						);
					
					cloned->blocks.resize( blocks.size() );
					
					for ( uint64_t i = 0; i < blocks.size(); ++i )
					{
						cloned->blocks[i] = typename autoarray::AutoArray<node_type>::shared_ptr_type( new block_type );
						*(cloned->blocks[i]) = blocks[i]->clone();
					}
					
					cloned->nodeFreeList = nodeFreeList;
					
					return cloned;
				}
				
				uint64_t bitSize() const
				{
					uint64_t s = 0;
					
					if ( blocks.size() )
						s += blocks.size() * blocks[0]->byteSize() * 8;

					s += nodeFreeList.size() * sizeof(node_ptr_type) * 8;
						
					return s;
				} 
			};
			
			static bool isLeaf(node_ptr_type node)
			{
				return node & bitbtree_leaf_mask;
			}

			node_ptr_type root;
			uint64_t root_cnt;
			uint64_t root_bcnt;
			bool empty;
			
			uint64_t depth;
			
			// typedef NodeAllocator<inner_node_type,node_ptr_type,0,bitbtree_node_base_mask> inner_node_allocator_type;
			// typedef NodeAllocator<leaf_type,node_ptr_type,bitbtree_leaf_mask,bitbtree_node_base_mask> leaf_node_allocator_type;

			typedef BlockNodeAllocator<inner_node_type,node_ptr_type,0,bitbtree_node_base_mask> inner_node_allocator_type;
			typedef BlockNodeAllocator<leaf_type,node_ptr_type,bitbtree_leaf_mask,bitbtree_node_base_mask> leaf_node_allocator_type;

			typename inner_node_allocator_type::shared_ptr_type ainnernodes;
			inner_node_allocator_type * innernodes;
			typename leaf_node_allocator_type::shared_ptr_type aleafnodes;
			leaf_node_allocator_type * leafnodes;
			
			uint64_t bitSize() const
			{
				return
					sizeof(node_ptr_type)*8+
					sizeof(uint64_t)*8+
					sizeof(uint64_t)*8+
					sizeof(bool)*8+
					sizeof(uint64_t)*8+
					innernodes->bitSize()+
					leafnodes->bitSize();
			}
			
			uint64_t count1() const
			{
				if ( empty )
					return 0;
				else
					return root_bcnt;
			}
			
			uint64_t count0() const
			{
				if ( empty )
					return 0;
				else
					return root_cnt - root_bcnt;
			}
			
			bool hasNext1() const
			{
				return count1() != 0;
			}
			
			uint64_t next1(uint64_t const p) const
			{
				assert ( hasNext1() );
				
				std::pair<uint64_t,uint64_t> const is1 = inverseSelect1(p);
				
				// 1 bit at position p?
				if ( is1.first )
					return p;
				else
				{
					if ( is1.second == count1() )
						return select1(0);
					else
						return select1(is1.second);
				}
			}
			
			uint64_t size() const
			{
				if ( empty )
					return 0;
				else
					return root_cnt;
			}

			bool checkTree() const
			{
				if ( empty )
					return true;
				else
					return checkNode(root);
			}

			bool checkNode(node_ptr_type node) const
			{
				if ( isLeaf(node) )
					return true;
				
				inner_node_type const & innernode = getNode(node);
				
				if ( innernode.dataFilled == 0 )
				{
					std::cerr << "Found empty inner node." << std::endl;
					return false;
				}
				
				bool const childrenAreLeafs = isLeaf(innernode.data[0].ptr);
				
				for ( uint64_t i = 0; i < innernode.dataFilled; ++i )
					if ( isLeaf(innernode.data[i].ptr) != childrenAreLeafs )
					{
						std::cerr << "Found mixed node." << std::endl;
						return false;
					}
				
				if ( ! childrenAreLeafs )
				{
					for ( uint64_t i = 0; i < innernode.dataFilled; ++i )
					{
						inner_node_type const & innersubnode = getNode(innernode.data[i].ptr);
						uint64_t subcnt = 0;
						uint64_t subbcnt = 0;
						for ( uint64_t j = 0; j < innersubnode.dataFilled; ++j )
						{
							subcnt += innersubnode.data[j].cnt;
							subbcnt += innersubnode.data[j].bcnt;
						}
						
						if ( subcnt != innernode.data[i].cnt )
						{
							std::cerr << "cnt is wrong for node " << node << " child " << innernode.data[i].ptr << std::endl;
							return false;
						}
						if ( subbcnt != innernode.data[i].bcnt )
						{
							std::cerr << "bcnt is wrong." << std::endl;
							return false;
						}
					}	
				}
				else
				{
					for ( uint64_t i = 0; i < innernode.dataFilled; ++i )
					{
						assert ( isLeaf ( innernode.data[i].ptr ) );
						leaf_type const & leaf = getLeaf(innernode.data[i].ptr);
						assert ( innernode.data[i].cnt != 0 );
						
						uint64_t const bcnt = leaf.data.rank1(innernode.data[i].cnt-1);
						if ( bcnt != innernode.data[i].bcnt )
						{
							std::cerr << "leaf bcnt is wrong for node " << node << " child " << innernode.data[i].ptr << std::endl;
							std::cerr << "computed " << bcnt << " stored " << innernode.data[i].bcnt 
								<< " cnt " << innernode.data[i].cnt
								<< std::endl;
							std::cerr << "uinit: " << leaf.data << std::endl;
						}
					}
				}

				for ( uint64_t i = 0; i < innernode.dataFilled; ++i )
					if ( ! checkNode ( innernode.data[i].ptr ) )
						return false;

				return true;
			}
			
			unique_ptr_type clone()
			{
				unique_ptr_type newtree ( new BitBTree<k,w> );
				
				newtree->root = root;
				newtree->root_cnt = root_cnt;
				newtree->root_bcnt = root_bcnt;
				newtree->empty = empty;
				newtree->depth = depth;
				
				newtree->ainnernodes = ainnernodes->clone();
				newtree->innernodes = newtree->ainnernodes.get();
				newtree->aleafnodes = aleafnodes->clone();
				newtree->leafnodes = newtree->aleafnodes.get();
				
				return UNIQUE_PTR_MOVE(newtree);
			}
			
			struct VectorConstructionTuple
			{
				node_ptr_type node;
				bit_count_type cnt; 
				one_count_type bcnt;
				
				VectorConstructionTuple() : node(0), cnt(0), bcnt(0) {}
				VectorConstructionTuple(
					node_ptr_type const rnode,
					bit_count_type const rcnt,
					one_count_type const rbcnt)
				: node(rnode), cnt(rcnt), bcnt(rbcnt) {}
			};
			
			std::vector < VectorConstructionTuple > getHomogenuousLeafs(uint64_t const bsize, bool const v)
			{
				uint64_t const bitsperleaf = w*(8*sizeof(uint64_t));
				uint64_t const fullleafs = bsize / bitsperleaf;
				uint64_t const fracsyms = bsize - fullleafs*bitsperleaf;
			
				// std::cerr << "Full leafs " << fullleafs << " fracsyms " << fracsyms << " bits per leaf " << bitsperleaf << std::endl;
			
				// std::vector < std::pair<node_ptr_type,uint64_t> > todo;
				std::vector < VectorConstructionTuple > todo;
			
				for ( uint64_t i = 0; i < fullleafs; ++i )
				{
					node_ptr_type const ptr = allocateLeaf();
					leaf_type & lnode = getLeaf(ptr);
					
					for ( uint64_t j = 0; j < bitsperleaf; ++j )
						lnode.data.setBit(j,v);
					
					// todo.push_back( std::pair<node_ptr_type,uint64_t>(ptr,bitsperleaf));
					todo . push_back(VectorConstructionTuple(ptr,bitsperleaf,v?bitsperleaf:0));
					
					// std::cerr << "pushed " << todo.back().first << " with " << todo.back().second << " bits." << std::endl;
				}
			
				if ( fracsyms )
				{
					node_ptr_type const ptr = allocateLeaf();
					leaf_type & lnode = getLeaf(ptr);

					for ( uint64_t j = 0; j < fracsyms; ++j )
						lnode.data.setBit(j,v);

					// todo.push_back( std::pair<node_ptr_type,uint64_t>(ptr,fracsyms));
					todo . push_back(VectorConstructionTuple(ptr,fracsyms,v?fracsyms:0));

					// std::cerr << "pushed " << todo.back().first << " with " << todo.back().second << " bits." << std::endl;
				}
			
				return todo;
			}

			std::vector < VectorConstructionTuple > getLeafsFromVector(uint64_t const * A, uint64_t const bsize)
			{
				uint64_t const bitsperleaf = w*(8*sizeof(uint64_t));
				uint64_t const fullleafs = bsize / bitsperleaf;
				uint64_t const fracsyms = bsize - fullleafs*bitsperleaf;
			
				// std::cerr << "Full leafs " << fullleafs << " fracsyms " << fracsyms << " bits per leaf " << bitsperleaf << std::endl;
			
				// std::vector < std::pair<node_ptr_type,uint64_t> > todo;
				std::vector < VectorConstructionTuple > todo;
			
				uint64_t z = 0;
				for ( uint64_t i = 0; i < fullleafs; ++i )
				{
					node_ptr_type const ptr = allocateLeaf();
					leaf_type & lnode = getLeaf(ptr);
					
					for ( uint64_t j = 0; j < bitsperleaf; ++j )
						lnode.data.setBit(j,::libmaus::bitio::getBit(A,z++));
					
					// todo.push_back( std::pair<node_ptr_type,uint64_t>(ptr,bitsperleaf));
					todo . push_back(VectorConstructionTuple(ptr,bitsperleaf,lnode.data.rank1(bitsperleaf-1)));
					
					// std::cerr << "pushed " << todo.back().first << " with " << todo.back().second << " bits." << std::endl;
				}
			
				if ( fracsyms )
				{
					node_ptr_type const ptr = allocateLeaf();
					leaf_type & lnode = getLeaf(ptr);

					for ( uint64_t j = 0; j < fracsyms; ++j )
						lnode.data.setBit(j,::libmaus::bitio::getBit(A,z++));

					// todo.push_back( std::pair<node_ptr_type,uint64_t>(ptr,fracsyms));
					todo . push_back(VectorConstructionTuple(ptr,fracsyms,lnode.data.rank1(fracsyms-1)));

					// std::cerr << "pushed " << todo.back().first << " with " << todo.back().second << " bits." << std::endl;
				}
			
				return todo;
			}
			
			void mergeConstructionTuples(std::vector < VectorConstructionTuple > & todo)
			{			
				while ( todo.size() > 1 )
				{
					std::vector < VectorConstructionTuple > newtodo;
					
					uint64_t const numinner = (todo.size() + 2*k-1)/(2*k);
					// std::cerr << "numinner " << numinner << std::endl;
					
					for ( uint64_t i = 0; i < numinner; ++i )
					{
						uint64_t const low = i*2*k;
						uint64_t const high = std::min(static_cast<uint64_t>(low+2*k),static_cast<uint64_t>(todo.size()));

						node_ptr_type const ptr = allocateNode();
						inner_node_type & node = getNode(ptr);

						node.dataFilled = high-low;

						// std::cerr << "low=" << low << " high=" << high << " dataFilled " << node.dataFilled << std::endl;
						
						uint64_t cnt = 0;
						uint64_t bcnt = 0;
						for ( uint64_t j = low; j < high; ++j )
						{
							// std::cerr << "j=" << j << std::endl;
							node.data[j-low].ptr = todo[j].node;
							node.data[j-low].cnt = todo[j].cnt;
							node.data[j-low].bcnt = todo[j].bcnt;
							cnt += todo[j].cnt;
							bcnt += todo[j].bcnt;
						}
						
						// newtodo.push_back( std::pair<node_ptr_type,uint64_t >(ptr,cnt) );
						newtodo.push_back( VectorConstructionTuple(ptr,cnt,bcnt) );
					}						

					depth++;
					todo = newtodo;
					// todo.swap(newtodo);
				}
				
				// std::cerr << "Total bits " << todo[0].second << std::endl;

				root = todo[0].node;
				root_cnt = todo[0].cnt;
				root_bcnt = todo[0].bcnt;
				empty = false;
			}

			BitBTree(uint64_t const bsize, bool const v)
			: root(0), root_cnt(0), root_bcnt(0), empty(true), depth(0),
			  ainnernodes ( new inner_node_allocator_type ),
			  innernodes( ainnernodes.get() ),
			  aleafnodes ( new leaf_node_allocator_type ),
			  leafnodes ( aleafnodes.get() )
			{
				if ( bsize )
				{
					std::vector < VectorConstructionTuple > todo = getHomogenuousLeafs(bsize,v);
					mergeConstructionTuples(todo);
					assert ( root_cnt == bsize );
				}
			}

			BitBTree(uint64_t const bsize, uint64_t const * A)
			: root(0), root_cnt(0), root_bcnt(0), empty(true), depth(0),
			  ainnernodes ( new inner_node_allocator_type ),
			  innernodes( ainnernodes.get() ),
			  aleafnodes ( new leaf_node_allocator_type ),
			  leafnodes ( aleafnodes.get() )
			{
				if ( bsize )
				{
					std::vector < VectorConstructionTuple > todo = getLeafsFromVector(A,bsize);
					mergeConstructionTuples(todo);
					assert ( root_cnt == bsize );
				}
			}
			
			BitBTree()
			: root(0), root_cnt(0), root_bcnt(0), empty(true), depth(0),
			  ainnernodes ( new inner_node_allocator_type ),
			  innernodes( ainnernodes.get() ),
			  aleafnodes ( new leaf_node_allocator_type ),
			  leafnodes ( aleafnodes.get() )
			{
			}

			BitBTree(
				typename inner_node_allocator_type::shared_ptr_type & rainnernodes,
				typename leaf_node_allocator_type::shared_ptr_type  & raleafnodes
			)
			: root(0), root_cnt(0), root_bcnt(0), empty(true), depth(0),
			  ainnernodes ( rainnernodes ),
			  innernodes( ainnernodes.get() ),
			  aleafnodes ( raleafnodes ),
			  leafnodes ( aleafnodes.get() )
			{
			}
			
			struct InsertInfo
			{
				bool nodeCreated;
				node_ptr_type newnode;
				
				bit_count_type oldnodecnt;
				bit_count_type newnodecnt;
				
				one_count_type oldnodebcnt;
				one_count_type newnodebcnt;

				InsertInfo() : nodeCreated(false), newnode(0), oldnodecnt(0), newnodecnt(0), oldnodebcnt(0), newnodebcnt(0) {}
			};

			struct DeleteInfo
			{
				bool deleteNode;
				
				bit_count_type oldnodecnt;
				one_count_type oldnodebcnt;
				
				DeleteInfo()
				: deleteNode(false), oldnodecnt(0), oldnodebcnt(0)
				{}
			};
			
			/**
			 * merge node i and a neighbour
			 **/
			void mergeNodes(
				uint64_t parent, uint64_t i
			)
			{
				#if 0
				std::cerr << "mergeNodes(parent=" << parent << ",i=" << i << ")" << std::endl;	
				#endif

				/**
				 * we only merge if the parent is an inner node
				 **/
				if ( !isLeaf(parent) )
				{
					#if 0
					std::cerr << "Parent not leaf." << std::endl;
					#endif
					
					// get parent node
					inner_node_type * pparent = &getNode(parent);

					// only merge if there is more than one child
					if ( pparent->dataFilled > 1 )
					{
						#if 0
						std::cerr << "Parent is not singular." << std::endl;
						#endif
					
						// determine left and right merged index
						uint64_t left_index, right_index;
						
						if ( i+1 < pparent->dataFilled )
						{
							left_index = i;
							right_index = i+1;
						}
						else
						{
							left_index = i-1;
							right_index = i;
						}
						
						#if 0
						std::cerr << "i=" << i << " left_index=" << left_index << " right_index=" << right_index << std::endl;
						#endif
						
						if ( isLeaf ( pparent->data[left_index].ptr ) )
						{
							uint64_t const left_cnt = pparent->data[left_index].cnt;
							uint64_t const right_cnt = pparent->data[right_index].cnt;
							uint64_t const total_cnt = left_cnt+right_cnt;
							
							node_ptr_type const left_ptr = pparent->data[left_index].ptr;
							leaf_type * const left_leaf = &(getLeaf(left_ptr));
							node_ptr_type const right_ptr = pparent->data[right_index].ptr;
							leaf_type * const right_leaf = &(getLeaf(right_ptr));
							
							::libmaus::uint::UInt<2*w> U(right_leaf->data);
							U <<= left_cnt;
							U |= left_leaf->data;

							/**
							 * total bits fit into one leaf, delete one leaf
							 **/
							if ( total_cnt <= w*64 )
							{
								#if 0
								std::cerr << "Merged data fits in one leaf." << std::endl;
								#endif
								
								// remove right index
								for ( uint64_t j = right_index; j+1 < pparent->dataFilled; ++j )
									pparent->data[j] = pparent->data[j+1];

								// copy data to left leaf
								left_leaf->data = U;
								
								#if 0	
								for ( uint64_t j = 0; j < w; ++j )
									left_leaf->data.A[j] = U.A[j];
								#endif

								pparent->data[left_index].cnt = total_cnt;
								pparent->data[left_index].bcnt = left_leaf->data.rank1(total_cnt-1);
								pparent->dataFilled -= 1;
								freeLeaf(right_ptr);
							}
							/**
							 * total bits do not fit into one leaf, redistribute bits
							 **/
							else
							{
								#if 0
								std::cerr << "Merged data does not fit in one leaf." << std::endl;
								#endif
							
								uint64_t const left_cnt = (total_cnt + 1)/2;
								uint64_t const right_cnt = total_cnt - left_cnt;
								
								::libmaus::uint::UInt<2*w> UUleft = U;
								UUleft.keepLowBits(left_cnt);							
								U >>= left_cnt;
								
								left_leaf->data = UUleft;
								right_leaf->data = U; // UUright;
								
								pparent->data[left_index].cnt = left_cnt;
								pparent->data[left_index].bcnt = left_leaf->data.rank1(left_cnt-1);
								pparent->data[right_index].cnt = right_cnt;
								pparent->data[right_index].bcnt = right_leaf->data.rank1(right_cnt-1);
							}	
						}
						else
						{
							#if 0
							std::cerr << "Merging inner nodes." << std::endl;
							// sleep ( 3 );
							#endif
						
							node_ptr_type left_ptr = pparent->data[left_index].ptr;
							node_ptr_type right_ptr = pparent->data[right_index].ptr;
							inner_node_type * left_node = &(getNode(left_ptr));
							inner_node_type * right_node = &(getNode(right_ptr));
							
							typedef typename inner_node_type::data_type data_type;
							data_type data[4*k];
							
							for ( uint64_t j = 0; j < left_node->dataFilled; ++j )
								data[j] = left_node->data[j];
							for ( uint64_t j = 0; j < right_node->dataFilled; ++j )
								data[j+left_node->dataFilled] = right_node->data[j];
							
							uint64_t totalFilled = left_node->dataFilled + right_node->dataFilled;
							uint64_t totalcnt = pparent->data[left_index].cnt + pparent->data[right_index].cnt;
							uint64_t totalbcnt = pparent->data[left_index].bcnt + pparent->data[right_index].bcnt;
							
							if ( totalFilled <= 2*k )
							{
								#if 0
								std::cerr << "Merged data fits in one node." << std::endl;
								#endif
							
								// remove right index
								for ( uint64_t j = right_index; j+1 < pparent->dataFilled; ++j )
									pparent->data[j] = pparent->data[j+1];
									
								for ( uint64_t j = 0; j < totalFilled; ++j )
									left_node->data[j] = data[j];
								left_node->dataFilled = totalFilled;
								
								pparent->data[left_index].cnt = totalcnt;
								pparent->data[left_index].bcnt = totalbcnt;
								pparent->dataFilled -= 1;
								freeNode(right_ptr);
							}
							else
							{
								#if 0
								std::cerr << "Merged data does not fit in one node." << std::endl;
								#endif
								uint64_t const leftFilled = (totalFilled + 1)/2;
								uint64_t const rightFilled = totalFilled - leftFilled;
								
								uint64_t left_cnt = 0; 
								uint64_t left_bcnt = 0;
								for ( uint64_t j = 0; j < leftFilled; ++j )
								{
									left_node->data[j] = data[j];
									left_cnt += left_node->data[j].cnt;
									left_bcnt += left_node->data[j].bcnt;
								}
								pparent->data[left_index].cnt = left_cnt;
								pparent->data[left_index].bcnt = left_bcnt;
								left_node->dataFilled = leftFilled;
									
								uint64_t right_cnt = 0; 
								uint64_t right_bcnt = 0;
								for ( uint64_t j = 0; j < rightFilled; ++j )
								{
									right_node->data[j] = data[leftFilled+j];
									right_cnt += right_node->data[j].cnt;
									right_bcnt += right_node->data[j].bcnt;							
								}
								pparent->data[right_index].cnt = right_cnt;
								pparent->data[right_index].bcnt = right_bcnt;
								right_node->dataFilled = rightFilled;
							}
						}
					}
				}
			}
			
			DeleteInfo deleteBit(uint64_t node, uint64_t pos, uint64_t const cnt, uint64_t const bcnt)
			{
				if ( isLeaf(node) )
				{
					DeleteInfo info;
					
					leaf_type & leaf = getLeaf(node);
					
					bool const bit = leaf.data.getBit(pos);
					leaf.data.deleteBit(pos);
					
					info.oldnodecnt = cnt-1;
					info.oldnodebcnt = info.oldnodecnt ? leaf.data.rank1(info.oldnodecnt-1) : 0;
					
					if ( bit )
					{
						if ( info.oldnodebcnt != bcnt-1 )
						{
							std::cerr << "expected " << bcnt-1 << " got " << info.oldnodebcnt << std::endl;
						}
						assert ( info.oldnodebcnt == bcnt-1 );
					}
					else
						assert ( info.oldnodebcnt == bcnt );
					
					#if 0
					std::cerr << "node=" << node << " cnt=" << cnt << " info.oldnodecnt=" << info.oldnodecnt << std::endl;
					#endif
					
					info.deleteNode = info.oldnodecnt < (64*w)/2;
					
					return info;
				}
				else
				{
					DeleteInfo info;
					
					inner_node_type * innernode = &(getNode(node));
					
					bool deleted = false;
					
					for ( uint64_t i = 0; (!deleted) && (i < innernode->dataFilled); ++i )
					{
						if ( pos < innernode->data[i].cnt )
						{
							DeleteInfo subinfo = deleteBit(innernode->data[i].ptr, pos, innernode->data[i].cnt, innernode->data[i].bcnt);
							innernode = &(getNode(node));
							innernode->data[i].cnt = subinfo.oldnodecnt;
							innernode->data[i].bcnt = subinfo.oldnodebcnt;
							
							if ( subinfo.deleteNode )
								mergeNodes(node,i);

							innernode = &(getNode(node));
							
							for ( uint64_t j = 0; j < innernode->dataFilled; ++j )
							{
								info.oldnodecnt += innernode->data[j].cnt;
								info.oldnodebcnt += innernode->data[j].bcnt;
							}
							info.deleteNode = (innernode->dataFilled < k);

							deleted = true;
						}
						else
						{
							pos -= innernode->data[i].cnt;
						}
					}
					
					if ( ! deleted )
						throw std::out_of_range("deleteBit called on non-existent position.");
					
					return info;
				}
			}
			void deleteBit(uint64_t pos)
			{
				if ( pos >= getN() )
					throw std::out_of_range("deleteBit called on non-existent position.");
			
				DeleteInfo info = deleteBit(root,pos,root_cnt,root_bcnt);
			
				if ( info.oldnodecnt == 0 )
				{
					if ( isLeaf(root ) )
						freeLeaf(root);
					else
						freeNode(root);
					empty = true;
				}
				
				root_cnt = info.oldnodecnt;
				root_bcnt = info.oldnodebcnt;

		#if defined(BITTREE_DEBUG)
				if ( ! checkTree() )
					throw std::runtime_error("Tree structure corrupt.");
		#endif
			}
			
			InsertInfo insertBit(uint64_t const node, uint64_t pos, bool const b, uint64_t const cnt)
			{
				if ( isLeaf(node) )
				{
					InsertInfo info;

					#if 0			
					std::cerr << "Inserting into leaf." << std::endl;
					#endif
					
					if ( cnt < 64*w )
					{
						info.nodeCreated = false;
						leaf_type & leaf = getLeaf(node);
						leaf.data.insertBit(pos,b);
						info.oldnodecnt = cnt+1;
						info.oldnodebcnt = leaf.data.rank1(info.oldnodecnt-1);
						#if 0
						std::cerr << "Leaf not split." << std::endl;
						#endif
					}
					else
					{
						info.nodeCreated = true;
						
						info.newnode = allocateLeaf();
						leaf_type & leaf = getLeaf(node);
						leaf_type & newleaf = getLeaf(info.newnode);
						
						newleaf.data = leaf.data;
						newleaf.data >>= ((64*w)/2);
						leaf.data.keepLowHalf();
						
						if ( pos <= ((64*w)/2) )
						{
							leaf.data.insertBit(pos, b);
							info.oldnodecnt = ((64*w)/2)+1;
							info.newnodecnt = ((64*w)/2);
						}
						else
						{
							newleaf.data.insertBit(pos-((64*w)/2),b);
							info.oldnodecnt = ((64*w)/2);
							info.newnodecnt = ((64*w)/2)+1;
						}
						
						info.oldnodebcnt = leaf.data.rank1(info.oldnodecnt-1);
						info.newnodebcnt = newleaf.data.rank1(info.newnodecnt-1);								
						
						#if 0
						std::cerr << "Leaf split." << std::endl;
						#endif
					}		
					
					return info;
				}
				else
				{
					InsertInfo info;
					inner_node_type * innernode = &(getNode(node));

					assert ( innernode->dataFilled );
					
					bool const childrenAreLeafs = isLeaf ( innernode->data[0].ptr );

					#if 0
					std::cerr << "Entering with pos " << pos << std::endl;
					
					for ( uint64_t i = 0; i < innernode->dataFilled; ++i )
					{
						std::cerr << "subtree size " << i << " is " << innernode->data[i].cnt << std::endl;
					}
					#endif

					uint64_t const prepos = pos;
					bool continued = false;
					
					bool inserted = false;
					for ( uint64_t i = 0; (!inserted) && i < innernode->dataFilled; ++i )
					{
						if ( continued )
						{
							// std::cerr << "***** i = " << i << std::endl;
							continued = false;
						}
					
						if( pos <= innernode->data[i].cnt )
						{
							if ( childrenAreLeafs )
							{
								if ( innernode->data[i].cnt == (64*w) )
								{
									bool threewaymerge = false;
								
									if ( i == 0 && (innernode->dataFilled>=2) )
									{
										uint64_t const middle_index = i;
										uint64_t const right_index = i+1;
										uint64_t const middle_ptr = innernode->data[middle_index].ptr;
										uint64_t const right_ptr = innernode->data[right_index].ptr;
										leaf_type * const middle_leaf =  &(getLeaf(middle_ptr));
										leaf_type * const right_leaf = &(getLeaf(right_ptr));
										uint64_t const middle_cnt = innernode->data[middle_index].cnt;
										uint64_t const right_cnt = innernode->data[right_index].cnt;

										if ( right_cnt + middle_cnt <= (2*64*w)-2 )
										{
											// std::cerr << "here." << std::endl;
										
											::libmaus::uint::UInt<2*w> U ( right_leaf->data );
											U <<= middle_cnt;
											U |= middle_leaf->data;

											uint64_t const total_cnt = middle_cnt + right_cnt;
											uint64_t const new_middle_cnt = (total_cnt+1)/2;
											uint64_t const new_right_cnt = total_cnt - new_middle_cnt;
												
											middle_leaf->data = U;
											middle_leaf->data.keepLowBits(new_middle_cnt);
											innernode->data[middle_index].cnt = new_middle_cnt;
											innernode->data[middle_index].bcnt = middle_leaf->data.rank1(new_middle_cnt-1);
											
											U >>= new_middle_cnt;
											right_leaf->data = U;
											innernode->data[right_index].cnt = new_right_cnt;
											innernode->data[right_index].bcnt = right_leaf->data.rank1(new_right_cnt-1);
												
											/**
											 * restart loop
											 **/
											i = -1;
											pos = prepos;
											continued = true;
											
											continue;
										}
										else
										{
											threewaymerge = true;
											// std::cerr << "*** right " << right_cnt << " middle " << middle_cnt << std::endl;		
										}
									}
									else if ( (i == innernode->dataFilled-1) && (innernode->dataFilled>=2) )
									{
										uint64_t const left_index = i-1;
										uint64_t const middle_index = i;
										uint64_t const left_ptr = innernode->data[left_index].ptr;
										uint64_t const middle_ptr = innernode->data[middle_index].ptr;
										leaf_type * const left_leaf =  &(getLeaf(left_ptr));
										leaf_type * const middle_leaf =  &(getLeaf(middle_ptr));
										uint64_t const left_cnt = innernode->data[left_index].cnt;
										uint64_t const middle_cnt = innernode->data[middle_index].cnt;
										
										if ( left_cnt + middle_cnt <= (2*64*w)-2 )
										{
											::libmaus::uint::UInt<2*w> U ( middle_leaf->data );
											U <<= left_cnt;
											U |= left_leaf->data;
											
											uint64_t const total_cnt = left_cnt + middle_cnt;
											uint64_t const new_left_cnt = (total_cnt+1)/2;
											uint64_t const new_middle_cnt = total_cnt - new_left_cnt;
											
											left_leaf->data = U;
											left_leaf->data.keepLowBits(new_left_cnt);
											innernode->data[left_index].cnt = new_left_cnt;
											innernode->data[left_index].bcnt = left_leaf->data.rank1(new_left_cnt-1);
											
											U >>= new_left_cnt;
											middle_leaf->data = U;
											innernode->data[middle_index].cnt = new_middle_cnt;
											innernode->data[middle_index].bcnt = middle_leaf->data.rank1(new_middle_cnt-1);
											
											/**
											 * restart loop
											 **/
											i = -1LL;
											// i -= 1;
											pos = prepos;
											continued = true;
											
											continue;
										}
										else
										{
											threewaymerge = true;
											// std::cerr << "*** left " << left_cnt << " middle " << middle_cnt << std::endl;												
										}
									}
									else if ( innernode->dataFilled >= 3 )
									{
										uint64_t const left_index = i-1;
										uint64_t const middle_index = i;
										uint64_t const right_index = i+1;
										uint64_t const left_ptr = innernode->data[left_index].ptr;
										uint64_t const middle_ptr = innernode->data[middle_index].ptr;
										uint64_t const right_ptr = innernode->data[right_index].ptr;
										leaf_type * const left_leaf =  &(getLeaf(left_ptr));
										leaf_type * const middle_leaf =  &(getLeaf(middle_ptr));
										leaf_type * const right_leaf = &(getLeaf(right_ptr));
										uint64_t const left_cnt = innernode->data[left_index].cnt;
										uint64_t const middle_cnt = innernode->data[middle_index].cnt;
										uint64_t const right_cnt = innernode->data[right_index].cnt;
										
										if ( left_cnt < right_cnt )
										{
											if ( left_cnt + middle_cnt <= (2*64*w)-2 )
											{
												::libmaus::uint::UInt<2*w> U ( middle_leaf->data );
												U <<= left_cnt;
												U |= left_leaf->data;
												
												uint64_t const total_cnt = left_cnt + middle_cnt;
												uint64_t const new_left_cnt = (total_cnt+1)/2;
												uint64_t const new_middle_cnt = total_cnt - new_left_cnt;
												
												left_leaf->data = U;
												left_leaf->data.keepLowBits(new_left_cnt);
												innernode->data[left_index].cnt = new_left_cnt;
												innernode->data[left_index].bcnt = left_leaf->data.rank1(new_left_cnt-1);
												
												U >>= new_left_cnt;
												middle_leaf->data = U;
												innernode->data[middle_index].cnt = new_middle_cnt;
												innernode->data[middle_index].bcnt = middle_leaf->data.rank1(new_middle_cnt-1);
												
												/**
												 * restart loop
												 **/
												i = -1LL;
												// i -= 1;
												pos = prepos;
												continued = true;
												
												continue;
											}
											else
											{
												threewaymerge = true;
												// std::cerr << "*** left " << left_cnt << " middle " << middle_cnt << std::endl;												
											}
										}
										else // if ( right_cnt < left_cnt )											
										{
											if ( right_cnt + middle_cnt <= (2*64*w)-2 )
											{
												::libmaus::uint::UInt<2*w> U ( right_leaf->data );
												U <<= middle_cnt;
												U |= middle_leaf->data;

												uint64_t const total_cnt = middle_cnt + right_cnt;
												uint64_t const new_middle_cnt = (total_cnt+1)/2;
												uint64_t const new_right_cnt = total_cnt - new_middle_cnt;
												
												middle_leaf->data = U;
												middle_leaf->data.keepLowBits(new_middle_cnt);
												innernode->data[middle_index].cnt = new_middle_cnt;
												innernode->data[middle_index].bcnt = middle_leaf->data.rank1(new_middle_cnt-1);
												
												U >>= new_middle_cnt;
												right_leaf->data = U;
												innernode->data[right_index].cnt = new_right_cnt;
												innernode->data[right_index].bcnt = right_leaf->data.rank1(new_right_cnt-1);
												
												/**
												 * restart loop
												 **/
												i = -1;
												pos = prepos;
												continued = true;
												
												continue;
											}
											else
											{
												threewaymerge = true;
												// std::cerr << "*** right " << right_cnt << " middle " << middle_cnt << std::endl;		
											}
										}
										
									}
									
									if ( threewaymerge )
									{
										uint64_t left_index, middle_index;
										
										if ( i == innernode->dataFilled-1 )
										{
											left_index = i-1;
											middle_index = i;
										}
										else
										{
											left_index = i;
											middle_index = i+1;
										}
										
										uint64_t const left_cnt = innernode->data[left_index].cnt;
										uint64_t const middle_cnt = innernode->data[middle_index].cnt;
										uint64_t const total_cnt = left_cnt + middle_cnt;
										uint64_t const left_ptr = innernode->data[left_index].ptr;
										uint64_t const middle_ptr = innernode->data[middle_index].ptr;
										leaf_type * const left_leaf =  &(getLeaf(left_ptr));
										leaf_type * const middle_leaf = &(getLeaf(middle_ptr));
										
										uint64_t const new_left_cnt = (total_cnt + 2)/3;
										uint64_t const new_rest_cnt = total_cnt - new_left_cnt;
										uint64_t const new_middle_cnt = (new_rest_cnt + 1)/2;
										uint64_t const new_right_cnt = new_rest_cnt - new_middle_cnt;
										
										assert ( (new_left_cnt + new_middle_cnt + new_right_cnt) == total_cnt );
										
										::libmaus::uint::UInt<2*w> U = middle_leaf->data;
										U <<= left_cnt;
										U |= left_leaf->data;
										
										// std::cerr << "three way merge " << left_cnt << " " << middle_cnt << std::endl;
									}
								}
							}
						
							#if 0
							std::cerr << "Inserting into node " << node << " after child " << i << " at position " << pos << std::endl;
							#endif
							InsertInfo subinfo = insertBit(innernode->data[i].ptr, pos, b, innernode->data[i].cnt);
							// pointer might be invalid, set it up again
							innernode = &(getNode(node));

							#if 0
							std::cerr << "Returned from inserting at node " << node << " child " << i << std::endl;
							#endif
							
							if ( subinfo.nodeCreated )
							{
								#if 0
								std::cerr << "New node was created." << std::endl;
								#endif
							
								if ( innernode->dataFilled < 2*k )
								{
									#if 0
									std::cerr << "Inserting node " 
										<< subinfo.newnode 
										<< " into inner node " << node 
										<< " after child " << i << ", no split." << std::endl;
									#endif
								
									for ( uint64_t j = 0; j < 2*k-i-2; ++j )
									{
										#if 0
										std::cerr << "copy " <<
											2*k-j-2 << " to " << 2*k-j-1 << std::endl;
										#endif

										innernode->data[2*k-j-1] = innernode->data[2*k-j-2];
									}
									#if 0
									std::cerr << (i+1) << " should now be free." << std::endl;
									#endif
									
									innernode->data[i].cnt = subinfo.oldnodecnt;
									innernode->data[i].bcnt = subinfo.oldnodebcnt;
									innernode->data[i+1].cnt = subinfo.newnodecnt;
									innernode->data[i+1].bcnt = subinfo.newnodebcnt;
									innernode->data[i+1].ptr = subinfo.newnode;
									innernode->dataFilled += 1;							
								}
								else
								{
									#if 0
									std::cerr << "Inserting node " 
										<< subinfo.newnode 
										<< " into inner node "<< node << " after child " << i << ", split." << std::endl;
									#endif
								
									typedef typename inner_node_type::data_type data_type;
									data_type localdata[2*k+1];
									for ( uint64_t j = 0 ; j < innernode->dataFilled; ++j )
										localdata[j] = innernode->data[j];

									for ( uint64_t j = 0; j < (2*k+1)-i-2; ++j )
									{
										#if 0
										std::cerr << "copy " <<
											(2*k+1)-j-2 << " to " << (2*k+1)-j-1 << std::endl;
										#endif

										localdata[(2*k+1)-j-1] = localdata[(2*k+1)-j-2];
									}
									#if 0
									std::cerr << (i+1) << " should now be free." << std::endl;
									#endif
									
									localdata[i].cnt = subinfo.oldnodecnt;
									localdata[i].bcnt = subinfo.oldnodebcnt;
									localdata[i+1].cnt = subinfo.newnodecnt;
									localdata[i+1].bcnt = subinfo.newnodebcnt;
									localdata[i+1].ptr = subinfo.newnode;
									
									info.newnode = allocateNode();
									#if 0
									std::cerr << "allocated inner node " << info.newnode << std::endl;
									#endif
									info.nodeCreated = true;
									inner_node_type * newinnernode = &(getNode(info.newnode));
									innernode = &(getNode(node));
									
									for ( uint64_t j = 0; j < k; ++j )
										innernode->data[j] = localdata[j];
									innernode->dataFilled = k;
									
									for ( uint64_t j = 0; j < k+1; ++j )
										newinnernode->data[j] = localdata[k+j];
									newinnernode->dataFilled = k+1;
								
									// throw std::runtime_error("Node insertion not supported.");
								}
							}
							else
							{
								innernode->data[i].cnt = subinfo.oldnodecnt;
								innernode->data[i].bcnt = subinfo.oldnodebcnt;			
							}
							
							for ( uint64_t j = 0; j < innernode->dataFilled; ++j )
							{
								info.oldnodecnt += innernode->data[j].cnt;
								info.oldnodebcnt += innernode->data[j].bcnt;
							}
							if ( info.nodeCreated )
							{
								inner_node_type * newinnernode = &(getNode(info.newnode));

								for ( uint64_t j = 0; j < newinnernode->dataFilled; ++j )
								{
									info.newnodecnt += newinnernode->data[j].cnt;
									info.newnodebcnt += newinnernode->data[j].bcnt;
								}
							
							}
							
							inserted = true;
						}
					
						#if 0
						std::cerr << "Subtracting " << innernode->data[i].cnt << " from pos " << std::endl;	
						#endif
						pos -= innernode->data[i].cnt;
					}
					
					if ( ! inserted )
						throw std::out_of_range("Inserted bit beyond last position.");
					
					return info;
				}		
			}

			void set(uint64_t const pos, bool const b)
			{
				assert ( pos < size() );

				#if 0
				uint64_t const precnt = root_cnt, prebcnt = root_bcnt;
				#endif
				
				deleteBit(pos);
				
				#if 0
				checkTree();
				assert ( root_cnt == precnt-1 );
				assert ( root_bcnt == prebcnt || root_bcnt == prebcnt-1 );
				#endif
				
				insertBit(pos,b);
				
				#if 0
				assert ( root_cnt == precnt );
				#endif
			}

			void insertBit(uint64_t pos, bool b)
			{
				if ( empty )
				{
					assert ( pos == 0 );
					root = allocateLeaf();
					empty = false;
				}

				InsertInfo info = insertBit(root,pos,b,root_cnt);
				
				if ( info.nodeCreated )
				{
					node_ptr_type newroot = allocateNode();
					#if 0
					std::cerr << "newroot=" << newroot << std::endl;
					#endif
					inner_node_type & newrootnode = getNode(newroot);
					
					newrootnode.data[0].ptr = root;
					newrootnode.data[0].cnt = info.oldnodecnt;
					newrootnode.data[0].bcnt = info.oldnodebcnt;
					
					newrootnode.data[1].ptr = info.newnode;
					newrootnode.data[1].cnt = info.newnodecnt;
					newrootnode.data[1].bcnt = info.newnodebcnt;
					
					newrootnode.dataFilled = 2;		
					
					root = newroot;			
					root_cnt = info.oldnodecnt + info.newnodecnt;
					root_bcnt = info.oldnodebcnt + info.newnodebcnt;

					depth += 1;
				}
				else
				{
					root_cnt = info.oldnodecnt;
					root_bcnt = info.oldnodebcnt;	
				}
				
		#if defined(BITTREE_DEBUG)
				if ( ! checkTree() )
					throw std::runtime_error("Tree structure corrupt.");
		#endif
			}
			
			uint64_t getN() const
			{
				return root_cnt;
			}
			
			bool setBitQuick(uint64_t i, bool const b)
			{
				assert ( i < size() );
				bool const changed = setBitQuick(root,i,b);	
				
				if ( changed )
				{
					if ( b )
						++root_bcnt;
					else
						--root_bcnt;
				}
				
				return changed;
			}
			
			bool setBitQuick(uint64_t node, uint64_t i, bool const b)
			{
				if ( isLeaf(node) )
				{
					leaf_type & leaf = getLeaf(node);
					bool const pre = leaf.data.getBit(i);
					if ( b != pre )
					{
						leaf.data.setBit(i,b);
						return true;
					}
					else
					{
						return false;
					}
				}
				else
				{
					inner_node_type & innernode = getNode(node);
					
					for ( uint64_t j = 0; j < innernode.dataFilled; ++j )
					{
						if ( i < innernode.data[j].cnt )
						{
							bool const changed = setBitQuick(innernode.data[j].ptr,i,b);
							if ( changed )
							{
								if ( b )
									innernode.data[j].bcnt++;
								else
									innernode.data[j].bcnt--;
							}
							return changed;
						}
						else
							i -= innernode.data[j].cnt;
					}
					
					throw std::out_of_range("Position does not exist.");
				}
			}

			bool access(uint64_t node, uint64_t i) const
			{
				if ( isLeaf(node) )
				{
					leaf_type const & leaf = getLeaf(node);
					return leaf.data.getBit(i);
				}
				else
				{
					inner_node_type const & innernode = getNode(node);
					
					for ( uint64_t j = 0; j < innernode.dataFilled; ++j )
					{
						if ( i < innernode.data[j].cnt )
							return access(innernode.data[j].ptr,i);
						else
							i -= innernode.data[j].cnt;
					}
					
					throw std::out_of_range("Position does not exist.");
				}
			}

			uint64_t rank1(uint64_t node, uint64_t i) const
			{
				if ( isLeaf(node) )
				{
					leaf_type const & leaf = getLeaf(node);
					return leaf.data.rank1(i);
				}
				else
				{
					inner_node_type const & innernode = getNode(node);
					
					uint64_t bcnt = 0;
					
					for ( uint64_t j = 0; j < innernode.dataFilled; ++j )
					{
						if ( i < innernode.data[j].cnt )
							return bcnt + rank1(innernode.data[j].ptr,i);
						else
						{
							i -= innernode.data[j].cnt;
							bcnt += innernode.data[j].bcnt;
						}
					}
					
					throw std::out_of_range("Position does not exist.");
				}
			}

			std::pair<uint64_t,uint64_t> inverseSelect1(uint64_t node, uint64_t i) const
			{
				if ( isLeaf(node) )
				{
					leaf_type const & leaf = getLeaf(node);
					return std::pair<uint64_t,uint64_t>(leaf.data.getBit(i),leaf.data.rank1(i));
				}
				else
				{
					inner_node_type const & innernode = getNode(node);
					
					uint64_t bcnt = 0;
					
					for ( uint64_t j = 0; j < innernode.dataFilled; ++j )
					{
						if ( i < innernode.data[j].cnt )
						{
							std::pair < uint64_t, uint64_t > P = inverseSelect1(innernode.data[j].ptr,i);
							return std::pair<uint64_t,uint64_t>(P.first,bcnt + P.second);
						}
						else
						{
							i -= innernode.data[j].cnt;
							bcnt += innernode.data[j].bcnt;
						}
					}
					
					throw std::out_of_range("Position does not exist.");
				}
			}

			uint64_t select1(uint64_t node, uint64_t i) const
			{
				if ( isLeaf(node) )
				{
					leaf_type const & leaf = getLeaf(node);
					return leaf.data.select1(i);
				}
				else
				{
					inner_node_type const & innernode = getNode(node);
					
					uint64_t cnt = 0;

					for ( uint64_t j = 0; j < innernode.dataFilled; ++j )
					{
						if ( i < innernode.data[j].bcnt )
							return cnt + select1(innernode.data[j].ptr,i);
						else
						{
							i -= innernode.data[j].bcnt;
							cnt += innernode.data[j].cnt;
						}
					}
					
					throw std::out_of_range("Rank does not exist.");
				}
			}

			uint64_t select0(uint64_t node, uint64_t i) const
			{
				if ( isLeaf(node) )
				{
					leaf_type const & leaf = getLeaf(node);
					return leaf.data.select0(i);
				}
				else
				{
					inner_node_type const & innernode = getNode(node);
					
					uint64_t cnt = 0;

					for ( uint64_t j = 0; j < innernode.dataFilled; ++j )
					{
						uint64_t bcnt = 
							innernode.data[j].cnt-innernode.data[j].bcnt;
					
						if ( i < bcnt )
							return cnt + select0(innernode.data[j].ptr,i);
						else
						{
							i -= bcnt;
							cnt += innernode.data[j].cnt;
						}
					}
					
					throw std::out_of_range("Rank does not exist.");
				}
			}
			
			bool operator==(this_type const & o) const
			{
				if ( 
					this->root_cnt != o.root_cnt ||
					this->root_bcnt != o.root_bcnt ||
					this->empty != o.empty
				)
					return false;
				else
					return true;
			}
			
			bool operator[](uint64_t i) const
			{
				return access(root,i);
			}

			std::pair<uint64_t,uint64_t> inverseSelect1(uint64_t i) const
			{
				return inverseSelect1(root,i);
			}

			uint64_t rank1(uint64_t i) const
			{
				return rank1(root,i);
			}

			uint64_t rank0(uint64_t i) const
			{
				return (i+1) - rank1(i);
			}

			uint64_t select1(uint64_t i) const
			{
				return select1(root,i);
			}

			uint64_t select0(uint64_t i) const
			{
				return select0(root,i);
			}
			
			void toString(std::ostream & out) const
			{
				if ( empty )
					out << "empty";
				else
					toString(out,root,root_cnt,0);
			}
			static void depthPrint(std::ostream & out, uint64_t depth)
			{
				for ( uint64_t i = 0; i < depth; ++i )
					out << " ";
			}
			void toString(std::ostream & out, uint64_t node, uint64_t numbits, uint64_t depth) const
			{
				if ( isLeaf(node) )
				{
					leaf_type const & leaf = getLeaf(node);
					std::ostringstream ostr;
					depthPrint(out,depth);
					out << "Leaf["<<node<<"](";
					ostr << leaf.data;
					out << ostr.str().substr(0,numbits);
					out << ")" << std::endl;
				}
				else
				{
					inner_node_type const & innernode = getNode(node);
					depthPrint(out,depth);
					out << "Node["<<node<<"](" << std::endl;
					for ( uint64_t i = 0; i < innernode.dataFilled; ++i )
					{
						depthPrint(out,depth+1);
						out << "(" << std::endl;
						toString(out,innernode.data[i].ptr,innernode.data[i].cnt,depth+1);
						depthPrint(out,depth+1);
						out << "," << innernode.data[i].cnt;
						out << "," << innernode.data[i].bcnt;
						out << ")" << std::endl;
					}
					depthPrint(out,depth);
					out << ")" << std::endl;
				}
			}
					
			node_ptr_type allocateNode() { return innernodes->allocate(); }
			void freeNode(node_ptr_type const node) { innernodes->free(node); }
			inner_node_type & getNode(node_ptr_type node) { return (*innernodes)[node]; }
			inner_node_type const & getNode(node_ptr_type node) const { return (*innernodes)[node]; }

			node_ptr_type allocateLeaf() { return leafnodes->allocate(); }
			void freeLeaf(node_ptr_type const node) { leafnodes->free(node); }
			leaf_type & getLeaf(node_ptr_type node) { return (*leafnodes)[node]; }
			leaf_type const & getLeaf(node_ptr_type node) const { return (*leafnodes)[node]; }
		};

		template<unsigned int k, unsigned int w>
		std::ostream & operator<< ( std::ostream & out, BitBTree<k,w> const & B)
		{
			// B.toString(out);
			for ( uint64_t i = 0; i < B.getN(); ++i )
				out << B[i];
			return out;
		}
	}
}
#endif
