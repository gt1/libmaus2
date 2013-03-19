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

#if ! defined(IMPEXTERNALWAVELETGENERATORHUFFMANPARALLEL_HPP)
#define IMPEXTERNALWAVELETGENERATORHUFFMANPARALLEL_HPP

#include <libmaus/util/TempFileNameGenerator.hpp>
#include <libmaus/bitio/BufferIterator.hpp>
#include <libmaus/bitio/BitVectorConcat.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/rank/ERankBase.hpp>
#include <libmaus/huffman/HuffmanTreeNode.hpp>
#include <libmaus/huffman/HuffmanTreeInnerNode.hpp>
#include <libmaus/huffman/HuffmanTreeLeaf.hpp>
#include <libmaus/util/unordered_map.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>

namespace libmaus
{
	namespace wavelet
	{
		struct ImpExternalWaveletGeneratorHuffmanParallel : public ::libmaus::rank::ERankBase
		{
			::libmaus::huffman::HuffmanTreeNode const * root;
			::libmaus::util::TempFileNameGenerator & tmpgen;
			::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray< std::pair<uint64_t,bool> > > bv;
			typedef ::libmaus::util::unordered_map<int64_t,uint64_t>::type leafToIdType;
			leafToIdType leafToId;
			
			struct BufferTypeSet
			{
				typedef BufferTypeSet this_type;
				typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
				typedef ::libmaus::bitio::BufferIterator8 buffer_type;
				typedef buffer_type::unique_ptr_type buffer_ptr_type;
				
				::libmaus::autoarray::AutoArray<buffer_ptr_type> B;
				std::vector<std::string> outputfilenames;

				::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray< std::pair<uint64_t,bool> > > const & bv;
				leafToIdType const & leafToId;
				uint64_t symbols;
				
				BufferTypeSet(
					uint64_t const numbuffers, 
					::libmaus::util::TempFileNameGenerator & tmpgen, 
					::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray< std::pair<uint64_t,bool> > > const & rbv,
					leafToIdType const & rleafToId,
					uint64_t const bufsize = 64*1024
				)
				: B(numbuffers), bv(rbv), leafToId(rleafToId), symbols(0)
				{
					for ( uint64_t i = 0; i < numbuffers; ++i )
					{
						outputfilenames.push_back(tmpgen.getFileName());
						B [ i ] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(outputfilenames.back(),bufsize)));
					}
				}

				void putSymbol(uint64_t const s)
				{
					assert ( leafToId.find(s) != leafToId.end() );
					::libmaus::autoarray::AutoArray< std::pair<uint64_t,bool> > const & b = bv [ leafToId.find(s)->second ];
					
					for ( uint64_t i = 0; i < b.size(); ++i )
						B [ b [ i ] . first ] -> writeBit( b[i].second );
	
					symbols += 1;
				}

				void flush(bool const last)
				{
					for ( uint64_t i = 0; i < B.size(); ++i )
					{
						if ( last )
							B[i]->writeBit(0);
						B[i]->flush();
					}
				}
				
				std::vector < std::pair< std::string, uint64_t >  > getOutputVector() const
				{
					std::vector < std::pair< std::string, uint64_t >  > V;
					for ( uint64_t i = 0; i < B.size(); ++i )
						V.push_back ( 
							std::pair< std::string, uint64_t >(outputfilenames[i],B[i]->bits)
							);
					return V;
				}
				
				void addConcatElement(uint64_t const nodeid, std::vector < std::pair< std::string, uint64_t >  > & V)
				{
					V.push_back( std::pair< std::string, uint64_t >(outputfilenames[nodeid],B[nodeid]->bits) );
				}
				
				void removeFiles()
				{
					for ( uint64_t i = 0; i < outputfilenames.size(); ++i )
						remove ( outputfilenames[i].c_str() );
				}
			};
			
			::libmaus::autoarray::AutoArray < BufferTypeSet::unique_ptr_type > B;
			
			BufferTypeSet & operator[](uint64_t const thread)
			{
				return *(B [ thread ]);
			}
			
			std::vector < std::pair< std::string, uint64_t >  > getIdConcatVector(uint64_t const nodeid) const
			{
				std::vector < std::pair< std::string, uint64_t >  > V;
				for ( uint64_t i = 0; i < B.size(); ++i )
				{
					B[i]->addConcatElement(nodeid,V);
				}
				return V;
			}
			
			void removeFiles()
			{
				for ( uint64_t i = 0; i < B.size(); ++i )
					B[i]->removeFiles();
			}
			
			ImpExternalWaveletGeneratorHuffmanParallel(
				::libmaus::huffman::HuffmanTreeNode const * rroot, 
				::libmaus::util::TempFileNameGenerator & rtmpgen,
				uint64_t const threads
			)
			: root(rroot), tmpgen(rtmpgen), B(threads)
			{
				std::map < ::libmaus::huffman::HuffmanTreeNode const * , ::libmaus::huffman::HuffmanTreeInnerNode const * > parentMap;
				std::map < ::libmaus::huffman::HuffmanTreeInnerNode const *, uint64_t > nodeToId;
				std::map < int64_t, ::libmaus::huffman::HuffmanTreeLeaf const * > leafMap;
				
				std::stack < ::libmaus::huffman::HuffmanTreeNode const * > S;
				S.push(root);
				
				while ( ! S.empty() )
				{
					::libmaus::huffman::HuffmanTreeNode const * cur = S.top();
					S.pop();

					// leaf
					if ( cur->isLeaf() )
					{
						::libmaus::huffman::HuffmanTreeLeaf const * leaf = dynamic_cast< ::libmaus::huffman::HuffmanTreeLeaf const *>(cur);
						leafMap [ leaf->symbol ] = leaf;
					}
					// inner node
					else
					{
						::libmaus::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus::huffman::HuffmanTreeInnerNode const *>(cur);
						uint64_t const id = nodeToId.size();

						// assert ( id == outputfilenames.size() );
						nodeToId [ node ] = id;
						// outputfilenames.push_back(tmpgen.getFileName());
						
						parentMap [ node->left ] = node;
						parentMap [ node->right ] = node;

						// push children
						S.push ( node->right );
						S.push ( node->left );
					}
				}

				bv = ::libmaus::autoarray::AutoArray< ::libmaus::autoarray::AutoArray< std::pair<uint64_t,bool> > > (leafMap.size());

				uint64_t lid = 0;
				for ( std::map < int64_t, ::libmaus::huffman::HuffmanTreeLeaf const * >::const_iterator ita = leafMap.begin();
					ita != leafMap.end(); ++ita, ++lid )
				{
					uint64_t d = 0;
					::libmaus::huffman::HuffmanTreeLeaf const * const leaf = ita->second;
					::libmaus::huffman::HuffmanTreeNode const * cur = leaf;
					
					// compute code length
					while ( parentMap.find(cur) != parentMap.end() )
					{
						cur = parentMap.find(cur)->second;
						++d;
					}
					
					bv [ lid ] = ::libmaus::autoarray::AutoArray< std::pair<uint64_t,bool> >(d);
					leafToId[leaf->symbol] = lid;
					
					cur = leaf;
					d = 0;
					
					// store code vector and adjoint contexts
					while ( parentMap.find(cur) != parentMap.end() )
					{
						::libmaus::huffman::HuffmanTreeInnerNode const * parent = parentMap.find(cur)->second;
						uint64_t const parentid = nodeToId.find(parent)->second;

						bv [lid][d] = std::pair<uint64_t,bool>(parentid,cur == parent->right);

						cur = parent;
						++d;
					}
				}

				for ( uint64_t i = 0; i < B.size(); ++i )
					B[i] = UNIQUE_PTR_MOVE(BufferTypeSet::unique_ptr_type(new BufferTypeSet( nodeToId.size(), tmpgen, bv, leafToId )));
			}

			template<typename ostream_type>
			uint64_t createFinalStreamTemplate(ostream_type & out)
			{	
				for ( uint64_t i = 0; i < B.size(); ++i )
					B[i]->flush(i+1==B.size());
				
				uint64_t symbols = 0;
				for ( uint64_t i = 0; i < B.size(); ++i )
				{
					symbols += B[i]->symbols;
				}

				uint64_t p = 0;

				p += ::libmaus::util::NumberSerialisation::serialiseNumber(out,symbols); // n
				p += root->serialize(out);
				uint64_t const numnodes = B.size() ? B[0]->B.size() : 0;
				p += ::libmaus::util::NumberSerialisation::serialiseNumber(out,numnodes); // number of bit vectors

				std::vector<std::string> concatTempFileNames;
				::libmaus::autoarray::AutoArray<uint64_t> wordsv(numnodes);
				for ( uint64_t i = 0; i < numnodes; ++i )
					concatTempFileNames.push_back(tmpgen.getFileName());
				
				/*
				 * concatenate partial bit streams for each tree node
				 */
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(numnodes); ++i )
				{
					std::vector < std::pair< std::string, uint64_t >  > const concVec = getIdConcatVector(i);

					std::string const tmpfilename = concatTempFileNames[i];
					std::ofstream ostr(tmpfilename.c_str(),std::ios::binary);
					wordsv[i] = ::libmaus::bitio::BitVectorConcat::concatenateBitVectors(concVec,ostr,6 /* write multiple of 6 words */);
					assert ( ostr );
					ostr.flush();
					ostr.close();
				}
				
				std::vector<uint64_t> nodeposvec;	
				/*
				 * concatenate nodes in final output stream
				 */
				for ( int64_t i = 0; i < static_cast<int64_t>(numnodes); ++i )
				{
					nodeposvec.push_back(p);
				
					std::string const tmpfilename = concatTempFileNames[i];
					std::ifstream istr(tmpfilename.c_str(),std::ios::binary);
					uint64_t inwords;
					::libmaus::serialize::Serialize<uint64_t>::deserialize(istr,&inwords);
					assert ( inwords == wordsv[i] );
					assert ( inwords % 6 == 0 );
					
					uint64_t const bufblocks = 4096;
					uint64_t const bufpaywords = bufblocks*6;
					uint64_t const buftotalwords = bufblocks*8;
					uint64_t const numblocks = (inwords + bufpaywords - 1) / bufpaywords;
					
					::libmaus::autoarray::AutoArray<uint64_t> inbuf(bufpaywords);
					::libmaus::autoarray::AutoArray<uint64_t> outbuf(buftotalwords);
					uint64_t s = 0;

					p += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,inwords*64); // number of bits
					p += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,(inwords/6)*8); // number of words
					
					for ( uint64_t j = 0; j < numblocks; ++j )
					{
						uint64_t const low = std::min(j * bufpaywords,inwords);
						uint64_t const high = std::min(low + bufpaywords,inwords);
						uint64_t const size = high-low;
						assert ( size % 6 == 0 );
						uint64_t const bufs = size/6;
						
						uint64_t const readbytes = bufs * 6 * sizeof(uint64_t);
						istr.read(reinterpret_cast<char *>(inbuf.get()), readbytes);
						assert ( istr.gcount() == static_cast<int64_t>(readbytes) );
						
						uint64_t const * inptr = inbuf.begin();
						uint64_t * outptr = outbuf.begin();
						for ( uint64_t k = 0; k < bufs; ++k )
						{
							(*(outptr++)) = s;
							uint64_t * subptr = outptr++;
							*subptr = 0;
							unsigned int shift = 0;
							
							uint64_t m = 0;
							for ( uint64_t l = 0; l < 6; ++l, shift+=9 )
							{
								*subptr |= (m << shift);
								uint64_t const v = *(inptr++);
								(*(outptr++)) = v;
								m += popcount8(v);
							}
							*subptr |= (m << shift); // set complete

							s += m;
						}
						
						assert ( inptr == inbuf.begin() + 6 * bufs );
						assert ( outptr == outbuf.begin() + 8 * bufs );

						uint64_t const writebytes = bufs * 8 * sizeof(uint64_t);
						out.write(reinterpret_cast<char const *>(outbuf.get()),  writebytes );
						assert ( out );
						p += writebytes;
					}
					
					// remove ( tmpfilename.c_str() );
				}

				uint64_t const indexpos = p;
				p += ::libmaus::util::NumberSerialisation::serialiseNumberVector(out,nodeposvec);
				p += ::libmaus::util::NumberSerialisation::serialiseNumber(out,indexpos);
		
				out.flush();

				for ( uint64_t i = 0; i < numnodes; ++i )
					remove ( concatTempFileNames[i].c_str() );
				
				removeFiles();
				
				return p;
			}

			void createFinalStream(std::string const & filename)
			{
				::libmaus::aio::CheckedOutputStream ostr(filename);
				// std::ofstream ostr(filename.c_str(),std::ios::binary);
				createFinalStreamTemplate< ::libmaus::aio::CheckedOutputStream >(ostr);
				ostr.flush();
				ostr.close();
			}
		
		};
	}
}
#endif
