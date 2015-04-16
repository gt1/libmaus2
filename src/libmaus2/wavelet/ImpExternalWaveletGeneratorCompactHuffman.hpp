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

#if ! defined(LIBMAUS2_WAVELET_IMPEXTERNALWAVELETGENERATORCOMPACTHUFFMAN_HPP)
#define LIBMAUS2_WAVELET_IMPEXTERNALWAVELETGENERATORCOMPACTHUFFMAN_HPP

#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/bitio/BufferIterator.hpp>
#include <libmaus2/bitio/BitVectorConcat.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/rank/ERankBase.hpp>
#include <libmaus2/huffman/HuffmanTreeNode.hpp>
#include <libmaus2/huffman/HuffmanTreeInnerNode.hpp>
#include <libmaus2/huffman/HuffmanTreeLeaf.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/aio/CheckedOutputStream.hpp>
#include <libmaus2/aio/CheckedInputStream.hpp>
#include <libmaus2/util/TempFileContainer.hpp>
#include <libmaus2/huffman/HuffmanTree.hpp>

#include <libmaus2/util/unordered_map.hpp>

namespace libmaus2
{
	namespace wavelet
	{
		struct ImpExternalWaveletGeneratorCompactHuffman
		{
			typedef ImpExternalWaveletGeneratorCompactHuffman this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			// rank type
			typedef ::libmaus2::rank::ImpCacheLineRank rank_type;
			// bit output stream contexts
			typedef rank_type::WriteContextExternal context_type;
			// pointer to output context
			typedef context_type::unique_ptr_type context_ptr_type;
			// vector of contexts
			typedef ::libmaus2::autoarray::AutoArray<context_ptr_type> context_vector_type;
			
			// pair of context pointer and bit
			typedef std::pair < context_type * , bool > bit_type;
			// vector of bit type
			typedef ::libmaus2::autoarray::AutoArray < bit_type > bit_vector_type;
			// vector of bit vector type
			typedef ::libmaus2::autoarray::AutoArray < bit_vector_type > bit_vectors_type;

			private:
			// huffman tree		
			libmaus2::huffman::HuffmanTree const & H;
			// huffman encode table
			libmaus2::huffman::HuffmanTree::EncodeTable const E;
			// temp file container
			::libmaus2::util::TempFileContainer & tmpcnt;
			// bit writing contexts
			::libmaus2::autoarray::AutoArray<context_ptr_type> contexts;
			// symbol to rank (position of leaf in H)
			libmaus2::autoarray::AutoArray<uint64_t> symtorank;
			// symbol to offset in bv
			libmaus2::autoarray::AutoArray<uint64_t> symtooffset;
			// (contexts,bit) vector
			bit_vector_type bv;
			// number of symbols written
			uint64_t symbols;

			void flush()
			{
				for ( uint64_t i = 0; i < contexts.size(); ++i )
				{
					contexts[i]->writeBit(0);
					contexts[i]->flush();
					// std::cerr << "Flushed context " << i << std::endl;
				}
			}
			
			public:
			ImpExternalWaveletGeneratorCompactHuffman(
				libmaus2::huffman::HuffmanTree const & rH,
				::libmaus2::util::TempFileContainer & rtmpcnt
			)
			: H(rH), E(H), tmpcnt(rtmpcnt), contexts(H.inner()), symtorank(E.maxsym-E.minsym+1,false), symtooffset(E.maxsym-E.minsym+1,false), symbols(0)
			{
				// set up contexts
				for ( uint64_t i = 0; i < H.inner(); ++i )
				{				
					context_ptr_type tcontextsi(new context_type(tmpcnt.openOutputTempFile(i), 0, false /* no header */));
					contexts[i] = UNIQUE_PTR_MOVE(tcontextsi);
				}
				
				// compute parent map
				libmaus2::autoarray::AutoArray<uint32_t> P(H.leafs()+H.inner(),false);
				for ( uint64_t i = 0; i < H.inner(); ++i )
				{
					P[H.leftChild(H.leafs()+i)] = H.leafs()+i;
					P[H.rightChild(H.leafs()+i)] = H.leafs()+i;					
				}
				P[H.root()] = H.root();
				
				uint64_t totalbits = 0;
				for ( uint64_t i = 0; i < H.leafs(); ++i )
				{
					int64_t const sym = H.getSymbol(i);
					totalbits += E.getCodeLength(sym);
					symtorank[sym-E.minsym] = i;
				}
				
				bv = bit_vector_type(totalbits,false);
				
				uint64_t offset = 0;

				for ( uint64_t i = 0; i < H.leafs(); ++i )
				{
					// get symbol
					int64_t const sym = H.getSymbol(i);
					// store offset in bv table
					symtooffset[sym-E.minsym] = offset;
					// code length
					unsigned int const numbits = E.getCodeLength(sym);
					// code
					uint64_t const code = E.getCode(sym);
					
					uint64_t node = i;
					for ( uint64_t j = 0; j < numbits; ++j )
					{
						node = P[node];
						assert ( ! H.isLeaf(node) );
						bv[offset + numbits - j - 1] = bit_type(contexts[node-H.leafs()].get(),code&(1ull<<j));
					}
					
					offset += numbits;
				}
			}
			
			void putSymbol(int64_t const s)
			{
				unsigned int codelen = E.getCodeLength(s);				
				bit_type const * btp = bv.begin() + symtooffset[s-E.minsym];
				
				for ( unsigned int i = 0; i < codelen; ++i )
					btp[i].first->writeBit(btp[i].second);
				
				#if 0
				uint64_t const code = E.getCode(s);
				
				uint64_t node = H.root();
				uint64_t flagmask = 1ull << (codelen-1);
				
				while ( codelen-- )
				{
					bool const b = (code & flagmask);
					flagmask >>= 1;
					
					contexts[node-H.leafs()]->writeBit(b);
					
					if ( b )
						node = H.rightChild(node);
					else
						node = H.leftChild(node);
				}
				#endif

				symbols += 1;
			}

			template<typename stream_type>
			uint64_t createFinalStream(stream_type & out)
			{			
				flush();

				uint64_t p = 0;
				p += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,symbols); // n
				p += H.serialise(out); // root->serialize(out); // huffman code tree
				p += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,contexts.size()); // number of bit vectors
				
				std::vector<uint64_t> nodeposvec;

				for ( uint64_t i = 0; i < contexts.size(); ++i )
				{
					nodeposvec.push_back(p);
				
					uint64_t const blockswritten = contexts[i]->blockswritten;
					uint64_t const datawordswritten = 6*blockswritten;
					uint64_t const allwordswritten = 8*blockswritten;
						
					contexts[i].reset();
					tmpcnt.closeOutputTempFile(i);	
					
					// bits written
					p += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,64*datawordswritten);
					// auto array header (words written)
					p += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,allwordswritten);
					//std::string const filename = outputfilenames[i];
					//::libmaus2::aio::CheckedInputStream istr(filename);
					std::istream & istr = tmpcnt.openInputTempFile(i);
					// std::ifstream istr(filename.c_str(),std::ios::binary);
					// std::cerr << "Copying " << allwordswritten << " from stream " << filename << std::endl;
					::libmaus2::util::GetFileSize::copy (istr, out, allwordswritten, sizeof(uint64_t));
					p += allwordswritten * sizeof(uint64_t);
					tmpcnt.closeInputTempFile(i);

					// remove(filename.c_str());
				}
				
				uint64_t const indexpos = p;
				p += ::libmaus2::util::NumberSerialisation::serialiseNumberVector(out,nodeposvec);
				p += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,indexpos);
					
				out.flush();
				
				return p;
			}
			
			void createFinalStream(std::string const & filename)
			{
				::libmaus2::aio::CheckedOutputStream ostr(filename);
				createFinalStream(ostr);
				ostr.flush();
				ostr.close();
			}
		};
	}
}
#endif
