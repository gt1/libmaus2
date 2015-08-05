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
#if ! defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_HPP)
#define LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/util/Utf8String.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/gamma/GammaRLDecoder.hpp>
#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/parallel/OMPNumThreadsScope.hpp>
#include <libmaus2/util/PutObjectReverse.hpp>
#include <libmaus2/autoarray/AutoArray2d.hpp>

namespace libmaus2
{
	namespace wavelet
	{
		struct Utf8ToImpHuffmanWaveletTree
		{
			struct ImpWaveletStackElement
			{
				uint64_t bleft;
				uint64_t bright;
				uint64_t sleft;
				uint64_t sright;
				unsigned int level;
				::libmaus2::huffman::HuffmanTreeNode const * hnode;
				
				ImpWaveletStackElement() : bleft(0), bright(0), sleft(0), sright(0), level(0), hnode(0) {}
				ImpWaveletStackElement(
					uint64_t const rbleft,
					uint64_t const rbright,
					uint64_t const rsleft,
					uint64_t const rsright,
					unsigned int const rlevel,
					::libmaus2::huffman::HuffmanTreeNode const * const rhnode
				) : bleft(rbleft), bright(rbright), sleft(rsleft), sright(rsright), level(rlevel), hnode(rhnode) {}
			};

			template<bool radixsort>
			static void constructWaveletTree(
				std::string const & fn, std::string const & outputfilename,
				::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type htree = 
					::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type(),
				uint64_t const numthreads = ::libmaus2::parallel::OMPNumThreadsScope::getMaxThreads()
			)
			{
				// ::libmaus2::parallel::OMPNumThreadsScope numthreadsscope(numthreads);
				::libmaus2::util::TempFileRemovalContainer::setup();

				if ( ! htree.get() )
				{
					::libmaus2::autoarray::AutoArray< std::pair<int64_t,uint64_t> > const ahist = 
						::libmaus2::util::Utf8String::getHistogramAsArray(fn);
					htree = ::libmaus2::huffman::HuffmanBase::createTree(ahist);
				}
				
				::libmaus2::huffman::EncodeTable<1> ET(htree.get());

				// #define HWTDEBUG
				
				::libmaus2::timing::RealTimeClock rtc; rtc.start();	
				if ( ! htree->isLeaf() )
				{
					uint64_t const infs = ::libmaus2::util::GetFileSize::getFileSize(fn);
					uint64_t const tpartsize = std::min(static_cast<uint64_t>(256*1024), (infs+numthreads-1)/numthreads);
					uint64_t const tnumparts = (infs + tpartsize - 1) / tpartsize;

					::libmaus2::autoarray::AutoArray<uint64_t> const partstarts = ::libmaus2::util::Utf8String::computePartStarts(fn,tnumparts);
					uint64_t const numparts = partstarts.size()-1;
					
					::libmaus2::autoarray::AutoArray<uint64_t> symsperpart(numparts+1);
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(numparts); ++i )
					{
						libmaus2::aio::InputStream::unique_ptr_type PCIS(libmaus2::aio::InputStreamFactoryContainer::constructUnique(fn));
						libmaus2::aio::InputStream & CIS = *PCIS;
						CIS.seekg(partstarts[i]);
						
						uint64_t lsyms = 0;
						uint64_t partlen = partstarts[i+1]-partstarts[i];

						while ( partlen-- )
							if ( (CIS.get() & 0xc0) != 0x80 )
								++lsyms;
						
						symsperpart[i] = lsyms;
					}
					symsperpart.prefixSums();

					std::string const tmpfilenamebase = fn+"_tmp";
					std::vector<std::string> tmpfilenames;
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						tmpfilenames.push_back(
							tmpfilenamebase + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(i,6)
						);
						::libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilenames[i]);
						// touch file
						::libmaus2::aio::OutputStreamInstance tmpCOS(tmpfilenames[i]);
					}
				
					uint64_t const numnodes = htree->numsyms()-1;
					#if 0
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodebitcnt(numparts);
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodewordcnt(numparts+1);
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						vnodebitcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes);
						vnodewordcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes+1);
					}
					#endif
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodebitcnt(numparts,numnodes);
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodewordcnt(numparts+1,numnodes+1);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t partid = 0; partid < static_cast<int64_t>(numparts); ++partid )
					{
						#if 0
						::libmaus2::autoarray::AutoArray<uint64_t> & nodebitcnt = vnodebitcnt[partid];
						::libmaus2::autoarray::AutoArray<uint64_t> & nodewordcnt = vnodewordcnt[partid];
						#endif
						uint64_t * const nodebitcnt = vnodebitcnt[partid];
						uint64_t * const nodewordcnt = vnodewordcnt[partid];
						uint64_t lnodeid = 0;

						uint64_t const partsize = partstarts[partid+1]-partstarts[partid];

						/* read text */
						::libmaus2::autoarray::AutoArray<uint8_t> A(partsize,false);
						::libmaus2::aio::InputStreamInstance::unique_ptr_type textCIS(new 
							::libmaus2::aio::InputStreamInstance(fn)
						);
						textCIS->seekg(partstarts[partid]);
						textCIS->read(reinterpret_cast<char *>(A.begin()),partsize);
						textCIS.reset();
						
						uint64_t const pbleft = 0;
						uint64_t const pbright = partsize;
						uint64_t const lnumsyms = symsperpart[partid+1]-symsperpart[partid];
						
						::libmaus2::autoarray::AutoArray<uint8_t> Z;
						if ( radixsort )
							Z = ::libmaus2::autoarray::AutoArray<uint8_t>(pbright-pbleft,false);

						std::stack<ImpWaveletStackElement> S;
						S.push(ImpWaveletStackElement(pbleft,pbright,0,lnumsyms,0,htree.get()));
					
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type tmpCOS(new ::libmaus2::aio::OutputStreamInstance(tmpfilenames[partid]));
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tmpSGO(
							new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(*tmpCOS,8*1024));
					
						while ( ! S.empty() )
						{
							ImpWaveletStackElement const T = S.top();
							S.pop();
							
							assert ( ! T.hnode->isLeaf() );
						
							unsigned int const level = T.level;
							uint64_t const srange = T.sright-T.sleft;
							
							nodebitcnt[lnodeid] = srange;
							nodewordcnt[lnodeid] = (srange + 63)/64;
							lnodeid++;
						
							::libmaus2::util::GetObject<uint8_t const *> G(A.begin()+T.bleft);
							
							uint64_t const prewords = tmpSGO->getWrittenWords();
							uint64_t numsyms0 = 0;
							uint64_t numsyms1 = 0;
							unsigned int av = 64;
							uint64_t outword = 0;
							uint64_t codelen0 = 0;
							uint64_t codelen1 = 0;
							
							if ( radixsort )
							{
								::libmaus2::util::PutObject<uint8_t *> P0(Z.begin());
								::libmaus2::util::PutObjectReverse<uint8_t *> P1(Z.end());

								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										P1.write(G.p-codelen,codelen);
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										P0.write(G.p-codelen,codelen);
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}

								codelen0 = (P0.p-Z.begin());
								codelen1 = (Z.end()-P1.p);
							}
							else
							{
								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										codelen1 += codelen;
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										codelen0 += codelen;
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}
							}
							
							if ( av != 64 )
							{
								outword <<= av;
								tmpSGO->put(outword);
							}


							uint64_t const postwords = tmpSGO->getWrittenWords();
							uint64_t const wordswritten = postwords-prewords;
							assert ( wordswritten == nodewordcnt[lnodeid-1] );
							
							if ( radixsort )
							{
								std::copy(Z.begin(),Z.begin()+codelen0,A.begin()+T.bleft);
								uint8_t const * zp = Z.end();
								uint8_t * p1 = A.begin()+T.bleft+codelen0;
								uint8_t * const p1e = p1 + codelen1;
								while ( p1 != p1e )
									*(p1++) = *(--zp);
							}
							else
							{
								for ( uint64_t sorthalf = 1; sorthalf < srange; sorthalf *=2 )
								{
									uint64_t const sortfull = 2*sorthalf;
									uint64_t const sortparts = (srange + sortfull-1)/sortfull;
									uint64_t bleft = T.bleft;
													
									for ( uint64_t s = 0; s < sortparts; ++s )
									{
										uint64_t const lsortbase = s*sortfull;
										uint64_t const lsortrighta = std::min(lsortbase + sorthalf,srange);
										uint64_t const lsortrightb = std::min(lsortrighta + sorthalf,srange);
										
										uint64_t l_codelen0 = 0;
										uint64_t l_codelen1 = 0;
										uint64_t l_numsyms0 = 0;
										uint64_t l_numsyms1 = 0;
										uint64_t r_codelen0 = 0;
										uint64_t r_codelen1 = 0;
										uint64_t r_numsyms0 = 0;
										uint64_t r_numsyms1 = 0;
										
										::libmaus2::util::GetObject<uint8_t const *> LG(A.begin()+bleft);
										for ( uint64_t i = 0; i < (lsortrighta-lsortbase); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												l_codelen1 += codelen;
												l_numsyms1 += 1;
											}
											else
											{
												l_codelen0 += codelen;
												l_numsyms0 += 1;
											}
											
										}
										
										for ( uint64_t i = 0; i < (lsortrightb-lsortrighta); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												r_codelen1 += codelen;
												r_numsyms1 += 1;
											}
											else
											{
												r_codelen0 += codelen;
												r_numsyms0 += 1;
											}	
										}

										if ( l_numsyms1 && r_numsyms0 )
										{
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1              );
											std::reverse(A.begin() + bleft + l_codelen0 + l_codelen1, A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
										}
										
										bleft += (l_codelen0+l_codelen1+r_codelen0+r_codelen1);
									}
								}
							}
						
							::libmaus2::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const * >(T.hnode);
							
							if ( (!(node->right->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft+codelen0,T.bright,T.sleft+numsyms0,T.sright,level+1,node->right));
							if ( (!(node->left->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft,T.bleft+codelen0,T.sleft,T.sleft+numsyms0,level+1,node->left));

							#if defined(HWTDEBUG)
							::libmaus2::util::GetObject<uint8_t const *> DG(A.begin()+T.bleft);
							uint64_t d_codelen0 = 0;
							uint64_t d_codelen1 = 0;
							uint64_t d_numsyms0 = 0;
							uint64_t d_numsyms1 = 0;
							for ( uint64_t i = 0; i < srange; ++i )
							{
								uint64_t codelen = 0;
								wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(DG,codelen);
								bool const wbit = ET.getBitFromTop(sym,level);
							
								if ( wbit )
								{
									if ( ! d_numsyms1 )
									{
										assert ( codelen0 == d_codelen0 );
										assert ( numsyms0 == d_numsyms0 );
									}
								
									d_codelen1 += codelen;
									d_numsyms1 += 1;
								}
								else
								{
									d_codelen0 += codelen;
									d_numsyms0 += 1;
								}
							}

							assert ( codelen0 == d_codelen0 );
							assert ( codelen1 == d_codelen1 );
							assert ( numsyms0 == d_numsyms0 );
							assert ( numsyms1 == d_numsyms1 );
							#endif			
						}

						// assert ( lnodeid == nodebitcnt.size() );
						assert ( lnodeid == numnodes );
						
						tmpSGO->flush();
						tmpSGO.reset();
						tmpCOS->flush();
						tmpCOS.reset();
					}

					// accumulate word offsets
					for ( uint64_t i = 0; i < numparts; ++i )
						// vnodewordcnt[i].prefixSums();
						vnodewordcnt.prefixSums(i);
					
					::libmaus2::autoarray::AutoArray<uint64_t> vnodebits(numnodes);
					uint64_t tnumbits = 0;
					for ( uint64_t nodeid = 0; nodeid < numnodes; ++nodeid )
					{
						uint64_t nodebits = 0;
						for ( uint64_t p = 0; p < numparts; ++p )
							nodebits += vnodebitcnt[p][nodeid];
						
						vnodebits[nodeid] = nodebits;
						tnumbits += nodebits;
					}
							
					uint64_t const tpartnodebits = (tnumbits+numthreads-1)/numthreads;
					std::vector < std::pair<uint64_t,uint64_t> > nodepacks;
					
					uint64_t llow = 0;
					while ( llow != numnodes )
					{
						uint64_t lhigh = llow;
						uint64_t s = 0;
						
						while ( lhigh != numnodes && s < tpartnodebits )
							s += vnodebits[lhigh++];

						nodepacks.push_back(std::pair<uint64_t,uint64_t>(llow,lhigh));

						llow = lhigh;
					}
					
					assert ( nodepacks.size() <= numthreads );

					std::vector<std::string> nptempfilenames;
					::libmaus2::autoarray::AutoArray< ::libmaus2::aio::OutputStreamInstance::unique_ptr_type > tmpCOS(nodepacks.size());
					for ( uint64_t np = 0; np < nodepacks.size(); ++np )
					{
						nptempfilenames.push_back(tmpfilenamebase + "_np_" + ::libmaus2::util::NumberSerialisation::formatNumber(np,6));
						::libmaus2::util::TempFileRemovalContainer::addTempFile(nptempfilenames[np]);
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type ttmpCOSnp(
                                                                new ::libmaus2::aio::OutputStreamInstance(nptempfilenames[np])
                                                        );
						tmpCOS[np] = UNIQUE_PTR_MOVE(ttmpCOSnp);
					}
					
					::libmaus2::autoarray::AutoArray<uint64_t> nodebytesizes(numnodes);
					
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t np = 0; np < static_cast<int64_t>(nodepacks.size()); ++np )
					{
						uint64_t const nplow = nodepacks[np].first;
						uint64_t const nphigh = nodepacks[np].second;
						::libmaus2::aio::OutputStreamInstance & npout = *(tmpCOS[np]);
						
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::InputStreamInstance::unique_ptr_type > tmpCIS(numparts);
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > tmpSGI(numparts);
						
						for ( uint64_t i = 0; i < numparts; ++i )
						{
							::libmaus2::aio::InputStreamInstance::unique_ptr_type ttmpCISi(new ::libmaus2::aio::InputStreamInstance(tmpfilenames[i]));
							tmpCIS[i] = UNIQUE_PTR_MOVE(ttmpCISi);
							tmpCIS[i]->seekg(vnodewordcnt[i][nplow]*sizeof(uint64_t));
							::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type ttmpSGIi(new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(*tmpCIS[i],1024));
							tmpSGI[i] = UNIQUE_PTR_MOVE(ttmpSGIi);
						}

						for ( uint64_t npi = nplow; npi < nphigh; ++npi )
						{
							uint64_t const totalnodebits = vnodebits[npi];

							::libmaus2::rank::ImpCacheLineRank::WriteContextExternal context(npout,totalnodebits+1);
							
							for ( uint64_t p = 0; p < numparts; ++p )
							{
								uint64_t bitstowrite = vnodebitcnt[p][npi];
								uint64_t word = 0;
								int shift = -1;
								
								while ( bitstowrite )
								{
									if ( shift < 0 )
									{
										tmpSGI[p]->getNext(word);
										shift = 63;
									}
									
									bool const bit = (word >> shift) & 1;
									context.writeBit(bit);
									
									--bitstowrite;
									--shift;
								}
							}
							
							context.writeBit(0);
							context.flush();
							
							nodebytesizes[npi] = (2 /* header */+context.wordsWritten())*sizeof(uint64_t);
						}
						
						npout.flush();
						tmpCOS[np].reset();
					}

					for ( uint64_t i = 0; i < tmpfilenames.size(); ++i )
						remove ( tmpfilenames[i].c_str() );
						
					nodebytesizes.prefixSums();

					uint64_t outfilepos = 0;
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type Pfinalout(new ::libmaus2::aio::OutputStreamInstance(outputfilename));
					::libmaus2::aio::OutputStreamInstance & finalout = *Pfinalout;
					
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,symsperpart[numparts]);
					outfilepos += htree->serialize(finalout);
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					
					uint64_t const dictbasepos = outfilepos;
					for ( uint64_t i = 0; i < numnodes; ++i )
						nodebytesizes[i] += dictbasepos;
					
					for ( uint64_t i = 0; i < nptempfilenames.size(); ++i )
					{
						::libmaus2::aio::InputStreamInstance tmpCIS(nptempfilenames[i]);
						uint64_t const tmpfilesize = ::libmaus2::util::GetFileSize::getFileSize(tmpCIS);
						::libmaus2::util::GetFileSize::copy(tmpCIS,finalout,tmpfilesize);
						outfilepos += tmpfilesize;
						remove ( nptempfilenames[i].c_str() );
					}
					
					uint64_t const indexpos = outfilepos;	

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					for ( uint64_t i = 0; i < numnodes; ++i )
						outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,nodebytesizes[i]);

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,indexpos);
					
					finalout.flush();
					Pfinalout.reset();
					
					#if defined(HWTDEBUG)
					/**
					 * load tree and write out text
					 **/
					::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type PIHWT = UNIQUE_PTR_MOVE(
						::libmaus2::wavelet::ImpHuffmanWaveletTree::load(outputfilename)
					);
					::libmaus2::wavelet::ImpHuffmanWaveletTree const & IHWT = *PIHWT;
					assert ( IHWT.getN() == symsperpart[symsperpart.size()-1] );
					
					::libmaus2::aio::OutputStreamInstance debCOS(fn + ".debug");
					for ( uint64_t i = 0; i < IHWT.size(); ++i )
						::libmaus2::util::UTF8::encodeUTF8(IHWT[i],debCOS);
					debCOS.flush();
					debCOS.close();
					#endif
				}		
			}

			template<bool radixsort>
			static void constructWaveletTree(
				::libmaus2::autoarray::AutoArray<uint8_t> & A, std::string const & outputfilename,
				std::string const & tmpfilenamebase,
				::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type htree = 
					::libmaus2::huffman::HuffmanTreeNode::shared_ptr_type(),
				uint64_t const numthreads = ::libmaus2::parallel::OMPNumThreadsScope::getMaxThreads()
			)
			{
				// ::libmaus2::parallel::OMPNumThreadsScope numthreadsscope(numthreads);
				::libmaus2::util::TempFileRemovalContainer::setup();

				if ( ! htree.get() )
				{
					::libmaus2::autoarray::AutoArray< std::pair<int64_t,uint64_t> > const ahist = 
						::libmaus2::util::Utf8String::getHistogramAsArray(A);
					htree = ::libmaus2::huffman::HuffmanBase::createTree(ahist);
				}
				
				::libmaus2::huffman::EncodeTable<1> ET(htree.get());

				// #define HWTDEBUG
				
				::libmaus2::timing::RealTimeClock rtc; rtc.start();	
				if ( ! htree->isLeaf() )
				{
					uint64_t const infs = A.size();
					uint64_t const tpartsize = std::min(static_cast<uint64_t>(256*1024), (infs+numthreads-1)/numthreads);
					uint64_t const tnumparts = (infs + tpartsize - 1) / tpartsize;

					::libmaus2::autoarray::AutoArray<uint64_t> const partstarts = ::libmaus2::util::Utf8String::computePartStarts(A,tnumparts);
					uint64_t const numparts = partstarts.size()-1;
					
					::libmaus2::autoarray::AutoArray<uint64_t> symsperpart(numparts+1);
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(numparts); ++i )
					{
						::libmaus2::util::GetObject<uint8_t const *> CIS(A.begin() + partstarts[i]);
						
						uint64_t lsyms = 0;
						uint64_t partlen = partstarts[i+1]-partstarts[i];

						while ( partlen-- )
							if ( (CIS.get() & 0xc0) != 0x80 )
								++lsyms;
						
						symsperpart[i] = lsyms;
					}
					symsperpart.prefixSums();

					std::vector<std::string> tmpfilenames;
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						tmpfilenames.push_back(
							tmpfilenamebase + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(i,6)
						);
						::libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilenames[i]);
						// touch file
						::libmaus2::aio::OutputStreamInstance tmpCOS(tmpfilenames[i]);
					}
				
					uint64_t const numnodes = htree->numsyms()-1;
					#if 0
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodebitcnt(numparts);
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodewordcnt(numparts+1);
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						vnodebitcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes);
						vnodewordcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes+1);
					}
					#endif
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodebitcnt(numparts,numnodes);
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodewordcnt(numparts+1,numnodes+1);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t partid = 0; partid < static_cast<int64_t>(numparts); ++partid )
					{
						#if 0
						::libmaus2::autoarray::AutoArray<uint64_t> & nodebitcnt = vnodebitcnt[partid];
						::libmaus2::autoarray::AutoArray<uint64_t> & nodewordcnt = vnodewordcnt[partid];
						#endif
						uint64_t * const nodebitcnt = vnodebitcnt[partid];
						uint64_t * const nodewordcnt = vnodewordcnt[partid];
						uint64_t lnodeid = 0;

						uint64_t const partsize = partstarts[partid+1]-partstarts[partid];

						uint64_t const pbleft = partstarts[partid];
						uint64_t const pbright = pbleft + partsize;
						uint64_t const lnumsyms = symsperpart[partid+1]-symsperpart[partid];
						
						::libmaus2::autoarray::AutoArray<uint8_t> Z;
						if ( radixsort )
							Z = ::libmaus2::autoarray::AutoArray<uint8_t>(pbright-pbleft,false);

						std::stack<ImpWaveletStackElement> S;
						S.push(ImpWaveletStackElement(pbleft,pbright,0,lnumsyms,0,htree.get()));
					
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type tmpCOS(new ::libmaus2::aio::OutputStreamInstance(tmpfilenames[partid]));
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tmpSGO(
							new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(*tmpCOS,8*1024));
					
						while ( ! S.empty() )
						{
							ImpWaveletStackElement const T = S.top();
							S.pop();
							
							assert ( ! T.hnode->isLeaf() );
						
							unsigned int const level = T.level;
							uint64_t const srange = T.sright-T.sleft;
							
							nodebitcnt[lnodeid] = srange;
							nodewordcnt[lnodeid] = (srange + 63)/64;
							lnodeid++;
						
							::libmaus2::util::GetObject<uint8_t const *> G(A.begin()+T.bleft);
							
							uint64_t const prewords = tmpSGO->getWrittenWords();
							uint64_t numsyms0 = 0;
							uint64_t numsyms1 = 0;
							unsigned int av = 64;
							uint64_t outword = 0;
							uint64_t codelen0 = 0;
							uint64_t codelen1 = 0;
							
							if ( radixsort )
							{
								::libmaus2::util::PutObject<uint8_t *> P0(Z.begin());
								::libmaus2::util::PutObjectReverse<uint8_t *> P1(Z.end());

								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										P1.write(G.p-codelen,codelen);
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										P0.write(G.p-codelen,codelen);
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}

								codelen0 = (P0.p-Z.begin());
								codelen1 = (Z.end()-P1.p);
							}
							else
							{
								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										codelen1 += codelen;
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										codelen0 += codelen;
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}
							}
							
							if ( av != 64 )
							{
								outword <<= av;
								tmpSGO->put(outword);
							}


							uint64_t const postwords = tmpSGO->getWrittenWords();
							uint64_t const wordswritten = postwords-prewords;
							assert ( wordswritten == nodewordcnt[lnodeid-1] );
							
							if ( radixsort )
							{
								std::copy(Z.begin(),Z.begin()+codelen0,A.begin()+T.bleft);
								uint8_t const * zp = Z.end();
								uint8_t * p1 = A.begin()+T.bleft+codelen0;
								uint8_t * const p1e = p1 + codelen1;
								while ( p1 != p1e )
									*(p1++) = *(--zp);
							}
							else
							{
								for ( uint64_t sorthalf = 1; sorthalf < srange; sorthalf *=2 )
								{
									uint64_t const sortfull = 2*sorthalf;
									uint64_t const sortparts = (srange + sortfull-1)/sortfull;
									uint64_t bleft = T.bleft;
													
									for ( uint64_t s = 0; s < sortparts; ++s )
									{
										uint64_t const lsortbase = s*sortfull;
										uint64_t const lsortrighta = std::min(lsortbase + sorthalf,srange);
										uint64_t const lsortrightb = std::min(lsortrighta + sorthalf,srange);
										
										uint64_t l_codelen0 = 0;
										uint64_t l_codelen1 = 0;
										uint64_t l_numsyms0 = 0;
										uint64_t l_numsyms1 = 0;
										uint64_t r_codelen0 = 0;
										uint64_t r_codelen1 = 0;
										uint64_t r_numsyms0 = 0;
										uint64_t r_numsyms1 = 0;
										
										::libmaus2::util::GetObject<uint8_t const *> LG(A.begin()+bleft);
										for ( uint64_t i = 0; i < (lsortrighta-lsortbase); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												l_codelen1 += codelen;
												l_numsyms1 += 1;
											}
											else
											{
												l_codelen0 += codelen;
												l_numsyms0 += 1;
											}
											
										}
										
										for ( uint64_t i = 0; i < (lsortrightb-lsortrighta); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												r_codelen1 += codelen;
												r_numsyms1 += 1;
											}
											else
											{
												r_codelen0 += codelen;
												r_numsyms0 += 1;
											}	
										}

										if ( l_numsyms1 && r_numsyms0 )
										{
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1              );
											std::reverse(A.begin() + bleft + l_codelen0 + l_codelen1, A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
										}
										
										bleft += (l_codelen0+l_codelen1+r_codelen0+r_codelen1);
									}
								}
							}
						
							::libmaus2::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const * >(T.hnode);
							
							if ( (!(node->right->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft+codelen0,T.bright,T.sleft+numsyms0,T.sright,level+1,node->right));
							if ( (!(node->left->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft,T.bleft+codelen0,T.sleft,T.sleft+numsyms0,level+1,node->left));

							#if defined(HWTDEBUG)
							::libmaus2::util::GetObject<uint8_t const *> DG(A.begin()+T.bleft);
							uint64_t d_codelen0 = 0;
							uint64_t d_codelen1 = 0;
							uint64_t d_numsyms0 = 0;
							uint64_t d_numsyms1 = 0;
							for ( uint64_t i = 0; i < srange; ++i )
							{
								uint64_t codelen = 0;
								wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(DG,codelen);
								bool const wbit = ET.getBitFromTop(sym,level);
							
								if ( wbit )
								{
									if ( ! d_numsyms1 )
									{
										assert ( codelen0 == d_codelen0 );
										assert ( numsyms0 == d_numsyms0 );
									}
								
									d_codelen1 += codelen;
									d_numsyms1 += 1;
								}
								else
								{
									d_codelen0 += codelen;
									d_numsyms0 += 1;
								}
							}

							assert ( codelen0 == d_codelen0 );
							assert ( codelen1 == d_codelen1 );
							assert ( numsyms0 == d_numsyms0 );
							assert ( numsyms1 == d_numsyms1 );
							#endif			
						}

						#if 0
						assert ( lnodeid == nodebitcnt.size() );
						#endif
						assert ( lnodeid == numnodes );
						
						tmpSGO->flush();
						tmpSGO.reset();
						tmpCOS->flush();
						tmpCOS.reset();
					}

					// accumulate word offsets
					for ( uint64_t i = 0; i < numparts; ++i )
						//vnodewordcnt[i].prefixSums();
						vnodewordcnt.prefixSums(i);
					
					::libmaus2::autoarray::AutoArray<uint64_t> vnodebits(numnodes);
					uint64_t tnumbits = 0;
					for ( uint64_t nodeid = 0; nodeid < numnodes; ++nodeid )
					{
						uint64_t nodebits = 0;
						for ( uint64_t p = 0; p < numparts; ++p )
							nodebits += vnodebitcnt[p][nodeid];
						
						vnodebits[nodeid] = nodebits;
						tnumbits += nodebits;
					}
							
					uint64_t const tpartnodebits = (tnumbits+numthreads-1)/numthreads;
					std::vector < std::pair<uint64_t,uint64_t> > nodepacks;
					
					uint64_t llow = 0;
					while ( llow != numnodes )
					{
						uint64_t lhigh = llow;
						uint64_t s = 0;
						
						while ( lhigh != numnodes && s < tpartnodebits )
							s += vnodebits[lhigh++];

						nodepacks.push_back(std::pair<uint64_t,uint64_t>(llow,lhigh));

						llow = lhigh;
					}
					
					// assert ( nodepacks.size() <= numthreads );

					std::vector<std::string> nptempfilenames;
					::libmaus2::autoarray::AutoArray< ::libmaus2::aio::OutputStreamInstance::unique_ptr_type > tmpCOS(nodepacks.size());
					for ( uint64_t np = 0; np < nodepacks.size(); ++np )
					{
						nptempfilenames.push_back(tmpfilenamebase + "_np_" + ::libmaus2::util::NumberSerialisation::formatNumber(np,6));
						::libmaus2::util::TempFileRemovalContainer::addTempFile(nptempfilenames[np]);
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type ttmpCOSi(
                                                                new ::libmaus2::aio::OutputStreamInstance(nptempfilenames[np])
                                                        );
						tmpCOS[np] = UNIQUE_PTR_MOVE(ttmpCOSi);
					}
					
					::libmaus2::autoarray::AutoArray<uint64_t> nodebytesizes(numnodes);
					
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t np = 0; np < static_cast<int64_t>(nodepacks.size()); ++np )
					{
						uint64_t const nplow = nodepacks[np].first;
						uint64_t const nphigh = nodepacks[np].second;
						::libmaus2::aio::OutputStreamInstance & npout = *(tmpCOS[np]);
						
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::InputStreamInstance::unique_ptr_type > tmpCIS(numparts);
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > tmpSGI(numparts);
						
						for ( uint64_t i = 0; i < numparts; ++i )
						{
							::libmaus2::aio::InputStreamInstance::unique_ptr_type ttmpCISi(new ::libmaus2::aio::InputStreamInstance(tmpfilenames[i]));
							tmpCIS[i] = UNIQUE_PTR_MOVE(ttmpCISi);
							tmpCIS[i]->seekg(vnodewordcnt[i][nplow]*sizeof(uint64_t));
							::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type ttmpSGIi(new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(*tmpCIS[i],1024));
							tmpSGI[i] = UNIQUE_PTR_MOVE(ttmpSGIi);
						}

						for ( uint64_t npi = nplow; npi < nphigh; ++npi )
						{
							uint64_t const totalnodebits = vnodebits[npi];

							::libmaus2::rank::ImpCacheLineRank::WriteContextExternal context(npout,totalnodebits+1);
							
							for ( uint64_t p = 0; p < numparts; ++p )
							{
								uint64_t bitstowrite = vnodebitcnt[p][npi];
								uint64_t word = 0;
								int shift = -1;
								
								while ( bitstowrite )
								{
									if ( shift < 0 )
									{
										tmpSGI[p]->getNext(word);
										shift = 63;
									}
									
									bool const bit = (word >> shift) & 1;
									context.writeBit(bit);
									
									--bitstowrite;
									--shift;
								}
							}
							
							context.writeBit(0);
							context.flush();
							
							nodebytesizes[npi] = (2 /* header */+context.wordsWritten())*sizeof(uint64_t);
						}
						
						npout.flush();
						tmpCOS[np].reset();
					}

					for ( uint64_t i = 0; i < tmpfilenames.size(); ++i )
						remove ( tmpfilenames[i].c_str() );
						
					nodebytesizes.prefixSums();

					uint64_t outfilepos = 0;
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type Pfinalout(new ::libmaus2::aio::OutputStreamInstance(outputfilename));
					::libmaus2::aio::OutputStreamInstance & finalout = *Pfinalout;
					
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,symsperpart[numparts]);
					outfilepos += htree->serialize(finalout);
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					
					uint64_t const dictbasepos = outfilepos;
					for ( uint64_t i = 0; i < numnodes; ++i )
						nodebytesizes[i] += dictbasepos;
					
					for ( uint64_t i = 0; i < nptempfilenames.size(); ++i )
					{
						::libmaus2::aio::InputStreamInstance tmpCIS(nptempfilenames[i]);
						uint64_t const tmpfilesize = ::libmaus2::util::GetFileSize::getFileSize(tmpCIS);
						::libmaus2::util::GetFileSize::copy(tmpCIS,finalout,tmpfilesize);
						outfilepos += tmpfilesize;
						remove ( nptempfilenames[i].c_str() );
					}
					
					uint64_t const indexpos = outfilepos;	

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					for ( uint64_t i = 0; i < numnodes; ++i )
						outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,nodebytesizes[i]);

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,indexpos);
					
					finalout.flush();
					Pfinalout.reset();
					
					#if defined(HWTDEBUG)
					/**
					 * load tree and write out text
					 **/
					::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type PIHWT = UNIQUE_PTR_MOVE(
						::libmaus2::wavelet::ImpHuffmanWaveletTree::load(outputfilename)
					);
					::libmaus2::wavelet::ImpHuffmanWaveletTree const & IHWT = *PIHWT;
					assert ( IHWT.getN() == symsperpart[symsperpart.size()-1] );
					
					::libmaus2::aio::OutputStreamInstance debCOS(fn + ".debug");
					for ( uint64_t i = 0; i < IHWT.size(); ++i )
						::libmaus2::util::UTF8::encodeUTF8(IHWT[i],debCOS);
					debCOS.flush();
					debCOS.close();
					#endif
				}		
			}

			template<typename rl_decoder, bool radixsort>
			static void constructWaveletTreeFromRl(
				std::string const & fn, std::string const & outputfilename,
				std::string const & tmpfilenamebase,
				::libmaus2::huffman::HuffmanTreeNode const * htree,
				uint64_t const tpartsizemax = 1024ull*1024ull,
				uint64_t const numthreads = ::libmaus2::parallel::OMPNumThreadsScope::getMaxThreads()
			)
			{
				// ::libmaus2::parallel::OMPNumThreadsScope numthreadsscope(numthreads);
				::libmaus2::util::TempFileRemovalContainer::setup();

				::libmaus2::huffman::EncodeTable<1> ET(htree);

				// #define HWTDEBUG
				
				::libmaus2::timing::RealTimeClock rtc; rtc.start();	
				if ( ! htree->isLeaf() )
				{
					uint64_t const infs = rl_decoder::getLength(fn);
					uint64_t const tpartsize = std::min(static_cast<uint64_t>(tpartsizemax), (infs+numthreads-1)/numthreads);
					uint64_t const numparts = (infs + tpartsize - 1) / tpartsize;
					
					::libmaus2::autoarray::AutoArray<uint64_t> symsperpart(numparts+1);
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						uint64_t const slow  = std::min(i * tpartsize,infs);
						uint64_t const shigh = std::min(slow+tpartsize,infs);
						symsperpart[i] = shigh-slow;
					}
					symsperpart.prefixSums();

					// std::string const tmpfilenamebase = fn+"_tmp";
					std::vector<std::string> tmpfilenames;
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						tmpfilenames.push_back(
							tmpfilenamebase + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(i,6)
						);
						::libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilenames[i]);
						// touch file
						::libmaus2::aio::OutputStreamInstance tmpCOS(tmpfilenames[i]);
					}
				
					uint64_t const numnodes = htree->numsyms()-1;
					#if 0
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodebitcnt(numparts);
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodewordcnt(numparts+1);
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						vnodebitcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes);
						vnodewordcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes+1);
					}
					#endif
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodebitcnt(numparts,numnodes);
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodewordcnt(numparts+1,numnodes+1);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t partid = 0; partid < static_cast<int64_t>(numparts); ++partid )
					{
						#if 0
						::libmaus2::autoarray::AutoArray<uint64_t> & nodebitcnt = vnodebitcnt[partid];
						::libmaus2::autoarray::AutoArray<uint64_t> & nodewordcnt = vnodewordcnt[partid];
						#endif
						uint64_t * const nodebitcnt = vnodebitcnt[partid];
						uint64_t * const nodewordcnt = vnodewordcnt[partid];
						uint64_t lnodeid = 0;
						
						uint64_t const numsyms = symsperpart[partid+1]-symsperpart[partid];

						typename rl_decoder::unique_ptr_type rldec(new rl_decoder(
							std::vector<std::string>(1,fn),
							symsperpart[partid]));
						::libmaus2::util::CountPutObject CPO;
						for ( uint64_t i = 0; i < numsyms; ++i )
							::libmaus2::util::UTF8::encodeUTF8(rldec->decode(),CPO);

						uint64_t const partsize = CPO.c;

						/* read text */
						::libmaus2::autoarray::AutoArray<uint8_t> A(partsize,false);

						rldec = UNIQUE_PTR_MOVE(typename rl_decoder::unique_ptr_type(new rl_decoder(
							std::vector<std::string>(1,fn),symsperpart[partid])));
						::libmaus2::util::PutObject<uint8_t *> PO(A.begin());
						for ( uint64_t i = 0; i < numsyms; ++i )
							::libmaus2::util::UTF8::encodeUTF8(rldec->decode(),PO);
						
						uint64_t const pbleft = 0;
						uint64_t const pbright = partsize;
						uint64_t const lnumsyms = numsyms;
						
						::libmaus2::autoarray::AutoArray<uint8_t> Z;
						if ( radixsort )
							Z = ::libmaus2::autoarray::AutoArray<uint8_t>(pbright-pbleft,false);

						std::stack<ImpWaveletStackElement> S;
						S.push(ImpWaveletStackElement(pbleft,pbright,0,lnumsyms,0,htree));
					
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type tmpCOS(new ::libmaus2::aio::OutputStreamInstance(tmpfilenames[partid]));
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tmpSGO(
							new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(*tmpCOS,8*1024));
					
						while ( ! S.empty() )
						{
							ImpWaveletStackElement const T = S.top();
							S.pop();
							
							assert ( ! T.hnode->isLeaf() );
						
							unsigned int const level = T.level;
							uint64_t const srange = T.sright-T.sleft;
							
							nodebitcnt[lnodeid] = srange;
							nodewordcnt[lnodeid] = (srange + 63)/64;
							lnodeid++;
						
							::libmaus2::util::GetObject<uint8_t const *> G(A.begin()+T.bleft);
							
							uint64_t const prewords = tmpSGO->getWrittenWords();
							uint64_t numsyms0 = 0;
							uint64_t numsyms1 = 0;
							unsigned int av = 64;
							uint64_t outword = 0;
							uint64_t codelen0 = 0;
							uint64_t codelen1 = 0;
							
							if ( radixsort )
							{
								::libmaus2::util::PutObject<uint8_t *> P0(Z.begin());
								::libmaus2::util::PutObjectReverse<uint8_t *> P1(Z.end());

								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										P1.write(G.p-codelen,codelen);
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										P0.write(G.p-codelen,codelen);
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}

								codelen0 = (P0.p-Z.begin());
								codelen1 = (Z.end()-P1.p);
							}
							else
							{
								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										codelen1 += codelen;
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										codelen0 += codelen;
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}
							}
							
							if ( av != 64 )
							{
								outword <<= av;
								tmpSGO->put(outword);
							}


							uint64_t const postwords = tmpSGO->getWrittenWords();
							uint64_t const wordswritten = postwords-prewords;
							assert ( wordswritten == nodewordcnt[lnodeid-1] );
							
							if ( radixsort )
							{
								std::copy(Z.begin(),Z.begin()+codelen0,A.begin()+T.bleft);
								uint8_t const * zp = Z.end();
								uint8_t * p1 = A.begin()+T.bleft+codelen0;
								uint8_t * const p1e = p1 + codelen1;
								while ( p1 != p1e )
									*(p1++) = *(--zp);
							}
							else
							{
								for ( uint64_t sorthalf = 1; sorthalf < srange; sorthalf *=2 )
								{
									uint64_t const sortfull = 2*sorthalf;
									uint64_t const sortparts = (srange + sortfull-1)/sortfull;
									uint64_t bleft = T.bleft;
													
									for ( uint64_t s = 0; s < sortparts; ++s )
									{
										uint64_t const lsortbase = s*sortfull;
										uint64_t const lsortrighta = std::min(lsortbase + sorthalf,srange);
										uint64_t const lsortrightb = std::min(lsortrighta + sorthalf,srange);
										
										uint64_t l_codelen0 = 0;
										uint64_t l_codelen1 = 0;
										uint64_t l_numsyms0 = 0;
										uint64_t l_numsyms1 = 0;
										uint64_t r_codelen0 = 0;
										uint64_t r_codelen1 = 0;
										uint64_t r_numsyms0 = 0;
										uint64_t r_numsyms1 = 0;
										
										::libmaus2::util::GetObject<uint8_t const *> LG(A.begin()+bleft);
										for ( uint64_t i = 0; i < (lsortrighta-lsortbase); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												l_codelen1 += codelen;
												l_numsyms1 += 1;
											}
											else
											{
												l_codelen0 += codelen;
												l_numsyms0 += 1;
											}
											
										}
										
										for ( uint64_t i = 0; i < (lsortrightb-lsortrighta); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												r_codelen1 += codelen;
												r_numsyms1 += 1;
											}
											else
											{
												r_codelen0 += codelen;
												r_numsyms0 += 1;
											}	
										}

										if ( l_numsyms1 && r_numsyms0 )
										{
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1              );
											std::reverse(A.begin() + bleft + l_codelen0 + l_codelen1, A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
										}
										
										bleft += (l_codelen0+l_codelen1+r_codelen0+r_codelen1);
									}
								}
							}
						
							::libmaus2::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const * >(T.hnode);
							
							if ( (!(node->right->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft+codelen0,T.bright,T.sleft+numsyms0,T.sright,level+1,node->right));
							if ( (!(node->left->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft,T.bleft+codelen0,T.sleft,T.sleft+numsyms0,level+1,node->left));

							#if defined(HWTDEBUG)
							::libmaus2::util::GetObject<uint8_t const *> DG(A.begin()+T.bleft);
							uint64_t d_codelen0 = 0;
							uint64_t d_codelen1 = 0;
							uint64_t d_numsyms0 = 0;
							uint64_t d_numsyms1 = 0;
							for ( uint64_t i = 0; i < srange; ++i )
							{
								uint64_t codelen = 0;
								wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(DG,codelen);
								bool const wbit = ET.getBitFromTop(sym,level);
							
								if ( wbit )
								{
									if ( ! d_numsyms1 )
									{
										assert ( codelen0 == d_codelen0 );
										assert ( numsyms0 == d_numsyms0 );
									}
								
									d_codelen1 += codelen;
									d_numsyms1 += 1;
								}
								else
								{
									d_codelen0 += codelen;
									d_numsyms0 += 1;
								}
							}

							assert ( codelen0 == d_codelen0 );
							assert ( codelen1 == d_codelen1 );
							assert ( numsyms0 == d_numsyms0 );
							assert ( numsyms1 == d_numsyms1 );
							#endif			
						}

						// assert ( lnodeid == nodebitcnt.size() );
						assert ( lnodeid == numnodes );
						
						tmpSGO->flush();
						tmpSGO.reset();
						tmpCOS->flush();
						tmpCOS.reset();
					}

					// accumulate word offsets
					for ( uint64_t i = 0; i < numparts; ++i )
						//vnodewordcnt[i].prefixSums();
						vnodewordcnt.prefixSums(i);
					
					::libmaus2::autoarray::AutoArray<uint64_t> vnodebits(numnodes);
					uint64_t tnumbits = 0;
					for ( uint64_t nodeid = 0; nodeid < numnodes; ++nodeid )
					{
						uint64_t nodebits = 0;
						for ( uint64_t p = 0; p < numparts; ++p )
							nodebits += vnodebitcnt[p][nodeid];
						
						vnodebits[nodeid] = nodebits;
						tnumbits += nodebits;
					}
							
					uint64_t const tpartnodebits = (tnumbits+numthreads-1)/numthreads;
					std::vector < std::pair<uint64_t,uint64_t> > nodepacks;
					
					uint64_t llow = 0;
					while ( llow != numnodes )
					{
						uint64_t lhigh = llow;
						uint64_t s = 0;
						
						while ( lhigh != numnodes && s < tpartnodebits )
							s += vnodebits[lhigh++];

						nodepacks.push_back(std::pair<uint64_t,uint64_t>(llow,lhigh));

						llow = lhigh;
					}
					
					assert ( nodepacks.size() <= numthreads );

					std::vector<std::string> nptempfilenames;
					::libmaus2::autoarray::AutoArray< ::libmaus2::aio::OutputStreamInstance::unique_ptr_type > tmpCOS(nodepacks.size());
					for ( uint64_t np = 0; np < nodepacks.size(); ++np )
					{
						nptempfilenames.push_back(tmpfilenamebase + "_np_" + ::libmaus2::util::NumberSerialisation::formatNumber(np,6));
						::libmaus2::util::TempFileRemovalContainer::addTempFile(nptempfilenames[np]);
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type ttmpCOSnp(
                                                                new ::libmaus2::aio::OutputStreamInstance(nptempfilenames[np])
                                                        );
						tmpCOS[np] = UNIQUE_PTR_MOVE(ttmpCOSnp);
					}
					
					::libmaus2::autoarray::AutoArray<uint64_t> nodebytesizes(numnodes);
					
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t np = 0; np < static_cast<int64_t>(nodepacks.size()); ++np )
					{
						uint64_t const nplow = nodepacks[np].first;
						uint64_t const nphigh = nodepacks[np].second;
						::libmaus2::aio::OutputStreamInstance & npout = *(tmpCOS[np]);
						
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::InputStreamInstance::unique_ptr_type > tmpCIS(numparts);
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > tmpSGI(numparts);
						
						for ( uint64_t i = 0; i < numparts; ++i )
						{
							::libmaus2::aio::InputStreamInstance::unique_ptr_type ttmpCISi(new ::libmaus2::aio::InputStreamInstance(tmpfilenames[i]));
							tmpCIS[i] = UNIQUE_PTR_MOVE(ttmpCISi);
							tmpCIS[i]->seekg(vnodewordcnt[i][nplow]*sizeof(uint64_t));
							::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type ttmpSGIi(new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(*tmpCIS[i],1024));
							tmpSGI[i] = UNIQUE_PTR_MOVE(ttmpSGIi);
						}

						for ( uint64_t npi = nplow; npi < nphigh; ++npi )
						{
							uint64_t const totalnodebits = vnodebits[npi];

							::libmaus2::rank::ImpCacheLineRank::WriteContextExternal context(npout,totalnodebits+1);
							
							for ( uint64_t p = 0; p < numparts; ++p )
							{
								uint64_t bitstowrite = vnodebitcnt[p][npi];
								uint64_t word = 0;
								int shift = -1;
								
								while ( bitstowrite )
								{
									if ( shift < 0 )
									{
										tmpSGI[p]->getNext(word);
										shift = 63;
									}
									
									bool const bit = (word >> shift) & 1;
									context.writeBit(bit);
									
									--bitstowrite;
									--shift;
								}
							}
							
							context.writeBit(0);
							context.flush();
							
							nodebytesizes[npi] = (2 /* header */+context.wordsWritten())*sizeof(uint64_t);
						}
						
						npout.flush();
						tmpCOS[np].reset();
					}

					for ( uint64_t i = 0; i < tmpfilenames.size(); ++i )
						remove ( tmpfilenames[i].c_str() );
						
					nodebytesizes.prefixSums();

					uint64_t outfilepos = 0;
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type Pfinalout(new ::libmaus2::aio::OutputStreamInstance(outputfilename));
					::libmaus2::aio::OutputStreamInstance & finalout = *Pfinalout;
					
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,symsperpart[numparts]);
					outfilepos += htree->serialize(finalout);
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					
					uint64_t const dictbasepos = outfilepos;
					for ( uint64_t i = 0; i < numnodes; ++i )
						nodebytesizes[i] += dictbasepos;
					
					for ( uint64_t i = 0; i < nptempfilenames.size(); ++i )
					{
						::libmaus2::aio::InputStreamInstance tmpCIS(nptempfilenames[i]);
						uint64_t const tmpfilesize = ::libmaus2::util::GetFileSize::getFileSize(tmpCIS);
						::libmaus2::util::GetFileSize::copy(tmpCIS,finalout,tmpfilesize);
						outfilepos += tmpfilesize;
						remove ( nptempfilenames[i].c_str() );
					}
					
					uint64_t const indexpos = outfilepos;	

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					for ( uint64_t i = 0; i < numnodes; ++i )
						outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,nodebytesizes[i]);

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,indexpos);
					
					finalout.flush();
					Pfinalout.reset();
					
					#if defined(HWTDEBUG)
					/**
					 * load tree and write out text
					 **/
					::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type PIHWT = UNIQUE_PTR_MOVE(
						::libmaus2::wavelet::ImpHuffmanWaveletTree::load(outputfilename)
					);
					::libmaus2::wavelet::ImpHuffmanWaveletTree const & IHWT = *PIHWT;
					assert ( IHWT.getN() == symsperpart[symsperpart.size()-1] );
					
					::libmaus2::aio::OutputStreamInstance debCOS(fn + ".debug");
					for ( uint64_t i = 0; i < IHWT.size(); ++i )
						::libmaus2::util::UTF8::encodeUTF8(IHWT[i],debCOS);
					debCOS.flush();
					debCOS.close();
					#endif
				}		
			}

			template<typename rl_decoder, bool radixsort>
			static void constructWaveletTreeFromRlWithTerm(
				std::string const & fn, std::string const & outputfilename,
				std::string const & tmpfilenamebase,
				::libmaus2::huffman::HuffmanTreeNode const * htree,
				uint64_t const termrank,
				uint64_t const bwtterm,
				uint64_t const tpartsizemax = 1024ull*1024ull,
				uint64_t const numthreads = ::libmaus2::parallel::OMPNumThreadsScope::getMaxThreads()
			)
			{
				libmaus2::parallel::OMPLock cerrlock;
				// ::libmaus2::parallel::OMPNumThreadsScope numthreadsscope(numthreads);
				::libmaus2::util::TempFileRemovalContainer::setup();

				::libmaus2::huffman::EncodeTable<1> ET(htree);

				// #define HWTDEBUG
				
				::libmaus2::timing::RealTimeClock rtc; rtc.start();	
				if ( ! htree->isLeaf() )
				{
					uint64_t const infs = rl_decoder::getLength(fn);
					
					uint64_t const pretermsyms = termrank;
					uint64_t const termsyms = 1;
					uint64_t const posttermsyms = infs-(pretermsyms+termsyms);
					
					uint64_t const tpartsize = std::min(static_cast<uint64_t>(tpartsizemax), (infs+numthreads-1)/numthreads);
					
					uint64_t const pretermparts = (pretermsyms + tpartsize - 1) / tpartsize;
					uint64_t const termparts = (termsyms + tpartsize - 1) / tpartsize;
					uint64_t const posttermparts = (posttermsyms + tpartsize -1) / tpartsize;
					
					uint64_t const numparts = pretermparts + termparts + posttermparts;
					
					::libmaus2::autoarray::AutoArray<uint64_t> symsperpart(numparts+1);
					#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
					std::cerr << "Allocated " << symsperpart.byteSize() << " bytes for symsperpart." << std::endl;
					#endif
					uint64_t jj = 0;
					for ( uint64_t i = 0; i < pretermparts; ++i )
					{					
						uint64_t const slow  = std::min(i * tpartsize,pretermsyms);
						uint64_t const shigh = std::min(slow+tpartsize,pretermsyms);
						symsperpart [ jj++ ] = shigh-slow;
					}
					symsperpart [ jj ++ ] = termsyms;
					for ( uint64_t i = 0; i < posttermparts; ++i )
					{					
						uint64_t const slow  = std::min(i * tpartsize,posttermsyms);
						uint64_t const shigh = std::min(slow+tpartsize,posttermsyms);
						symsperpart [ jj++ ] = shigh-slow;
					}
					
					symsperpart.prefixSums();
					
					assert ( symsperpart[symsperpart.size()-1] == infs );

					// std::string const tmpfilenamebase = fn+"_tmp";
					std::vector<std::string> tmpfilenames;
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						tmpfilenames.push_back(
							tmpfilenamebase + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(i,6)
						);
						::libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilenames[i]);
						// touch file
						::libmaus2::aio::OutputStreamInstance tmpCOS(tmpfilenames[i]);
					}
				
					uint64_t const numnodes = htree->numsyms()-1;
					#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
					std::cerr << "Num nodes " << numnodes << " numparts " << numparts << std::endl;
					#endif
					#if 0
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodebitcnt(numparts);
					::libmaus2::autoarray::AutoArray< ::libmaus2::autoarray::AutoArray<uint64_t> > vnodewordcnt(numparts+1);
					for ( uint64_t i = 0; i < numparts; ++i )
					{
						vnodebitcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes);
						vnodewordcnt[i] = ::libmaus2::autoarray::AutoArray<uint64_t>(numnodes+1);
					}
					#endif
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodebitcnt(numparts,numnodes);
					::libmaus2::autoarray::AutoArray2d<uint64_t> vnodewordcnt(numparts+1,numnodes+1);
					#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
					std::cerr << "Bytes for numnodes*numparts*sizeof(uint64_t)=" << numnodes*numparts*sizeof(uint64_t) << std::endl;
					#endif

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t partid = 0; partid < static_cast<int64_t>(numparts); ++partid )
					{
						#if 0
						::libmaus2::autoarray::AutoArray<uint64_t> & nodebitcnt = vnodebitcnt[partid];
						::libmaus2::autoarray::AutoArray<uint64_t> & nodewordcnt = vnodewordcnt[partid];
						#endif
						uint64_t * const nodebitcnt = vnodebitcnt[partid];
						uint64_t * const nodewordcnt = vnodewordcnt[partid];
						uint64_t lnodeid = 0;
						
						uint64_t const numsyms = symsperpart[partid+1]-symsperpart[partid];
						
						::libmaus2::util::CountPutObject CPO;
						if ( partid == static_cast<int64_t>(pretermparts) )
						{
							assert ( numsyms == 1 );
							::libmaus2::util::UTF8::encodeUTF8(bwtterm,CPO);	
						}
						else
						{
							typename rl_decoder::unique_ptr_type rldec(new rl_decoder(
								std::vector<std::string>(1,fn),
								symsperpart[partid]));
							for ( uint64_t i = 0; i < numsyms; ++i )
								::libmaus2::util::UTF8::encodeUTF8(rldec->decode(),CPO);
						}

						uint64_t const partsize = CPO.c;

						/* read text */
						::libmaus2::autoarray::AutoArray<uint8_t> A(partsize,false);
						
						#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
						#if defined(_OPENMP)
						unsigned int const tid = omp_get_thread_num();
						#else
						unsigned int const tid = 0;
						#endif
						#endif

						#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
						cerrlock.lock();
						std::cerr << "{" << tid << "}" << " Allocated " << A.byteSize() << " bytes for A array." << std::endl;
						cerrlock.unlock();
						#endif
						
						::libmaus2::util::PutObject<uint8_t *> PO(A.begin());

						if ( partid == static_cast<int64_t>(pretermparts) )
						{
							::libmaus2::util::UTF8::encodeUTF8(bwtterm,PO);						
						}
						else
						{
							typename rl_decoder::unique_ptr_type rldec(new rl_decoder(
								std::vector<std::string>(1,fn),
								symsperpart[partid]));

							for ( uint64_t i = 0; i < numsyms; ++i )
								::libmaus2::util::UTF8::encodeUTF8(rldec->decode(),PO);
						}
							
						uint64_t const pbleft = 0;
						uint64_t const pbright = partsize;
						uint64_t const lnumsyms = numsyms;
						
						::libmaus2::autoarray::AutoArray<uint8_t> Z;
						if ( radixsort )
						{
							Z = ::libmaus2::autoarray::AutoArray<uint8_t>(pbright-pbleft,false);
							#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
							cerrlock.lock();
							std::cerr << "{" << tid << "} Allocated " << Z.byteSize() << " bytes for Z array." << std::endl;
							cerrlock.unlock();
							#endif
						}

						std::stack<ImpWaveletStackElement> S;
						S.push(ImpWaveletStackElement(pbleft,pbright,0,lnumsyms,0,htree));
					
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type tmpCOS(new ::libmaus2::aio::OutputStreamInstance(tmpfilenames[partid]));
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tmpSGO(
							new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(*tmpCOS,8*1024));
					
						while ( ! S.empty() )
						{
							ImpWaveletStackElement const T = S.top();
							S.pop();
							
							assert ( ! T.hnode->isLeaf() );
						
							unsigned int const level = T.level;
							uint64_t const srange = T.sright-T.sleft;
							
							nodebitcnt[lnodeid] = srange;
							nodewordcnt[lnodeid] = (srange + 63)/64;
							lnodeid++;
						
							::libmaus2::util::GetObject<uint8_t const *> G(A.begin()+T.bleft);
							
							uint64_t const prewords = tmpSGO->getWrittenWords();
							uint64_t numsyms0 = 0;
							uint64_t numsyms1 = 0;
							unsigned int av = 64;
							uint64_t outword = 0;
							uint64_t codelen0 = 0;
							uint64_t codelen1 = 0;
							
							if ( radixsort )
							{
								::libmaus2::util::PutObject<uint8_t *> P0(Z.begin());
								::libmaus2::util::PutObjectReverse<uint8_t *> P1(Z.end());

								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										P1.write(G.p-codelen,codelen);
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										P0.write(G.p-codelen,codelen);
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}

								codelen0 = (P0.p-Z.begin());
								codelen1 = (Z.end()-P1.p);
							}
							else
							{
								for ( uint64_t i = 0; i < srange; ++i )
								{
									uint64_t codelen = 0;
									wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(G,codelen);
									bool const wbit = ET.getBitFromTop(sym,level);
								
									if ( wbit )
									{
										codelen1 += codelen;
										numsyms1 += 1;

										
										outword <<= 1;
										outword |= 1ull;
									}
									else
									{
										codelen0 += codelen;
										numsyms0 += 1;
										

										outword <<= 1;
									}
									
									if ( ! --av )
									{
										tmpSGO->put(outword);
										av = 64;
									}
								}
							}
							
							if ( av != 64 )
							{
								outword <<= av;
								tmpSGO->put(outword);
							}


							uint64_t const postwords = tmpSGO->getWrittenWords();
							uint64_t const wordswritten = postwords-prewords;
							assert ( wordswritten == nodewordcnt[lnodeid-1] );
							
							if ( radixsort )
							{
								std::copy(Z.begin(),Z.begin()+codelen0,A.begin()+T.bleft);
								uint8_t const * zp = Z.end();
								uint8_t * p1 = A.begin()+T.bleft+codelen0;
								uint8_t * const p1e = p1 + codelen1;
								while ( p1 != p1e )
									*(p1++) = *(--zp);
							}
							else
							{
								for ( uint64_t sorthalf = 1; sorthalf < srange; sorthalf *=2 )
								{
									uint64_t const sortfull = 2*sorthalf;
									uint64_t const sortparts = (srange + sortfull-1)/sortfull;
									uint64_t bleft = T.bleft;
													
									for ( uint64_t s = 0; s < sortparts; ++s )
									{
										uint64_t const lsortbase = s*sortfull;
										uint64_t const lsortrighta = std::min(lsortbase + sorthalf,srange);
										uint64_t const lsortrightb = std::min(lsortrighta + sorthalf,srange);
										
										uint64_t l_codelen0 = 0;
										uint64_t l_codelen1 = 0;
										uint64_t l_numsyms0 = 0;
										uint64_t l_numsyms1 = 0;
										uint64_t r_codelen0 = 0;
										uint64_t r_codelen1 = 0;
										uint64_t r_numsyms0 = 0;
										uint64_t r_numsyms1 = 0;
										
										::libmaus2::util::GetObject<uint8_t const *> LG(A.begin()+bleft);
										for ( uint64_t i = 0; i < (lsortrighta-lsortbase); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												l_codelen1 += codelen;
												l_numsyms1 += 1;
											}
											else
											{
												l_codelen0 += codelen;
												l_numsyms0 += 1;
											}
											
										}
										
										for ( uint64_t i = 0; i < (lsortrightb-lsortrighta); ++i )
										{
											uint64_t codelen = 0;
											wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(LG,codelen);
											bool const wbit = ET.getBitFromTop(sym,level);
										
											if ( wbit )
											{
												r_codelen1 += codelen;
												r_numsyms1 += 1;
											}
											else
											{
												r_codelen0 += codelen;
												r_numsyms0 += 1;
											}	
										}

										if ( l_numsyms1 && r_numsyms0 )
										{
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1              );
											std::reverse(A.begin() + bleft + l_codelen0 + l_codelen1, A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
											std::reverse(A.begin() + bleft + l_codelen0             , A.begin() + bleft + l_codelen0 + l_codelen1 + r_codelen0 );
										}
										
										bleft += (l_codelen0+l_codelen1+r_codelen0+r_codelen1);
									}
								}
							}
						
							::libmaus2::huffman::HuffmanTreeInnerNode const * node = dynamic_cast< ::libmaus2::huffman::HuffmanTreeInnerNode const * >(T.hnode);
							
							if ( (!(node->right->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft+codelen0,T.bright,T.sleft+numsyms0,T.sright,level+1,node->right));
							if ( (!(node->left->isLeaf())) )
								S.push(ImpWaveletStackElement(T.bleft,T.bleft+codelen0,T.sleft,T.sleft+numsyms0,level+1,node->left));

							#if defined(HWTDEBUG)
							::libmaus2::util::GetObject<uint8_t const *> DG(A.begin()+T.bleft);
							uint64_t d_codelen0 = 0;
							uint64_t d_codelen1 = 0;
							uint64_t d_numsyms0 = 0;
							uint64_t d_numsyms1 = 0;
							for ( uint64_t i = 0; i < srange; ++i )
							{
								uint64_t codelen = 0;
								wchar_t const sym = ::libmaus2::util::UTF8::decodeUTF8(DG,codelen);
								bool const wbit = ET.getBitFromTop(sym,level);
							
								if ( wbit )
								{
									if ( ! d_numsyms1 )
									{
										assert ( codelen0 == d_codelen0 );
										assert ( numsyms0 == d_numsyms0 );
									}
								
									d_codelen1 += codelen;
									d_numsyms1 += 1;
								}
								else
								{
									d_codelen0 += codelen;
									d_numsyms0 += 1;
								}
							}

							assert ( codelen0 == d_codelen0 );
							assert ( codelen1 == d_codelen1 );
							assert ( numsyms0 == d_numsyms0 );
							assert ( numsyms1 == d_numsyms1 );
							#endif			
						}

						// assert ( lnodeid == nodebitcnt.size() );
						assert ( lnodeid == numnodes );
						
						tmpSGO->flush();
						tmpSGO.reset();
						tmpCOS->flush();
						tmpCOS.reset();
					}

					// accumulate word offsets
					for ( uint64_t i = 0; i < numparts; ++i )
						//vnodewordcnt[i].prefixSums();
						vnodewordcnt.prefixSums(i);
					
					::libmaus2::autoarray::AutoArray<uint64_t> vnodebits(numnodes);
					#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
					std::cerr << "Allocated " << vnodebits.byteSize() << " bytes for vnodebits array." << std::endl;
					#endif
					uint64_t tnumbits = 0;
					for ( uint64_t nodeid = 0; nodeid < numnodes; ++nodeid )
					{
						uint64_t nodebits = 0;
						for ( uint64_t p = 0; p < numparts; ++p )
							nodebits += vnodebitcnt[p][nodeid];
						
						vnodebits[nodeid] = nodebits;
						tnumbits += nodebits;
					}
							
					uint64_t const tpartnodebits = (tnumbits+numthreads-1)/numthreads;
					std::vector < std::pair<uint64_t,uint64_t> > nodepacks;
					
					uint64_t llow = 0;
					while ( llow != numnodes )
					{
						uint64_t lhigh = llow;
						uint64_t s = 0;
						
						while ( lhigh != numnodes && s < tpartnodebits )
							s += vnodebits[lhigh++];

						nodepacks.push_back(std::pair<uint64_t,uint64_t>(llow,lhigh));

						llow = lhigh;
					}
					
					assert ( nodepacks.size() <= numthreads );

					std::vector<std::string> nptempfilenames;
					::libmaus2::autoarray::AutoArray< ::libmaus2::aio::OutputStreamInstance::unique_ptr_type > tmpCOS(nodepacks.size());
					for ( uint64_t np = 0; np < nodepacks.size(); ++np )
					{
						nptempfilenames.push_back(tmpfilenamebase + "_np_" + ::libmaus2::util::NumberSerialisation::formatNumber(np,6));
						::libmaus2::util::TempFileRemovalContainer::addTempFile(nptempfilenames[np]);
						::libmaus2::aio::OutputStreamInstance::unique_ptr_type tmpCOSnp(
                                                                new ::libmaus2::aio::OutputStreamInstance(nptempfilenames[np])
                                                        );
						tmpCOS[np] = UNIQUE_PTR_MOVE(tmpCOSnp);
					}
					
					::libmaus2::autoarray::AutoArray<uint64_t> nodebytesizes(numnodes);
					#if defined(LIBMAUS2_WAVELET_UTF8TOIMPHUFFMANWAVELETTREE_DEBUG)
					std::cerr << "Allocated " << nodebytesizes.byteSize() << " bytes per nodebytesizes array." << std::endl;
					#endif
					
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t np = 0; np < static_cast<int64_t>(nodepacks.size()); ++np )
					{
						uint64_t const nplow = nodepacks[np].first;
						uint64_t const nphigh = nodepacks[np].second;
						::libmaus2::aio::OutputStreamInstance & npout = *(tmpCOS[np]);
						
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::InputStreamInstance::unique_ptr_type > tmpCIS(numparts);
						::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > tmpSGI(numparts);
						
						for ( uint64_t i = 0; i < numparts; ++i )
						{
							::libmaus2::aio::InputStreamInstance::unique_ptr_type ttmpCISi(new ::libmaus2::aio::InputStreamInstance(tmpfilenames[i]));
							tmpCIS[i] = UNIQUE_PTR_MOVE(ttmpCISi);
							tmpCIS[i]->seekg(vnodewordcnt[i][nplow]*sizeof(uint64_t));
							::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type ttmpSGIi(new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(*tmpCIS[i],1024));
							tmpSGI[i] = UNIQUE_PTR_MOVE(ttmpSGIi);
						}

						for ( uint64_t npi = nplow; npi < nphigh; ++npi )
						{
							uint64_t const totalnodebits = vnodebits[npi];

							::libmaus2::rank::ImpCacheLineRank::WriteContextExternal context(npout,totalnodebits+1);
							
							for ( uint64_t p = 0; p < numparts; ++p )
							{
								uint64_t bitstowrite = vnodebitcnt[p][npi];
								uint64_t word = 0;
								int shift = -1;
								
								while ( bitstowrite )
								{
									if ( shift < 0 )
									{
										tmpSGI[p]->getNext(word);
										shift = 63;
									}
									
									bool const bit = (word >> shift) & 1;
									context.writeBit(bit);
									
									--bitstowrite;
									--shift;
								}
							}
							
							context.writeBit(0);
							context.flush();
							
							nodebytesizes[npi] = (2 /* header */+context.wordsWritten())*sizeof(uint64_t);
						}
						
						npout.flush();
						tmpCOS[np].reset();
					}

					for ( uint64_t i = 0; i < tmpfilenames.size(); ++i )
						remove ( tmpfilenames[i].c_str() );
						
					nodebytesizes.prefixSums();

					uint64_t outfilepos = 0;
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type Pfinalout(new ::libmaus2::aio::OutputStreamInstance(outputfilename));
					::libmaus2::aio::OutputStreamInstance & finalout = *Pfinalout;
					
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,symsperpart[numparts]);
					outfilepos += htree->serialize(finalout);
					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					
					uint64_t const dictbasepos = outfilepos;
					for ( uint64_t i = 0; i < numnodes; ++i )
						nodebytesizes[i] += dictbasepos;
					
					for ( uint64_t i = 0; i < nptempfilenames.size(); ++i )
					{
						::libmaus2::aio::InputStreamInstance tmpCIS(nptempfilenames[i]);
						uint64_t const tmpfilesize = ::libmaus2::util::GetFileSize::getFileSize(tmpCIS);
						::libmaus2::util::GetFileSize::copy(tmpCIS,finalout,tmpfilesize);
						outfilepos += tmpfilesize;
						remove ( nptempfilenames[i].c_str() );
					}
					
					uint64_t const indexpos = outfilepos;	

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,numnodes);
					for ( uint64_t i = 0; i < numnodes; ++i )
						outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,nodebytesizes[i]);

					outfilepos += ::libmaus2::util::NumberSerialisation::serialiseNumber(finalout,indexpos);
					
					finalout.flush();
					Pfinalout.reset();
					
					#if defined(HWTDEBUG)
					/**
					 * load tree and write out text
					 **/
					::libmaus2::wavelet::ImpHuffmanWaveletTree::unique_ptr_type PIHWT = UNIQUE_PTR_MOVE(
						::libmaus2::wavelet::ImpHuffmanWaveletTree::load(outputfilename)
					);
					::libmaus2::wavelet::ImpHuffmanWaveletTree const & IHWT = *PIHWT;
					assert ( IHWT.getN() == symsperpart[symsperpart.size()-1] );
					
					::libmaus2::aio::OutputStreamInstance debCOS(fn + ".debug");
					for ( uint64_t i = 0; i < IHWT.size(); ++i )
						::libmaus2::util::UTF8::encodeUTF8(IHWT[i],debCOS);
					debCOS.flush();
					debCOS.close();
					#endif
				}		
			}

		};
	}
}
#endif
