/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_WAVELET_RLTOHWTBASE_HPP)
#define LIBMAUS2_WAVELET_RLTOHWTBASE_HPP

#include <libmaus2/autoarray/AutoArray2d.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/huffman/IndexDecoderDataArray.hpp>
#include <libmaus2/huffman/IndexLoader.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGeneratorCompactHuffmanParallel.hpp>
#include <libmaus2/wavelet/Utf8ToImpCompactHuffmanWaveletTree.hpp>
#include <stack>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace libmaus2
{
	namespace wavelet
	{
		template<bool _utf8_input_type, typename _rl_decoder>
		struct RlToHwtBase
		{
			typedef _rl_decoder rl_decoder;
			static bool const utf8_input_type = _utf8_input_type;
			typedef RlToHwtBase<utf8_input_type,rl_decoder> this_type;

			static bool utf8Wavelet()
			{
				return _utf8_input_type;
			}
			
			static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type loadWaveletTree(std::string const & hwt)
			{
				libmaus2::aio::InputStreamInstance CIS(hwt);
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type pICHWT(new libmaus2::wavelet::ImpCompactHuffmanWaveletTree(CIS));
				return UNIQUE_PTR_MOVE(pICHWT);
			}
			
			struct RlDecoderInfoObject
			{
				rl_decoder * dec;
				uint64_t block;
				uint64_t blocksleft;
				
				uint64_t low;
				uint64_t end;
				uint64_t blocksize;
				uint64_t blockoffset;
				
				RlDecoderInfoObject() : dec(0), block(0), blocksleft(0), low(0), end(0), blocksize(0), blockoffset(0) {}
				RlDecoderInfoObject(
					rl_decoder * rdec,
					uint64_t const rblock,
					uint64_t const rblocksleft,
					uint64_t const rlow,
					uint64_t const rend,
					uint64_t const rblocksize,
					uint64_t const rblockoffset
				) : dec(rdec), block(rblock), blocksleft(rblocksleft), low(rlow), end(rend), blocksize(rblocksize), blockoffset(rblockoffset)
				{
				
				}
				
				bool hasNextBlock() const
				{
					return blocksleft > 1;
				}
				
				RlDecoderInfoObject nextBlock() const
				{
					assert ( hasNextBlock() );
					return RlDecoderInfoObject(dec,block+1,blocksleft-1,low+blocksize,end,blocksize,blockoffset);
				}
				
				uint64_t getLow() const
				{
					return low;	
				}
				
				uint64_t getHigh() const
				{
					return std::min(low+blocksize,end);
				}
				
				uint64_t getAbsoluteBlock() const
				{
					return block + blockoffset;
				}
			};
			
			template<typename _object_type>
			struct Todo
			{
				typedef _object_type object_type;
				std::stack<object_type> todo;
				libmaus2::parallel::OMPLock lock;
				
				Todo()
				{
				
				}
				
				void push(object_type const & o)
				{
					libmaus2::parallel::ScopeLock sl(lock);
					todo.push(o);
				}
				
				bool pop(object_type & o)
				{
					libmaus2::parallel::ScopeLock sl(lock);
					if ( todo.size() )
					{
						o = todo.top();
						todo.pop();
						return true;
					}
					else
					{
						return false;
					}	
				}
			};

			struct WtTodo
			{
				uint64_t low;
				uint64_t high;
				unsigned int depth;
				uint32_t node;
				
				WtTodo() : low(0), high(0), depth(0), node(0) {}
				WtTodo(
					uint64_t const rlow,
					uint64_t const rhigh,
					unsigned int const rdepth,
					uint32_t const rnode
				) : low(rlow), high(rhigh), depth(rdepth), node(rnode) {}
				
				std::ostream & output(std::ostream & out) const
				{
					return out << "WtTodo(low=" << low << ",high=" << high << ",depth=" << depth << ",node=" << node << ")";
				}
				
				std::string toString() const
				{
					std::ostringstream ostr;
					output(ostr);
					return ostr.str();
				}
			};

			/**
			 * specialised version for small alphabets
			 **/
			template<typename entity_type>
			static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type rlToHwtTermSmallAlphabet(
				std::vector<std::string> const & bwt, 
				std::string const & huftreefilename,
				uint64_t const bwtterm,
				uint64_t const p0r
				)
			{
				// std::cerr << "(" << libmaus2::util::Demangle::demangle<entity_type>() << ")";
			
				// load the huffman tree
				::libmaus2::huffman::HuffmanTree::unique_ptr_type UH = loadCompactHuffmanTree(huftreefilename);
				::libmaus2::huffman::HuffmanTree & H = *UH;
				// check depth is low
				assert ( H.maxDepth() <= 8*sizeof(entity_type) );
				// get encoding table
				::libmaus2::huffman::HuffmanTree::EncodeTable const E(H);
				// get symbol array
				libmaus2::autoarray::AutoArray<int64_t> const symbols = H.symbolArray();
				// get maximum symbol
				int64_t const maxsym = symbols.size() ? symbols[symbols.size()-1] : -1;
				// check it is in range
				assert ( 
					maxsym < 0
					||
					static_cast<uint64_t>(maxsym) <= static_cast<uint64_t>(std::numeric_limits<entity_type>::max()) 
				);
				// size of symbol table
				uint64_t const tablesize = maxsym+1;
				// number of inner nodes in Huffman tree
				uint64_t const inner = H.inner();

				::libmaus2::huffman::IndexDecoderDataArray IDD(bwt);
				::libmaus2::huffman::IndexEntryContainerVector::unique_ptr_type IECV = ::libmaus2::huffman::IndexLoader::loadAccIndex(bwt);

				// compute symbol to node mapping
				uint64_t symtonodesvecsize = 0;
				// depth <= 16, tablesize <= 64k
				libmaus2::autoarray::AutoArray<uint32_t> symtonodevecoffsets(tablesize,false);
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					uint64_t const sym = symbols[i];
					symtonodevecoffsets[sym] = symtonodesvecsize;
					assert ( symtonodesvecsize <= std::numeric_limits<uint32_t>::max() );
					symtonodesvecsize += E.getCodeLength(sym);
				}
				
				libmaus2::autoarray::AutoArray<uint32_t> symtonodes(symtonodesvecsize,false);
				uint32_t * symtonodesp = symtonodes.begin();
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					// symbol
					uint64_t const sym = symbols[i];
					// node
					uint64_t node = H.root();
					
					assert ( symtonodesp-symtonodes.begin() == symtonodevecoffsets[sym] );
					
					for ( uint64_t j = 0; j < E.getCodeLength(sym); ++j )
					{
						// add symbol to inner node
						//symtonodes [ sym ] . push_back( node - H.leafs() );
						*(symtonodesp++) = node - H.leafs();
						
						// follow tree link according to code of symbol
						if ( E.getBitFromTop(sym,j) )
							node = H.rightChild(node);
						else
							node = H.leftChild(node);
					}
				}
				assert ( symtonodesp = symtonodes.end() );
						
				// total size
				uint64_t const n = rl_decoder::getLength(bwt);
				// before
				uint64_t const n_0 = p0r;
				// terminator
				uint64_t const n_1 = 1;
				// after
				uint64_t const n_2 = n-(n_0+n_1);
				
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif
				
				assert ( numthreads );
				
				uint64_t const blocksperthread = 4;
				uint64_t const targetblocks = numthreads * blocksperthread;
				// maximum internal memory for blocks
				uint64_t const blockmemthres = 1024ull*1024ull;
				// per thread
				uint64_t const blocksizethres = (blockmemthres*sizeof(entity_type) + numthreads-1)/numthreads;
				
				// block sizes
				uint64_t const t_0 = std::min(blocksizethres,(n_0 + (targetblocks-1))/targetblocks);
				uint64_t const t_1 = std::min(blocksizethres,(n_1 + (targetblocks-1))/targetblocks);
				uint64_t const t_2 = std::min(blocksizethres,(n_2 + (targetblocks-1))/targetblocks);
				uint64_t const t_g = std::max(std::max(t_0,t_1),t_2);
				// actual number of blocks
				uint64_t const b_0 = t_0 ? ((n_0 + (t_0-1))/t_0) : 0;
				uint64_t const b_1 = t_1 ? ((n_1 + (t_1-1))/t_1) : 0;
				uint64_t const b_2 = t_2 ? ((n_2 + (t_2-1))/t_2) : 0;
				uint64_t const b_g = b_0 + b_1 + b_2;
				// blocks per thread
				uint64_t const blocks_per_thread_0 = (b_0 + numthreads-1)/numthreads;
				uint64_t const blocks_per_thread_2 = (b_2 + numthreads-1)/numthreads;
				
				// local character histograms
				libmaus2::autoarray::AutoArray<uint64_t> localhist(numthreads * tablesize,false);
				// global node sizes
				libmaus2::autoarray::AutoArray2d<uint64_t> localnodehist(inner,b_g+1,true);

				#if 0
				std::cerr << "(" << symtonodevecoffsets.byteSize() << "," << symtonodes.byteSize() << "," << localhist.byteSize() << "," 
					<< (inner * (b_g+1) * sizeof(uint64_t)) << ")";
				#endif
				
				libmaus2::parallel::OMPLock cerrlock;
				
				libmaus2::autoarray::AutoArray<typename rl_decoder::unique_ptr_type> rldecs(2*numthreads);
				Todo<RlDecoderInfoObject> todostack;
				// set up block information for symbols after terminator
				for ( uint64_t ii = 0; ii < numthreads; ++ii )
				{
					uint64_t const i = numthreads-ii-1;
					uint64_t const baseblock = i*blocks_per_thread_2;
					uint64_t const highblock = std::min(baseblock + blocks_per_thread_2, b_2);
					uint64_t const low = n_0+n_1+baseblock * t_2;
								
					if ( low < n )
					{
						typename rl_decoder::unique_ptr_type tptr(new rl_decoder(IDD,IECV.get(),low));

						rldecs[numthreads+i] = UNIQUE_PTR_MOVE(tptr);
						todostack.push(RlDecoderInfoObject(
							rldecs[numthreads+i].get(),baseblock,highblock-baseblock,
							low,n_0+n_1+n_2,t_2,b_0+b_1
							)
						);
					}
				}
				// set up block information for symbols before terminator
				for ( uint64_t ii = 0; ii < numthreads; ++ii )
				{
					uint64_t const i = numthreads-ii-1;
					uint64_t const baseblock = i*blocks_per_thread_0;
					uint64_t const highblock = std::min(baseblock + blocks_per_thread_0, b_0);
					uint64_t const low = baseblock * t_0;
								
					if ( low < n_0 )
					{
						typename rl_decoder::unique_ptr_type tptr(new rl_decoder(IDD,IECV.get(),low));

						rldecs[i] = UNIQUE_PTR_MOVE(tptr);
						todostack.push(RlDecoderInfoObject(
							rldecs[i].get(),baseblock,highblock-baseblock,
							low,n_0,t_0,0
							)
						);				
					}
				}
				
				#if defined(_OPENMP)
				#pragma omp parallel
				#endif
				{
					RlDecoderInfoObject dio;
					
					while ( todostack.pop(dio) )
					{
						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif
					
						// pointer to histogram memory for this block
						uint64_t * const hist = localhist.begin() + tid*tablesize;
						// erase histogram
						std::fill(hist,hist+tablesize,0ull);

						// lower and upper bound of block in symbols
						uint64_t const low  = dio.getLow();
						uint64_t const high = dio.getHigh();
						
						assert ( high > low );
						
						rl_decoder & dec = *(dio.dec);

						// symbols to be processed
						uint64_t todo = high-low;
						std::pair<int64_t,uint64_t> R(-1,0);
						uint64_t toadd = 0;
							
						while ( todo )
						{
							// decode a run
							R = dec.decodeRun();
							// this should not be an end of file marker
							// assert ( R.first >= 0 );
							// count to be added
							toadd = std::min(todo,R.second);
							// add it
							hist [ R.first ] += toadd;
							// reduce todo
							todo -= toadd;					
						}
						
						if ( R.first != -1 && toadd != R.second )
							dec.putBack(std::pair<int64_t,uint64_t>(R.first,R.second-toadd));
						
						if ( dio.hasNextBlock() )
							todostack.push(dio.nextBlock());

						uint64_t const blockid = dio.getAbsoluteBlock();
						// compute bits per node in this block
						for ( uint64_t sym = 0; sym < tablesize; ++sym )
							if ( E.hasSymbol(sym) )
							{
								uint32_t * symtonodesp = symtonodes.begin() + symtonodevecoffsets[sym];
							
								for ( uint64_t i = 0; i < E.getCodeLength(sym); ++i )
									localnodehist(*(symtonodesp++) /* symtonodes[sym][i] */,blockid) += hist[sym];
									// localnodehist(symtonodes[sym][i],blockid) += hist[sym];
							}
					}
				}
				
				for ( uint64_t i = 0; i < rldecs.size(); ++i )
					rldecs[i].reset();
				
				// terminator
				for ( uint64_t i = 0; i < E.getCodeLength(bwtterm); ++i )
					// localnodehist ( symtonodes[bwtterm][i], b_0 ) += 1;
					localnodehist ( symtonodes[symtonodevecoffsets[bwtterm] + i], b_0 ) += 1;

				// compute prefix sums for each node
				localnodehist.prefixSums();
				
				#if 0
				for ( uint64_t node = 0; node < inner; ++node )
					for ( uint64_t b = 0; b < b_g+1; ++b )
						std::cerr << "localnodehist(" << node << "," << b << ")=" << localnodehist(node,b) << std::endl;
				#endif

				typedef libmaus2::rank::ImpCacheLineRank rank_type;
				typedef rank_type::unique_ptr_type rank_ptr_type;
				libmaus2::autoarray::AutoArray<rank_ptr_type> R(inner);
				libmaus2::autoarray::AutoArray<uint64_t *> P(inner);
				libmaus2::autoarray::AutoArray<entity_type> U(numthreads*t_g*2,false);

				for ( uint64_t node = 0; node < inner; ++node )
				{
					uint64_t const nodebits = localnodehist(node,b_g) + 1;
					uint64_t const nodedatawords = (nodebits+63)/64;
						
					rank_ptr_type tRnode(new rank_type(nodebits));
					R[node] = UNIQUE_PTR_MOVE(tRnode);
					P[node] = R[node]->A.end() - nodedatawords;
					
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(nodedatawords); ++i )
						P[node][i] = 0;
				}
				
				libmaus2::parallel::OMPLock nodelock;

				// set up block information for symbols after terminator
				for ( uint64_t ii = 0; ii < numthreads; ++ii )
				{
					uint64_t const i = numthreads-ii-1;
					uint64_t const baseblock = i*blocks_per_thread_2;
					uint64_t const highblock = std::min(baseblock + blocks_per_thread_2, b_2);
					uint64_t const low = n_0+n_1+baseblock * t_2;
								
					if ( low < n )
					{
						typename rl_decoder::unique_ptr_type tptr(new rl_decoder(IDD,IECV.get(),low));

						rldecs[numthreads+i] = UNIQUE_PTR_MOVE(tptr);
						todostack.push(RlDecoderInfoObject(
							rldecs[numthreads+i].get(),baseblock,highblock-baseblock,
							low,n_0+n_1+n_2,t_2,b_0+b_1
							)
						);
					}
				}
				// set up block information for symbols before terminator
				for ( uint64_t ii = 0; ii < numthreads; ++ii )
				{
					uint64_t const i = numthreads-ii-1;
					uint64_t const baseblock = i*blocks_per_thread_0;
					uint64_t const highblock = std::min(baseblock + blocks_per_thread_0, b_0);
					uint64_t const low = baseblock * t_0;
								
					if ( low < n_0 )
					{
						typename rl_decoder::unique_ptr_type tptr(new rl_decoder(IDD,IECV.get(),low));

						rldecs[i] = UNIQUE_PTR_MOVE(tptr);
						todostack.push(RlDecoderInfoObject(
							rldecs[i].get(),baseblock,highblock-baseblock,
							low,n_0,t_0,0
							)
						);				
					}
				}

				#if defined(_OPENMP)
				#pragma omp parallel
				#endif
				{
					RlDecoderInfoObject dio;
					
					while ( todostack.pop(dio) )
					{
						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif

						#if 0
						nodelock.lock();
						std::cerr << "[" << tid << "] processing block " << dio.getAbsoluteBlock() << std::endl;
						nodelock.unlock();
						#endif
						
						// lower and upper bound of block in symbols
						uint64_t const low  = dio.getLow();
						uint64_t const high = dio.getHigh();
						
						assert ( high > low );
									
						entity_type * const block = U.begin() + (tid*t_g*2);
						entity_type * const tblock = block + t_g;
						
						// open decoder
						// rl_decoder dec(bwt,low);
						
						// symbols to be processed
						uint64_t todo = high-low;
						
						entity_type * tptr = block;

						rl_decoder & dec = *(dio.dec);

						// symbols to be processed
						std::pair<int64_t,uint64_t> R(-1,0);
						uint64_t toadd = 0;
						
						// load data
						while ( todo )
						{
							// decode a run
							R = dec.decodeRun();
							// this should not be an end of file marker
							// assert ( R.first >= 0 );
							// count to be added
							toadd = std::min(todo,R.second);
							// reduce todo
							todo -= toadd;
							
							for ( uint64_t i = 0; i < toadd; ++i )
								*(tptr++) = R.first;
						}

						if ( R.first != -1 && toadd != R.second )
							dec.putBack(std::pair<int64_t,uint64_t>(R.first,R.second-toadd));
						
						if ( dio.hasNextBlock() )
							todostack.push(dio.nextBlock());
						
						for ( uint64_t i = 0; i < (high-low); ++i )
						{
							uint64_t const sym = block[i];
							block[i] = E.getCode(sym) << (8*sizeof(entity_type)-E.getCodeLength(sym));
						}
						
						std::stack<WtTodo> todostack;
						todostack.push(WtTodo(0,high-low,0,H.root()));
						
						while ( todostack.size() )
						{
							WtTodo const wtcur = todostack.top();
							todostack.pop();
							
							#if 0
							std::cerr << wtcur << std::endl;
							#endif
							
							// inner node id
							uint64_t const unode = wtcur.node - H.root();
							// shift for relevant bit
							unsigned int const shift = (8*sizeof(entity_type)-1)-wtcur.depth;
							
							// bit offset
							uint64_t bitoff = localnodehist(unode,dio.getAbsoluteBlock());
							uint64_t towrite = wtcur.high-wtcur.low;
							entity_type * ptr = block+wtcur.low;
							
							entity_type * optrs[2] = { tblock, tblock+t_g-1 };
							static int64_t const inc[2] = { 1, -1 };
							uint64_t bcnt[2]; bcnt[0] = 0; bcnt[1] = 0;
						
							// align to word boundary	
							nodelock.lock();
							while ( towrite && ((bitoff%64)!=0) )
							{
								entity_type const sym = *(ptr++);
								uint64_t const bit = (sym >> shift) & 1;
								// assert ( bit < 2 );
								bcnt[bit]++;
								(*optrs[bit]) = sym;
								optrs[bit] += inc[bit];
							
								libmaus2::bitio::putBit(P[unode],bitoff,bit);
							
								towrite--;
								bitoff++;
							}
							nodelock.unlock();
							
							// write full words
							uint64_t * PP = P[unode] + (bitoff/64);
							while ( towrite >= 64 )
							{
								uint64_t v = 0;
								for ( unsigned int i = 0; i < 64; ++i )
								{
									entity_type const sym = *(ptr++);
									uint64_t const bit = (sym >> shift) & 1;
									// assert ( bit < 2 );
									bcnt[bit]++;
									(*optrs[bit]) = sym;
									optrs[bit] += inc[bit];

									v <<= 1;
									v |= bit;
								}
							
								*(PP++) = v;
								towrite -= 64;
								bitoff += 64;
							}
							
							// write rest
							// assert ( towrite < 64 );
							nodelock.lock();
							while ( towrite )
							{
								entity_type const sym = *(ptr++);
								uint64_t const bit = (sym >> shift) & 1;
								// assert ( bit < 2 );
								bcnt[bit]++;
								(*optrs[bit]) = sym;
								optrs[bit] += inc[bit];
			
								libmaus2::bitio::putBit(P[unode],bitoff,bit);
							
								towrite--;
								bitoff++;	
							}
							nodelock.unlock();					

							/* write data back */	
							ptr = block + wtcur.low;
							entity_type * inptr = tblock;
							for ( uint64_t i = 0; i < bcnt[0]; ++i )
								*(ptr++) = *(inptr++);

							inptr = tblock + t_g;
							for ( uint64_t i = 0; i < bcnt[1]; ++i )
								*(ptr++) = *(--inptr);
								
							// recursion
							if ( ( ! H.isLeaf( H.rightChild(wtcur.node) ) ) && (bcnt[1] != 0) )
								todostack.push(WtTodo(wtcur.high-bcnt[1],wtcur.high,wtcur.depth+1,H.rightChild(wtcur.node)));
							if ( (! H.isLeaf( H.leftChild(wtcur.node) )) && (bcnt[0] != 0) )
								todostack.push(WtTodo(wtcur.low,wtcur.low+bcnt[0],wtcur.depth+1,H.leftChild(wtcur.node)));
						}
					}
				}

				// reset decoders
				for ( uint64_t i = 0; i < numthreads; ++i )
					rldecs[i].reset();

				// terminator
				{
					uint64_t node = H.root();

					for ( uint64_t i = 0; i < E.getCodeLength(bwtterm); ++i )
					{
						uint64_t const bit = E.getBitFromTop(bwtterm,i);
						uint64_t const unode = node - H.root();				
						libmaus2::bitio::putBit ( P[unode] , localnodehist ( unode , b_0 ), bit );
						
						if ( bit )
							node = H.rightChild(node);
						else
							node = H.leftChild(node);
					}
				}

				// set up rank dictionaries
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( uint64_t node = 0; node < inner; ++node )
				{
					uint64_t const nodebits = localnodehist(node,b_g) + 1;
					uint64_t const nodedatawords = (nodebits+63)/64;
					
					uint64_t * inptr = P[node];
					uint64_t * outptr = R[node]->A.begin();	
					
					uint64_t accb = 0;
					
					uint64_t todo = nodedatawords;
					
					while ( todo )
					{
						uint64_t const toproc = std::min(todo,static_cast<uint64_t>(6));
						
						uint64_t miniword = 0;
						uint64_t miniacc = 0;
						for ( uint64_t i = 0; i < toproc; ++i )
						{
							miniword |= miniacc << (i*9);
							miniacc  += libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(inptr[i]);
						}
						miniword |= (miniacc << (toproc*9));
						
						for ( uint64_t i = 0; i < toproc; ++i )
							outptr[2+i] = inptr[i];
						outptr[0] = accb;
						outptr[1] = miniword;
						
						accb += miniacc;
						inptr += toproc;
						outptr += 2+toproc;
						todo -= toproc;
					}
				}
				
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type pICHWT(
					new libmaus2::wavelet::ImpCompactHuffmanWaveletTree(n,H,R)
				);
				
				#if 0
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree const & ICHWT = *pICHWT;
				// open decoder
				rl_decoder dec(bwt,0);
				libmaus2::autoarray::AutoArray<uint64_t> ranktable(tablesize);
				for ( uint64_t i = 0; i < ICHWT.size(); ++i )
				{
					int64_t const sym = dec.decode();
					#if 0
					std::cerr << ICHWT[i] << "(" << sym << ")" << ";";
					#endif
								
					if ( i == p0r )
						assert ( ICHWT[i] == bwtterm );
					else
					{
						assert ( sym == ICHWT[i] );
						assert ( ranktable[sym] == ICHWT.rankm(sym,i) );
						assert ( ICHWT.select(sym,ranktable[sym]) == i );
						ranktable[sym]++;
						assert ( ranktable[sym] == ICHWT.rank(sym,i) );				
					}
				}
				#if 0
				std::cerr << std::endl;
				#endif
				#endif
				
				return UNIQUE_PTR_MOVE(pICHWT);
			}

			/**
			 * specialised version for small alphabets
			 **/
			static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type rlToHwtSmallAlphabet(
				std::string const & bwt, 
				std::string const & huftreefilename
			)
			{
				// load the huffman tree
				::libmaus2::huffman::HuffmanTree::unique_ptr_type UH = loadCompactHuffmanTree(huftreefilename);
				::libmaus2::huffman::HuffmanTree & H = *UH;
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ptr(rlToHwtSmallAlphabet(bwt,H));		
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			/**
			 * specialised version for small alphabets
			 **/
			template<typename entity_type>
			static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type rlToHwtSmallAlphabet(
				std::string const & bwt, 
				::libmaus2::huffman::HuffmanTree const & H
			)
			{
				// check depth is low
				assert ( H.maxDepth() <= 8*sizeof(entity_type) );
				// get encoding table
				::libmaus2::huffman::HuffmanTree::EncodeTable const E(H);
				// get symbol array
				libmaus2::autoarray::AutoArray<int64_t> const symbols = H.symbolArray();
				// get maximum symbol
				int64_t const maxsym = symbols.size() ? symbols[symbols.size()-1] : -1;
				// check it is in range
				assert (
					(maxsym < 0)
					|| 
					static_cast<uint64_t>(maxsym) <= static_cast<uint64_t>(std::numeric_limits<entity_type>::max()) 
				);
				// size of symbol table
				uint64_t const tablesize = maxsym+1;
				// number of inner nodes in Huffman tree
				uint64_t const inner = H.inner();

				// compute symbol to node mapping
				uint64_t symtonodesvecsize = 0;
				// depth <= 16, tablesize <= 64k
				libmaus2::autoarray::AutoArray<uint32_t> symtonodevecoffsets(tablesize,false);
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					uint64_t const sym = symbols[i];
					symtonodevecoffsets[sym] = symtonodesvecsize;
					assert ( symtonodesvecsize <= std::numeric_limits<uint32_t>::max() );
					symtonodesvecsize += E.getCodeLength(sym);
				}
				
				libmaus2::autoarray::AutoArray<uint32_t> symtonodes(symtonodesvecsize,false);
				uint32_t * symtonodesp = symtonodes.begin();
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					// symbol
					uint64_t const sym = symbols[i];
					// node
					uint64_t node = H.root();
					
					assert ( symtonodesp-symtonodes.begin() == symtonodevecoffsets[sym] );
					
					for ( uint64_t j = 0; j < E.getCodeLength(sym); ++j )
					{
						// add symbol to inner node
						//symtonodes [ sym ] . push_back( node - H.leafs() );
						*(symtonodesp++) = node - H.leafs();
						
						// follow tree link according to code of symbol
						if ( E.getBitFromTop(sym,j) )
							node = H.rightChild(node);
						else
							node = H.leftChild(node);
					}
				}
				assert ( symtonodesp = symtonodes.end() );



				#if 0		
				// compute symbol to node mapping
				std::vector < std::vector < uint32_t > > symtonodes(tablesize);
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					// symbol
					uint64_t const sym = symbols[i];
					// node
					uint64_t node = H.root();
					
					for ( uint64_t j = 0; j < E.getCodeLength(sym); ++j )
					{
						// add symbol to inner node
						symtonodes [ sym ] . push_back( node - H.leafs() );
						
						// follow tree link according to code of symbol
						if ( E.getBitFromTop(sym,j) )
							node = H.rightChild(node);
						else
							node = H.leftChild(node);
					}
				}
				#endif
				
				// total size
				uint64_t const n = rl_decoder::getLength(bwt);

				::libmaus2::huffman::IndexDecoderDataArray IDD(std::vector<std::string>(1,bwt));
				::libmaus2::huffman::IndexEntryContainerVector::unique_ptr_type IECV = 
					::libmaus2::huffman::IndexLoader::loadAccIndex(std::vector<std::string>(1,bwt));
				
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif
				
				assert ( numthreads );
				
				uint64_t const blocksperthread = 4;
				uint64_t const targetblocks = numthreads * blocksperthread;
				// maximum internal memory for blocks
				uint64_t const blockmemthres = 1024ull*1024ull;
				// per thread
				uint64_t const blocksizethres = (blockmemthres*sizeof(entity_type) + numthreads-1)/numthreads;
				
				// block size
				uint64_t const t_g = std::min(blocksizethres,(n + (targetblocks-1))/targetblocks);
				// actual number of blocks
				uint64_t const b_g = t_g ? ((n + (t_g-1))/t_g) : 0;
				// blocks per thread
				uint64_t const blocks_per_thread_g = (b_g + numthreads-1)/numthreads;

				// local character histograms
				libmaus2::autoarray::AutoArray<uint64_t> localhist(numthreads * tablesize,false);
				// global node sizes
				libmaus2::autoarray::AutoArray2d<uint64_t> localnodehist(inner,b_g+1,true);

				#if 0
				std::cerr << "(" << symtonodevecoffsets.byteSize() << "," << symtonodes.byteSize() << "," << localhist.byteSize() << "," 
					<< (inner * (b_g+1) * sizeof(uint64_t)) << ")";
				#endif
				
				libmaus2::parallel::OMPLock cerrlock;

				libmaus2::autoarray::AutoArray<typename rl_decoder::unique_ptr_type> rldecs(numthreads);
				Todo<RlDecoderInfoObject> todostack;
				// set up block information
				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					uint64_t const baseblock = i*blocks_per_thread_g;
					uint64_t const highblock = std::min(baseblock + blocks_per_thread_g, b_g);
					uint64_t const low = baseblock * t_g;
								
					if ( low < n )
					{
						typename rl_decoder::unique_ptr_type tptr(new rl_decoder(IDD,IECV.get(),low));

						rldecs[i] = UNIQUE_PTR_MOVE(tptr);
						todostack.push(
							RlDecoderInfoObject(
								rldecs[i].get(),baseblock,highblock-baseblock,
								low,n,t_g,0
							)
						);
					}
				}
			
				#if defined(_OPENMP)
				#pragma omp parallel
				#endif
				{
					RlDecoderInfoObject dio;
					
					while ( todostack.pop(dio) )
					{
						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif
					
						// pointer to histogram memory for this block
						uint64_t * const hist = localhist.begin() + tid*tablesize;
						// erase histogram
						std::fill(hist,hist+tablesize,0ull);

						// lower and upper bound of block in symbols
						uint64_t const low  = dio.getLow();
						uint64_t const high = dio.getHigh();
						
						assert ( high > low );

						rl_decoder & dec = *(dio.dec);

						// symbols to be processed
						uint64_t todo = high-low;
						std::pair<int64_t,uint64_t> R(-1,0);
						uint64_t toadd = 0;
						
						while ( todo )
						{
							// decode a run
							R = dec.decodeRun();
							// this should not be an end of file marker
							// assert ( R.first >= 0 );
							// count to be added
							toadd = std::min(todo,R.second);
							// add it
							hist [ R.first ] += toadd;
							// reduce todo
							todo -= toadd;					
						}

						if ( R.first != -1 && toadd != R.second )
							dec.putBack(std::pair<int64_t,uint64_t>(R.first,R.second-toadd));
						
						if ( dio.hasNextBlock() )
							todostack.push(dio.nextBlock());

						// compute bits per node in this block
						uint64_t const blockid = dio.getAbsoluteBlock();
						for ( uint64_t sym = 0; sym < tablesize; ++sym )
							if ( E.hasSymbol(sym) )
							{
								uint32_t * symtonodesp = symtonodes.begin() + symtonodevecoffsets[sym];
								
								for ( uint64_t i = 0; i < E.getCodeLength(sym) /* symtonodes[sym].size() */; ++i )
									localnodehist(*(symtonodesp++) /* symtonodes[sym][i] */,blockid) += hist[sym];
									// localnodehist(symtonodes[sym][i],blockid) += hist[sym];
							}
					}
				}

				for ( uint64_t i = 0; i < numthreads; ++i )
					rldecs[i].reset();
				
				// compute prefix sums for each node
				localnodehist.prefixSums();
				
				#if 0
				for ( uint64_t node = 0; node < inner; ++node )
					for ( uint64_t b = 0; b < b_g+1; ++b )
						std::cerr << "localnodehist(" << node << "," << b << ")=" << localnodehist(node,b) << std::endl;
				#endif

				typedef libmaus2::rank::ImpCacheLineRank rank_type;
				typedef rank_type::unique_ptr_type rank_ptr_type;
				libmaus2::autoarray::AutoArray<rank_ptr_type> R(inner);
				libmaus2::autoarray::AutoArray<uint64_t *> P(inner);
				libmaus2::autoarray::AutoArray<entity_type> U(numthreads*t_g*2,false);

				for ( uint64_t node = 0; node < inner; ++node )
				{
					uint64_t const nodebits = localnodehist(node,b_g) + 1;
					uint64_t const nodedatawords = (nodebits+63)/64;

					#if 0
					std::cerr << "node " << node << " bits " << localnodehist(node,b_g) << " words " <<
						(localnodehist(node,b_g)+63)/64 << std::endl;
					#endif
						
					rank_ptr_type tRnode(new rank_type(nodebits));
					R[node] = UNIQUE_PTR_MOVE(tRnode);
					P[node] = R[node]->A.end() - nodedatawords;
					
					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(nodedatawords); ++i )
						P[node][i] = 0;
				}
				
				libmaus2::parallel::OMPLock nodelock;

				// set up block information
				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					uint64_t const baseblock = i*blocks_per_thread_g;
					uint64_t const highblock = std::min(baseblock + blocks_per_thread_g, b_g);
					uint64_t const low = baseblock * t_g;
								
					if ( low < n )
					{
						typename rl_decoder::unique_ptr_type tptr(new rl_decoder(IDD,IECV.get(),low));

						rldecs[i] = UNIQUE_PTR_MOVE(tptr);
						todostack.push(
							RlDecoderInfoObject(
								rldecs[i].get(),baseblock,highblock-baseblock,
								low,n,t_g,0
							)
						);
					}
				}

				#if defined(_OPENMP)
				#pragma omp parallel
				#endif
				{
					RlDecoderInfoObject dio;
					
					while ( todostack.pop(dio) )
					{
						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif

						#if 0
						nodelock.lock();
						std::cerr << "[" << tid << "] processing block " << b << std::endl;
						nodelock.unlock();
						#endif
						
						// lower and upper bound of block in symbols
						uint64_t const low  = dio.getLow();
						uint64_t const high = dio.getHigh();
						
						assert ( high > low );

						entity_type * const block = U.begin() + (tid*t_g*2);
						entity_type * const tblock = block + t_g;
						
						entity_type * tptr = block;

						rl_decoder & dec = *(dio.dec);

						// symbols to be processed
						uint64_t todo = high-low;
						std::pair<int64_t,uint64_t> R(-1,0);
						uint64_t toadd = 0;

						// load data
						while ( todo )
						{
							// decode a run
							R = dec.decodeRun();
							// this should not be an end of file marker
							// assert ( R.first >= 0 );
							// count to be added
							toadd = std::min(todo,R.second);
							// reduce todo
							todo -= toadd;
							
							for ( uint64_t i = 0; i < toadd; ++i )
								*(tptr++) = R.first;
						}

						if ( R.first != -1 && toadd != R.second )
							dec.putBack(std::pair<int64_t,uint64_t>(R.first,R.second-toadd));
						
						if ( dio.hasNextBlock() )
							todostack.push(dio.nextBlock());
						
						for ( uint64_t i = 0; i < (high-low); ++i )
						{
							uint64_t const sym = block[i];
							block[i] = E.getCode(sym) << (8*sizeof(entity_type)-E.getCodeLength(sym));
						}
						
						std::stack<WtTodo> todostack;
						todostack.push(WtTodo(0,high-low,0,H.root()));
						
						while ( todostack.size() )
						{
							WtTodo const wtcur = todostack.top();
							todostack.pop();
							
							#if 0
							std::cerr << wtcur << std::endl;
							#endif
							
							// inner node id
							uint64_t const unode = wtcur.node - H.root();
							// shift for relevant bit
							unsigned int const shift = (8*sizeof(entity_type)-1)-wtcur.depth;
							
							// bit offset
							uint64_t bitoff = localnodehist(unode,dio.getAbsoluteBlock());
							uint64_t towrite = wtcur.high-wtcur.low;
							entity_type * ptr = block+wtcur.low;
							
							entity_type * optrs[2] = { tblock, tblock+t_g-1 };
							static int64_t const inc[2] = { 1, -1 };
							uint64_t bcnt[2]; bcnt[0] = 0; bcnt[1] = 0;
						
							// align to word boundary	
							nodelock.lock();
							while ( towrite && ((bitoff%64)!=0) )
							{
								entity_type const sym = *(ptr++);
								uint64_t const bit = (sym >> shift) & 1;
								// assert ( bit < 2 );
								bcnt[bit]++;
								(*optrs[bit]) = sym;
								optrs[bit] += inc[bit];
							
								libmaus2::bitio::putBit(P[unode],bitoff,bit);
							
								towrite--;
								bitoff++;
							}
							nodelock.unlock();
							
							// write full words
							uint64_t * PP = P[unode] + (bitoff/64);
							while ( towrite >= 64 )
							{
								uint64_t v = 0;
								for ( unsigned int i = 0; i < 64; ++i )
								{
									entity_type const sym = *(ptr++);
									uint64_t const bit = (sym >> shift) & 1;
									// assert ( bit < 2 );
									bcnt[bit]++;
									(*optrs[bit]) = sym;
									optrs[bit] += inc[bit];

									v <<= 1;
									v |= bit;
								}
							
								*(PP++) = v;
								towrite -= 64;
								bitoff += 64;
							}
							
							// write rest
							// assert ( towrite < 64 );
							nodelock.lock();
							while ( towrite )
							{
								entity_type const sym = *(ptr++);
								uint64_t const bit = (sym >> shift) & 1;
								// assert ( bit < 2 );
								bcnt[bit]++;
								(*optrs[bit]) = sym;
								optrs[bit] += inc[bit];

								libmaus2::bitio::putBit(P[unode],bitoff,bit);
							
								towrite--;
								bitoff++;	
							}
							nodelock.unlock();					

							/* write data back */	
							ptr = block + wtcur.low;
							entity_type * inptr = tblock;
							for ( uint64_t i = 0; i < bcnt[0]; ++i )
								*(ptr++) = *(inptr++);

							inptr = tblock + t_g;
							for ( uint64_t i = 0; i < bcnt[1]; ++i )
								*(ptr++) = *(--inptr);
								
							// recursion
							if ( ( ! H.isLeaf( H.rightChild(wtcur.node) ) ) && (bcnt[1] != 0) )
								todostack.push(WtTodo(wtcur.high-bcnt[1],wtcur.high,wtcur.depth+1,H.rightChild(wtcur.node)));
							if ( (! H.isLeaf( H.leftChild(wtcur.node) )) && (bcnt[0] != 0) )
								todostack.push(WtTodo(wtcur.low,wtcur.low+bcnt[0],wtcur.depth+1,H.leftChild(wtcur.node)));
						}
					}
				}

				for ( uint64_t i = 0; i < numthreads; ++i )
					rldecs[i].reset();

				// set up rank dictionaries
				for ( uint64_t node = 0; node < inner; ++node )
				{
					uint64_t const nodebits = localnodehist(node,b_g) + 1;
					uint64_t const nodedatawords = (nodebits+63)/64;
					
					uint64_t * inptr = P[node];
					uint64_t * outptr = R[node]->A.begin();	
					
					uint64_t accb = 0;
					
					uint64_t todo = nodedatawords;
					
					while ( todo )
					{
						uint64_t const toproc = std::min(todo,static_cast<uint64_t>(6));
						
						uint64_t miniword = 0;
						uint64_t miniacc = 0;
						for ( uint64_t i = 0; i < toproc; ++i )
						{
							miniword |= miniacc << (i*9);
							miniacc  += libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(inptr[i]);
						}
						miniword |= (miniacc << (toproc*9));
						
						for ( uint64_t i = 0; i < toproc; ++i )
							outptr[2+i] = inptr[i];
						outptr[0] = accb;
						outptr[1] = miniword;
						
						accb += miniacc;
						inptr += toproc;
						outptr += 2+toproc;
						todo -= toproc;
					}
				}
				
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type pICHWT(
					new libmaus2::wavelet::ImpCompactHuffmanWaveletTree(n,H,R)
				);
				
				#if 0
				libmaus2::wavelet::ImpCompactHuffmanWaveletTree const & ICHWT = *pICHWT;
				// open decoder
				rl_decoder dec(std::vector<std::string>(1,bwt),0);
				libmaus2::autoarray::AutoArray<uint64_t> ranktable(tablesize);
				for ( uint64_t i = 0; i < ICHWT.size(); ++i )
				{
					int64_t const sym = dec.decode();
					std::cerr << ICHWT[i] << "(" << sym << ")" << ";";
								
					assert ( sym == ICHWT[i] );
					assert ( ranktable[sym] == ICHWT.rankm(sym,i) );
					assert ( ICHWT.select(sym,ranktable[sym]) == i );
					ranktable[sym]++;
					assert ( ranktable[sym] == ICHWT.rank(sym,i) );				
				}

				std::cerr << std::endl;

				#endif
				
				return UNIQUE_PTR_MOVE(pICHWT);
			}

			static ::libmaus2::util::Histogram::unique_ptr_type computeRlSymHist(std::string const & bwt)
			{
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif

				uint64_t const n = rl_decoder::getLength(bwt);
				
				uint64_t const numpacks = 4*numthreads;
				uint64_t const packsize = (n + numpacks - 1)/numpacks;
				::libmaus2::parallel::OMPLock histlock;
				::libmaus2::util::Histogram::unique_ptr_type mhist(new ::libmaus2::util::Histogram);

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( int64_t t = 0; t < static_cast<int64_t>(numpacks); ++t )
				{
					uint64_t const low = std::min(t*packsize,n);
					uint64_t const high = std::min(low+packsize,n);
					::libmaus2::util::Histogram lhist;
					
					if ( high-low )
					{
						rl_decoder dec(std::vector<std::string>(1,bwt),low);
						
						uint64_t todec = high-low;
						std::pair<int64_t,uint64_t> P;
						
						while ( todec )
						{
							P = dec.decodeRun();
							assert ( P.first >= 0 );
							P.second = std::min(P.second,todec);
							lhist.add(P.first,P.second);
							
							todec -= P.second;
						}
					}
					
					histlock.lock();
					mhist->merge(lhist);
					histlock.unlock();
				}

				return UNIQUE_PTR_MOVE(mhist);
			}
			
			static unsigned int utf8WaveletMaxThreads()
			{
				return 24;
			}
			
			static uint64_t utf8WaveletMaxPartMem()
			{
				return 64ull*1024ull*1024ull;
			}

			static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type rlToHwt(
				std::string const & bwt, 
				std::string const & hwt, 
				std::string const tmpprefix
			)
			{
				::libmaus2::util::Histogram::unique_ptr_type mhist(computeRlSymHist(bwt));
				::std::map<int64_t,uint64_t> const chist = mhist->getByType<int64_t>();

				::libmaus2::huffman::HuffmanTree H ( chist.begin(), chist.size(), false, true, true );
				
				if ( utf8Wavelet() )
				{
					::libmaus2::wavelet::Utf8ToImpCompactHuffmanWaveletTree::constructWaveletTreeFromRl<rl_decoder,true /* radix sort */>(
						bwt,hwt,tmpprefix,H,
						utf8WaveletMaxPartMem() /* part size maximum */,
						utf8WaveletMaxThreads());

					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type IHWT(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(hwt));
					return UNIQUE_PTR_MOVE(IHWT);

					#if 0
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type IHWT(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(hwt));
					libmaus2::lf::ImpCompactHuffmanWaveletLF IHWL(hwt);
					rl_decoder rldec(std::vector<std::string>(1,bwt));
					std::cerr << "Checking output bwt of length " << IHWT->size() << "...";
					for ( uint64_t i = 0; i < IHWT->size(); ++i )
					{
						uint64_t const sym = rldec.get();
						assert ( sym == (*IHWT)[i] );
						assert ( sym == IHWL[i] );
					}
					std::cerr << "done." << std::endl;
					
					std::cerr << "Running inverse select loop...";
					uint64_t r = 0;
					for ( uint64_t i = 0; i < IHWT->size(); ++i )
					{
						IHWT->inverseSelect(i);
						// r = IHWL(r);
					}
					std::cerr << "done." << std::endl;
					#endif
				}
				else
				{
					// special case for very small alphabets
					if ( H.maxDepth() <= 8*sizeof(uint8_t) && H.maxSymbol() <= std::numeric_limits<uint8_t>::max() )
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ptr(rlToHwtSmallAlphabet<uint8_t>(bwt,H));
						libmaus2::aio::OutputStreamInstance COS(hwt);
						ptr->serialise(COS);
						COS.flush();
						// COS.close();
						return UNIQUE_PTR_MOVE(ptr);
					}
					else if ( H.maxDepth() <= 8*sizeof(uint16_t) && H.maxSymbol() <= std::numeric_limits<uint16_t>::max() )
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ptr(rlToHwtSmallAlphabet<uint16_t>(bwt,H));
						libmaus2::aio::OutputStreamInstance COS(hwt);
						ptr->serialise(COS);
						COS.flush();
						//COS.close();
						return UNIQUE_PTR_MOVE(ptr);
					}
					else if ( H.maxDepth() <= 8*sizeof(uint32_t) && H.maxSymbol() <= std::numeric_limits<uint16_t>::max() )
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ptr(rlToHwtSmallAlphabet<uint32_t>(bwt,H));
						libmaus2::aio::OutputStreamInstance COS(hwt);
						ptr->serialise(COS);
						COS.flush();
						//COS.close();
						return UNIQUE_PTR_MOVE(ptr);
					}
					else if ( H.maxDepth() <= 8*sizeof(uint64_t) && H.maxSymbol() <= std::numeric_limits<uint16_t>::max() )
					{
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ptr(rlToHwtSmallAlphabet<uint64_t>(bwt,H));
						libmaus2::aio::OutputStreamInstance COS(hwt);
						ptr->serialise(COS);
						COS.flush();
						//COS.close();
						return UNIQUE_PTR_MOVE(ptr);
					}
					else
					{
						::libmaus2::util::TempFileNameGenerator tmpgen(tmpprefix,3);

						#if defined(_OPENMP)
						uint64_t const numthreads = omp_get_max_threads();
						#else
						uint64_t const numthreads = 1;
						#endif

						uint64_t const n = rl_decoder::getLength(bwt);
						uint64_t const packsize = (n + numthreads - 1)/numthreads;
						uint64_t const numpacks = (n + packsize-1)/packsize;

						::libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel IEWGH(H,tmpgen,numthreads);

						#if defined(_OPENMP)
						#pragma omp parallel for
						#endif
						for ( int64_t t = 0; t < static_cast<int64_t>(numpacks); ++t )
						{
							assert ( t < static_cast<int64_t>(numthreads) );
							uint64_t const low = std::min(t*packsize,n);
							uint64_t const high = std::min(low+packsize,n);
							
							::libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel::BufferType & BTS = IEWGH[t];

							if ( high-low )
							{
								rl_decoder dec(std::vector<std::string>(1,bwt),low);

								uint64_t todec = high-low;
								std::pair<int64_t,uint64_t> P;
								
								while ( todec )
								{
									P = dec.decodeRun();
									assert ( P.first >= 0 );
									P.second = std::min(P.second,todec);

									for ( uint64_t i = 0; i < P.second; ++i )
										BTS.putSymbol(P.first);
									
									todec -= P.second;
								}
							}
						}
						
						IEWGH.createFinalStream(hwt);

						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type IHWT(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(hwt));
						return UNIQUE_PTR_MOVE(IHWT);
					}
				}
			}
			
			static void rlToHwtTerm(
				std::vector<std::string> const & bwt, 
				std::string const & hwt, 
				std::string const tmpprefix,
				::libmaus2::huffman::HuffmanTree & H,
				uint64_t const bwtterm,
				uint64_t const p0r
				)
			{
				if ( utf8Wavelet() )
				{
					::libmaus2::wavelet::Utf8ToImpCompactHuffmanWaveletTree::constructWaveletTreeFromRlWithTerm<rl_decoder,true /* radix sort */>(
						bwt,hwt,tmpprefix,H,p0r,bwtterm,
						utf8WaveletMaxPartMem() /* maximum part size */,
						utf8WaveletMaxThreads()
					);
					
					#if 0
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type IHWT(libmaus2::wavelet::ImpCompactHuffmanWaveletTree::load(hwt));
					rl_decoder rldec(bwt);

					std::cerr << "Checking output bwt of length " << IHWT->size() << "...";
					for ( uint64_t i = 0; i < IHWT->size(); ++i )
					{
						uint64_t const sym = rldec.get();
						assert ( sym == (*IHWT)[i] );
					}
					std::cerr << "done." << std::endl;
					#endif
				}
				else
				{			
					::libmaus2::util::TempFileNameGenerator tmpgen(tmpprefix,3);

					#if defined(_OPENMP)
					uint64_t const numthreads = omp_get_max_threads();
					#else
					uint64_t const numthreads = 1;
					#endif

					uint64_t const n = rl_decoder::getLength(bwt);

					assert ( p0r < n );
					uint64_t const nlow = p0r;
					uint64_t const nhigh = n - (nlow + 1);
					
					uint64_t const packsizelow  = (nlow + numthreads - 1)/numthreads;
					uint64_t const numpackslow  = packsizelow ? ( (nlow + packsizelow-1)/packsizelow ) : 0;
					uint64_t const packsizehigh = (nhigh + numthreads - 1)/numthreads;
					uint64_t const numpackshigh = packsizehigh ? ( (nhigh + packsizehigh-1)/packsizehigh ) : 0;

					::libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel IEWGH(H,tmpgen,2*numthreads+1);

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(numpackslow); ++t )
					{
						assert ( t < static_cast<int64_t>(numthreads) );
						uint64_t const low  = std::min(t*packsizelow,nlow);
						uint64_t const high = std::min(low+packsizelow,nlow);
						
						::libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel::BufferType & BTS = IEWGH[t];

						if ( high-low )
						{
							rl_decoder dec(bwt,low);

							uint64_t todec = high-low;
							std::pair<int64_t,uint64_t> P;
							
							while ( todec )
							{
								P = dec.decodeRun();
								assert ( P.first >= 0 );
								P.second = std::min(P.second,todec);

								for ( uint64_t i = 0; i < P.second; ++i )
									BTS.putSymbol(P.first);
								
								todec -= P.second;
							}
						}
					}
					
					IEWGH[numthreads].putSymbol(bwtterm);

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(numpackshigh); ++t )
					{
						assert ( t < static_cast<int64_t>(numthreads) );
						uint64_t const low  = std::min(nlow + 1 + t*packsizehigh,n);
						uint64_t const high = std::min(low+packsizehigh,n);
						
						::libmaus2::wavelet::ImpExternalWaveletGeneratorCompactHuffmanParallel::BufferType & BTS = IEWGH[numthreads+1+t];

						if ( high-low )
						{
							rl_decoder dec(bwt,low);

							uint64_t todec = high-low;
							std::pair<int64_t,uint64_t> P;
							
							while ( todec )
							{
								P = dec.decodeRun();
								assert ( P.first >= 0 );
								P.second = std::min(P.second,todec);

								for ( uint64_t i = 0; i < P.second; ++i )
									BTS.putSymbol(P.first);
								
								todec -= P.second;
							}
						}
					}
					
					IEWGH.createFinalStream(hwt);
				}
			}
			
			static void rlToHwtTerm(
				std::vector<std::string> const & bwt, 
				std::string const & hwt, 
				std::string const tmpprefix,
				::std::map<int64_t,uint64_t> const & chist,
				uint64_t const bwtterm,
				uint64_t const p0r
				)
			{
				::libmaus2::huffman::HuffmanTree H(chist.begin(),chist.size(),false,true,true);
				rlToHwtTerm(bwt,hwt,tmpprefix,H,bwtterm,p0r);
			}

			static ::libmaus2::huffman::HuffmanTree::unique_ptr_type loadCompactHuffmanTree(std::string const & huftreefilename)
			{
				libmaus2::aio::InputStreamInstance::unique_ptr_type CIN(new libmaus2::aio::InputStreamInstance(huftreefilename));
				::libmaus2::huffman::HuffmanTree::unique_ptr_type tH(new ::libmaus2::huffman::HuffmanTree(*CIN));
				// CIN->close();
				CIN.reset();
				
				return UNIQUE_PTR_MOVE(tH);
			}

			static ::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type loadHuffmanTree(std::string const & huftreefilename)
			{
				// deserialise symbol frequences
				libmaus2::aio::InputStreamInstance::unique_ptr_type chistCIN(new libmaus2::aio::InputStreamInstance(huftreefilename));
				::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type shnode = 
					::libmaus2::huffman::HuffmanTreeNode::deserialize(*chistCIN);
				//chistCIN->close();
				chistCIN.reset();
				
				return shnode;
			}

			static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type rlToHwtTerm(
				std::vector<std::string> const & bwt, 
				std::string const & hwt, 
				std::string const tmpprefix,
				std::string const huftreefilename,
				uint64_t const bwtterm,
				uint64_t const p0r
				)
			{
				::libmaus2::huffman::HuffmanTree::unique_ptr_type UH = loadCompactHuffmanTree(huftreefilename);
				::libmaus2::huffman::HuffmanTree & H = *UH;

				// std::cerr << "(maxdepth=" << H.maxDepth() << ",maxSymbol=" << H.maxSymbol() << ")";

				if ( H.maxDepth() <= 8*sizeof(uint8_t) && H.maxSymbol() <= std::numeric_limits<uint8_t>::max() )
				{
					// std::cerr << "(small)";
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(rlToHwtTermSmallAlphabet<uint8_t>(bwt,huftreefilename,bwtterm,p0r));			
					return UNIQUE_PTR_MOVE(tICHWT);
				}
				else if ( H.maxDepth() <= 8*sizeof(uint16_t) && H.maxSymbol() <= std::numeric_limits<uint16_t>::max() )
				{
					// std::cerr << "(small)";
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(rlToHwtTermSmallAlphabet<uint16_t>(bwt,huftreefilename,bwtterm,p0r));			
					return UNIQUE_PTR_MOVE(tICHWT);
				}
				else if ( H.maxDepth() <= 8*sizeof(uint32_t) && H.maxSymbol() <= std::numeric_limits<uint16_t>::max() )
				{
					// std::cerr << "(small)";
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(rlToHwtTermSmallAlphabet<uint32_t>(bwt,huftreefilename,bwtterm,p0r));			
					return UNIQUE_PTR_MOVE(tICHWT);
				}
				else if ( H.maxDepth() <= 8*sizeof(uint64_t) && H.maxSymbol() <= std::numeric_limits<uint16_t>::max() )
				{
					// std::cerr << "(small)";
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(rlToHwtTermSmallAlphabet<uint64_t>(bwt,huftreefilename,bwtterm,p0r));			
					return UNIQUE_PTR_MOVE(tICHWT);
				}
				else
				{
				
					// std::cerr << "(large)";
					rlToHwtTerm(bwt,hwt,tmpprefix,H,bwtterm,p0r);
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(loadWaveletTree(hwt));
					return UNIQUE_PTR_MOVE(tICHWT);
				}
			}

		};
	}
}
#endif
