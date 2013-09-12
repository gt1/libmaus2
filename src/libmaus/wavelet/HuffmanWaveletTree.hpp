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

#if ! defined(HUFFMANWAVELETTREE_HPP)
#define HUFFMANWAVELETTREE_HPP

#include <libmaus/huffman/HuffmanSorting.hpp>
#include <libmaus/util/stringFunctions.hpp>
#include <libmaus/wavelet/WaveletTree.hpp>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/rank/ERank222BAppend.hpp>
#include <libmaus/bitio/CheckedBitWriter.hpp>
#include <libmaus/bitio/CompactArray.hpp>
#include <libmaus/util/unique_ptr.hpp>

#include <set>
#include <ctime>


namespace libmaus
{
	namespace wavelet
	{
		struct HuffmanWaveletTree
		{
			typedef ::libmaus::rank::ERank222B rank_type;
			typedef ::libmaus::util::unique_ptr < HuffmanWaveletTree >::type unique_ptr_type;

			struct HuffmanWaveletTreeNavigationNode
			{
				typedef ::libmaus::util::unique_ptr<HuffmanWaveletTreeNavigationNode >::type unique_ptr_type;

				unique_ptr_type left;
				unique_ptr_type right;
				uint64_t offset;
				uint64_t prerank0;
				uint64_t prerank1;
				uint64_t num0;
				uint64_t num1;
				uint64_t n;
				
				void fillIdMap(std::map<  HuffmanWaveletTreeNavigationNode const *, uint64_t > & idmap, uint64_t & curid ) const
				{
					idmap[this] = curid++;

					if ( getLeft() )
						getLeft()->fillIdMap(idmap,curid);
					if ( getRight() )
						getRight()->fillIdMap(idmap,curid);
				}

				void fillIdMap(std::map<  HuffmanWaveletTreeNavigationNode const *, uint64_t > & idmap ) const
				{
					uint64_t curid = 0;
					fillIdMap(idmap,curid);
				}
				
				std::map<  HuffmanWaveletTreeNavigationNode const *, uint64_t > getIdMap() const
				{
					std::map<  HuffmanWaveletTreeNavigationNode const *, uint64_t > idmap;
					fillIdMap(idmap);
					return idmap;
				}
				
				void lineSerialise(std::ostream & out, std::map<  HuffmanWaveletTreeNavigationNode const *, uint64_t > const & idmap) const
				{
					if ( getLeft() )
						getLeft()->lineSerialise(out,idmap);
					if ( getRight() )
						getRight()->lineSerialise(out,idmap);
						
					out 
						<< idmap.find(this)->second << "\t"
						<< offset << "\t"
						<< (getLeft() ? idmap.find(getLeft())->second : 0) << "\t"
						<< (getRight() ? idmap.find(getRight())->second : 0) << "\n";
				}

				void lineSerialise(std::ostream & out) const
				{
					std::map<  HuffmanWaveletTreeNavigationNode const *, uint64_t > idmap = getIdMap();
					out << "HuffmanWaveletTreeNavigationNode" << "\t" << idmap.size() << "\n";
					lineSerialise(out,idmap);
				}
				
				static unique_ptr_type deserialiseSimple(std::istream & in)
				{
					std::string line;
					std::getline(in,line);

					std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");

					if ( tokens.size() != 2 || (tokens[0] != "HuffmanWaveletTreeNavigationNode") )
						throw std::runtime_error("Malformed input in HuffmanWaveletTreeNavigationNode::deserialiseSimple()");
					
					uint64_t const numnodes = atol( tokens[1].c_str() );
					std::vector < HuffmanWaveletTreeNavigationNode * > nodes(numnodes);

					try
					{
						HuffmanWaveletTreeNavigationNode::unique_ptr_type uroot;
						
						for ( uint64_t i = 0; i < numnodes; ++i )
						{
							std::getline(in,line);
							
							if ( ! in )
								throw std::runtime_error("Stream invalid before read complete in HuffmanWaveletTreeNavigationNode::deserialiseSimple()");
							
							tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");	

							if ( tokens.size() != 4 )
								throw std::runtime_error("Invalid input in HuffmanWaveletTreeNavigationNode::deserialiseSimple()");
							
							uint64_t const nodeid = atol( tokens[0].c_str() );
							
							if ( nodeid >= numnodes )
								throw std::runtime_error("Invalid node id in HuffmanWaveletTreeNavigationNode::deserialiseSimple()");
								
							uint64_t const offset = atoll(tokens[1].c_str());
								
							nodes[nodeid] = new HuffmanWaveletTreeNavigationNode(offset);
							
							uint64_t const left = atol(tokens[2].c_str());
							uint64_t const right = atol(tokens[3].c_str());

							if ( left >= numnodes || (left && (!nodes[left])) )
								throw std::runtime_error("Invalid node id in HuffmanWaveletTreeNavigationNode::deserialiseSimple()");
							if ( right >= numnodes || (right && (!nodes[right])) )
								throw std::runtime_error("Invalid node id in HuffmanWaveletTreeNavigationNode::deserialiseSimple()");
								
							if ( left )
							{
								HuffmanWaveletTreeNavigationNode::unique_ptr_type tnodesnodeidleft(nodes[left]);
								nodes[nodeid]->left = UNIQUE_PTR_MOVE(tnodesnodeidleft);
								nodes[left] = 0;
							}
							if ( right )
							{
								HuffmanWaveletTreeNavigationNode::unique_ptr_type tnodesnodeidright(nodes[right]);
								nodes[nodeid]->right = UNIQUE_PTR_MOVE(tnodesnodeidright);
								nodes[right] = 0;
							}
						}
						
						if ( ! nodes[0] )
							throw std::runtime_error("No root node produced in huffman::HuffmanTreeNode::simpleDeserialise()");
							
						for ( uint64_t i = 1; i < numnodes; ++i )
							if ( nodes[i] )
								throw std::runtime_error("Unlinked node produced in huffman::HuffmanTreeNode::simpleDeserialise()");

						HuffmanWaveletTreeNavigationNode::unique_ptr_type turoot(nodes[0]);
						uroot = UNIQUE_PTR_MOVE(turoot);
						nodes[0] = 0;
					
						return UNIQUE_PTR_MOVE(uroot);
					}
					catch(...)
					{
						for ( uint64_t i = 0; i < nodes.size(); ++i )
							delete  nodes[i];
						throw;
					}
				
				}
				
				void setupRank(rank_type const & R, uint64_t const rn)
				{
					n = rn;
				
					if ( offset )
					{
						prerank0 = R.rank0(offset-1);
						prerank1 = R.rank1(offset-1);	
					}
					else
					{
						prerank0 = 0;
						prerank1 = 0;
					}
					if ( n )
					{
						num0 = R.rank0(offset + n - 1 ) - prerank0;
						num1 = R.rank1(offset + n - 1 ) - prerank1; 
					}
					else
					{
						num0 = 0;
						num1 = 0;
					}
					
					if ( getLeft() )
						left->setupRank(R,num0);
					if ( getRight() )
						right->setupRank(R,num1);
				}
				
				HuffmanWaveletTreeNavigationNode(uint64_t const roffset)
				: offset(roffset), prerank0(0), prerank1(0)
				{
				
				}
				~HuffmanWaveletTreeNavigationNode()
				{
				
				}
				
				HuffmanWaveletTreeNavigationNode const * getLeft() const
				{
					return left.get();
				}
				HuffmanWaveletTreeNavigationNode const * getRight() const
				{
					return right.get();
				}
				
				void structureVector(std::vector<bool> & B) const
				{
					B.push_back(0);
					
					if ( getLeft() )
					{
						B.push_back(1);
						getLeft()->structureVector(B);
					}
					else
					{
						B.push_back(0);
					}
						
					if ( getRight() )
					{
						B.push_back(1);
						getRight()->structureVector(B);
					}
					else
					{
						B.push_back(0);			
					}
					B.push_back(1);
				}
				static std::vector<bool> structureVector(HuffmanWaveletTreeNavigationNode const * H)
				{
					std::vector<bool> B;
					if ( H )
						H->structureVector(B);
					return B;
				}
				static autoarray::AutoArray<uint64_t> structureArray(HuffmanWaveletTreeNavigationNode const * H)
				{
					std::vector < bool > B = structureVector(H);
					
					autoarray::AutoArray<uint64_t> A( (B.size() + 63) / 64 );
					bitio::BitWriter8 W(A.get());
					for ( uint64_t i = 0; i < B.size(); ++i )
					{
						W.writeBit(B[i]);
					}
					W.flush();
					
					return A;
				}
				
				void offsetVector(std::vector < uint64_t > & V) const
				{
					V.push_back(offset);
					if ( getLeft() )
						getLeft()->offsetVector(V);
					if ( getRight() )
						getRight()->offsetVector(V);
				}
				static std::vector<uint64_t> offsetVector(HuffmanWaveletTreeNavigationNode const * H)
				{
					std::vector<uint64_t> V;
					if ( H )
						H->offsetVector(V);
					return V;
				}
				static autoarray::AutoArray<uint64_t> offsetArray(HuffmanWaveletTreeNavigationNode const * H)
				{
					std::vector<uint64_t> V = offsetVector(H);
					autoarray::AutoArray<uint64_t> A(V.size());
					std::copy ( V.begin(), V.end(), A.get() );
					return A;
				}

				static uint64_t serialize(std::ostream & out, HuffmanWaveletTreeNavigationNode const * H)
				{
					autoarray::AutoArray<uint64_t> offset = offsetArray(H);
					autoarray::AutoArray<uint64_t> struc = structureArray(H);
					
					uint64_t s = 0;
					s += offset.serialize(out);
					s += struc.serialize(out);
					
					return s;
				}
				
				static unique_ptr_type deserialize(
					uint64_t const * const offset,
					uint64_t & ioffset,
					uint64_t const * const struc,
					uint64_t & istruc
					)
				{
					assert ( ! bitio::getBit(struc,istruc) );
					istruc++;
					
					unique_ptr_type N ( new HuffmanWaveletTreeNavigationNode(offset[ioffset++]) );
					
					if ( bitio::getBit(struc,istruc++) )
					{
						unique_ptr_type tNleft(deserialize(offset,ioffset,struc,istruc));
						N->left = UNIQUE_PTR_MOVE(tNleft);
					}
					if ( bitio::getBit(struc,istruc++) )
					{
						unique_ptr_type tNright(deserialize(offset,ioffset,struc,istruc));
						N->right = UNIQUE_PTR_MOVE(tNright);
					}
					
					assert (   bitio::getBit(struc,istruc) );
					istruc++;
					
					return UNIQUE_PTR_MOVE(N);
				}
				
				static unique_ptr_type deserialize(std::istream & in, uint64_t & s)
				{
					autoarray::AutoArray<uint64_t> offset;
					autoarray::AutoArray<uint64_t> struc;
					
					s += offset.deserialize(in);
					s += struc.deserialize(in);
					
					if ( ! offset.getN() )
					{
						unique_ptr_type ptr;
						return UNIQUE_PTR_MOVE(ptr);
					}
					else
					{
						uint64_t ioffset = 0;
						uint64_t istruc = 0;
						unique_ptr_type ptr(deserialize(offset.get(),ioffset,struc.get(),istruc));
						return UNIQUE_PTR_MOVE(ptr);
					}
				}
				
				void print() const
				{
					std::cerr << "HuffmanWaveletTreeNavigationNode(offset=" << offset << ",";
					
					if ( left.get() )
						left->print();
					else
						std::cerr << "0";
						
					std::cerr << ",";

					if ( right.get() )
						right->print();
					else
						std::cerr << "0";
					
					std::cerr << ")";
				}
			};

			static unsigned int const lookupwords = 1;

			uint64_t const n;
			::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type aroot;
			huffman::HuffmanTreeNode const * root;
			huffman::EncodeTable<lookupwords> const enctable;
			autoarray::AutoArray < uint64_t > acode;
			uint64_t * const code;
			HuffmanWaveletTreeNavigationNode::unique_ptr_type anavroot;
			HuffmanWaveletTreeNavigationNode const * const navroot;
			::libmaus::util::unique_ptr<rank_type>::type UR;
			rank_type const * R;
			
			private:
			HuffmanWaveletTree(
				uint64_t const rn,
				::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type raroot,
				autoarray::AutoArray < uint64_t > & racode,
				HuffmanWaveletTreeNavigationNode::unique_ptr_type & ranavroot
			)
			: n(rn),  aroot(raroot), root(aroot.get()),  enctable(root), acode(racode), code(acode.get()),
			  anavroot(UNIQUE_PTR_MOVE(ranavroot)), navroot(anavroot.get()),
			  UR ( new rank_type(acode.get(), acode.getN()*64) ),
			  R ( UR.get() )
			{

			}
			
			public:
			void simpleSerialise(std::ostream & out) const
			{
				out << "HuffmanWaveletTree" << "\t" << n << std::endl;
				root->lineSerialise(out);
				navroot->lineSerialise(out);
				out << "Code" << "\t" << acode.getN() << "\n";
				
				for ( uint64_t i = 0; i < acode.getN(); ++i )
				{
					uint64_t const v = acode[i];
					
					out.put ( (v >> (7*8)) & 0xFF );
					out.put ( (v >> (6*8)) & 0xFF );
					out.put ( (v >> (5*8)) & 0xFF );
					out.put ( (v >> (4*8)) & 0xFF );
					out.put ( (v >> (3*8)) & 0xFF );
					out.put ( (v >> (2*8)) & 0xFF );
					out.put ( (v >> (1*8)) & 0xFF );
					out.put ( (v >> (0*8)) & 0xFF );
				}
			}
			

			static unique_ptr_type simpleDeserialise(std::istream & in)
			{
				std::string line;
				std::getline(in,line);
				
				if ( ! in )
					throw std::runtime_error("Stream failure in HuffmanWaveleTree::simpleDeserialise()");
					
				std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");
				
				if ( tokens.size() != 2 || tokens[0] != "HuffmanWaveletTree" )
					throw std::runtime_error("Malformed input in HuffmanWaveleTree::simpleDeserialise()");
					
				uint64_t const n = atoll(tokens[1].c_str());
				
				::libmaus::util::shared_ptr<huffman::HuffmanTreeNode>::type aroot = huffman::HuffmanTreeNode::simpleDeserialise(in);
				HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type anavroot = HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::deserialiseSimple(in);

				std::getline(in,line);
				
				if ( ! in )
					throw std::runtime_error("Stream failure in HuffmanWaveleTree::simpleDeserialise()");

				tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");
				
				if ( tokens.size() != 2 || tokens[0] != "Code" )
					throw std::runtime_error("Malformed input in HuffmanWaveleTree::simpleDeserialise()");
					
				uint64_t const codewords = atoll(tokens[1].c_str());
				autoarray::AutoArray<uint64_t> acode(codewords);
				
				for ( uint64_t i = 0; i < codewords; ++i )
				{
					uint64_t v = 0;
					for ( unsigned int j = 0; j < 8; ++j )
					{
						v <<= 8;
						v |= in.get();
					}
					
					acode[i] = v;
				}

				HuffmanWaveletTree::unique_ptr_type ptr( new HuffmanWaveletTree(n,aroot,acode,anavroot) );
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			uint64_t serialize(std::ostream & out) const
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				s += aroot->serialize(out);
				s += acode.serialize(out);
				s += HuffmanWaveletTreeNavigationNode::serialize(out,navroot);
				return s;
			}
			
			struct Node
			{
				HuffmanWaveletTreeNavigationNode const * node;
				uint64_t n;
				
				Node() : node(0), n(0) {}
				Node(HuffmanWaveletTreeNavigationNode const * rnode, uint64_t rn) : node(rnode), n(rn) {}
			};
			
			std::ostream & toString(std::ostream & out, Node const node, unsigned int depth) const
			{
				out << std::string(depth,' ');
				out << "Node(node.node=" << node.node
					<< ",node.node->getLeft()=" << node.node->getLeft()
					<< ",node.node->getRight()=" << node.node->getRight()
					<< ",node.node->offset=" << node.node->offset
					<< ",node.n=" << node.n << ",";
					
				for ( uint64_t i = 0; i < node.n; ++i )
					out << bitio::getBit(code,node.node->offset + i);
					
				out
					<<")" << std::endl;
					
				Node const left = leftChild(node);
				Node const right = rightChild(node);
				
				if ( left.node )
					toString(out,left,depth+1);
				if ( right.node )
					toString(out,right,depth+1);
				
				return out;
			}
			std::ostream & toString(std::ostream & out) const
			{
				Node const node = rootNode();
				return toString(out,node,0);
			}
			
			uint64_t exrank0(uint64_t const i) const { return i ? R->rank0(i-1) : 0; }
			uint64_t exrank1(uint64_t const i) const { return i ? R->rank1(i-1) : 0; }
			
			Node leftChild(Node const & node) const
			{
				if ( node.node )
					return Node( node.node->getLeft(), exrank0(node.node->offset + node.n) - exrank0(node.node->offset) );
				else
					return Node( 0, 0 );
			}

			Node rightChild(Node const & node) const
			{
				if ( node.node )
					return Node( node.node->getRight(), exrank1(node.node->offset + node.n) - exrank1(node.node->offset) );
				else
					return Node( 0, 0);
			}
			
			uint64_t select0(HuffmanWaveletTreeNavigationNode const * node, uint64_t i) const
			{
				return R->select0( node->prerank0 + i ) - node->offset ;
			}

			uint64_t select1(HuffmanWaveletTreeNavigationNode const * node, uint64_t i) const
			{
				return R->select1( node->prerank1 + i ) - node->offset ;
			}
			
			uint64_t exrank0(HuffmanWaveletTreeNavigationNode const * node, uint64_t i) const
			{
				return exrank0( node->offset + i ) - node->prerank0; // exrank0(node.node->offset);
			}

			uint64_t exrank1(HuffmanWaveletTreeNavigationNode const * node, uint64_t i) const
			{
				return exrank1( node->offset + i ) - node->prerank1; // exrank1(node.node->offset);
			}
			
			Node rootNode() const
			{
				return Node ( navroot , n );
			}
			
			struct NodePortion
			{
				HuffmanWaveletTreeNavigationNode const * node;
				uint64_t left;
				uint64_t right;
				
				NodePortion() : node(0), left(0), right(0) {}
				NodePortion(HuffmanWaveletTreeNavigationNode const * rnode, uint64_t rleft, uint64_t rright)
				: node(rnode), left(rleft), right(rright) {}
			};
			
			NodePortion leftChild(NodePortion const & portion) const
			{
				return NodePortion(
					portion.node->getLeft(),
					exrank0(portion.node, portion.left),
					exrank0(portion.node, portion.right)
				);	
			}
			NodePortion rightChild(NodePortion const & portion) const
			{
				return NodePortion(
					portion.node->getRight(),
					exrank1(portion.node, portion.left),
					exrank1(portion.node, portion.right)
				);	
			}

			uint64_t rank0(NodePortion const & portion, uint64_t i) const
			{
				return R->rank0( portion.node->offset + portion.left + i ) - exrank0(portion.node->offset + portion.left);
			}
			uint64_t rank1(NodePortion const & portion, uint64_t i) const
			{
				return R->rank1( portion.node->offset + portion.left + i ) - exrank1(portion.node->offset + portion.left);
			}

			
			NodePortion rootNode(uint64_t const left, uint64_t const right) const
			{
				return NodePortion(navroot,left,right);
			}


			template<unsigned int lookupwords>
			static void countMsbLsb(
				uint64_t * const acode,
				uint64_t const offset,
				uint64_t const n,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable,
				uint64_t & lsb,
				uint64_t & lsbbits,
				uint64_t & msb,
				uint64_t & msbbits,
				uint64_t * const msbspace
				)
			{
				unsigned int nodeid = 0;
				uint64_t decsyms = 0;
				msb = 0; lsb = 0; msbbits = 0; lsbbits = 0;
				for ( unsigned int i = 0; decsyms != n; ++i )
				{
					huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode,offset + dectable.lookupbits*i,dectable.lookupbits));

					for ( unsigned int j = 0; j < entry.symbols.size() && decsyms != n; ++j, ++decsyms )
					{
						std::pair < ::libmaus::uint::UInt < lookupwords >, unsigned int > const & code = enctable[entry.symbols[j]];
						bool const ismsb = (code.first >> (code.second-1)) == ::libmaus::uint::UInt<lookupwords>(1ull);
						
						// msb
						if ( ismsb )
						{
							msb++;
							msbbits += code.second;
						}
						// lsb
						else
						{
							lsb++;
							lsbbits += code.second;
						}
						
						bitio::putBit ( msbspace, decsyms, ismsb );
					}
						
					nodeid = entry.nexttable;
				}
			}


			static HuffmanWaveletTreeNavigationNode::unique_ptr_type huffmanWaveletTreeBits ( 
				uint64_t * const acode, 
				uint64_t const offset, 
				uint64_t const n, 
				huffman::HuffmanTreeNode const * rnode
			)
			{
				std::cerr << "huffmanWaveletTreeBits(offset=" << offset << ",n=" << n << ")" << std::endl;
			
				if ( rnode->isLeaf() )
					return HuffmanWaveletTreeNavigationNode::unique_ptr_type();
				if ( ! n )
					return HuffmanWaveletTreeNavigationNode::unique_ptr_type();
					
				huffman::HuffmanTreeInnerNode const * const node = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(rnode);
				assert ( node );

				// build encoding table
				unsigned int const lookupwords = 2;
				huffman::EncodeTable<lookupwords> enctable ( node );

				unsigned int const lookupbits = 8;
				huffman::DecodeTable dectable ( node, lookupbits );

				huffman::DecodeTable dectable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(node)->left, dectable.lookupbits );
				huffman::DecodeTable dectable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(node)->right, dectable.lookupbits );
				huffman::EncodeTable<lookupwords> enctable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(node)->left );
				huffman::EncodeTable<lookupwords> enctable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(node)->right );

				bitio::ReverseTable revtable(8);

				uint64_t msb, lsb, msbbits, lsbbits;
				autoarray::AutoArray < uint64_t > msbspace ( (n+63)/64 );
				countMsbLsb(acode,offset,n,enctable,dectable,lsb,lsbbits,msb,msbbits,msbspace.get());
				
				// std::cerr << "path " << spath << " lsb " << lsb << " msb " << msb << std::endl;
				
				// std::cerr << "Sorting recursive..." << std::endl;
				::libmaus::huffman::HuffmanSorting::huffmanSortRecursive(acode, offset, n, node, enctable, dectable, revtable, dectable0, dectable1, enctable0, enctable1);
				// std::cerr << "Sorting recursive done." << std::endl;

				#if 0
				uint64_t const firsthighsym = 
				#endif
				// std::cerr << "Compressing code for transposition...";
				::libmaus::huffman::HuffmanSorting::compressHuffmanCoded(acode, offset, n, enctable, dectable, node );
				// std::cerr << "done." << std::endl;
				
				// std::cerr << "Copying msb vector back to code...";
				for ( uint64_t i = 0; i < n; ++i )
					bitio::putBit ( acode, offset + i, bitio::getBit ( msbspace.get(), i ) );
				// std::cerr << "done."<< std::endl;

				// std::cerr << "Creating tree node...";
				HuffmanWaveletTreeNavigationNode::unique_ptr_type navnode ( new HuffmanWaveletTreeNavigationNode(offset) );
				// std::cerr << "done." << std::endl;
				
				assert ( node );
				assert ( node->left );
				assert ( node->right );
				
				if ( !node->left->isLeaf() )
				{
					// std::cerr << "Left recursion..." << std::endl;
					HuffmanWaveletTreeNavigationNode::unique_ptr_type tnavnodeleft(huffmanWaveletTreeBits ( acode, offset + n , lsb , node->left ));				
	
					navnode->left = UNIQUE_PTR_MOVE(tnavnodeleft);
					// std::cerr << "Left recursion done." << std::endl;
				}
				if ( !node->right->isLeaf() )
				{
					// std::cerr << "Right recursion..." << std::endl;
					HuffmanWaveletTreeNavigationNode::unique_ptr_type tnavnoderight(huffmanWaveletTreeBits ( acode, offset + n + (lsbbits-lsb), msb , node->right ));
					navnode->right = UNIQUE_PTR_MOVE(tnavnoderight);
					// std::cerr << "Right recursion done." << std::endl;
				}
				
				return UNIQUE_PTR_MOVE(navnode);
			}

			static HuffmanWaveletTreeNavigationNode::unique_ptr_type huffmanWaveletTreeBits ( 
				uint64_t * const acode, 
				uint64_t const n, 
				huffman::HuffmanTreeNode const * rnode
			)
			{
				// std::cerr << "Running huffmanWaveletTreeBits through wrapper..." << std::endl;
				HuffmanWaveletTreeNavigationNode::unique_ptr_type retnode(huffmanWaveletTreeBits ( acode, 0, n, rnode ));
				// std::cerr << "Running huffmanWaveletTreeBits through wrapper done." << std::endl;		
				return UNIQUE_PTR_MOVE(retnode);
			}

			static uint64_t deserializeNumber(std::istream & in, uint64_t & s)
			{
				uint64_t n;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n); // n
				return n;
			}
			static ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type deserializeHuffmanTree(std::istream & in, uint64_t & s)
			{
				return huffman::HuffmanTreeNode::deserialize(in,s); // aroot
			}
			static autoarray::AutoArray < uint64_t > deserializeCodeArray(std::istream & in, uint64_t & s)
			{
				autoarray::AutoArray < uint64_t > acode;
				s += acode.deserialize(in); // acode
				return acode;
			}
			static HuffmanWaveletTreeNavigationNode::unique_ptr_type deserializeHuffmanNavigationTree(std::istream & in, uint64_t & s)
			{
				HuffmanWaveletTreeNavigationNode::unique_ptr_type ptr(HuffmanWaveletTreeNavigationNode::deserialize(in,s));
				return UNIQUE_PTR_MOVE(ptr); // anavroot
			}

			static uint64_t deserializeNumber(std::istream & in)
			{
				uint64_t n;
				::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n); // n
				return n;
			}
			static ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type deserializeHuffmanTree(std::istream & in)
			{
				uint64_t s = 0;
				return huffman::HuffmanTreeNode::deserialize(in,s); // aroot
			}
			static autoarray::AutoArray < uint64_t > deserializeCodeArray(std::istream & in)
			{
				autoarray::AutoArray < uint64_t > acode;
				acode.deserialize(in); // acode
				return acode;
			}
			static HuffmanWaveletTreeNavigationNode::unique_ptr_type deserializeHuffmanNavigationTree(std::istream & in)
			{
				uint64_t s = 0;
				HuffmanWaveletTreeNavigationNode::unique_ptr_type ptr(HuffmanWaveletTreeNavigationNode::deserialize(in,s));
				return UNIQUE_PTR_MOVE(ptr); // anavroot
			}

			HuffmanWaveletTree(std::istream & in)
			: n(deserializeNumber(in)), 
			  aroot(deserializeHuffmanTree(in)), 
			  root(aroot.get()),
			  enctable(root),
			  acode ( deserializeCodeArray(in) ), 
			  code(acode.get()),
			  anavroot(deserializeHuffmanNavigationTree(in)),
			  navroot(anavroot.get()),
			  UR ( new rank_type(acode.get(), acode.getN()*64) ),
			  R ( UR.get() )
			{
				if ( navroot )
					anavroot->setupRank(*R,n);
			}

			HuffmanWaveletTree(std::istream & in, uint64_t & s)
			: n(deserializeNumber(in,s)), 
			  aroot(deserializeHuffmanTree(in,s)), 
			  root(aroot.get()),
			  enctable(root),
			  acode ( deserializeCodeArray(in,s) ), 
			  code(acode.get()),
			  anavroot(deserializeHuffmanNavigationTree(in,s)),
			  navroot(anavroot.get()),
			  UR ( new rank_type(acode.get(), acode.getN()*64) ),
			  R ( UR.get() )
			{
				if ( navroot )
					anavroot->setupRank(*R,n);
			}
			
			template<typename iterator>
			uint64_t getCodeLength(iterator a, iterator e) const
			{
				uint64_t l = 0;
				for ( iterator i = a; i != e; ++i )
					l += enctable[*i].second;
				return l;
			}

			unsigned int bitsPerNum(uint64_t i) const
			{
				unsigned int b = 0;
				
				while ( i )
				{
					i >>= 1;
					b++;
				}
				
				return b;
			}
			
			HuffmanWaveletTreeNavigationNode::unique_ptr_type
				generateBits(
					bitio::CheckedBitWriter8 & W,
					uint64_t & Wptr,
					uint64_t const left,
					uint64_t const right,
					bitio::CompactArray & B,
					uint64_t const level,
					huffman::HuffmanTreeNode const * const node
				)
			{
				std::cerr << "Wptr=" << Wptr << " left=" << left << " right=" << right << " level=" << level << std::endl;
			
				if ( !node->isLeaf() )
				{
					HuffmanWaveletTreeNavigationNode::unique_ptr_type HN( new HuffmanWaveletTreeNavigationNode(Wptr) );
					
					/** write top bit and count */
					uint64_t cnt[2] = { 0,0 };
					for ( uint64_t i = left; i < right; ++i )
					{
						bool const bit = enctable.getBitFromTop( B.get(i) , level );
						W . writeBit ( bit );
						cnt[bit]++;
					}
					Wptr += (right-left);
					
					/** sort */
					bitio::CompactArray T0(cnt[0], B.getB());
					bitio::CompactArray T1(cnt[1], B.getB());
					uint64_t c0 = 0;
					uint64_t c1 = 0;
					
					for ( uint64_t i = left; i < right; ++i )
					{
						uint64_t const v = B.get(i);
						bool const bit = enctable.getBitFromTop( v , level );
						if ( bit )
							T1.set(c1++,v);
						else
							T0.set(c0++,v);
					}
					
					for ( uint64_t i = 0; i < cnt[0]; ++i )
						B.set ( left + i , T0.get(i) );
					for ( uint64_t i = 0; i < cnt[1]; ++i )
						B.set ( left + cnt[0] + i, T1.get(i) );
						
					huffman::HuffmanTreeInnerNode const * const inode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(node);
					
					HuffmanWaveletTreeNavigationNode::unique_ptr_type tHNleft(generateBits(W,Wptr,left,left+cnt[0],B,level+1,inode->left));	
					HN->left = UNIQUE_PTR_MOVE(tHNleft);
					HuffmanWaveletTreeNavigationNode::unique_ptr_type tHNright(generateBits(W,Wptr,left+cnt[0],right,B,level+1,inode->right));
					HN->right = UNIQUE_PTR_MOVE(tHNright);
					
					return UNIQUE_PTR_MOVE(HN);
				}
				else
				{
					HuffmanWaveletTreeNavigationNode::unique_ptr_type ptr;
					return UNIQUE_PTR_MOVE(ptr);
				}
			}

			template<typename iterator>
			autoarray::AutoArray<uint64_t> setupBitArray(iterator a, iterator e)
			{
				std::cerr << "n=" << e-a << std::endl;
				uint64_t const codelength = getCodeLength(a,e);
				std::cerr << "code length " << codelength << " bits." << std::endl;
				uint64_t const codewords = (codelength + 63)/64;
				std::cerr << "code words " << codewords << std::endl;
				autoarray::AutoArray<uint64_t> acode(codewords);
				return acode;
			}

			template<typename iterator>
			HuffmanWaveletTreeNavigationNode::unique_ptr_type generateBits(iterator a, iterator e)
			{
				uint64_t const n = e-a;
				
				std::cerr << "Determining maximal value...";
				uint64_t maxval = 0;
				for ( iterator i = a; i != e; ++i )
					maxval = std::max(maxval,static_cast<uint64_t>(*i));
				std::cerr << "done, " << maxval << std::endl;
				
				std::cerr << "Setting up compact array...";
				unsigned int const b = bitsPerNum(maxval);
				bitio::CompactArray B(n,b);
				uint64_t j = 0;
				for ( iterator i = a; i != e; ++i )
					B.set(j++,*i);
				std::cerr << "done." << std::endl;
				
				bitio::CheckedBitWriter8 W(acode.get(), acode.get() + acode.getN() );

				uint64_t Wptr = 0;
				HuffmanWaveletTreeNavigationNode::unique_ptr_type anavroot(generateBits(W,Wptr,0,n,B,0,root));
				
				W.flush();
				
				return UNIQUE_PTR_MOVE(anavroot);
			}
			
			template<typename iterator>
			HuffmanWaveletTree(iterator a, iterator e, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type raroot )
			: n(e-a), aroot(raroot), root(aroot.get()),
			  enctable(root),
			  /*
			  acode ( hufEncodeString(a,e,root) ), code(acode.get()),
			  anavroot(huffmanWaveletTreeBits(code,n,root)),
			  navroot(anavroot.get()),
			  */
			  acode ( setupBitArray(a,e) ),
			  code ( acode.get() ),
			  anavroot ( generateBits(a,e) ),
			  navroot ( anavroot.get() ),	  
			  UR ( new rank_type(acode.get(), acode.getN()*64) ),
			  R ( UR.get() )
			{
				if ( navroot )
					anavroot->setupRank(*R,n);
			}

			template<typename iterator>
			HuffmanWaveletTree(iterator a, iterator e)
			: n(e-a), 
			  aroot(huffman::HuffmanBase::createTree(a,e)), root(aroot.get()),
			  enctable(root),
			  /*
			  acode ( hufEncodeString(a,e,root) ), code(acode.get()),
			  anavroot(huffmanWaveletTreeBits(code,n,root)),
			  navroot(anavroot.get()),
			  */
			  acode ( setupBitArray(a,e) ),
			  code ( acode.get() ),
			  anavroot ( generateBits(a,e) ),
			  navroot ( anavroot.get() ),	  
			  UR ( new rank_type(acode.get(), acode.getN()*64) ),
			  R ( UR.get() )
			{
				if ( navroot )
					anavroot->setupRank(*R,n);
			}
			
			static void testTree(std::string const & s)
			{
				uint64_t const n = s.size();
				HuffmanWaveletTree H0(s.begin(),s.end());
				
				for ( uint64_t i = 0; i < n; ++i )
					assert ( H0[i] == s[i] );

				std::map< int, uint64_t > smap;

				for ( uint64_t i = 0; i < n; ++i )
				{
					smap [ s[i] ]++;
					uint64_t srank = smap[s[i]];
					
					std::pair < ::libmaus::uint::UInt < lookupwords >, unsigned int > code = H0.enctable [ s[i] ];
					
					uint64_t frank = H0.rank(s[i],i);
					
					assert ( srank == frank );
					
					assert ( H0.select(s[i],frank-1) == i );
					
					/*
					std::cerr << "i=" << i << " select=" << H0.select(s[i],frank-1) << std::endl;
					*/
				}

			}

			static void testTree()
			{
				std::cerr << "Testing Huffman wavelet tree...";
			
				for ( unsigned int j = 0; j < 20; ++j )
				{
					std::cerr << ".";

					// unsigned int const n = 1 + (rand() % (1ull << 16));
					uint64_t const n = 1024*1024;
					std::string s(n,' ');
					unsigned int const alphabet_size = 1 + (rand() % 16);
					
					for ( uint64_t i = 0; i < n; ++i )
						s[i] = 'a' + rand() % alphabet_size;

					// std::cerr << s << std::endl;

					#if 1
					double const bef = clock();
					#endif
					testTree(s);
					#if 1
					double const aft = clock();
					#endif
					
					#if 1
					std::cerr << "time for n=" << n << " is " << (aft-bef)/CLOCKS_PER_SEC 
						<< " per mb " <<  ((aft-bef)/CLOCKS_PER_SEC) / (n/(1024.0*1024.0)) << std::endl;
					#endif
				}
				std::cerr << "done." << std::endl;
			}

			uint64_t getCodeLength(uint64_t const sym) const
			{
				return enctable[sym].second;
			}

			int operator[](uint64_t i) const
			{
				huffman::HuffmanTreeNode const * hnode = root;
				HuffmanWaveletTreeNavigationNode const * node = navroot;
				
				// uint64_t v = 0;
				
				while ( !(hnode->isLeaf()) )
				{
					huffman::HuffmanTreeInnerNode const * inode = dynamic_cast < huffman::HuffmanTreeInnerNode const * > ( hnode );

					if ( bitio::getBit ( code, node->offset + i ) )
					{
						uint64_t const pre = node->offset ? R->rank1(node->offset-1):0;
						i = R->rank1(node->offset+i) - 1 - pre;
						node = node->getRight();
						hnode = inode->right;
					}
					else
					{
						uint64_t const pre = node->offset ? R->rank0(node->offset-1):0;
						i = R->rank0(node->offset+i) - 1 - pre;
						node = node->getLeft();
						hnode = inode->left;			
					}
				}
				
				return dynamic_cast < huffman::HuffmanTreeLeaf const * > ( hnode ) -> symbol ;
			}

			uint64_t rank(int const sym, uint64_t i) const
			{
				HuffmanWaveletTreeNavigationNode const * node = navroot;
				uint64_t code = static_cast<uint64_t>(enctable[sym].first);
				int shift = static_cast < int > ( enctable[sym].second ) - 1;

				while ( shift >= 0 )
				{
					if ( (code >> shift) & 1 )
					{
						uint64_t const localrank = R->rank1(node->offset + i)-node->prerank1;
						
						if ( ! localrank )
							return 0;
						else
						{
							node = node->getRight();
							shift -= 1;
							i = localrank-1;
						}
					}
					else
					{
						uint64_t const localrank = R->rank0(node->offset + i)-node->prerank0;
						
						if ( ! localrank )
							return 0;
						else
						{
							node = node->getLeft();
							shift -= 1;
							i = localrank-1;
						}
					}
				}
				
				return i+1;
			}

			uint64_t rank(int const sym, uint64_t i, unsigned int depth) const
			{
				HuffmanWaveletTreeNavigationNode const * node = navroot;
				uint64_t code = static_cast<uint64_t>(enctable[sym].first);
				int shift = static_cast < int > ( enctable[sym].second ) - 1;
				unsigned int d = 0;

				while ( (d++ < depth) && (shift >= 0) )
				{
					if ( (code >> shift) & 1 )
					{
						uint64_t const localrank = R->rank1(node->offset + i)-node->prerank1;
						
						if ( ! localrank )
							return 0;
						else
						{
							node = node->getRight();
							shift -= 1;
							i = localrank-1;
						}
					}
					else
					{
						uint64_t const localrank = R->rank0(node->offset + i)-node->prerank0;
						
						if ( ! localrank )
							return 0;
						else
						{
							node = node->getLeft();
							shift -= 1;
							i = localrank-1;
						}
					}
				}
				
				return i+1;
			}

			uint64_t smaller(
				uint64_t const sym, 
				uint64_t const left, 
				uint64_t const right
			) const
			{
				uint64_t code = static_cast<uint64_t>(enctable[sym].first);	
				int shift = static_cast < int > ( enctable[sym].second ) - 1;
				NodePortion portion = rootNode(left, right);
				uint64_t smaller = 0;
				
				while ( (portion.right-portion.left) && shift >= 0 )
				{
					if ( (code>>shift) & 1 )
					{
						if ( portion.node )
						{
							smaller += rank0(portion,portion.right-portion.left-1);
							portion = rightChild(portion);
						}
						else
						{
							smaller += portion.right-portion.left;
							portion.left = portion.right = 0;
						}
							
					}
					else
					{
						if ( portion.node )
							portion = leftChild(portion);
						else
							portion.left = portion.right = 0;
					}
					
					shift -= 1;
				}
				
				return smaller;
			}

			uint64_t rangeQuantile(uint64_t l, uint64_t r, uint64_t i) const
			{
				assert ( i < r-l );
				NodePortion portion = rootNode(l,r);
				huffman::HuffmanTreeNode const * hnode = root;
				
				while ( ! hnode->isLeaf() )
				{
					uint64_t zrank = (portion.right-portion.left) ? rank0( portion, portion.right - portion.left - 1 ) : 0;
				
					if ( i < zrank )
					{
						portion = leftChild(portion);
						hnode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hnode)->left;
					}
					else
					{
						i -= zrank;
						portion = rightChild(portion);
						hnode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hnode)->right;
					}
				}
				
				return dynamic_cast<huffman::HuffmanTreeLeaf const *>(hnode)->symbol;
			}
			std::pair<uint64_t,uint64_t> rangeQuantileIndex(uint64_t l, uint64_t r, uint64_t i) const
			{
				assert ( i < r-l );
				NodePortion portion = rootNode(l,r);
				huffman::HuffmanTreeNode const * hnode = root;
				
				while ( ! hnode->isLeaf() )
				{
					uint64_t zrank = (portion.right-portion.left) ? rank0( portion, portion.right - portion.left - 1 ) : 0;
				
					if ( i < zrank )
					{
						portion = leftChild(portion);
						hnode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hnode)->left;
					}
					else
					{
						i -= zrank;
						portion = rightChild(portion);
						hnode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hnode)->right;
					}
				}
				
				return 
					std::pair<uint64_t,uint64_t>(
						dynamic_cast<huffman::HuffmanTreeLeaf const *>(hnode)->symbol,
						i
					);
			}
			

			/**
			 * return the index of the i'th occurence of sym
			 *
			 * @param sym
			 * @param i
			 * @return index of the i'th occurence of k
			 **/
			uint64_t select(uint64_t const sym, uint64_t i) const
			{
				HuffmanWaveletTreeNavigationNode const * node = navroot;
				// huffman::HuffmanTreeNode const * hnode = root;
				
				uint64_t code = static_cast<uint64_t>(enctable[sym].first);	
				int shift = static_cast < int > ( enctable[sym].second ) - 1;
				
				typedef HuffmanWaveletTreeNavigationNode const * stack_element_type;
				#if defined(_MSC_VER) || defined(__MINGW32__)
				stack_element_type * S = reinterpret_cast<stack_element_type *>(_alloca( shift * sizeof(stack_element_type) ));
				#else
				stack_element_type * S = reinterpret_cast<stack_element_type *>(alloca( shift * sizeof(stack_element_type) ));
				#endif
				
				// while ( ! hnode->isLeaf() )
				while ( node )
				{
					*(S++) = node;

					if ( (code>>shift) & 1 )
					{
						// hnode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hnode)->right;
						node = node->getRight();
					}
					else
					{
						// hnode = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(hnode)->left;
						node = node->getLeft();
					}
					
					shift--;
				}
				
				shift = 0;
				
				while ( shift < static_cast<int>(enctable[sym].second) )
				{
					stack_element_type node = *(--S);
					
					if ( (code>>shift) & 1 )
					{
						i = select1 ( node, i );
					}
					else
					{
						i = select0 ( node, i);
					}
				
					++shift;
				}
				
				return i;
			}

		};
	}
}
#endif
