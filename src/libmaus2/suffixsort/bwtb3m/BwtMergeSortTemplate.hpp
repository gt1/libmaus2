/**
    libmaus2
    Copyright (C) 2009-2016 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_BWTMERGESORTTEMPLATE_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_BWTMERGESORTTEMPLATE_HPP

#include <libmaus2/suffixsort/bwtb3m/BWTB3MBase.hpp>
#include <libmaus2/lf/ImpCompactHuffmanWaveletLF.hpp>
#include <libmaus2/suffixsort/GapArrayByte.hpp>
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSortOptions.hpp>
#include <libmaus2/suffixsort/BwtMergeBlockSortResult.hpp>
#include <libmaus2/lf/DArray.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequest.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalSmallBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeExternalBlock.hpp>
#include <libmaus2/util/SimpleCountingHash.hpp>
#include <libmaus2/util/UnsignedCharVariant.hpp>
#include <libmaus2/suffixsort/GapMergePacket.hpp>
#include <libmaus2/bitio/BitVectorOutput.hpp>
#include <libmaus2/bitio/BitVectorInput.hpp>
#include <libmaus2/wavelet/RlToHwtTermRequest.hpp>
#include <libmaus2/gamma/SparseGammaGapMultiFileLevelSet.hpp>
#include <libmaus2/parallel/LockedBool.hpp>
#include <libmaus2/sorting/PairFileSorting.hpp>
#include <libmaus2/suffixsort/BwtMergeBlockSortRequestBase.hpp>
#include <libmaus2/suffixsort/BwtMergeTempFileNameSetVector.hpp>
#include <libmaus2/util/NumberMapSerialisation.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBaseBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyConstruction.hpp>
#include <libmaus2/suffixsort/bwtb3m/BaseBlockSorting.hpp>
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSortResult.hpp>
#include <libmaus2/aio/ArrayFile.hpp>
#include <libmaus2/math/ilog.hpp>
#include <libmaus2/util/PrefixSums.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			template<typename input_types_type>
			struct BwtMergeSortTemplate : public libmaus2::suffixsort::bwtb3m::BWTB3MBase
			{
				static ::std::map<int64_t,uint64_t> computeSymbolHistogram(std::string const & fn, uint64_t const numthreads)
				{
					typedef typename input_types_type::linear_wrapper stream_type;
					typedef typename input_types_type::base_input_stream::char_type char_type;
					typedef typename ::libmaus2::util::UnsignedCharVariant<char_type>::type unsigned_char_type;

					uint64_t const fs = stream_type::getFileSize(fn);

					uint64_t const symsperfrag = (fs + numthreads - 1)/numthreads;
					uint64_t const numfrags = (fs + symsperfrag - 1)/symsperfrag;

					libmaus2::util::HistogramSet HS(numfrags,256);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(numfrags); ++t )
					{
						uint64_t const low = t * symsperfrag;
						uint64_t const high = std::min(low+symsperfrag,fs);
						uint64_t const size = high-low;
						uint64_t todo = size;

						stream_type CIS(fn);
						CIS.seekg(low);

						::libmaus2::autoarray::AutoArray<unsigned_char_type> B(16*1024,false);
						libmaus2::util::Histogram & H = HS[t];

						while ( todo )
						{
							uint64_t const toread = std::min(todo,B.size());
							CIS.read(reinterpret_cast<char_type *>(B.begin()),toread);
							assert ( CIS.gcount() == static_cast<int64_t>(toread) );
							for ( uint64_t i = 0; i < toread; ++i )
								H (B[i]);
							todo -= toread;
						}
					}

					::libmaus2::util::Histogram::unique_ptr_type PH(HS.merge());
					::libmaus2::util::Histogram & H(*PH);

					return H.getByType<int64_t>();
				}

				static ::std::map<int64_t,uint64_t> computeSymbolHistogramPlusTerm(std::string const & fn, int64_t & term, uint64_t const numthreads)
				{
					::std::map<int64_t,uint64_t> M = computeSymbolHistogram(fn,numthreads);

					term = M.size() ? (M.rbegin()->first+1) : 0;
					M [ term ] ++;

					return M;
				}

				template<typename hef_type>
				static void appendBitVectorToFile(
					std::string const & fn,
					uint64_t const n,
					hef_type & HEF
				)
				{
					typedef ::libmaus2::huffman::BitInputBuffer4 sbis_type;
					::libmaus2::aio::InputStreamInstance::unique_ptr_type istr;
					sbis_type::raw_input_ptr_type ript;
					sbis_type::unique_ptr_type SBIS;

					::libmaus2::aio::InputStreamInstance::unique_ptr_type tistr(new ::libmaus2::aio::InputStreamInstance(fn));
					istr = UNIQUE_PTR_MOVE(tistr);
					sbis_type::raw_input_ptr_type tript(new sbis_type::raw_input_type(*istr));
					ript = UNIQUE_PTR_MOVE(tript);
					sbis_type::unique_ptr_type tSBIS(new sbis_type(ript,static_cast<uint64_t>(64*1024)));
					SBIS = UNIQUE_PTR_MOVE(tSBIS);

					uint64_t const fullbytes = n / 8;
					for ( uint64_t i = 0; i < fullbytes; ++i )
						HEF.write(SBIS->read(8),8);

					for ( uint64_t i = fullbytes*8; i < n; ++i )
					{
						HEF.writeBit(SBIS->readBit());
					}
				}

				static void checkBwtBlockDecode(
					std::pair<uint64_t,uint64_t> const & isai,
					std::pair<uint64_t,uint64_t> const & isapre,
					std::string const & fn,
					uint64_t const fs,
					::libmaus2::lf::ImpCompactHuffmanWaveletLF const & IHWT,
					::libmaus2::aio::SynchronousGenericOutput<uint64_t> & SGO,
					::libmaus2::aio::SynchronousGenericOutput<uint64_t> & ISGO,
					uint64_t const sasamplingrate,
					uint64_t const isasamplingrate,
					int64_t const ibs = -1
				)
				{
					assert ( ::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(sasamplingrate) == 1 );
					assert ( ::libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(isasamplingrate) == 1 );

					uint64_t r = isai.first;
					uint64_t p = isai.second;
					uint64_t const sasamplingmask = sasamplingrate-1;
					uint64_t const isasamplingmask = isasamplingrate-1;
					// size of block
					uint64_t const bs = ibs >= 0 ? ibs : ((p >  isapre.second) ? (p-isapre.second) : (fs - isapre.second));

					// (*logstr) << "bs=" << bs << std::endl;

					typename input_types_type::circular_reverse_wrapper CRWR(fn,p);

					if ( ! p )
					{
						for ( uint64_t j = 0; j < bs; ++j )
						{
							if ( !(r & sasamplingmask) )
							{
								SGO.put(r);
								SGO.put(p);
							}
							if ( !(p & isasamplingmask) )
							{
								ISGO.put(p);
								ISGO.put(r);
							}

							int64_t syma = CRWR.get();
							int64_t symb = IHWT[r];
							assert ( syma == symb );
							// (*logstr) << "(" << syma << "," << symb << ")";

							r = IHWT(r);

							if ( ! p )
								p = fs;
							--p;
						}
					}
					else
					{
						for ( uint64_t j = 0; j < bs; ++j )
						{
							if ( !(r & sasamplingmask) )
							{
								SGO.put(r);
								SGO.put(p);
							}
							if ( !(p & isasamplingmask) )
							{
								ISGO.put(p);
								ISGO.put(r);
							}

							int64_t syma = CRWR.get();
							int64_t symb = IHWT[r];
							assert ( syma == symb );
							// (*logstr) << "(" << syma << "," << symb << ")";

							r = IHWT(r);

							--p;
						}
					}

					assert ( r == isapre.first );
				}

				static uint64_t getRlLength(std::vector < std::vector < std::string > > const & bwtfilenames, uint64_t const numthreads)
				{
					libmaus2::parallel::PosixSpinLock PSL;
					uint64_t fs = 0;
					for ( uint64_t i = 0; i < bwtfilenames.size(); ++i )
					{
						uint64_t const lfs = rl_decoder::getLength(bwtfilenames[i],numthreads);
						libmaus2::parallel::ScopePosixSpinLock slock(PSL);
						fs += lfs;
					}
					return fs;
				}

				static std::vector< std::vector<std::string> > stringVectorPack(std::vector<std::string> const & Vin)
				{
					std::vector< std::vector<std::string> > Vout;
					for ( uint64_t i = 0; i < Vin.size(); ++i )
						Vout.push_back(std::vector<std::string>(1,Vin[i]));
					return Vout;
				}

				static std::vector<std::string> parallelGapFragMerge(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::vector < std::vector < std::string > > const & bwtfilenames,
					std::vector < std::vector < std::string > > const & gapfilenames,
					// std::string const & outputfilenameprefix,
					// std::string const & tempfilenameprefix,
					uint64_t const numthreads,
					uint64_t const lfblockmult,
					uint64_t const rlencoderblocksize,
					std::ostream * logstr,
					int const verbose
				)
				{
					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] entering parallelGapFragMerge for " << bwtfilenames.size() << " inputs" << std::endl;
					}

					std::vector<std::string> goutputfilenames;

					// no bwt input files, create empty bwt file
					if ( ! bwtfilenames.size() )
					{
						std::string const outputfilename = gtmpgen.getFileName() + ".bwt";

						rl_encoder rlenc(outputfilename,0 /* alphabet */,0,rlencoderblocksize);

						rlenc.flush();

						goutputfilenames = std::vector<std::string>(1,outputfilename);
					}
					// no gap files, rename input
					else if ( ! gapfilenames.size() )
					{
						assert ( bwtfilenames.size() == 1 );

						std::vector<std::string> outputfilenames;

						for ( uint64_t i = 0; i < bwtfilenames[0].size(); ++i )
						{
							std::ostringstream outputfilenamestr;
							outputfilenamestr << gtmpgen.getFileName() << '_'
								<< std::setw(4) << std::setfill('0') << i << std::setw(0)
								<< ".bwt";
							std::string const outputfilename = outputfilenamestr.str();

							// ::libmaus2::util::GetFileSize::copy(bwtfilenames[0][i],outputfilename);
							libmaus2::aio::OutputStreamFactoryContainer::rename ( bwtfilenames[0][i].c_str(), outputfilename.c_str() );

							outputfilenames.push_back(outputfilename);
						}

						goutputfilenames = outputfilenames;
					}
					// at least one gap file, merge
					else
					{
						#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_HUFGAP)
						typedef ::libmaus2::huffman::GapDecoder gapfile_decoder_type;
						#else
						typedef ::libmaus2::gamma::GammaGapDecoder gapfile_decoder_type;
						#endif

						unsigned int const albits = rl_decoder::haveAlphabetBits() ? rl_decoder::getAlBits(bwtfilenames.front()) : 0;

						uint64_t const firstblockgapfilesize = gapfilenames.size() ? gapfile_decoder_type::getLength(gapfilenames[0]) : 0;
						assert ( firstblockgapfilesize );

						// uint64_t const fs = rl_decoder::getLength(bwtfilenames);
						uint64_t const fs = getRlLength(bwtfilenames,numthreads);

						// first gap file meta information
						::libmaus2::huffman::IndexDecoderDataArray::unique_ptr_type Pfgapidda(new ::libmaus2::huffman::IndexDecoderDataArray(gapfilenames[0],numthreads));
						// first gap file
						gapfile_decoder_type::unique_ptr_type fgap(new gapfile_decoder_type(*Pfgapidda /* gapfilenames[0] */));
						// low marker
						uint64_t hlow = 0;
						// target g parts
						uint64_t const tgparts = numthreads*lfblockmult;
						// size of parts
						uint64_t const gpartsize = (fs + tgparts - 1) / tgparts;
						// maximum parts
						uint64_t const maxgparts = (fs + gpartsize - 1) / gpartsize;
						::libmaus2::autoarray::AutoArray< ::libmaus2::suffixsort::GapMergePacket> gmergepackets(maxgparts);
						// actual merge parts
						uint64_t actgparts = 0;
						uint64_t gs = 0;

						// (*logstr) << "fs=" << fs << " tgparts=" << tgparts << " gpartsize=" << gpartsize << std::endl;

						// compute number of suffixes per gblock
						while ( hlow != firstblockgapfilesize )
						{
							uint64_t const gsrest = (fs-gs);
							uint64_t const gskip = std::min(gsrest,gpartsize);
							libmaus2::huffman::KvInitResult kvinit;
							gapfile_decoder_type lgapdec(*Pfgapidda,gs + gskip, kvinit);

							// avoid small rest
							if ( fs - kvinit.kvoffset <= 1024 )
							{
								kvinit.kvoffset = fs;
								kvinit.koffset  = firstblockgapfilesize;
								// (*logstr) << "+++ end" << std::endl;
							}
							// we did not end up on a gap array element, go to the next one
							else if ( kvinit.kvtarget )
							{
								kvinit.koffset += 1;
								kvinit.voffset += kvinit.kvtarget + lgapdec.peek();
								kvinit.kvoffset += (1 + kvinit.kvtarget + lgapdec.peek());

								// there is no new suffix after the last gap element, so deduct 1 if we reached the end of the array
								if ( kvinit.koffset == firstblockgapfilesize )
									kvinit.kvoffset -= 1;

								// (*logstr) << "intermediate partial" << std::endl;
							}

							#if 1
							uint64_t const s = kvinit.kvoffset-gs;
							uint64_t const hhigh = kvinit.koffset;
							#else
							uint64_t hhigh = hlow;
							uint64_t s = 0;

							// sum up until we have reached at least gpartsize or the end of the file
							while ( hhigh != firstblockgapfilesize && s < gpartsize )
							{
								uint64_t const d = fgap->decode();
								s += (d + 1);
								hhigh += 1;
							}

							// last gap file value has no following suffix in block merged into
							if ( hhigh == firstblockgapfilesize )
								s -= 1;

							// if there is only one suffix left, then include it
							if ( hhigh+1 == firstblockgapfilesize && fgap->peek() == 0 )
							{
								fgap->decode();
								hhigh += 1;
							}

							// (*logstr) << "*** hhigh: " << hhigh << " kvinit.koffset: " << kvinit.koffset << " s=" << s << " kvinit.kvoffset-gs=" << kvinit.kvoffset-gs << std::endl;
							// kvinit.koffset, kvinit.kvoffset, rest: kvinit.kvtarget

							if ( actgparts >= maxgparts )
							{
								::libmaus2::exception::LibMausException se;
								se.getStream() << "acgtgparts=" << actgparts << " >= maxgparts=" << maxgparts << std::endl;
								se.finish();
								throw se;
								// assert ( actgparts < maxgparts );
							}
							#endif

							// save interval on first gap array and number of suffixes on this interval
							gmergepackets[actgparts++] = ::libmaus2::suffixsort::GapMergePacket(hlow,hhigh,s);

							// (*logstr) << "got packet " << gmergepackets[actgparts-1] << " firstblockgapfilesize=" << firstblockgapfilesize << std::endl;

							// add suffixes in this block to global count
							gs += s;

							// set new low marker
							hlow = hhigh;
						}

						// we should have seen all the suffixes
						assert ( gs == fs );

						#if 0
						(*logstr) << "actgparts=" << actgparts << std::endl;

						for ( uint64_t i = 0; i < actgparts; ++i )
							(*logstr) << gmergepackets[i] << std::endl;
						#endif

						// compute prefix sums over number of suffixes per gblock
						::libmaus2::autoarray::AutoArray<uint64_t> spref(actgparts+1,false);
						for ( uint64_t i = 0; i < actgparts; ++i )
							spref[i] = gmergepackets[i].s;
						spref.prefixSums();

						// compute prefix sums over number of suffixes per block used for each gpart
						::libmaus2::autoarray::AutoArray < uint64_t > bwtusedcntsacc( (actgparts+1)*(gapfilenames.size()+1), false );

						#if defined(_OPENMP)
						#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
						#endif
						for ( int64_t z = 0; z < static_cast<int64_t>(spref.size()); ++z )
						{
							#if 0
							(*logstr) << "proc first: " << gmergepackets[z] << ",spref=" << spref[z] << std::endl;
							#endif

							// offset in first gap array
							uint64_t lspref = spref[z];

							// array of decoders
							::libmaus2::autoarray::AutoArray < gapfile_decoder_type::unique_ptr_type > gapdecs(gapfilenames.size());

							for ( uint64_t j = 0; j < gapfilenames.size(); ++j )
							{
								::libmaus2::huffman::KvInitResult kvinitresult;
								gapfile_decoder_type::unique_ptr_type tgapdecsj(
									new gapfile_decoder_type(
										gapfilenames[j],
										lspref,kvinitresult,
										numthreads
									)
								);
								gapdecs[j] = UNIQUE_PTR_MOVE(tgapdecsj);

								// key offset for block z and file j
								bwtusedcntsacc [ j * (actgparts+1) + z ] = kvinitresult.koffset;

								#if 0
								(*logstr) << "lspref=" << lspref << "," << kvinitresult << std::endl;
								#endif

								// we should be on a key if j is the first file
								if ( j == 0 )
								{
									if ( kvinitresult.kvtarget != 0 )
									{
										if ( logstr )
											(*logstr) << "j=0 " << " z=" << z << " lspref=" << lspref <<
												" kvinitresult.koffset=" << kvinitresult.koffset <<
												" kvinitresult.voffset=" << kvinitresult.voffset <<
												" kvinitresult.kvoffset=" << kvinitresult.kvoffset <<
												" kvinitresult.kvtarget=" << kvinitresult.kvtarget << std::endl;
									}
									assert ( kvinitresult.kvtarget == 0 );
								}

								// offset for next gap file:
								// sum of values up to key lspref in this file + number of values used for next key
								lspref = kvinitresult.voffset + kvinitresult.kvtarget;

							}

							#if 0
							if ( logstr )
								(*logstr) << "lspref=" << lspref << std::endl;
							#endif

							// set end pointer
							bwtusedcntsacc [ gapfilenames.size() * (actgparts+1) + z ] = lspref;
						}

						// how many suffixes of each block do we use in each gpart?
						// (turn prefix sums in count per block)
						::libmaus2::autoarray::AutoArray < uint64_t > bwtusedcnts( (gapfilenames.size()+1) * actgparts, false );
						for ( uint64_t block = 0; block < gapfilenames.size()+1; ++block )
						{
							uint64_t const * lbwtusedcntsacc = bwtusedcntsacc.begin() + block*(actgparts+1);

							for ( uint64_t i = 0; i < actgparts; ++i )
								bwtusedcnts [ block * actgparts + i ] = lbwtusedcntsacc[i+1]-lbwtusedcntsacc[i];

							assert ( lbwtusedcntsacc [ actgparts ] == rl_decoder::getLength(bwtfilenames[block],numthreads) );
						}

						// vector of output file names
						std::vector < std::string > gpartfrags(actgparts);

						// now use the block counts computed above
						#if defined(_OPENMP)
						#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
						#endif
						for ( int64_t z = 0; z < static_cast<int64_t>(actgparts); ++z )
						{
							std::ostringstream ostr;
							ostr << gtmpgen.getFileName() << "_" << std::setw(4) << std::setfill('0') << z << std::setw(0) << ".bwt";
							std::string const gpartfrag = ostr.str();
							::libmaus2::util::TempFileRemovalContainer::addTempFile(gpartfrag);
							gpartfrags[z] = gpartfrag;

							#if 0
							if ( logstr )
								(*logstr) << gmergepackets[z] << ",spref=" << spref[z] << std::endl;
							#endif
							uint64_t lspref = spref[z];
							::libmaus2::autoarray::AutoArray < gapfile_decoder_type::unique_ptr_type > gapdecoders(gapfilenames.size());
							::libmaus2::autoarray::AutoArray< uint64_t > gapcur(gapfilenames.size());

							// set up gap file decoders at the proper offsets
							for ( uint64_t j = 0; j < gapfilenames.size(); ++j )
							{
								// sum up number of suffixes in later blocks for this gpart
								uint64_t suflat = 0;
								for ( uint64_t k = j+1; k < bwtfilenames.size(); ++k )
									suflat += bwtusedcnts [ k*actgparts + z ];

								::libmaus2::huffman::KvInitResult kvinitresult;
								gapfile_decoder_type::unique_ptr_type tgapdecodersj(
									new gapfile_decoder_type(
										gapfilenames[j],
										lspref,kvinitresult,
										numthreads
									)
								);
								gapdecoders[j] = UNIQUE_PTR_MOVE(tgapdecodersj);
								if ( suflat )
									gapcur[j] = gapdecoders[j]->decode();
								else
									gapcur[j] = 0;

								lspref = kvinitresult.voffset + kvinitresult.kvtarget;

								if ( j == 0 )
									assert ( kvinitresult.kvtarget == 0 );
							}

							::libmaus2::autoarray::AutoArray < uint64_t > bwttowrite(bwtfilenames.size(),false);
							::libmaus2::autoarray::AutoArray < rl_decoder::unique_ptr_type > bwtdecoders(bwtfilenames.size());

							for ( uint64_t j = 0; j < bwtfilenames.size(); ++j )
							{
								uint64_t const bwtoffset = bwtusedcntsacc [ j * (actgparts+1) + z ];
								bwttowrite[j] = bwtusedcnts [ j * actgparts + z ];

								rl_decoder::unique_ptr_type tbwtdecodersj(
									new rl_decoder(bwtfilenames[j],bwtoffset,numthreads)
								);
								bwtdecoders[j] = UNIQUE_PTR_MOVE(tbwtdecodersj);

								#if 0
								if ( logstr )
									(*logstr) << "block=" << j << " offset=" << bwtoffset << " bwttowrite=" << bwttowrite[j] << std::endl;
								#endif
							}

							uint64_t const totalbwt = std::accumulate(bwttowrite.begin(),bwttowrite.end(),0ull);

							rl_encoder bwtenc(gpartfrag,albits /* alphabet */,totalbwt,rlencoderblocksize);

							// start writing loop
							uint64_t written = 0;
							while ( written < totalbwt )
							{
								// determine file we next read/decode from
								// this is the leftmost one with gap value 0 and still data to write
								uint64_t writeindex = bwtdecoders.size()-1;
								for (
									uint64_t i = 0;
									i < gapcur.size();
									++i
								)
									if (
										(! gapcur[i])
										&&
										(bwttowrite[i])
									)
									{
										writeindex = i;
										break;
									}

								// sanity check
								if ( ! bwttowrite[writeindex] )
								{
									assert ( bwttowrite[writeindex] );
								}

								// adjust counters
								written++;
								bwttowrite[writeindex]--;
								// get next gap value if block is not the last one
								if ( bwttowrite[writeindex] && writeindex < gapcur.size() )
									gapcur[writeindex] = gapdecoders[writeindex]->decode();

								// copy symbol
								uint64_t const sym = bwtdecoders[writeindex]->decode();
								bwtenc.encode(sym);

								// adjust gap values of blocks to the left
								for ( uint64_t i = 0; i < writeindex; ++i )
									if ( bwttowrite[i] )
									{
										assert ( gapcur[i] > 0 );
										gapcur[i]--;
									}
							}
							//if ( logstr )
								// (*logstr) << "(1)";

							// all data should have been written now
							for ( uint64_t i = 0; i < bwttowrite.size(); ++i )
								assert ( !bwttowrite[i] );

							bwtenc.flush();
						}

						goutputfilenames = gpartfrags;
					}

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] checking length consistency" << std::endl;
					}

					assert (
						rl_decoder::getLength(goutputfilenames,numthreads) ==
						rl_decoder::getLength(bwtfilenames,numthreads)
					);

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] leaving parallelGapFragMerge for " << bwtfilenames.size() << " inputs" << std::endl;
					}

					return goutputfilenames;
				}

				template<typename gap_iterator>
				static uint64_t mergeIsa(
					std::vector<std::string> const & oldmergedisaname, // mergedisaname
					std::vector<std::string> const & newmergedisaname, // blockresults.files.sampledisa
					std::string const & mergedmergedisaname, // newmergedisaname
					uint64_t const blockstart, // blockstart
					gap_iterator & Gc,
					uint64_t const gn,
					std::ostream * logstr
				)
				{
					::libmaus2::timing::RealTimeClock rtc;

					// merge sampled inverse suffix arrays
					if ( logstr )
						(*logstr) << "[V] merging sampled inverse suffix arrays...";
					rtc.start();

					libmaus2::aio::ConcatInputStream ISIold(oldmergedisaname);
					libmaus2::aio::ConcatInputStream ISInew(newmergedisaname);

					::libmaus2::aio::SynchronousGenericInput<uint64_t> SGIISAold(ISIold,16*1024);
					::libmaus2::aio::SynchronousGenericInput<uint64_t> SGIISAnew(ISInew,16*1024);
					::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGOISA(mergedmergedisaname,16*1024);

					// sum over old (RHS block) suffixes
					uint64_t s = 0;
					// rank of position zero in merged block
					uint64_t blockp0rank = 0;

					// scan the gap array
					for ( uint64_t i = 0; i < gn; ++i )
					{
						s += Gc.get(); // *(Gc++);

						// while old suffixes are smaller than next new suffix
						int64_t re;
						while ( (re=SGIISAold.peek()) >= 0 && re < static_cast<int64_t>(s) )
						{
							// rank (add number of new suffixes processed before)
							uint64_t const r = SGIISAold.get() + i;
							// position (copy as is)
							uint64_t const p = SGIISAold.get();

							SGOISA . put ( r );
							SGOISA . put ( p );
						}

						// is next sample in next (LHS) block for rank i?
						if ( SGIISAnew.peek() == static_cast<int64_t>(i) )
						{
							// add number of old suffixes to rank
							uint64_t const r = SGIISAnew.get() + s;
							// keep absolute position as is
							uint64_t const p = SGIISAnew.get();

							SGOISA . put ( r );
							SGOISA . put ( p );

							// check whether this rank is for the leftmost position in the merged block
							if ( p == blockstart )
								blockp0rank = r;
						}
					}

					assert ( SGIISAnew.peek() < 0 );
					assert ( SGIISAold.peek() < 0 );

					SGOISA.flush();
					if ( logstr )
						(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

					return blockp0rank;
				}

				struct PreIsaAdapter
				{
					typedef PreIsaAdapter this_type;
					typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

					mutable libmaus2::aio::ConcatInputStream ISI;
					uint64_t const fs;
					uint64_t const n;

					typedef libmaus2::util::ConstIterator<this_type,std::pair<uint64_t,uint64_t> > const_iterator;

					PreIsaAdapter(std::vector<std::string> const & fn) : ISI(fn), fs(libmaus2::util::GetFileSize::getFileSize(ISI)), n(fs / (2*sizeof(uint64_t)))
					{
						assert ( (fs % (2*sizeof(uint64_t))) == 0 );
					}

					std::pair<uint64_t,uint64_t> get(uint64_t const i) const
					{
						ISI.clear();
						ISI.seekg(i*2*sizeof(uint64_t));
						libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(ISI,2);

						std::pair<uint64_t,uint64_t> P;
						bool ok = SGI.getNext(P.first);
						ok = ok && SGI.getNext(P.second);
						assert ( ok );
						return P;
					}

					std::pair<uint64_t,uint64_t> operator[](uint64_t const i) const
					{
						return get(i);
					}

					const_iterator begin() const
					{
						return const_iterator(this,0);
					}

					const_iterator end() const
					{
						return const_iterator(this,n);
					}
				};

				struct PairFirstComparator
				{
					bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
					{
						return A.first < B.first;
					}
				};

				static uint64_t getFileSize(std::vector<std::string> const & Vfn)
				{
					uint64_t s = 0;
					for ( uint64_t i = 0; i < Vfn.size(); ++i )
						s += ::libmaus2::util::GetFileSize::getFileSize(Vfn[i]);
					return s;
				}

				template<typename gap_array>
				static std::pair < uint64_t, std::vector<std::string> >
					mergeIsaParallel(
						libmaus2::util::TempFileNameGenerator & tmpgen,
						// work packages
						std::vector < std::pair<uint64_t,uint64_t> > const & wpacks,
						// prefix sums over G for low marks in wpacks
						std::vector < uint64_t > const & P,
						// old (RHS block pre isa)
						std::vector<std::string> const & oldmergedisaname,
						// new (LHS block pre isa)
						std::vector<std::string> const & newmergedisaname,
						// start of new (LHS) block in text
						uint64_t const blockstart,
						// the gap array
						gap_array & G,
						// number of threads
						uint64_t const numthreads,
						// log stream
						std::ostream * logstr
				)
				{
					::libmaus2::timing::RealTimeClock rtc;

					// merge sampled inverse suffix arrays
					if ( logstr )
						(*logstr) << "[V] merging sampled inverse suffix arrays in parallel...";
					rtc.start();

					std::vector<std::string> Vout(wpacks.size());
					for ( uint64_t i = 0; i < wpacks.size(); ++i )
					{
						std::ostringstream fnostr;
						fnostr << tmpgen.getFileName() << "_" << std::setw(6) << std::setfill('0') << i << std::setw(0) << ".preisa";
						Vout[i] = fnostr.str();
					}

					uint64_t volatile blockp0rank = std::numeric_limits<uint64_t>::max();
					libmaus2::parallel::PosixSpinLock blockp0ranklock;

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < wpacks.size(); ++t )
					{
						uint64_t const blow = wpacks[t].first;
						uint64_t const bhigh = wpacks[t].second;
						uint64_t const slow = P[t];

						typename PreIsaAdapter::unique_ptr_type PIA_LHS(new PreIsaAdapter(newmergedisaname));
						typename PreIsaAdapter::unique_ptr_type PIA_RHS(new PreIsaAdapter(oldmergedisaname));

						typename PreIsaAdapter::const_iterator it_LHS = ::std::lower_bound(PIA_LHS->begin(),PIA_LHS->end(),std::pair<uint64_t,uint64_t>(blow,0),PairFirstComparator());
						assert ( it_LHS == PIA_LHS->end() || it_LHS[0].first >= blow );
						assert ( it_LHS == PIA_LHS->begin() || it_LHS[-1].first < blow );
						uint64_t const off_LHS = it_LHS - PIA_LHS->begin();
						PIA_LHS.reset();

						typename PreIsaAdapter::const_iterator it_RHS = ::std::lower_bound(PIA_RHS->begin(),PIA_RHS->end(),std::pair<uint64_t,uint64_t>(slow,0),PairFirstComparator());
						assert ( it_RHS == PIA_RHS->end() || it_RHS[0].first >= slow );
						assert ( it_RHS == PIA_RHS->begin() || it_RHS[-1].first < slow );
						uint64_t const off_RHS = it_RHS - PIA_RHS->begin();
						PIA_RHS.reset();

						// LHS
						libmaus2::aio::ConcatInputStream ISInew(newmergedisaname);
						ISInew.clear();
						ISInew.seekg(off_LHS * 2 * sizeof(uint64_t));

						// RHS
						libmaus2::aio::ConcatInputStream ISIold(oldmergedisaname);
						ISIold.clear();
						ISIold.seekg(off_RHS * 2 * sizeof(uint64_t));

						::libmaus2::aio::SynchronousGenericInput<uint64_t> SGIISAold(ISIold,16*1024);
						::libmaus2::aio::SynchronousGenericInput<uint64_t> SGIISAnew(ISInew,16*1024);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGOISA(Vout[t],16*1024);

						uint64_t lblockp0rank = std::numeric_limits<uint64_t>::max();

						typedef typename gap_array::sequence_type sequence_type;
						sequence_type gp = G.getOffsetSequence(blow);
						uint64_t s = slow;
						// scan the gap array
						for ( uint64_t i = blow; i < bhigh; ++i )
						{
							s += gp.get(); // *(Gc++);

							// while old suffixes are smaller than next new suffix
							int64_t re;
							while ( (re=SGIISAold.peek()) >= 0 && re < static_cast<int64_t>(s) )
							{
								// rank (add number of new suffixes processed before)
								uint64_t const r = SGIISAold.get() + i;
								// position (copy as is)
								uint64_t const p = SGIISAold.get();

								SGOISA . put ( r );
								SGOISA . put ( p );
							}

							// is next sample in next (LHS) block for rank i?
							if ( SGIISAnew.peek() == static_cast<int64_t>(i) )
							{
								// add number of old suffixes to rank
								uint64_t const r = SGIISAnew.get() + s;
								// keep absolute position as is
								uint64_t const p = SGIISAnew.get();

								SGOISA . put ( r );
								SGOISA . put ( p );

								// check whether this rank is for the leftmost position in the merged block
								if ( p == blockstart )
									blockp0rank = r;
							}
						}

						SGOISA.flush();

						if ( lblockp0rank != std::numeric_limits<uint64_t>::max() )
						{
							blockp0ranklock.lock();
							blockp0rank = lblockp0rank;
							blockp0ranklock.unlock();
						}
					}

					// sanity check, input length should equal output length
					assert (
						getFileSize(oldmergedisaname) + getFileSize(newmergedisaname) ==
						getFileSize(Vout)
					);

					if ( logstr )
						(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

					return std::pair < uint64_t, std::vector<std::string> >(static_cast<uint64_t>(blockp0rank),Vout);
				}

				static void saveGapFile(
					::libmaus2::autoarray::AutoArray<uint32_t> const & G,
					std::string const & gapfile,
					std::ostream * logstr
				)
				{
					::libmaus2::timing::RealTimeClock rtc;

					if ( logstr )
						(*logstr) << "[V] saving gap file...";
					rtc.start();
					#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_HUFGAP)
					::libmaus2::util::Histogram gaphist;
					for ( uint64_t i = 0; i < G.size(); ++i )
						gaphist ( G[i] );
					::libmaus2::huffman::GapEncoder GE(gapfile,gaphist,G.size());
					GE.encode(G.begin(),G.end());
					GE.flush();
					#else
					::libmaus2::gamma::GammaGapEncoder GE(gapfile);
					GE.encode(G.begin(),G.end());
					#endif
					if ( logstr )
						(*logstr) << "done in time " << rtc.getElapsedSeconds() << std::endl;
				}

				static void concatenateGT(
					std::vector<std::string> const & gtpartnames,
					std::string const & newgtpart, // blockresults.files.gt
					std::string const & newmergedgtname,
					std::ostream * logstr
				)
				{
					::libmaus2::timing::RealTimeClock rtc;
					rtc.start();

					std::vector< std::string > allfiles(gtpartnames.begin(),gtpartnames.end());
					allfiles.push_back(newgtpart);

					#if 0
					// encoder for new gt stream
					libmaus2::bitio::BitVectorOutput GTHEFref(newmergedgtname);
					libmaus2::bitio::BitVectorInput BVI(allfiles);
					uint64_t const tn = libmaus2::bitio::BitVectorInput::getLength(allfiles);
					for ( uint64_t i = 0; i < tn; ++i )
						GTHEFref.writeBit(BVI.readBit());
					GTHEFref.flush();
					#endif

					libmaus2::bitio::BitVectorOutput GTHEF(newmergedgtname);

					unsigned int prevbits = 0;
					uint64_t prev = 0;

					// append part streams
					for ( uint64_t z = 0; z < allfiles.size(); ++z )
					{
						uint64_t const n = libmaus2::bitio::BitVectorInput::getLength(allfiles[z]);
						uint64_t const fullwords = n / 64;
						uint64_t const restbits = n-fullwords*64;
						libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(allfiles[z],8192);
						uint64_t v = 0;

						if ( !prevbits )
						{

							// copy full words
							for ( uint64_t i = 0; i < fullwords; ++i )
							{
								bool const ok = SGI.getNext(v);
								assert ( ok );
								GTHEF.SGO.put(v);
							}


							// get next word
							if ( restbits )
							{
								SGI.getNext(prev);
								// move bits to top of word
								prev <<= (64-restbits);
								// number of bits left
								prevbits = restbits;
							}
							else
							{
								prevbits = 0;
								prev = 0;
							}
						}
						else
						{
							// process full words
							for ( uint64_t i = 0; i < fullwords; ++i )
							{
								bool const ok = SGI.getNext(v);
								assert ( ok );

								GTHEF.SGO.put(prev | (v >> prevbits));
								prev = v << (64-prevbits);
							}

							// if there are bits left in this stream
							if ( restbits )
							{
								// get next word
								SGI.getNext(v);
								// move to top
								v <<= (64 - restbits);

								// rest with previous rest fill more than a word
								if ( restbits + prevbits >= 64 )
								{
									GTHEF.SGO.put(prev | (v >> prevbits));
									prev = v << (64-prevbits);
									prevbits = restbits + prevbits - 64;
								}
								else
								{
									prev = prev | (v >> prevbits);
									prevbits = prevbits + restbits;
								}
							}
							// leave as is
							else
							{

							}
						}

						libmaus2::aio::FileRemoval::removeFile(gtpartnames[z].c_str());
					}

					for ( uint64_t i = 0; i < prevbits; ++i )
						GTHEF.writeBit((prev >> (63-i)) & 1);

					// flush gt stream
					GTHEF.flush();

					if ( logstr )
						(*logstr) << "[V] concatenated bit vectors in time " << rtc.getElapsedSeconds() << std::endl;
				}

				struct GapArrayComputationResult
				{
					::libmaus2::autoarray::AutoArray<uint32_t> G;
					std::vector < std::string > gtpartnames;
					uint64_t zactive;
					::libmaus2::autoarray::AutoArray<uint64_t> zabsblockpos;

					GapArrayComputationResult()
					: zactive(0)
					{

					}

					GapArrayComputationResult(
						::libmaus2::autoarray::AutoArray<uint32_t> & rG,
						std::vector < std::string > const & rgtpartnames,
						uint64_t const rzactive,
						::libmaus2::autoarray::AutoArray<uint64_t> & rzabsblockpos
					)
					: G(rG), gtpartnames(rgtpartnames), zactive(rzactive), zabsblockpos(rzabsblockpos) {}
				};

				struct GapArrayByteComputationResult
				{
					libmaus2::suffixsort::GapArrayByte::shared_ptr_type G;
					std::vector < std::string > gtpartnames;
					uint64_t zactive;
					::libmaus2::autoarray::AutoArray<uint64_t> zabsblockpos;

					GapArrayByteComputationResult()
					: zactive(0)
					{

					}

					GapArrayByteComputationResult(
						libmaus2::suffixsort::GapArrayByte::shared_ptr_type rG,
						std::vector < std::string > const & rgtpartnames,
						uint64_t const rzactive,
						::libmaus2::autoarray::AutoArray<uint64_t> & rzabsblockpos
					)
					: G(rG), gtpartnames(rgtpartnames), zactive(rzactive), zabsblockpos(rzabsblockpos) {}
				};

				struct SparseGapArrayComputationResult
				{
					// name of gap file
					std::vector<std::string> fn;
					std::vector < std::string > gtpartnames;
					uint64_t zactive;
					::libmaus2::autoarray::AutoArray<uint64_t> zabsblockpos;

					SparseGapArrayComputationResult()
					: zactive(0)
					{

					}

					SparseGapArrayComputationResult(
						std::vector < std::string > const & rfn,
						std::vector < std::string > const & rgtpartnames,
						uint64_t const rzactive,
						::libmaus2::autoarray::AutoArray<uint64_t> & rzabsblockpos
					)
					: fn(rfn), gtpartnames(rgtpartnames), zactive(rzactive), zabsblockpos(rzabsblockpos) {}
				};

				static libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ensureWaveletTreeGenerated(
					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults,
					std::ostream * logstr
				)
				{
					// generate wavelet tree from request if necessary
					if ( ! libmaus2::util::GetFileSize::fileExists(blockresults.getFiles().getHWT()) )
					{
						libmaus2::timing::RealTimeClock rtc; rtc.start();
						if ( logstr )
							(*logstr) << "[V] Generating HWT for gap file computation...";
						assert ( libmaus2::util::GetFileSize::fileExists(blockresults.getFiles().getHWTReq() ) );
						libmaus2::wavelet::RlToHwtTermRequest::unique_ptr_type ureq(libmaus2::wavelet::RlToHwtTermRequest::load(blockresults.getFiles().getHWTReq()));
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type	ptr(ureq->dispatch<rl_decoder>());
						libmaus2::aio::FileRemoval::removeFile ( blockresults.getFiles().getHWTReq().c_str() );
						if ( logstr )
							(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;
						return UNIQUE_PTR_MOVE(ptr);
					}
					else
					{
						libmaus2::aio::InputStreamInstance CIS(blockresults.getFiles().getHWT());
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ptr(new libmaus2::wavelet::ImpCompactHuffmanWaveletTree(CIS));
						return UNIQUE_PTR_MOVE(ptr);
					}
				}

				static GapArrayComputationResult computeGapArray(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn, // name of text file
					uint64_t const fs, // length of text file in symbols
					uint64_t const blockstart, // start offset
					uint64_t const cblocksize, // block size
					uint64_t const nextblockstart, // start of next block (mod fs)
					uint64_t const mergeprocrightend, // right end of merged area
					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults, // information on block
					std::vector<std::string> const & mergedgtname, // previous gt file name
					// std::string const & newmergedgtname, // new gt file name
					::libmaus2::lf::DArray * const accD, // accumulated symbol freqs for block
					std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > const & zblocks, // lf starting points
					uint64_t const numthreads,
					std::ostream * logstr,
					int const verbose
				)
				{
					// gap array
					::libmaus2::autoarray::AutoArray<uint32_t> G(cblocksize+1,false);
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					for ( uint64_t i = 0; i < G.size(); ++i )
						G[i] = 0;
					#endif

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] loading histogram" << std::endl;
					}

					// set up lf mapping
					::libmaus2::lf::DArray D(static_cast<std::string const &>(blockresults.getFiles().getHist()));

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] loading histogram done" << std::endl;
					}

					accD->merge(D);
					#if 0
					bool const hwtdelayed = ensureWaveletTreeGenerated(blockresults);
					#endif
					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] loading HWT" << std::endl;
					}

					::libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ICHWL(ensureWaveletTreeGenerated(blockresults,logstr));

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] loading HWT done" << std::endl;
					}

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] setting up LF" << std::endl;
					}

					::libmaus2::lf::ImpCompactHuffmanWaveletLF IHWL(ICHWL);

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] setting up LF done" << std::endl;
					}

					IHWL.D = D.D;
					assert ( cblocksize == IHWL.n );

					// rank of position 0 in this block (for computing new gt array/stream)
					uint64_t const lp0 = blockresults.getBlockP0Rank();

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] calling getSymbolAtPosition for last symbol of first/left block" << std::endl;
					}

					// last symbol in this block
					int64_t const firstblocklast = input_types_type::linear_wrapper::getSymbolAtPosition(fn,(nextblockstart+fs-1)%fs);

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] calling getSymbolAtPosition for last symbol of first/left block done" << std::endl;
					}

					/**
					 * array of absolute positions
					 **/
					uint64_t const zactive = zblocks.size();
					::libmaus2::autoarray::AutoArray<uint64_t> zabsblockpos(zactive+1,false);
					for ( uint64_t z = 0; z < zactive; ++z )
						zabsblockpos[z] = zblocks[z].getZAbsPos();
					zabsblockpos [ zactive ] = blockstart + cblocksize;

					std::vector < std::string > gtpartnames(zactive);

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] entering gap array loop" << std::endl;
					}

					::libmaus2::timing::RealTimeClock rtc;
					rtc.start();
					#if defined(_OPENMP) && defined(LIBMAUS2_HAVE_SYNC_OPS)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( int64_t z = 0; z < static_cast<int64_t>(zactive); ++z )
					{
						::libmaus2::timing::RealTimeClock subsubrtc; subsubrtc.start();

						::libmaus2::suffixsort::BwtMergeZBlock const & zblock = zblocks[z];

						std::string const gtpartname = gtmpgen.getFileName() + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(z,4) + ".gt";
						::libmaus2::util::TempFileRemovalContainer::addTempFile(gtpartname);
						gtpartnames[z] = gtpartname;
						#if 0
						::libmaus2::huffman::HuffmanEncoderFileStd GTHEF(gtpartname);
						#endif
						libmaus2::bitio::BitVectorOutput GTHEF(gtpartname);

						#if 0
						::libmaus2::bitio::BitStreamFileDecoder gtfile(mergedgtname, (mergeprocrightend - zblock.getZAbsPos()) );
						#endif
						libmaus2::bitio::BitVectorInput gtfile(mergedgtname, (mergeprocrightend - zblock.getZAbsPos()) );

						typename input_types_type::circular_reverse_wrapper CRWR(fn,zblock.getZAbsPos() % fs);
						uint64_t r = zblock.getZRank();

						uint64_t const zlen = zabsblockpos [ z ] - zabsblockpos [z+1];

						for ( uint64_t i = 0; i < zlen; ++i )
						{
							GTHEF.writeBit(r > lp0);

							int64_t const sym = CRWR.get();
							bool const gtf = gtfile.readBit();

							r = IHWL.step(sym,r) + ((sym == firstblocklast)?gtf:0);

							#if defined(LIBMAUS2_HAVE_SYNC_OPS)
							__sync_fetch_and_add(G.begin()+r,1);
							#else
							G[r]++;
							#endif
						}

						GTHEF.flush();
					}
					if ( logstr )
						(*logstr) << "[V] computed gap array in time " << rtc.getElapsedSeconds() << std::endl;

					uint64_t const cperblock = (G.size() + numthreads - 1)/numthreads;
					uint64_t const cblocks = (G.size() + cperblock - 1)/cperblock;
					uint64_t volatile gs = 0;
					libmaus2::parallel::PosixSpinLock gslock;

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( uint64_t t = 0; t < cblocks; ++t )
					{
						uint64_t const low = t * cperblock;
						uint64_t const high = std::min(low+cperblock,G.size());
						uint64_t s = 0;
						uint32_t const * g = G.begin() + low;
						uint32_t const * const ge = G.begin() + high;
						while ( g != ge )
							s += *(g++);
						libmaus2::parallel::ScopePosixSpinLock slock(gslock);
						gs += s;
					}

					uint64_t es = 0;
					for ( int64_t z = 0; z < static_cast<int64_t>(zactive); ++z )
					{
						uint64_t const zlen = zabsblockpos [ z ] - zabsblockpos [z+1];
						es += zlen;
					}

					if ( logstr )
					{
						(*logstr) << "[V] gs=" << gs << " es=" << es << std::endl;
					}

					assert ( es == gs );

					return GapArrayComputationResult(G,gtpartnames,zactive,zabsblockpos);
				}

				static GapArrayComputationResult computeGapArray(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn, // name of text file
					uint64_t const fs, // length of text file in symbols
					libmaus2::suffixsort::bwtb3m::MergeStrategyMergeGapRequest const & msmgr, // merge request
					std::vector<std::string> const & mergedgtname, // previous gt file name
					//std::string const & newmergedgtname, // new gt file name
					::libmaus2::lf::DArray * const accD, // accumulated symbol freqs for block
					uint64_t const numthreads,
					std::ostream * logstr,
					int const verbose
				)
				{
					uint64_t const into = msmgr.into;

					std::vector<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type> const & children =
						*(msmgr.pchildren);

					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults = children[into]->sortresult;

					uint64_t const blockstart = blockresults.getBlockStart();
					uint64_t const cblocksize = blockresults.getCBlockSize();
					uint64_t const nextblockstart = (blockstart+cblocksize)%fs;
					uint64_t const mergeprocrightend =
						children.at(children.size()-1)->sortresult.getBlockStart() +
						children.at(children.size()-1)->sortresult.getCBlockSize();
					// use gap object's zblocks vector
					std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > const & zblocks = msmgr.zblocks;

					return computeGapArray(gtmpgen,fn,fs,blockstart,cblocksize,nextblockstart,mergeprocrightend,
						blockresults,mergedgtname,accD,zblocks,numthreads,logstr,verbose);
				}

				static GapArrayByteComputationResult computeGapArrayByte(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn, // name of text file
					uint64_t const fs, // length of text file in symbols
					uint64_t const blockstart, // start offset
					uint64_t const cblocksize, // block size
					uint64_t const nextblockstart, // start of next block (mod fs)
					uint64_t const mergeprocrightend, // right end of merged area
					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults, // information on block
					std::vector<std::string> const & mergedgtname, // previous gt file name
					#if 0
					std::string const & newmergedgtname, // new gt file name
					std::string const & gapoverflowtmpfilename, // gap overflow tmp file
					#endif
					::libmaus2::lf::DArray * const accD, // accumulated symbol freqs for block
					std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > const & zblocks, // lf starting points
					uint64_t const numthreads,
					std::ostream * logstr
				)
				{
					std::string const gapoverflowtmpfilename = gtmpgen.getFileName() + "_gapoverflow";

					// gap array
					::libmaus2::suffixsort::GapArrayByte::shared_ptr_type pG(
						new ::libmaus2::suffixsort::GapArrayByte(
							cblocksize+1,
							512, /* number of overflow words per thread */
							numthreads,
							gapoverflowtmpfilename
						)
					);
					::libmaus2::suffixsort::GapArrayByte & G = *pG;

					// set up lf mapping
					::libmaus2::lf::DArray D(static_cast<std::string const &>(blockresults.getFiles().getHist()));
					accD->merge(D);
					#if 0
					bool const hwtdelayed = ensureWaveletTreeGenerated(blockresults);
					#endif
					::libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ICHWL(ensureWaveletTreeGenerated(blockresults,logstr));
					::libmaus2::lf::ImpCompactHuffmanWaveletLF IHWL(ICHWL);
					IHWL.D = D.D;
					assert ( cblocksize == IHWL.n );

					// rank of position 0 in this block (for computing new gt array/stream)
					uint64_t const lp0 = blockresults.getBlockP0Rank();

					// last symbol in this block
					int64_t const firstblocklast = input_types_type::linear_wrapper::getSymbolAtPosition(fn,(nextblockstart+fs-1)%fs);

					/**
					 * array of absolute positions
					 **/
					uint64_t const zactive = zblocks.size();
					::libmaus2::autoarray::AutoArray<uint64_t> zabsblockpos(zactive+1,false);
					for ( uint64_t z = 0; z < zactive; ++z )
						zabsblockpos[z] = zblocks[z].getZAbsPos();
					zabsblockpos [ zactive ] = blockstart + cblocksize;

					std::vector < std::string > gtpartnames(zactive);

					::libmaus2::timing::RealTimeClock rtc;
					rtc.start();
					#if defined(_OPENMP) && defined(LIBMAUS2_HAVE_SYNC_OPS)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( int64_t z = 0; z < static_cast<int64_t>(zactive); ++z )
					{
						::libmaus2::timing::RealTimeClock subsubrtc; subsubrtc.start();

						::libmaus2::suffixsort::BwtMergeZBlock const & zblock = zblocks[z];

						std::string const gtpartname = gtmpgen.getFileName() + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(z,4) + ".gt";
						::libmaus2::util::TempFileRemovalContainer::addTempFile(gtpartname);
						gtpartnames[z] = gtpartname;
						#if 0
						::libmaus2::huffman::HuffmanEncoderFileStd GTHEF(gtpartname);
						#endif
						libmaus2::bitio::BitVectorOutput GTHEF(gtpartname);

						#if 0
						::libmaus2::bitio::BitStreamFileDecoder gtfile(mergedgtname, (mergeprocrightend - zblock.getZAbsPos()) );
						#endif
						libmaus2::bitio::BitVectorInput gtfile(mergedgtname, (mergeprocrightend - zblock.getZAbsPos()) );

						typename input_types_type::circular_reverse_wrapper CRWR(fn,zblock.getZAbsPos() % fs);
						uint64_t r = zblock.getZRank();

						uint64_t const zlen = zabsblockpos [ z ] - zabsblockpos [z+1];

						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif

						for ( uint64_t i = 0; i < zlen; ++i )
						{
							GTHEF.writeBit(r > lp0);

							int64_t const sym = CRWR.get();
							bool const gtf = gtfile.readBit();

							r = IHWL.step(sym,r) + ((sym == firstblocklast)?gtf:0);

							if ( G(r) )
								G(r,tid);
						}

						GTHEF.flush();
					}
					if ( logstr )
						(*logstr) << "[V] computed gap array in time " << rtc.getElapsedSeconds() << std::endl;

					G.flush();

					return GapArrayByteComputationResult(pG,gtpartnames,zactive,zabsblockpos);
				}

				static GapArrayByteComputationResult computeGapArrayByte(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn, // name of text file
					uint64_t const fs, // length of text file in symbols
					libmaus2::suffixsort::bwtb3m::MergeStrategyMergeGapRequest const & msmgr, // merge request
					std::vector<std::string> const & mergedgtname, // previous gt file name
					#if 0
					std::string const & newmergedgtname, // new gt file name
					std::string const & gapoverflowtmpfilename, // gap overflow tmp file
					#endif
					::libmaus2::lf::DArray * const accD, // accumulated symbol freqs for block
					uint64_t const numthreads,
					std::ostream * logstr
				)
				{
					uint64_t const into = msmgr.into;

					std::vector<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type> const & children =
						*(msmgr.pchildren);

					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults = children[into]->sortresult;

					uint64_t const blockstart = blockresults.getBlockStart();
					uint64_t const cblocksize = blockresults.getCBlockSize();
					uint64_t const nextblockstart = (blockstart+cblocksize)%fs;
					uint64_t const mergeprocrightend =
						children.at(children.size()-1)->sortresult.getBlockStart() +
						children.at(children.size()-1)->sortresult.getCBlockSize();
					// use gap object's zblocks vector
					std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > const & zblocks = msmgr.zblocks;

					return computeGapArrayByte(gtmpgen,fn,fs,blockstart,cblocksize,nextblockstart,mergeprocrightend,
						blockresults,mergedgtname/*,newmergedgtname,gapoverflowtmpfilename*/,accD,zblocks,numthreads,logstr);
				}

				struct ZNext
				{
					uint64_t znext;
					uint64_t znextcount;
					libmaus2::parallel::OMPLock lock;

					ZNext(uint64_t const rznextcount) : znext(0), znextcount(rznextcount) {}

					bool getNext(uint64_t & next)
					{
						libmaus2::parallel::ScopeLock slock(lock);

						if ( znext == znextcount )
							return false;
						else
						{
							next = znext++;
							return true;
						}
					}
				};

				static SparseGapArrayComputationResult computeSparseGapArray(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn,
					uint64_t const fs,
					uint64_t const blockstart,
					uint64_t const cblocksize,
					uint64_t const nextblockstart,
					uint64_t const mergeprocrightend,
					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults,
					std::vector<std::string> const & mergedgtname,
					// std::string const & newmergedgtname,
					::libmaus2::lf::DArray * const accD,
					//
					// std::string const & outputgapfilename,
					std::string const & tmpfileprefix,
					uint64_t const maxmem,
					std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > const & zblocks,
					uint64_t const numthreads,
					std::ostream * logstr
				)
				{
					uint64_t const memperthread = (maxmem + numthreads-1)/numthreads;
					uint64_t const wordsperthread = ( memperthread + sizeof(uint64_t) - 1 ) / sizeof(uint64_t);
					// uint64_t const wordsperthread = 600;
					uint64_t const parcheck = 64*1024;

					libmaus2::autoarray::AutoArray< libmaus2::autoarray::AutoArray<uint64_t> > GG(numthreads,false);
					for ( uint64_t i = 0; i < numthreads; ++i )
						GG[i] = libmaus2::autoarray::AutoArray<uint64_t>(wordsperthread,false);

					libmaus2::util::TempFileNameGenerator tmpgen(tmpfileprefix,3);
					libmaus2::gamma::SparseGammaGapMultiFileLevelSet SGGFS(tmpgen,numthreads);

					// set up lf mapping
					::libmaus2::lf::DArray D(static_cast<std::string const &>(blockresults.getFiles().getHist()));
					accD->merge(D);
					#if 0
					bool const hwtdelayed =
						ensureWaveletTreeGenerated(blockresults,logstr);
					#endif
					::libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type ICHWL(ensureWaveletTreeGenerated(blockresults,logstr));
					::libmaus2::lf::ImpCompactHuffmanWaveletLF IHWL(ICHWL);
					IHWL.D = D.D;
					assert ( cblocksize == IHWL.n );

					// rank of position 0 in this block (for computing new gt array/stream)
					uint64_t const lp0 = blockresults.getBlockP0Rank();

					// last symbol in this block
					int64_t const firstblocklast = input_types_type::linear_wrapper::getSymbolAtPosition(fn,(nextblockstart+fs-1)%fs);

					/**
					 * array of absolute positions
					 **/
					uint64_t const zactive = zblocks.size();
					::libmaus2::autoarray::AutoArray<uint64_t> zabsblockpos(zactive+1,false);
					for ( uint64_t z = 0; z < zactive; ++z )
						zabsblockpos[z] = zblocks[z].getZAbsPos();
					zabsblockpos [ zactive ] = blockstart + cblocksize;

					std::vector < std::string > gtpartnames(zactive);

					::libmaus2::timing::RealTimeClock rtc;
					rtc.start();

					ZNext znext(zactive);

					uint64_t termcnt = 0;
					libmaus2::parallel::OMPLock termcntlock;

					libmaus2::parallel::PosixSemaphore qsem; // queue semaphore
					libmaus2::parallel::PosixSemaphore tsem; // term semaphore
					libmaus2::parallel::PosixSemaphore globsem; // meta semaphore for both above
					libmaus2::parallel::LockedBool termflag(false);
					libmaus2::parallel::LockedBool qterm(false);

					SGGFS.registerMergePackSemaphore(&qsem);
					SGGFS.registerMergePackSemaphore(&globsem);
					SGGFS.registerTermSemaphore(&tsem);
					SGGFS.registerTermSemaphore(&globsem);
					SGGFS.setTermSemCnt(numthreads);

					#if defined(_OPENMP)
					#pragma omp parallel num_threads(numthreads)
					#endif
					{
						uint64_t z = 0;

						while ( znext.getNext(z) )
						{
							#if defined(_OPENMP)
							uint64_t const tid = omp_get_thread_num();
							#else
							uint64_t const tid = 0;
							#endif

							uint64_t * const Ga = GG[tid].begin();
							uint64_t *       Gc = Ga;
							uint64_t * const Ge = GG[tid].end();

							::libmaus2::timing::RealTimeClock subsubrtc; subsubrtc.start();

							::libmaus2::suffixsort::BwtMergeZBlock const & zblock = zblocks[z];

							std::string const gtpartname = gtmpgen.getFileName() + "_" + ::libmaus2::util::NumberSerialisation::formatNumber(z,4) + ".gt";
							::libmaus2::util::TempFileRemovalContainer::addTempFile(gtpartname);
							gtpartnames[z] = gtpartname;
							libmaus2::bitio::BitVectorOutput GTHEF(gtpartname);

							libmaus2::bitio::BitVectorInput gtfile(mergedgtname, (mergeprocrightend - zblock.getZAbsPos()) );

							typename input_types_type::circular_reverse_wrapper CRWR(fn,zblock.getZAbsPos() % fs);
							uint64_t r = zblock.getZRank();

							uint64_t const zlen = zabsblockpos [ z ] - zabsblockpos [z+1];
							uint64_t const fullblocks = zlen/(Ge-Ga);
							uint64_t const rest = zlen - fullblocks * (Ge-Ga);

							for ( uint64_t b = 0; b < fullblocks; ++b )
							{
								Gc = Ga;

								while ( Gc != Ge )
								{
									uint64_t * const Te = Gc + std::min(parcheck,static_cast<uint64_t>(Ge-Gc));

									for ( ; Gc != Te; ++Gc )
									{
										GTHEF.writeBit(r > lp0);
										int64_t const sym = CRWR.get();
										bool const gtf = gtfile.readBit();
										r = IHWL.step(sym,r) + ((sym == firstblocklast)?gtf:0);
										*Gc = r;
									}

									while ( globsem.trywait() )
									{
										qsem.wait();
										SGGFS.checkMergeSingle();
									}
								}

								std::string const tfn = tmpgen.getFileName();
								libmaus2::gamma::SparseGammaGapBlockEncoder::encodeArray(Ga,Gc,tfn);
								SGGFS.putFile(std::vector<std::string>(1,tfn));

								while ( globsem.trywait() )
								{
									qsem.wait();
									SGGFS.checkMergeSingle();
								}
							}

							if ( rest )
							{
								Gc = Ga;

								while ( Gc != Ga + rest )
								{
									uint64_t * const Te = Gc + std::min(parcheck,static_cast<uint64_t>((Ga+rest)-Gc));

									for ( ; Gc != Te; ++Gc )
									{
										GTHEF.writeBit(r > lp0);
										int64_t const sym = CRWR.get();
										bool const gtf = gtfile.readBit();
										r = IHWL.step(sym,r) + ((sym == firstblocklast)?gtf:0);
										*Gc = r;
									}

									while ( globsem.trywait() )
									{
										qsem.wait();
										SGGFS.checkMergeSingle();
									}
								}

								std::string const tfn = tmpgen.getFileName();
								libmaus2::gamma::SparseGammaGapBlockEncoder::encodeArray(Ga,Gc,tfn);
								SGGFS.putFile(std::vector<std::string>(1,tfn));

								while ( globsem.trywait() )
								{
									qsem.wait();
									SGGFS.checkMergeSingle();
								}
							}

							GTHEF.flush();

							while ( globsem.trywait() )
							{
								qsem.wait();
								SGGFS.checkMergeSingle();
							}
						}

						{
							termcntlock.lock();
							if ( ++termcnt == numthreads )
								termflag.set(true);
							termcntlock.unlock();
						}

						bool running = true;
						while ( running )
						{
							if ( termflag.get() && (!qterm.get()) && SGGFS.isMergingQueueEmpty() )
							{
								for ( uint64_t i = 0; i < numthreads; ++i )
								{
									tsem.post();
									globsem.post();
								}

								qterm.set(true);
							}

							// std::cerr << "waiting for glob...";
							globsem.wait();
							// std::cerr << "done." << std::endl;

							if ( qsem.trywait() )
								SGGFS.checkMergeSingle();
							else
							{
								bool const tsemok = tsem.trywait();
								assert ( tsemok );
								running = false;
							}
						}
					}

					std::vector<std::string> const outputgapfilenames = SGGFS.mergeToDense(gtmpgen,cblocksize+1,numthreads);

					if ( logstr )
						(*logstr) << "[V] computed gap array in time " << rtc.getElapsedSeconds() << std::endl;

					return SparseGapArrayComputationResult(outputgapfilenames,gtpartnames,zactive,zabsblockpos);
				}

				static SparseGapArrayComputationResult computeSparseGapArray(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn, // name of text file
					uint64_t const fs, // length of text file in symbols
					libmaus2::suffixsort::bwtb3m::MergeStrategyMergeGapRequest const & msmgr, // merge request
					uint64_t const ihwtspace,
					std::vector<std::string> const & mergedgtname, // previous gt file name
					// std::string const & newmergedgtname, // new gt file name
					::libmaus2::lf::DArray * const accD, // accumulated symbol freqs for block
					// std::string const & outputgapfilename,
					std::string const & tmpfileprefix,
					uint64_t const maxmem,
					uint64_t const numthreads,
					std::ostream * logstr
				)
				{
					uint64_t const into = msmgr.into;

					std::vector<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type> const & children =
						*(msmgr.pchildren);

					::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults = children[into]->sortresult;

					uint64_t const blockstart = blockresults.getBlockStart();
					uint64_t const cblocksize = blockresults.getCBlockSize();
					uint64_t const nextblockstart = (blockstart+cblocksize)%fs;
					uint64_t const mergeprocrightend =
						children.at(children.size()-1)->sortresult.getBlockStart() +
						children.at(children.size()-1)->sortresult.getCBlockSize();
					std::vector < ::libmaus2::suffixsort::BwtMergeZBlock > const & zblocks = msmgr.zblocks;

					return computeSparseGapArray(gtmpgen,fn,fs,blockstart,cblocksize,nextblockstart,mergeprocrightend,
						blockresults,mergedgtname,
						// newmergedgtname,
						accD,
						// outputgapfilename,
						tmpfileprefix,
						maxmem-ihwtspace,
						zblocks,
						numthreads,
						logstr
					);
				}

				static std::vector<std::string> stringVectorAppend(std::vector<std::string> V, std::vector<std::string> const & W)
				{
					for ( uint64_t i = 0; i < W.size(); ++i )
						V.push_back(W[i]);
					return V;
				}

				template<typename gap_array>
				static void splitGapArray(
					gap_array & G,
					uint64_t const Gsize,
					uint64_t const numthreads,
					std::vector < std::pair<uint64_t,uint64_t> > & wpacks,
					std::vector < uint64_t > & P,
					std::ostream * logstr,
					int const verbose
				)
				{
					typedef typename gap_array::sequence_type sequence_type;

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] computing work packets" << std::endl;
					}

					uint64_t const logG = std::max(libmaus2::math::ilog(Gsize),static_cast<unsigned int>(1));
					uint64_t const logG2 = logG*logG;
					// target number of G samples
					uint64_t const tnumGsamp = std::max(Gsize / logG2,static_cast<uint64_t>(256*numthreads));
					uint64_t const Gsampleblocksize = (Gsize + tnumGsamp - 1) / tnumGsamp;
					// number of G samples
					uint64_t const numGsamp = (Gsize + Gsampleblocksize - 1) / Gsampleblocksize;

					libmaus2::autoarray::AutoArray < uint64_t > Gsamples(numGsamp,false);

					uint64_t const samplesPerPackage = (numGsamp + numthreads - 1)/numthreads;
					uint64_t const numSamplePackages = (numGsamp + samplesPerPackage - 1)/samplesPerPackage;

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t tid = 0; tid < numSamplePackages; ++tid )
					{
						uint64_t const tlow  = tid * samplesPerPackage;
						uint64_t const thigh = std::min(tlow + samplesPerPackage, numGsamp);
						assert ( thigh >= tlow );

						sequence_type gp = G.getOffsetSequence(tlow * Gsampleblocksize);

						for ( uint64_t ti = tlow; ti < thigh; ++ti )
						{
							uint64_t s = 0;
							uint64_t const low = ti * Gsampleblocksize;
							uint64_t const high = std::min(low + Gsampleblocksize, Gsize);

							for ( uint64_t i = low; i < high; ++i )
								s += gp.get();
							s += (high-low);

							if ( high == Gsize && high != low )
								s -= 1;
							Gsamples[ti] = s;
						}
					}

					#if 0
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numGsamp; ++t )
					{
						uint64_t const low = t * Gsampleblocksize;
						uint64_t const high = std::min(low + Gsampleblocksize, Gsize);
						assert ( high >= low );
						uint64_t s = 0;
						sequence_type gp = G.getOffsetSequence(low);
						for ( uint64_t i = low; i < high; ++i )
							s += gp.get();
						s += (high-low);
						if ( high == Gsize && high != low )
							s -= 1;
						Gsamples[t] = s;
					}
					#endif

					#if 0
					std::vector<uint64_t> G_A(Gsamples.begin(),Gsamples.end());
					std::vector<uint64_t> G_B(Gsamples.begin(),Gsamples.end());

					libmaus2::util::PrefixSums::prefixSums(G_A.begin(),G_A.end());
					libmaus2::util::PrefixSums::parallelPrefixSums(G_A.begin(),G_A.end(),numthreads);
					#endif

					uint64_t const Gsum = libmaus2::util::PrefixSums::parallelPrefixSums(Gsamples.begin(),Gsamples.end(),numthreads);

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] G size " << Gsize << " number of G samples " << numGsamp << std::endl;
					}

					uint64_t const Gsumperthread = (Gsum + numthreads-1)/numthreads;
					wpacks = std::vector < std::pair<uint64_t,uint64_t> >(numthreads);
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						uint64_t const target = i * Gsumperthread;
						uint64_t const * p = ::std::lower_bound(Gsamples.begin(),Gsamples.end(),target);

						if ( p == Gsamples.end() )
							--p;
						while ( *p > target )
							--p;

						assert ( *p <= target );

						uint64_t iv = (p - Gsamples.begin()) * Gsampleblocksize;
						uint64_t s = *p;

						sequence_type gp = G.getOffsetSequence(iv);
						//G_array_iterator gp = G.getOffsetPointer(iv);
						while ( s < target && iv < Gsize )
						{
							s += (gp.get())+1;
							iv++;
						}
						if ( iv == Gsize )
							s -= 1;

						wpacks[i].first = iv;
						if ( i )
							wpacks[i-1].second = iv;
						// std::cerr << "i=" << i << " iv=" << iv << " Gsize=" << Gsize << std::endl;
					}
					wpacks.back().second = Gsize;

					// remove empty packages
					{
						uint64_t o = 0;
						for ( uint64_t i = 0; i < wpacks.size(); ++i )
							if ( wpacks[i].first != wpacks[i].second )
								wpacks[o++] = wpacks[i];
						wpacks.resize(o);
					}

					P.resize(wpacks.size()+1);
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t i = 0; i < wpacks.size(); ++i )
					{
						uint64_t const low = wpacks[i].first;
						uint64_t const high = wpacks[i].second;

						sequence_type gp = G.getOffsetSequence(low);
						// G_array_iterator gp = G.getOffsetPointer(low);
						uint64_t s = 0;
						for ( uint64_t i = low; i < high; ++i )
							s += gp.get();

						P[i] = s;

					}
					libmaus2::util::PrefixSums::prefixSums(P.begin(),P.end());
				}

				static void setupSampledISA(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					::libmaus2::suffixsort::BwtMergeBlockSortResult & result,
					uint64_t const numisa
				)
				{
					std::vector < std::string > Vsampledisa(numisa);
					for ( uint64_t i = 0; i < Vsampledisa.size(); ++i )
					{
						Vsampledisa[i] = (
							gtmpgen.getFileName()
							// result.getFiles().getBWT()
							+ "_"
							+ ::libmaus2::util::NumberSerialisation::formatNumber(Vsampledisa.size(),6)
							+ ".sampledisa"
						);
						::libmaus2::util::TempFileRemovalContainer::addTempFile(Vsampledisa[i]);

					}
					result.setSampledISA(Vsampledisa);
				}

				static bool compareFiles(std::vector<std::string> const & A, std::vector<std::string> const & B)
				{
					libmaus2::aio::ConcatInputStream CA(A);
					libmaus2::aio::ConcatInputStream CB(B);

					libmaus2::autoarray::AutoArray<char> BA(16*1024);
					libmaus2::autoarray::AutoArray<char> BB(BA.size());

					while ( CA && CB )
					{
						CA.read(BA.begin(),BA.size());
						CB.read(BB.begin(),BB.size());
						assert ( CA.gcount() == CB.gcount() );

						uint64_t const ca = CA.gcount();

						if (
							! std::equal(BA.begin(),BA.begin()+ca,BB.begin())
						)
							return false;
					}

					bool const oka = static_cast<bool>(CA);
					bool const okb = static_cast<bool>(CB);

					return oka == okb;
				}

				static void mergeBlocks(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalBlock & mergereq,
					std::string const fn,
					uint64_t const fs,
					// std::string const tmpfilenamebase,
					uint64_t const rlencoderblocksize,
					uint64_t const lfblockmult,
					uint64_t const numthreads,
					::std::map<int64_t,uint64_t> const & /* chist */,
					uint64_t const bwtterm,
					std::string const & huftreefilename,
					std::ostream * logstr,
					int const verbose
				)
				{

					if ( logstr )
						(*logstr) << "[V] Merging BWT blocks MergeStrategyMergeInternalBlock." << std::endl;

					assert ( mergereq.children.size() > 1 );
					assert ( mergereq.children.size() == mergereq.gaprequests.size()+1 );

					// std::cerr << "[V] Merging BWT blocks with gapmembound=" << gapmembound << std::endl;

					/*
					 * remove unused file
					 */
					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] removing unused file " << mergereq.children[mergereq.children.size()-1]->sortresult.getFiles().getHWT() << std::endl;
					}
					libmaus2::aio::FileRemoval::removeFile ( mergereq.children[mergereq.children.size()-1]->sortresult.getFiles().getHWT().c_str() );

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] setting meta info on BwtMergeBlockSortResult" << std::endl;
					}

					// get result object
					::libmaus2::suffixsort::BwtMergeBlockSortResult & result = mergereq.sortresult;
					// fill result structure
					result.setBlockStart( mergereq.children[0]->sortresult.getBlockStart() );
					result.setCBlockSize( 0 );
					for ( uint64_t i = 0; i < mergereq.children.size(); ++i )
						result.setCBlockSize( result.getCBlockSize() + mergereq.children[i]->sortresult.getCBlockSize() );
					// set up temp file names
					// of output bwt,
					// sampled inverse suffix array filename,
					// gt bit array,
					// huffman shaped wavelet tree and
					// histogram
					result.setTempPrefixAndRegisterAsTemp(gtmpgen,0 /* no preset bwt file names */, 0 /* no preset gt file names */, 0 /* no preset isa */);

					if ( verbose >= 5 && logstr )
					{
						(*logstr) << "[V] handling " << mergereq.children.size() << " child nodes" << std::endl;
					}

					// if we merge only two blocks together, then we do not need to write the gap array to disk
					if ( mergereq.children.size() == 2 )
					{
						// std::cerr << "** WHITEBOX INTERNAL 1 **" << std::endl;

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] loading block character histogram" << std::endl;
						}

						std::string const & sblockhist = mergereq.children[1]->sortresult.getFiles().getHist();
						// load char histogram for last/second block (for merging)
						::libmaus2::lf::DArray::unique_ptr_type accD(new ::libmaus2::lf::DArray(
							sblockhist
							)
						);
						// first block
						::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults =
							mergereq.children[0]->sortresult;

						// start of first block
						uint64_t const blockstart = blockresults.getBlockStart();
						// size of first block
						uint64_t const cblocksize = blockresults.getCBlockSize();

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] calling computeGapArray" << std::endl;
						}

						// compute gap array
						GapArrayComputationResult const GACR = computeGapArray(
							gtmpgen,
							fn,fs,*(mergereq.gaprequests[0]),
							mergereq.children[1]->sortresult.getFiles().getGT(), // previous gt files
							// tmpfilenamebase + "_gparts", // new gt files
							accD.get(),
							numthreads,
							logstr,
							verbose
						);

						uint64_t const Gsize = (cblocksize+1);

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] call to computeGapArray finished" << std::endl;
						}

						#if 0
						// concatenate gt vectors
						concatenateGT(
							GACR.gtpartnames,
							blockresults.getFiles().getGT(),
							result.getFiles().getGT()
						);
						#endif

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] renaming gt files" << std::endl;
						}

						std::vector<std::string> oldgtnames;
						for ( uint64_t i = 0; i < blockresults.getFiles().getGT().size(); ++i )
						{
							std::ostringstream ostr;
							ostr << gtmpgen.getFileName() << "_renamed_" << std::setw(6) << std::setfill('0') << i << std::setw(0) << ".gt";
							std::string const renamed = ostr.str();
							oldgtnames.push_back(ostr.str());
							::libmaus2::util::TempFileRemovalContainer::addTempFile(renamed);
							libmaus2::aio::OutputStreamFactoryContainer::rename(blockresults.getFiles().getGT()[i].c_str(), renamed.c_str());
						}

						result.setGT(stringVectorAppend(GACR.gtpartnames,oldgtnames));

						::libmaus2::timing::RealTimeClock rtc; rtc.start();

						if ( logstr )
							(*logstr) << "[V] computing gap array split...";

						std::vector < std::pair<uint64_t,uint64_t> > wpacks;
						std::vector < uint64_t > P;
						splitGapArray(GACR.G,Gsize,numthreads,wpacks,P,logstr,verbose);

						if ( logstr )
							(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] calling mergeIsa" << std::endl;
						}

						// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
						std::pair < uint64_t, std::vector<std::string> > const PPP = mergeIsaParallel(
							gtmpgen,wpacks,P,
							mergereq.children[1]->sortresult.getFiles().getSampledISAVector(),
							blockresults.getFiles().getSampledISAVector(),
							blockstart,
							GACR.G,
							numthreads,
							logstr
						);

						result.setBlockP0Rank(PPP.first);
						result.setSampledISA(PPP.second);

						// #define LIBMAUS2_SUFFIXSORT_BWTB3M_PARALLEL_ISA_MERGE_DEBUG

						#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_PARALLEL_ISA_MERGE_DEBUG)
						{
							libmaus2::util::GetObject<uint32_t const *> mergeGO(GACR.G.begin());
							std::string const isadebugtmp = gtmpgen.getFileName(true);
							uint64_t const serialblockp0rank = mergeIsa(
								mergereq.children[1]->sortresult.getFiles().getSampledISAVector(), // old sampled isa
								blockresults.getFiles().getSampledISAVector(), // new sampled isa
								isadebugtmp,blockstart,mergeGO/*GACR.G.begin()*/,cblocksize+1 /* Gsize */,logstr
							);

							assert ( PPP.first == serialblockp0rank );
							assert ( compareFiles(PPP.second,std::vector<std::string>(1,isadebugtmp)) );

							libmaus2::aio::FileRemoval::removeFile(isadebugtmp);
						}
						#endif

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] call to mergeIsa finished" << std::endl;
						}

						if ( logstr )
							(*logstr) << "[V] merging BWTs...";

						rtc.start();

						#if 0
						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] computing work packets" << std::endl;
						}

						uint64_t const logG = std::max(libmaus2::math::ilog(Gsize),static_cast<unsigned int>(1));
						uint64_t const logG2 = logG*logG;
						// target number of G samples
						uint64_t const tnumGsamp = std::max(Gsize / logG2,static_cast<uint64_t>(256*numthreads));
						uint64_t const Gsampleblocksize = (Gsize + tnumGsamp - 1) / tnumGsamp;
						// number of G samples
						uint64_t const numGsamp = (Gsize + Gsampleblocksize - 1) / Gsampleblocksize;

						libmaus2::autoarray::AutoArray < uint64_t > Gsamples(numGsamp,false);

						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads)
						#endif
						for ( uint64_t t = 0; t < numGsamp; ++t )
						{
							uint64_t const low = t * Gsampleblocksize;
							uint64_t const high = std::min(low + Gsampleblocksize, Gsize);
							assert ( high >= low );
							uint64_t s = 0;
							G_array_iterator gp = GACR.G.begin() + low;
							for ( uint64_t i = low; i < high; ++i )
								s += *(gp++);
							s += (high-low);
							if ( high == Gsize && high != low )
								s -= 1;
							Gsamples[t] = s;
						}

						#if 0
						std::vector<uint64_t> G_A(Gsamples.begin(),Gsamples.end());
						std::vector<uint64_t> G_B(Gsamples.begin(),Gsamples.end());

						libmaus2::util::PrefixSums::prefixSums(G_A.begin(),G_A.end());
						libmaus2::util::PrefixSums::parallelPrefixSums(G_A.begin(),G_A.end(),numthreads);
						#endif

						uint64_t const Gsum = libmaus2::util::PrefixSums::parallelPrefixSums(Gsamples.begin(),Gsamples.end(),numthreads);

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] G size " << Gsize << " number of G samples " << numGsamp << std::endl;
						}

						uint64_t const Gsumperthread = (Gsum + numthreads-1)/numthreads;
						std::vector < std::pair<uint64_t,uint64_t> > wpacks;
						wpacks = std::vector < std::pair<uint64_t,uint64_t> >(numthreads);
						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads)
						#endif
						for ( uint64_t i = 0; i < numthreads; ++i )
						{
							uint64_t const target = i * Gsumperthread;
							uint64_t const * p = ::std::lower_bound(Gsamples.begin(),Gsamples.end(),target);

							if ( p == Gsamples.end() )
								--p;
							while ( *p > target )
								--p;

							assert ( *p <= target );

							uint64_t iv = (p - Gsamples.begin()) * Gsampleblocksize;
							uint64_t s = *p;

							G_array_iterator gp = GACR.G.begin() + iv;
							while ( s < target && iv < Gsize )
							{
								s += (*(gp++))+1;
								iv++;
							}
							if ( iv == Gsize )
								s -= 1;

							wpacks[i].first = iv;
							if ( i )
								wpacks[i-1].second = iv;
							// std::cerr << "i=" << i << " iv=" << iv << " Gsize=" << Gsize << std::endl;
						}
						wpacks.back().second = Gsize;

						// remove empty packages
						{
							uint64_t o = 0;
							for ( uint64_t i = 0; i < wpacks.size(); ++i )
								if ( wpacks[i].first != wpacks[i].second )
									wpacks[o++] = wpacks[i];
							wpacks.resize(o);
						}

						std::vector < uint64_t > P;
						P.resize(wpacks.size()+1);
						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(numthreads)
						#endif
						for ( uint64_t i = 0; i < wpacks.size(); ++i )
						{
							uint64_t const low = wpacks[i].first;
							uint64_t const high = wpacks[i].second;

							G_array_iterator gp = GACR.G.begin() + low;
							uint64_t s = 0;
							for ( uint64_t i = low; i < high; ++i )
								s += *(gp++);

							P[i] = s;

						}
						libmaus2::util::PrefixSums::prefixSums(P.begin(),P.end());

						#if 0
						// std::cerr << "(computing work packets...";
						P.push_back(0);
						uint64_t ilow = 0;
						//
						uint64_t const totalsuf = result.getCBlockSize();
						// number of packets
						uint64_t const numpack = numthreads;
						// suffixes per thread
						uint64_t const tpacksize = (totalsuf + numpack-1)/numpack;
						while ( ilow != Gsize )
						{
							uint64_t s = 0;
							uint64_t ihigh = ilow;

							if ( verbose >= 5 && logstr )
							{
								(*logstr) << "[V] ilow=" << ilow << std::endl;
							}

							while ( ihigh != Gsize && s < tpacksize )
								s += (GACR.G[ihigh++]+1);

							uint64_t const p = s-(ihigh-ilow);

							if ( ihigh+1 == Gsize && GACR.G[ihigh] == 0 )
								ihigh++;


							if ( verbose >= 5 && logstr )
							{
								(*logstr) << "[V] ihigh=" << ilow << std::endl;
							}

							// std::cerr << "[" << ilow << "," << ihigh << ")" << std::endl;

							assert ( p == std::accumulate(GACR.G.begin()+ilow,GACR.G.begin()+ihigh,0ull) );

							if ( verbose >= 5 && logstr )
							{
								(*logstr) << "[V] accumulate check done" << std::endl;
							}

							P.push_back(P.back() + p);
							wpacks.push_back(std::pair<uint64_t,uint64_t>(ilow,ihigh));
							encfilenames.push_back(
								gtmpgen.getFileName()
								// result.getFiles().getBWT()
								+ "_"
								+ ::libmaus2::util::NumberSerialisation::formatNumber(encfilenames.size(),6)
								+ ".bwt"
							);
							::libmaus2::util::TempFileRemovalContainer::addTempFile(encfilenames.back());
							ilow = ihigh;

							if ( verbose >= 5 && logstr )
							{
								(*logstr) << "[V] end of single loop" << std::endl;
							}
						}
						#endif
						#endif

						std::vector < std::string > encfilenames(wpacks.size());
						for ( uint64_t i = 0; i < wpacks.size(); ++i )
						{
							encfilenames[i] = (
								gtmpgen.getFileName()
								// result.getFiles().getBWT()
								+ "_"
								+ ::libmaus2::util::NumberSerialisation::formatNumber(encfilenames.size(),6)
								+ ".bwt"
							);
							::libmaus2::util::TempFileRemovalContainer::addTempFile(encfilenames[i]);
						}

						encfilenames.resize(wpacks.size());

						assert ( wpacks.size() <= numthreads );
						// std::cerr << "done,time=" << wprtc.getElapsedSeconds() << ")";

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] generated " << wpacks.size() << " work packages" << std::endl;
						}

						// std::cerr << "(setting up IDDs...";
						::libmaus2::timing::RealTimeClock wprtc; wprtc.start();
						wprtc.start();

						unsigned int const albits = rl_decoder::haveAlphabetBits() ? rl_decoder::getAlBits(mergereq.children[0]->sortresult.getFiles().getBWT()) : 0;

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] setting up IDD" << std::endl;
						}

						::libmaus2::huffman::IndexDecoderDataArray IDD0(
							mergereq.children[0]->sortresult.getFiles().getBWT(),numthreads);
						::libmaus2::huffman::IndexDecoderDataArray IDD1(
							mergereq.children[1]->sortresult.getFiles().getBWT(),numthreads);

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] setting up IDD done" << std::endl;
						}

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] setting up IECV" << std::endl;
						}

						::libmaus2::huffman::IndexEntryContainerVector::unique_ptr_type IECV0 = ::libmaus2::huffman::IndexLoader::loadAccIndex(
							mergereq.children[0]->sortresult.getFiles().getBWT()
						);
						::libmaus2::huffman::IndexEntryContainerVector::unique_ptr_type IECV1 = ::libmaus2::huffman::IndexLoader::loadAccIndex(
							mergereq.children[1]->sortresult.getFiles().getBWT()
						);

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] setting up IECV done" << std::endl;
						}

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] performing merge" << std::endl;
						}

						#if defined(_OPENMP)
						#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
						#endif
						for ( int64_t b = 0; b < static_cast<int64_t>(wpacks.size()); ++b )
						{
							uint64_t const ilow = wpacks[b].first;
							uint64_t const ihigh = wpacks[b].second;

							if ( ilow != ihigh )
							{
								bool const islast = (ihigh == Gsize);
								std::string const encfilename = encfilenames[b];

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] setting up decoders for left and right block for merge package " << b << std::endl;
								}

								rl_decoder leftrlin(IDD0,IECV0.get(),ilow);
								rl_decoder rightrlin(IDD1,IECV1.get(),P[b]);

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] setting up decoders for left and right block for merge package " << b << " done." << std::endl;
								}

								uint64_t const outsuf = (ihigh-ilow)-(islast?1:0) + (P[b+1]-P[b]);

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] setting up encoder for merge package " << b << std::endl;
								}

								rl_encoder bwtenc(encfilename,albits,outsuf,rlencoderblocksize);

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] setting up encoder for merge package " << b << " done" << std::endl;
								}

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] entering merge loop for merge package " << b << std::endl;
								}

								if ( islast )
								{
									for ( uint64_t j = ilow; j < ihigh-1; ++j )
									{
										for ( uint64_t i = 0; i < GACR.G[j]; ++i )
											bwtenc.encode(rightrlin.decode());

										bwtenc.encode(leftrlin.decode());
									}

									for ( uint64_t i = 0; i < GACR.G[cblocksize]; ++i )
										bwtenc.encode(rightrlin.decode());
								}
								else
								{
									for ( uint64_t j = ilow; j < ihigh; ++j )
									{
										for ( uint64_t i = 0; i < GACR.G[j]; ++i )
											bwtenc.encode(rightrlin.decode());

										bwtenc.encode(leftrlin.decode());
									}
								}

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] left merge loop for merge package " << b << std::endl;
								}

								bwtenc.flush();

								if ( verbose >= 5 && logstr )
								{
									libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
									(*logstr) << "[V] flushed encoder for merge package " << b << std::endl;
								}
							}
						}
						if ( logstr )
							(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

						#if 0
						std::cerr << "[V] concatenating bwt parts...";
						rtc.start();
						rl_encoder::concatenate(encfilenames,result.getFiles().getBWT());
						std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;
						#endif

						result.setBWT(encfilenames);

						#if 0
						std::cerr << "[V] removing tmp files...";
						rtc.start();
						for ( uint64_t i = 0; i < encfilenames.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile ( encfilenames[i].c_str() );
						std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;
						#endif

						// save histogram
						// std::cerr << "[V] saving histogram...";

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] serialising accD" << std::endl;
						}

						rtc.start();
						accD->serialise(static_cast<std::string const & >(result.getFiles().getHist()));
						// std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;

						if ( verbose >= 5 && logstr )
						{
							(*logstr) << "[V] serialising accD done" << std::endl;
						}
					}
					else
					{
						// std::cerr << "** WHITEBOX INTERNAL 2 **" << std::endl;

						std::vector < std::string > gapfilenames;
						std::vector < std::vector<std::string> > bwtfilenames;
						for ( uint64_t bb = 0; bb < mergereq.children.size(); ++bb )
						{
							// gap file name
							if ( bb+1 < mergereq.children.size() )
							{
								std::string const newgapname = gtmpgen.getFileName() + "_merging_" + ::libmaus2::util::NumberSerialisation::formatNumber(bb,4) + ".gap";
								::libmaus2::util::TempFileRemovalContainer::addTempFile(newgapname);
								gapfilenames.push_back(newgapname);
							}

							// bwt name
							std::vector<std::string> newbwtnames;
							for ( uint64_t i = 0; i < mergereq.children[bb]->sortresult.getFiles().getBWT().size(); ++i )
							{
								std::string const newbwtname = gtmpgen.getFileName() + "_merging_"
									+ ::libmaus2::util::NumberSerialisation::formatNumber(bb,4)
									+ "_"
									+ ::libmaus2::util::NumberSerialisation::formatNumber(i,4)
									+ ".bwt";
								::libmaus2::util::TempFileRemovalContainer::addTempFile(newbwtname);
								newbwtnames.push_back(newbwtname);
							}
							bwtfilenames.push_back(newbwtnames);
						}

						// rename last bwt file set
						for ( uint64_t i = 0; i < mergereq.children.back()->sortresult.getFiles().getBWT().size(); ++i )
						{
							libmaus2::aio::OutputStreamFactoryContainer::rename (
								mergereq.children.back()->sortresult.getFiles().getBWT()[i].c_str(),
								bwtfilenames.back()[i].c_str()
							);
						}

						std::vector<std::string> mergedgtname  = mergereq.children.back()->sortresult.getFiles().getGT();
						std::vector<std::string> mergedisaname = mergereq.children.back()->sortresult.getFiles().getSampledISAVector();

						// load char histogram for last block
						std::string const & lblockhist = mergereq.children.back()->sortresult.getFiles().getHist();
						::libmaus2::lf::DArray::unique_ptr_type accD(new ::libmaus2::lf::DArray(lblockhist));

						/**
						 * iteratively merge blocks together
						 **/
						for ( uint64_t bb = 0; bb+1 < mergereq.children.size(); ++bb )
						{
							// block we merge into
							uint64_t const bx = mergereq.children.size()-bb-2;
							if ( logstr )
								(*logstr) << "[V] merging blocks " << bx+1 << " to end into " << bx << std::endl;
							::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults =
								mergereq.children[bx]->sortresult;

							// output files for this iteration
							std::string const newmergedgtname = gtmpgen.getFileName() + "_merged_" + ::libmaus2::util::NumberSerialisation::formatNumber(bx,4) + ".gt";
							::libmaus2::util::TempFileRemovalContainer::addTempFile(newmergedgtname);
							std::string const newmergedisaname = gtmpgen.getFileName() + "_merged_" + ::libmaus2::util::NumberSerialisation::formatNumber(bx,4) + ".sampledisa";
							::libmaus2::util::TempFileRemovalContainer::addTempFile(newmergedisaname);
							// gap file
							std::string const gapfile = gapfilenames[bx];

							// start of this block
							uint64_t const blockstart = blockresults.getBlockStart();
							// size of this block
							uint64_t const cblocksize = blockresults.getCBlockSize();

							// compute gap array
							GapArrayComputationResult const GACR = computeGapArray(
								gtmpgen,fn,fs,*(mergereq.gaprequests[bx]),
								mergedgtname,
								//newmergedgtname,
								accD.get(),
								numthreads,
								logstr,
								verbose
							);

							// save the gap file
							saveGapFile(GACR.G,gapfile,logstr);

							if ( logstr )
								(*logstr) << "[V] computing gap array split...";

							libmaus2::timing::RealTimeClock splitrtc; splitrtc.start();
							std::vector < std::pair<uint64_t,uint64_t> > wpacks;
							std::vector < uint64_t > P;
							splitGapArray(GACR.G,(cblocksize+1) /* Gsize */,numthreads,wpacks,P,logstr,verbose);

							if ( logstr )
								(*logstr) << "done, time " << splitrtc.getElapsedSeconds() << std::endl;

							std::pair < uint64_t, std::vector<std::string> > const PPP = mergeIsaParallel(
								gtmpgen,wpacks,P,
								mergedisaname,
								blockresults.getFiles().getSampledISAVector(),
								blockstart,
								GACR.G,
								numthreads,
								logstr
							);

							result.setBlockP0Rank(PPP.first);

							#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_PARALLEL_ISA_MERGE_DEBUG)
							{
								// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
								std::string const isadebugtmp = gtmpgen.getFileName(true);
								libmaus2::util::GetObject<uint32_t const *> mergeGO(GACR.G.begin());
								uint64_t const serialblockp0rank = mergeIsa(mergedisaname,blockresults.getFiles().getSampledISAVector(),isadebugtmp,blockstart,mergeGO /*GACR.G.begin()*/,cblocksize+1 /*Gsize*/,logstr);

								assert ( PPP.first == serialblockp0rank );
								assert ( compareFiles(PPP.second,std::vector<std::string>(1,isadebugtmp)) );

								libmaus2::aio::FileRemoval::removeFile(isadebugtmp);
							}
							#endif

							#if 0
							// concatenate gt vectors
							concatenateGT(GACR.gtpartnames,blockresults.getFiles().getGT(),newmergedgtname);
							#endif

							// rename files
							std::vector<std::string> oldgtnames;
							for ( uint64_t i = 0; i < blockresults.getFiles().getGT().size(); ++i )
							{
								std::ostringstream ostr;
								ostr << gtmpgen.getFileName()
									<< "_renamed_"
									<< std::setw(6) << std::setfill('0') << bx << std::setw(0)
									<< "_"
									<< std::setw(6) << std::setfill('0') << i << std::setw(0)
									<< ".gt";
								std::string const renamed = ostr.str();
								oldgtnames.push_back(ostr.str());
								::libmaus2::util::TempFileRemovalContainer::addTempFile(renamed);
								libmaus2::aio::OutputStreamFactoryContainer::rename(blockresults.getFiles().getGT()[i].c_str(), renamed.c_str());
							}

							// result.setGT(stringVectorAppend(GACR.gtpartnames,blockresults.getFiles().getGT()));

							/*
							 * remove files we no longer need
							 */
							// files local to this block
							for ( uint64_t i = 0; i < blockresults.getFiles().getBWT().size(); ++i )
								libmaus2::aio::OutputStreamFactoryContainer::rename ( blockresults.getFiles().getBWT()[i].c_str(), bwtfilenames[bx][i].c_str() );
							blockresults.removeFilesButBwt();
							// previous stage gt bit vector
							for ( uint64_t i = 0; i < mergedgtname.size(); ++i )
								libmaus2::aio::FileRemoval::removeFile ( mergedgtname[i].c_str() );

							// update current file names
							mergedgtname = stringVectorAppend(GACR.gtpartnames,oldgtnames);
							mergedisaname = PPP.second; // std::vector<std::string>(1,newmergedisaname);
						}

						// renamed sampled inverse suffix array
						result.setSampledISA(mergedisaname);
						// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergedisaname.c_str(), result.getFiles().getSampledISA().c_str() );
						// rename gt bit array filename
						// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergedgtname.c_str(), result.getFiles().getGT().c_str() );
						result.setGT(mergedgtname);
						// save histogram
						accD->serialise(static_cast<std::string const & >(result.getFiles().getHist()));

						if ( logstr )
							(*logstr) << "[V] merging parts...";
						::libmaus2::timing::RealTimeClock mprtc;
						mprtc.start();
						result.setBWT(parallelGapFragMerge(
							gtmpgen,
							bwtfilenames,
							stringVectorPack(gapfilenames),
							// result.getFiles().getBWT(),
							//gtmpgen.getFileName()+"_gpart",
							numthreads,
							lfblockmult,rlencoderblocksize,logstr,verbose));
						if ( logstr )
							(*logstr) << "done, time " << mprtc.getElapsedSeconds() << std::endl;

						for ( uint64_t i = 0; i < gapfilenames.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile ( gapfilenames[i].c_str() );
						for ( uint64_t i = 0; i < bwtfilenames.size(); ++i )
							for ( uint64_t j = 0; j < bwtfilenames[i].size(); ++j )
								libmaus2::aio::FileRemoval::removeFile ( bwtfilenames[i][j].c_str() );
					}

					#if 0
					if ( logstr )
						(*logstr) << "[V] computing term symbol hwt...";
					::libmaus2::timing::RealTimeClock mprtc;
					mprtc.start();
					if ( input_types_type::utf8Wavelet() )
						libmaus2::wavelet::RlToHwtBase<true,rl_decoder>::rlToHwtTerm(result.getFiles().getBWT(),result.getFiles().getHWT(),tmpfilenamebase + "_wt",chist,bwtterm,result.getBlockP0Rank());
					else
						libmaus2::wavelet::RlToHwtBase<false,rl_decoder>::rlToHwtTerm(result.getFiles().getBWT(),result.getFiles().getHWT(),tmpfilenamebase + "_wt",chist,bwtterm,result.getBlockP0Rank());
					if ( logstr )
						(*logstr) << "done, time " << mprtc.getElapsedSeconds() << std::endl;
					#endif

					libmaus2::util::TempFileRemovalContainer::addTempFile(result.getFiles().getHWTReq());
					{
					libmaus2::aio::OutputStreamInstance hwtreqCOS(result.getFiles().getHWTReq());
					libmaus2::wavelet::RlToHwtTermRequest::serialise(
						hwtreqCOS,
						result.getFiles().getBWT(),
						result.getFiles().getHWT(),
						gtmpgen.getFileName() + "_wt", // XXX replace
						huftreefilename,
						bwtterm,
						result.getBlockP0Rank(),
						input_types_type::utf8Wavelet(),
						numthreads
					);
					hwtreqCOS.flush();
					// hwtreqCOS.close();
					}

					// remove obsolete files
					for ( uint64_t b = 0; b < mergereq.children.size(); ++b )
						mergereq.children[b]->sortresult.removeFiles();

					mergereq.releaseChildren();
				}

				static void mergeBlocks(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalSmallBlock & mergereq,
					std::string const fn,
					uint64_t const fs,
					// std::string const tmpfilenamebase,
					uint64_t const rlencoderblocksize,
					uint64_t const lfblockmult,
					uint64_t const numthreads,
					::std::map<int64_t,uint64_t> const & /* chist */,
					uint64_t const bwtterm,
					std::string const & huftreefilename,
					std::ostream * logstr,
					int const verbose
				)
				{
					assert ( mergereq.children.size() > 1 );
					assert ( mergereq.children.size() == mergereq.gaprequests.size()+1 );

					if ( logstr )
						(*logstr) << "[V] Merging BWT blocks MergeStrategyMergeInternalSmallBlock." << std::endl;

					/*
					 * remove unused file
					 */
					libmaus2::aio::FileRemoval::removeFile ( mergereq.children[mergereq.children.size()-1]->sortresult.getFiles().getHWT().c_str() );

					// get result object
					::libmaus2::suffixsort::BwtMergeBlockSortResult & result = mergereq.sortresult;
					// fill result structure
					result.setBlockStart( mergereq.children[0]->sortresult.getBlockStart() );
					result.setCBlockSize( 0 );
					for ( uint64_t i = 0; i < mergereq.children.size(); ++i )
						result.setCBlockSize( result.getCBlockSize() + mergereq.children[i]->sortresult.getCBlockSize() );
					// set up temp file names
					// of output bwt,
					// sampled inverse suffix array filename,
					// gt bit array,
					// huffman shaped wavelet tree and
					// histogram
					result.setTempPrefixAndRegisterAsTemp(gtmpgen,0 /* no preset bwt file names */, 0 /* no preset gt file names */, 0 /* no preset isa */);

					// if we merge only two blocks together, then we do not need to write the gap array to disk
					if ( mergereq.children.size() == 2 )
					{
						// std::cerr << "** WHITEBOX INTERNAL SMALL 1 **" << std::endl;

						std::string const & sblockhist = mergereq.children[1]->sortresult.getFiles().getHist();
						// load char histogram for last/second block (for merging)
						::libmaus2::lf::DArray::unique_ptr_type accD(new ::libmaus2::lf::DArray(
							sblockhist
							)
						);
						// first block
						::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults =
							mergereq.children[0]->sortresult;

						// start of first block
						uint64_t const blockstart = blockresults.getBlockStart();
						// size of first block
						uint64_t const cblocksize = blockresults.getCBlockSize();

						// compute gap array
						GapArrayByteComputationResult const GACR = computeGapArrayByte(
							gtmpgen,
							fn,fs,*(mergereq.gaprequests[0]),
							mergereq.children[1]->sortresult.getFiles().getGT(), // previous gt files
							#if 0
							tmpfilenamebase + "_gparts", // new gt files
							tmpfilenamebase + "_gapoverflow",
							#endif
							accD.get(),
							numthreads,
							logstr
						);

						#if 0
						// concatenate gt vectors
						concatenateGT(
							GACR.gtpartnames,
							blockresults.getFiles().getGT(),
							result.getFiles().getGT()
						);
						#endif

						std::vector<std::string> oldgtnames;
						for ( uint64_t i = 0; i < blockresults.getFiles().getGT().size(); ++i )
						{
							std::ostringstream ostr;
							ostr << gtmpgen.getFileName() << "_renamed_" << std::setw(6) << std::setfill('0') << i << std::setw(0) << ".gt";
							std::string const renamed = ostr.str();
							oldgtnames.push_back(ostr.str());
							::libmaus2::util::TempFileRemovalContainer::addTempFile(renamed);
							libmaus2::aio::OutputStreamFactoryContainer::rename(blockresults.getFiles().getGT()[i].c_str(), renamed.c_str());
						}

						result.setGT(stringVectorAppend(GACR.gtpartnames,oldgtnames));

						::libmaus2::timing::RealTimeClock rtc; rtc.start();

						if ( logstr )
							(*logstr) << "[V] splitting gap array...";

						std::vector < std::pair<uint64_t,uint64_t> > wpacks;
						std::vector < uint64_t > P;
						splitGapArray(
							*(GACR.G),
							cblocksize+1,
							numthreads,
							wpacks,
							P,
							logstr,
							verbose
						);

						if ( logstr )
							(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

						// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
						std::pair < uint64_t, std::vector<std::string> > const PPP = mergeIsaParallel(
							gtmpgen,wpacks,P,
							mergereq.children[1]->sortresult.getFiles().getSampledISAVector(),
							blockresults.getFiles().getSampledISAVector(),
							blockstart,
							*(GACR.G),
							numthreads,
							logstr
						);

						result.setBlockP0Rank(PPP.first);
						result.setSampledISA(PPP.second);

						#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_PARALLEL_ISA_MERGE_DEBUG)
						{
							std::string const isadebugtmp = gtmpgen.getFileName(true);

							// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
							libmaus2::suffixsort::GapArrayByteDecoder::unique_ptr_type pgap0dec(GACR.G->getDecoder());
							libmaus2::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pgap0decbuf(new libmaus2::suffixsort::GapArrayByteDecoderBuffer(*pgap0dec,8192));
							// libmaus2::suffixsort::GapArrayByteDecoderBuffer::iterator gap0decbufit = pgap0decbuf->begin();
							uint64_t const serialblockp0rank = mergeIsa(
								mergereq.children[1]->sortresult.getFiles().getSampledISAVector(), // old sampled isa
								blockresults.getFiles().getSampledISAVector(), // new sampled isa
								isadebugtmp,blockstart,*pgap0decbuf/*gap0decbufit*//*GACR.G.begin()*/,cblocksize+1 /* GACR.G.size() */,logstr
							);
							pgap0decbuf.reset();
							pgap0dec.reset();

							assert ( PPP.first == serialblockp0rank );
							assert ( compareFiles(PPP.second,std::vector<std::string>(1,isadebugtmp)) );

							libmaus2::aio::FileRemoval::removeFile(isadebugtmp);
						}
						#endif

						rtc.start();
						if ( logstr )
							(*logstr) << "[V] merging BWTs...";

						#if 0
						//
						uint64_t const totalsuf = result.getCBlockSize();
						// number of packets
						uint64_t const numpack = numthreads;
						// suffixes per thread
						uint64_t const tpacksize = (totalsuf + numpack-1)/numpack;
						//
						uint64_t ilow = 0;
						// intervals on G
						std::vector < std::pair<uint64_t,uint64_t> > wpacks;
						// prefix sums over G
						std::vector < uint64_t > P;
						P.push_back(0);

						// std::cerr << "(computing work packets...";
						libmaus2::suffixsort::GapArrayByteDecoder::unique_ptr_type pgap1dec(GACR.G->getDecoder());
						libmaus2::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pgap1decbuf(new libmaus2::suffixsort::GapArrayByteDecoderBuffer(*pgap1dec,8192));
						libmaus2::suffixsort::GapArrayByteDecoderBuffer::iterator gap1decbufit = pgap1decbuf->begin();
						while ( ilow != (cblocksize+1) )
						{
							uint64_t s = 0;
							uint64_t ihigh = ilow;

							while ( ihigh != (cblocksize+1) && s < tpacksize )
							{
								s += *(gap1decbufit++) + 1; // (GACR.G[ihigh++]+1);
								ihigh += 1;
							}

							uint64_t const p = s-(ihigh-ilow);

							if ( ihigh+1 == (cblocksize+1) && (*gap1decbufit == 0) /* GACR.G[ihigh] == 0 */ )
							{
								ihigh++;
								gap1decbufit++;
							}

							// std::cerr << "[" << ilow << "," << ihigh << ")" << std::endl;

							#if defined(GAP_ARRAY_BYTE_DEBUG)
							{
								// check obtained prefix sum
								libmaus2::suffixsort::GapArrayByteDecoder::unique_ptr_type pgap2dec(GACR.G->getDecoder(ilow));
								libmaus2::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pgap2decbuf(new libmaus2::suffixsort::GapArrayByteDecoderBuffer(*pgap2dec,8192));
								libmaus2::suffixsort::GapArrayByteDecoderBuffer::iterator gap2decbufit = pgap2decbuf->begin();

								uint64_t a = 0;
								for ( uint64_t ia = ilow; ia < ihigh; ++ia )
									a += *(gap2decbufit++);

								assert ( p == a );
							}
							#endif

							P.push_back(P.back() + p);
							wpacks.push_back(std::pair<uint64_t,uint64_t>(ilow,ihigh));
							ilow = ihigh;
						}
						assert ( wpacks.size() <= numthreads );
						// std::cerr << "done,time=" << wprtc.getElapsedSeconds() << ")";
						pgap1decbuf.reset();
						pgap1dec.reset();
						#endif

						std::vector < std::string > encfilenames(wpacks.size());
						for ( uint64_t i = 0; i < wpacks.size(); ++i )
						{
							encfilenames[i] = (
								gtmpgen.getFileName()
								// result.getFiles().getBWT()
								+ "_"
								+ ::libmaus2::util::NumberSerialisation::formatNumber(encfilenames.size(),6)
								+ ".bwt"
							);
							::libmaus2::util::TempFileRemovalContainer::addTempFile(encfilenames[i]);
						}

						// std::cerr << "(setting up IDDs...";
						::libmaus2::timing::RealTimeClock wprtc; wprtc.start();
						wprtc.start();
						unsigned int const albits = rl_decoder::haveAlphabetBits() ? rl_decoder::getAlBits(mergereq.children[0]->sortresult.getFiles().getBWT()) : 0;
						::libmaus2::huffman::IndexDecoderDataArray IDD0(
							mergereq.children[0]->sortresult.getFiles().getBWT(),numthreads);
						::libmaus2::huffman::IndexDecoderDataArray IDD1(
							mergereq.children[1]->sortresult.getFiles().getBWT(),numthreads);

						::libmaus2::huffman::IndexEntryContainerVector::unique_ptr_type IECV0 = ::libmaus2::huffman::IndexLoader::loadAccIndex(
							mergereq.children[0]->sortresult.getFiles().getBWT()
						);
						::libmaus2::huffman::IndexEntryContainerVector::unique_ptr_type IECV1 = ::libmaus2::huffman::IndexLoader::loadAccIndex(
							mergereq.children[1]->sortresult.getFiles().getBWT()
						);

						#if defined(_OPENMP)
						#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
						#endif
						for ( int64_t b = 0; b < static_cast<int64_t>(wpacks.size()); ++b )
						{
							uint64_t const ilow = wpacks[b].first;
							uint64_t const ihigh = wpacks[b].second;

							if ( ilow != ihigh )
							{
								bool const islast = (ihigh == (cblocksize+1));
								std::string const encfilename = encfilenames[b];

								rl_decoder leftrlin(IDD0,IECV0.get(),ilow);
								rl_decoder rightrlin(IDD1,IECV1.get(),P[b]);

								uint64_t const outsuf = (ihigh-ilow)-(islast?1:0) + (P[b+1]-P[b]);

								rl_encoder bwtenc(encfilename,albits,outsuf,rlencoderblocksize);

								libmaus2::suffixsort::GapArrayByteDecoder::unique_ptr_type pgap3dec(GACR.G->getDecoder(ilow));
								libmaus2::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pgap3decbuf(new libmaus2::suffixsort::GapArrayByteDecoderBuffer(*pgap3dec,8192));
								libmaus2::suffixsort::GapArrayByteDecoderBuffer::iterator gap3decbufit = pgap3decbuf->begin();

								if ( islast )
								{
									for ( uint64_t j = ilow; j < ihigh-1; ++j )
									{
										uint64_t const GACRGj = *(gap3decbufit++);

										for ( uint64_t i = 0; i < GACRGj; ++i )
											bwtenc.encode(rightrlin.decode());

										bwtenc.encode(leftrlin.decode());
									}

									assert ( ihigh == cblocksize+1 );

									uint64_t const GACRGcblocksize = *(gap3decbufit++);
									for ( uint64_t i = 0; i < GACRGcblocksize; ++i )
										bwtenc.encode(rightrlin.decode());
								}
								else
								{
									for ( uint64_t j = ilow; j < ihigh; ++j )
									{
										uint64_t const GACRGj = *(gap3decbufit++);

										for ( uint64_t i = 0; i < GACRGj; ++i )
											bwtenc.encode(rightrlin.decode());

										bwtenc.encode(leftrlin.decode());
									}
								}

								bwtenc.flush();
							}
						}
						if ( logstr )
							(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

						#if 0
						std::cerr << "[V] concatenating bwt parts...";
						rtc.start();
						rl_encoder::concatenate(encfilenames,result.getFiles().getBWT());
						std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;
						#endif

						result.setBWT(encfilenames);

						#if 0
						std::cerr << "[V] removing tmp files...";
						rtc.start();
						for ( uint64_t i = 0; i < encfilenames.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile ( encfilenames[i].c_str() );
						std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;
						#endif

						// save histogram
						if ( logstr )
							(*logstr) << "[V] saving histogram...";
						rtc.start();
						accD->serialise(static_cast<std::string const & >(result.getFiles().getHist()));
						if ( logstr )
							(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;
					}
					else
					{
						// std::cerr << "** WHITEBOX INTERNAL 2 **" << std::endl;

						std::vector < std::string > gapfilenames;
						std::vector < std::vector<std::string> > bwtfilenames;
						for ( uint64_t bb = 0; bb < mergereq.children.size(); ++bb )
						{
							// gap file name
							if ( bb+1 < mergereq.children.size() )
							{
								std::string const newgapname = gtmpgen.getFileName() + "_merging_" + ::libmaus2::util::NumberSerialisation::formatNumber(bb,4) + ".gap";
								::libmaus2::util::TempFileRemovalContainer::addTempFile(newgapname);
								gapfilenames.push_back(newgapname);
							}

							// bwt name
							std::vector<std::string> newbwtnames;
							for ( uint64_t i = 0; i < mergereq.children[bb]->sortresult.getFiles().getBWT().size(); ++i )
							{
								std::string const newbwtname = gtmpgen.getFileName() + "_merging_"
									+ ::libmaus2::util::NumberSerialisation::formatNumber(bb,4)
									+ "_"
									+ ::libmaus2::util::NumberSerialisation::formatNumber(i,4)
									+ ".bwt";
								::libmaus2::util::TempFileRemovalContainer::addTempFile(newbwtname);
								newbwtnames.push_back(newbwtname);
							}
							bwtfilenames.push_back(newbwtnames);
						}

						// rename last bwt file set
						for ( uint64_t i = 0; i < mergereq.children.back()->sortresult.getFiles().getBWT().size(); ++i )
						{
							libmaus2::aio::OutputStreamFactoryContainer::rename (
								mergereq.children.back()->sortresult.getFiles().getBWT()[i].c_str(),
								bwtfilenames.back()[i].c_str()
							);
						}

						std::vector<std::string> mergedgtname  = mergereq.children.back()->sortresult.getFiles().getGT();
						std::vector<std::string> mergedisaname = mergereq.children.back()->sortresult.getFiles().getSampledISAVector();

						// load char histogram for last block
						std::string const & lblockhist = mergereq.children.back()->sortresult.getFiles().getHist();
						::libmaus2::lf::DArray::unique_ptr_type accD(new ::libmaus2::lf::DArray(lblockhist));

						/**
						 * iteratively merge blocks together
						 **/
						for ( uint64_t bb = 0; bb+1 < mergereq.children.size(); ++bb )
						{
							// block we merge into
							uint64_t const bx = mergereq.children.size()-bb-2;
							if ( logstr )
								(*logstr) << "[V] merging blocks " << bx+1 << " to end into " << bx << std::endl;
							::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults =
								mergereq.children[bx]->sortresult;

							// output files for this iteration
							std::string const newmergedisaname = gtmpgen.getFileName() + "_merged_" + ::libmaus2::util::NumberSerialisation::formatNumber(bx,4) + ".sampledisa";
							::libmaus2::util::TempFileRemovalContainer::addTempFile(newmergedisaname);

							// gap file
							std::string const gapfile = gapfilenames[bx];

							// start of this block
							uint64_t const blockstart = blockresults.getBlockStart();
							// size of this block
							uint64_t const cblocksize = blockresults.getCBlockSize();

							// compute gap array
							GapArrayByteComputationResult const GACR = computeGapArrayByte(
								gtmpgen,
								fn,fs,*(mergereq.gaprequests[bx]),
								mergedgtname,
								#if 0
								newmergedgtname,
								newmergedgapoverflow,
								#endif
								accD.get(),
								numthreads,
								logstr
							);

							// save the gap file
							#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_HUFGAP)
							GACR.G->saveHufGapArray(gapfile);
							#else
							GACR.G->saveGammaGapArray(gapfile);
							#endif

							::libmaus2::timing::RealTimeClock rtc; rtc.start();

							if ( logstr )
								(*logstr) << "[V] splitting gap array...";

							std::vector < std::pair<uint64_t,uint64_t> > wpacks;
							std::vector < uint64_t > P;
							splitGapArray(
								*(GACR.G),
								cblocksize+1,
								numthreads,
								wpacks,
								P,
								logstr,
								verbose
							);

							if ( logstr )
								(*logstr) << "done, time " << rtc.getElapsedSeconds() << std::endl;

							// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
							std::pair < uint64_t, std::vector<std::string> > const PPP = mergeIsaParallel(
								gtmpgen,wpacks,P,
								mergedisaname,
								blockresults.getFiles().getSampledISAVector(),
								blockstart,
								*(GACR.G),
								numthreads,
								logstr
							);

							result.setBlockP0Rank(PPP.first);

							#if defined(LIBMAUS2_SUFFIXSORT_BWTB3M_PARALLEL_ISA_MERGE_DEBUG)
							{
								std::string const isadebugtmp = gtmpgen.getFileName(true);

								// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
								// libmaus2::util::GetObject<uint32_t const *> mergeGO(GACR.G.begin());
								libmaus2::suffixsort::GapArrayByteDecoder::unique_ptr_type pgap0dec(GACR.G->getDecoder());
								libmaus2::suffixsort::GapArrayByteDecoderBuffer::unique_ptr_type pgap0decbuf(new libmaus2::suffixsort::GapArrayByteDecoderBuffer(*pgap0dec,8192));
								uint64_t const serialblockp0rank = mergeIsa(mergedisaname,blockresults.getFiles().getSampledISAVector(),isadebugtmp,blockstart,*pgap0decbuf/*mergeGO*/ /*GACR.G.begin()*/,cblocksize+1 /*GACR.G.size()*/,logstr);

								assert ( PPP.first == serialblockp0rank );
								assert ( compareFiles(PPP.second,std::vector<std::string>(1,isadebugtmp)) );

								libmaus2::aio::FileRemoval::removeFile(isadebugtmp);
							}
							#endif

							#if 0
							// concatenate gt vectors
							concatenateGT(GACR.gtpartnames,blockresults.getFiles().getGT(),newmergedgtname);
							#endif

							// rename files
							std::vector<std::string> oldgtnames;
							for ( uint64_t i = 0; i < blockresults.getFiles().getGT().size(); ++i )
							{
								std::ostringstream ostr;
								ostr << gtmpgen.getFileName()
									<< "_renamed_"
									<< std::setw(6) << std::setfill('0') << bx << std::setw(0)
									<< "_"
									<< std::setw(6) << std::setfill('0') << i << std::setw(0)
									<< ".gt";
								std::string const renamed = ostr.str();
								oldgtnames.push_back(ostr.str());
								::libmaus2::util::TempFileRemovalContainer::addTempFile(renamed);
								libmaus2::aio::OutputStreamFactoryContainer::rename(blockresults.getFiles().getGT()[i].c_str(), renamed.c_str());
							}

							// result.setGT(stringVectorAppend(GACR.gtpartnames,blockresults.getFiles().getGT()));

							/*
							 * remove files we no longer need
							 */
							// files local to this block
							for ( uint64_t i = 0; i < blockresults.getFiles().getBWT().size(); ++i )
								libmaus2::aio::OutputStreamFactoryContainer::rename ( blockresults.getFiles().getBWT()[i].c_str(), bwtfilenames[bx][i].c_str() );
							blockresults.removeFilesButBwt();
							// previous stage gt bit vector
							for ( uint64_t i = 0; i < mergedgtname.size(); ++i )
								libmaus2::aio::FileRemoval::removeFile ( mergedgtname[i].c_str() );

							// update current file names
							mergedgtname = stringVectorAppend(GACR.gtpartnames,oldgtnames);
							mergedisaname = PPP.second;
						}


						// renamed sampled inverse suffix array
						result.setSampledISA(mergedisaname);
						// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergedisaname.c_str(), result.getFiles().getSampledISA().c_str() );
						// rename gt bit array filename
						// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergedgtname.c_str(), result.getFiles().getGT().c_str() );
						result.setGT(mergedgtname);
						// save histogram
						accD->serialise(static_cast<std::string const & >(result.getFiles().getHist()));

						if ( logstr )
							(*logstr) << "[V] merging parts...";
						::libmaus2::timing::RealTimeClock mprtc;
						mprtc.start();
						result.setBWT(parallelGapFragMerge(
							gtmpgen,
							bwtfilenames,
							stringVectorPack(gapfilenames),
							// result.getFiles().getBWT(),
							// tmpfilenamebase+"_gpart",
							numthreads,
							lfblockmult,rlencoderblocksize,logstr,verbose));
						if ( logstr )
							(*logstr) << "done, time " << mprtc.getElapsedSeconds() << std::endl;

						for ( uint64_t i = 0; i < gapfilenames.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile ( gapfilenames[i].c_str() );
						for ( uint64_t i = 0; i < bwtfilenames.size(); ++i )
							for ( uint64_t j = 0; j < bwtfilenames[i].size(); ++j )
								libmaus2::aio::FileRemoval::removeFile ( bwtfilenames[i][j].c_str() );
					}

					#if 0
					std::cerr << "[V] computing term symbol hwt...";
					::libmaus2::timing::RealTimeClock mprtc;
					mprtc.start();
					if ( input_types_type::utf8Wavelet() )
						libmaus2::wavelet::RlToHwtBase<true,rl_decoder>::rlToHwtTerm(result.getFiles().getBWT(),result.getFiles().getHWT(),tmpfilenamebase + "_wt",chist,bwtterm,result.getBlockP0Rank());
					else
						libmaus2::wavelet::RlToHwtBase<false,rl_decoder>::rlToHwtTerm(result.getFiles().getBWT(),result.getFiles().getHWT(),tmpfilenamebase + "_wt",chist,bwtterm,result.getBlockP0Rank());
					std::cerr << "done, time " << mprtc.getElapsedSeconds() << std::endl;
					#endif

					libmaus2::util::TempFileRemovalContainer::addTempFile(result.getFiles().getHWTReq());
					{
					libmaus2::aio::OutputStreamInstance hwtreqCOS(result.getFiles().getHWTReq());
					libmaus2::wavelet::RlToHwtTermRequest::serialise(
						hwtreqCOS,
						result.getFiles().getBWT(),
						result.getFiles().getHWT(),
						gtmpgen.getFileName() + "_wt", // XXX replace
						huftreefilename,
						bwtterm,
						result.getBlockP0Rank(),
						input_types_type::utf8Wavelet(),
						numthreads
					);
					hwtreqCOS.flush();
					//hwtreqCOS.close();
					}

					// remove obsolete files
					for ( uint64_t b = 0; b < mergereq.children.size(); ++b )
						mergereq.children[b]->sortresult.removeFiles();

					mergereq.releaseChildren();
				}

				static void mergeBlocks(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					libmaus2::suffixsort::bwtb3m::MergeStrategyMergeExternalBlock & mergereq,
					std::string const fn,
					uint64_t const fs,
					// std::string const tmpfilenamebase,
					std::string const sparsetmpfilenamebase,
					uint64_t const rlencoderblocksize,
					uint64_t const lfblockmult,
					uint64_t const numthreads,
					::std::map<int64_t,uint64_t> const & /* chist */,
					uint64_t const bwtterm,
					uint64_t const mem,
					std::string const & huftreefilename,
					std::ostream * logstr,
					int const verbose
				)
				{
					if ( logstr )
						(*logstr) << "[V] Merging BWT blocks MergeStrategyMergeExternalBlock." << std::endl;

					assert ( mergereq.children.size() > 1 );
					assert ( mergereq.children.size() == mergereq.gaprequests.size()+1 );

					/*
					 * remove unused file
					 */
					libmaus2::aio::FileRemoval::removeFile ( mergereq.children[mergereq.children.size()-1]->sortresult.getFiles().getHWT().c_str() );

					// get result object
					::libmaus2::suffixsort::BwtMergeBlockSortResult & result = mergereq.sortresult;
					// fill result structure
					result.setBlockStart( mergereq.children[0]->sortresult.getBlockStart() );
					result.setCBlockSize ( 0 );
					for ( uint64_t i = 0; i < mergereq.children.size(); ++i )
						result.setCBlockSize( result.getCBlockSize() + mergereq.children[i]->sortresult.getCBlockSize() );
					// set up
					// filenames of output bwt,
					// sampled inverse suffix array filename,
					// gt bit array,
					// huffman shaped wavelet tree and
					// histogram
					result.setTempPrefixAndRegisterAsTemp(gtmpgen,0,0,0);

					{
						std::vector < std::vector < std::string > > gapfilenames;
						std::vector < std::vector < std::string > > bwtfilenames;
						for ( uint64_t bb = 0; bb < mergereq.children.size(); ++bb )
						{
							// gap file name
							if ( bb+1 < mergereq.children.size() )
								gapfilenames.push_back(std::vector<std::string>());

							// bwt name
							std::vector<std::string> newbwtnames;
							for ( uint64_t i = 0; i < mergereq.children[bb]->sortresult.getFiles().getBWT().size(); ++i )
							{
								std::string const newbwtname = gtmpgen.getFileName() + "_merging_"
									+ ::libmaus2::util::NumberSerialisation::formatNumber(bb,4)
									+ "_"
									+ ::libmaus2::util::NumberSerialisation::formatNumber(i,4)
									+ ".bwt";
								::libmaus2::util::TempFileRemovalContainer::addTempFile(newbwtname);
								newbwtnames.push_back(newbwtname);
							}
							bwtfilenames.push_back(newbwtnames);
						}

						// rename last bwt file set
						for ( uint64_t i = 0; i < mergereq.children.back()->sortresult.getFiles().getBWT().size(); ++i )
						{
							libmaus2::aio::OutputStreamFactoryContainer::rename (
								mergereq.children.back()->sortresult.getFiles().getBWT()[i].c_str(),
								bwtfilenames.back()[i].c_str()
							);
						}


						std::vector<std::string> mergedgtname  = mergereq.children.back()->sortresult.getFiles().getGT();
						std::vector<std::string> mergedisaname = mergereq.children.back()->sortresult.getFiles().getSampledISAVector();

						// load char histogram for last block
						std::string const & lblockhist = mergereq.children.back()->sortresult.getFiles().getHist();
						::libmaus2::lf::DArray::unique_ptr_type accD(new ::libmaus2::lf::DArray(lblockhist));

						/**
						 * iteratively merge blocks together
						 **/
						for ( uint64_t bb = 0; bb+1 < mergereq.children.size(); ++bb )
						{
							// std::cerr << "** WHITEBOX EXTERNAL **" << std::endl;

							// block we merge into
							uint64_t const bx = mergereq.children.size()-bb-2;
							if ( logstr )
								(*logstr) << "[V] merging blocks " << bx+1 << " to end into " << bx << std::endl;
							::libmaus2::suffixsort::BwtMergeBlockSortResult const & blockresults =
								mergereq.children[bx]->sortresult;

							// output files for this iteration
							std::string const newmergedisaname = gtmpgen.getFileName() + "_merged_" + ::libmaus2::util::NumberSerialisation::formatNumber(bx,4) + ".sampledisa";
							::libmaus2::util::TempFileRemovalContainer::addTempFile(newmergedisaname);

							// start of this block
							uint64_t const blockstart = blockresults.getBlockStart();
							// size of this block
							uint64_t const cblocksize = blockresults.getCBlockSize();

							// std::cerr << "*** compute sparse ***" << std::endl;

							SparseGapArrayComputationResult const GACR = computeSparseGapArray(
								gtmpgen,
								fn,fs,*(mergereq.gaprequests[bx]),
								mergereq.children[mergereq.gaprequests[bx]->into]->getIHWTSpaceBytes(),
								mergedgtname,
								accD.get(),
								sparsetmpfilenamebase+"_sparsegap",
								mem,
								numthreads,
								logstr
							);
							gapfilenames[bx] = GACR.fn;

							// merge sampled inverse suffix arrays, returns rank of position 0 (relative to block start)
							libmaus2::gamma::GammaGapDecoder GGD(gapfilenames[bx],0/* offset */,0 /* psymoff */,numthreads);
							result.setBlockP0Rank( mergeIsa(mergedisaname,blockresults.getFiles().getSampledISAVector(),newmergedisaname,blockstart,GGD,cblocksize+1 /*GACR.G.size()*/,logstr) );

							// concatenate gt vectors
							//concatenateGT(GACR.gtpartnames,blockresults.getFiles().getGT(),newmergedgtname);

							// rename files
							std::vector<std::string> oldgtnames;
							for ( uint64_t i = 0; i < blockresults.getFiles().getGT().size(); ++i )
							{
								std::ostringstream ostr;
								ostr << gtmpgen.getFileName()
									<< "_renamed_"
									<< std::setw(6) << std::setfill('0') << bx << std::setw(0)
									<< "_"
									<< std::setw(6) << std::setfill('0') << i << std::setw(0)
									<< ".gt";
								std::string const renamed = ostr.str();
								oldgtnames.push_back(ostr.str());
								::libmaus2::util::TempFileRemovalContainer::addTempFile(renamed);
								libmaus2::aio::OutputStreamFactoryContainer::rename(blockresults.getFiles().getGT()[i].c_str(), renamed.c_str());
							}

							/*
							 * remove files we no longer need
							 */
							// files local to this block
							for ( uint64_t i = 0; i < blockresults.getFiles().getBWT().size(); ++i )
								libmaus2::aio::OutputStreamFactoryContainer::rename ( blockresults.getFiles().getBWT()[i].c_str(), bwtfilenames[bx][i].c_str() );
							blockresults.removeFilesButBwt();
							// previous stage gt bit vector
							for ( uint64_t i = 0; i < mergedgtname.size(); ++i )
								libmaus2::aio::FileRemoval::removeFile ( mergedgtname[i].c_str() );

							// update current file names
							mergedgtname = stringVectorAppend(GACR.gtpartnames,oldgtnames);
							mergedisaname = std::vector<std::string>(1,newmergedisaname);
						}

						// renamed sampled inverse suffix array
						// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergedisaname.c_str(), result.getFiles().getSampledISA().c_str() );
						result.setSampledISA(mergedisaname);
						// rename gt bit array filename
						// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergedgtname.c_str(), result.getFiles().getGT().c_str() );
						result.setGT(mergedgtname);
						// save histogram
						accD->serialise(static_cast<std::string const & >(result.getFiles().getHist()));

						if ( logstr )
							(*logstr) << "[V] merging parts...";
						::libmaus2::timing::RealTimeClock mprtc;
						mprtc.start();
						result.setBWT(
							parallelGapFragMerge(gtmpgen,bwtfilenames,gapfilenames/* tmpfilenamebase+"_gpart" */,numthreads,
								lfblockmult,rlencoderblocksize,logstr,verbose)
						);
						if ( logstr )
							(*logstr) << "done, time " << mprtc.getElapsedSeconds() << std::endl;

						for ( uint64_t i = 0; i < gapfilenames.size(); ++i )
							for ( uint64_t j = 0; j < gapfilenames[i].size(); ++j )
								libmaus2::aio::FileRemoval::removeFile ( gapfilenames[i][j].c_str() );
						for ( uint64_t i = 0; i < bwtfilenames.size(); ++i )
							for ( uint64_t j = 0; j < bwtfilenames[i].size(); ++j )
								libmaus2::aio::FileRemoval::removeFile ( bwtfilenames[i][j].c_str() );
					}

					#if 0
					std::cerr << "[V] computing term symbol hwt...";
					::libmaus2::timing::RealTimeClock mprtc;
					mprtc.start();
					if ( input_types_type::utf8Wavelet() )
						libmaus2::wavelet::RlToHwtBase<true,rl_decoder>::rlToHwtTerm(result.getFiles().getBWT(),result.getFiles().getHWT(),tmpfilenamebase + "_wt",chist,bwtterm,result.getBlockP0Rank());
					else
						libmaus2::wavelet::RlToHwtBase<false,rl_decoder>::rlToHwtTerm(result.getFiles().getBWT(),result.getFiles().getHWT(),tmpfilenamebase + "_wt",chist,bwtterm,result.getBlockP0Rank());
					std::cerr << "done, time " << mprtc.getElapsedSeconds() << std::endl;
					#endif

					libmaus2::util::TempFileRemovalContainer::addTempFile(result.getFiles().getHWTReq());
					{
					libmaus2::aio::OutputStreamInstance hwtreqCOS(result.getFiles().getHWTReq());
					libmaus2::wavelet::RlToHwtTermRequest::serialise(hwtreqCOS,
						result.getFiles().getBWT(),
						result.getFiles().getHWT(),
						gtmpgen.getFileName() + "_wt", // XXX replace
						huftreefilename,
						bwtterm,
						result.getBlockP0Rank(),
						input_types_type::utf8Wavelet(),
						numthreads
					);
					hwtreqCOS.flush();
					//hwtreqCOS.close();
					}

					// remove obsolete files
					for ( uint64_t b = 0; b < mergereq.children.size(); ++b )
						mergereq.children[b]->sortresult.removeFiles();

					mergereq.releaseChildren();
				}

				static std::string sortIsaFile(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::vector<std::string> const & mergedisaname, uint64_t const blockmem,
					uint64_t const numthreads,
					std::ostream * logstr
				)
				{
					// sort sampled inverse suffix array file
					std::string const mergeisatmp = gtmpgen.getFileName(true);
					std::string const mergeisatmpout = gtmpgen.getFileName(true);
					::libmaus2::sorting::PairFileSorting::sortPairFile(
						mergedisaname,mergeisatmp,true /* second comp */,
						true,true,mergeisatmpout,blockmem/2/*par*/,numthreads /* parallel */,false /* delete input */,logstr);
					libmaus2::aio::FileRemoval::removeFile (mergeisatmp);
					return mergeisatmpout;
				}

				static uint64_t readBlockRanksSize(std::string const & mergedisaname)
				{
					return ::libmaus2::util::GetFileSize::getFileSize(mergedisaname)/(2*sizeof(uint64_t));
				}

				static ::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > readBlockRanks(std::string const & mergedisaname)
				{
					// read sampled isa
					uint64_t const nsisa = readBlockRanksSize(mergedisaname);
					::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > blockranks(nsisa,false);
					::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type SGIsisa(new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(mergedisaname,16*1024));
					for ( uint64_t i = 0; i < nsisa; ++i )
					{
						int64_t const r = SGIsisa->get();
						assert ( r >= 0 );
						int64_t const p = SGIsisa->get();
						assert ( p >= 0 );
						blockranks [ i ] = std::pair<uint64_t,uint64_t>(r,p);
					}
					SGIsisa.reset();

					return blockranks;
				}

				static void checkSampledSA(
					std::string const & fn,
					uint64_t const fs,
					std::string const mergedsaname,
					uint64_t const sasamplingrate,
					uint64_t const numthreads,
					uint64_t const lfblockmult,
					std::ostream * logstr
				)
				{
					::libmaus2::parallel::OMPLock cerrlock;
					// number of sampled suffix array elements
					uint64_t const nsa = (fs + sasamplingrate - 1) / sasamplingrate;

					// check that this matches what we have in the file
					assert ( ::libmaus2::util::GetFileSize::getFileSize(mergedsaname) / (sizeof(uint64_t)) ==  nsa + 2 );

					if ( nsa && nsa-1 )
					{
						uint64_t const checkpos = nsa-1;
						uint64_t const satcheckpacks = numthreads * lfblockmult;
						uint64_t const sacheckblocksize = (checkpos + satcheckpacks-1) / satcheckpacks;
						uint64_t const sacheckpacks = ( checkpos + sacheckblocksize - 1 ) / sacheckblocksize;

						if ( logstr )
							(*logstr) << "[V] checking suffix array on text...";
						::libmaus2::parallel::SynchronousCounter<uint64_t> SC;
						int64_t lastperc = -1;
						#if defined(_OPENMP)
						#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
						#endif
						for ( int64_t t = 0; t < static_cast<int64_t>(sacheckpacks); ++t )
						{
							uint64_t const low = t * sacheckblocksize;
							uint64_t const high = std::min(low+sacheckblocksize,checkpos);
							uint64_t const cnt = high-low;

							// std::cerr << "low=" << low << " high=" << high << " nsa=" << nsa << " cnt=" << cnt << std::endl;

							::libmaus2::aio::SynchronousGenericInput<uint64_t> SGIsa(mergedsaname,16*1024,low+2,cnt+1);

							typename input_types_type::circular_suffix_comparator CSC(fn,fs);

							int64_t const fp = SGIsa.get();
							assert ( fp >= 0 );

							uint64_t prev = fp;

							while ( SGIsa.peek() >= 0 )
							{
								int64_t const p = SGIsa.get();
								assert ( p >= 0 );

								// std::cerr << "pre " << prev.first << " SA[" << r << "]=" << p << std::endl;

								bool const ok = CSC (prev,p);
								assert ( ok );

								prev = p;
							}

							uint64_t const sc = ++SC;
							int64_t const newperc = (sc*100) / sacheckpacks;

							cerrlock.lock();
							if ( newperc != lastperc )
							{
								if ( logstr )
									(*logstr) << "(" << newperc << ")";
								lastperc = newperc;
							}
							cerrlock.unlock();
						}
						if ( logstr )
							(*logstr) << "done." << std::endl;
					}

				}

				static void computeSampledSA(
					libmaus2::util::TempFileNameGenerator & gtmpgen,
					std::string const & fn,
					uint64_t const fs,
					::libmaus2::lf::ImpCompactHuffmanWaveletLF const & IHWT,
					std::string const & mergedisaname,
					std::string const & outfn,
					// std::string const & tmpfilenamebase,
					uint64_t const numthreads,
					uint64_t const lfblockmult,
					uint64_t const sasamplingrate,
					uint64_t const isasamplingrate,
					uint64_t const blockmem,
					std::ostream * logstr
				)
				{
					// read size of sampled isa
					uint64_t const nsisa = readBlockRanksSize(mergedisaname);

					::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type SGIsisasa(new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(mergedisaname,16*1024));
					int64_t const fr = SGIsisasa->get(); assert ( fr != -1 );
					int64_t const fp = SGIsisasa->get(); assert ( fp != -1 );

					if ( logstr )
						(*logstr) << "[V] computing sampled suffix array parts...";

					std::pair<uint64_t,uint64_t> const isa0(fr,fp);
					std::pair<uint64_t,uint64_t> isapre(isa0);

					::std::vector< std::string > satempfilenames(numthreads);
					::std::vector< std::string > isatempfilenames(numthreads);
					::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type > SAF(numthreads);
					::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type > ISAF(numthreads);
					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						satempfilenames[i] = ( gtmpgen.getFileName() + ".sampledsa_" + ::libmaus2::util::NumberSerialisation::formatNumber(i,6) );
						::libmaus2::util::TempFileRemovalContainer::addTempFile(satempfilenames[i]);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tSAFi(
							new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(satempfilenames[i],8*1024)
						);
						SAF[i] = UNIQUE_PTR_MOVE(tSAFi);

						isatempfilenames[i] = ( gtmpgen.getFileName() + ".sampledisa_" + ::libmaus2::util::NumberSerialisation::formatNumber(i,6) );
						::libmaus2::util::TempFileRemovalContainer::addTempFile(isatempfilenames[i]);
						::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type tISAFi(
							new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(isatempfilenames[i],8*1024)
						);
						ISAF[i] = UNIQUE_PTR_MOVE(tISAFi);
					}

					std::vector < std::pair< std::pair<uint64_t,uint64_t>, std::pair<uint64_t,uint64_t> > > WV;
					int64_t lastperc = -1;

					if ( nsisa > 1 )
					{
						for ( int64_t i = 1; i <= static_cast<int64_t>(nsisa); ++i )
						{
							int64_t const nr = (i == static_cast<int64_t>(nsisa)) ? isa0.first : SGIsisasa->get();
							int64_t const np = (i == static_cast<int64_t>(nsisa)) ? isa0.second : ((nr != -1) ? SGIsisasa->get() : -1);
							assert ( np >= 0 );

							std::pair<uint64_t,uint64_t> isai(nr,np);

							WV.push_back(std::pair< std::pair<uint64_t,uint64_t>, std::pair<uint64_t,uint64_t> >(isai,isapre));

							isapre.first = nr;
							isapre.second = np;

							if ( ((WV.size() % (lfblockmult*numthreads)) == 0) || i == static_cast<int64_t>(nsisa) )
							{
								#if defined(_OPENMP)
								#pragma omp parallel for num_threads(numthreads)
								#endif
								for ( int64_t j = 0; j < static_cast<int64_t>(WV.size()); ++j )
								{
									#if defined(_OPENMP)
									uint64_t const tid = omp_get_thread_num();
									#else
									uint64_t const tid = 0;
									#endif
									checkBwtBlockDecode(WV[j].first,WV[j].second,fn,fs,IHWT,*SAF[tid],*ISAF[tid],sasamplingrate,isasamplingrate);
								}

								WV.resize(0);
							}

							int64_t const newperc = ((i)*100) / (nsisa);
							if ( newperc != lastperc )
							{
								if ( logstr )
									(*logstr) << "(" << newperc << ")";
								lastperc = newperc;
							}
						}
					}
					else
					{
						assert ( fp == 0 );
						checkBwtBlockDecode(
							std::pair<uint64_t,uint64_t>(fr,0),
							std::pair<uint64_t,uint64_t>(fr,0),
							fn,fs,IHWT,*SAF[0],*ISAF[0],sasamplingrate,isasamplingrate,
							fs);
					}

					SGIsisasa.reset();

					for ( uint64_t i = 0; i < SAF.size(); ++i )
					{
						SAF[i]->flush();
						SAF[i].reset();
					}
					for ( uint64_t i = 0; i < ISAF.size(); ++i )
					{
						ISAF[i]->flush();
						ISAF[i].reset();
					}

					if ( logstr )
						(*logstr) << "done." << std::endl;

					if ( logstr )
						(*logstr) << "[V] sorting and merging sampled suffix array parts...";
					std::string const mergedsaname = ::libmaus2::util::OutputFileNameTools::clipOff(outfn,".bwt") + ".sa";
					{
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type pmergedsa(new ::libmaus2::aio::OutputStreamInstance(mergedsaname));
					// write sampling rate
					::libmaus2::serialize::Serialize<uint64_t>::serialize(*pmergedsa,sasamplingrate);
					::libmaus2::serialize::Serialize<uint64_t>::serialize(*pmergedsa,(fs + sasamplingrate-1)/sasamplingrate);
					std::string const mergesatmp = mergedsaname + ".tmp";
					::libmaus2::util::TempFileRemovalContainer::addTempFile(mergesatmp);
					::libmaus2::sorting::PairFileSorting::sortPairFile(
						satempfilenames,mergesatmp,
						false /* second comp */,
						false /* keep first */,
						true /* keep second */,
						*pmergedsa /* output stream */,
						blockmem/* /2 par in place now*/,
						numthreads /* parallel */,
						true /* delete input */,
						logstr
					);
					pmergedsa->flush();
					pmergedsa.reset();
					libmaus2::aio::FileRemoval::removeFile(mergesatmp.c_str());
					}
					if ( logstr )
						(*logstr) << "done." << std::endl;

					if ( logstr )
						(*logstr) << "[V] sorting and merging sampled inverse suffix array parts...";
					std::string const mergedisaoutname = ::libmaus2::util::OutputFileNameTools::clipOff(outfn,".bwt") + ".isa";
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type pmergedisa(new ::libmaus2::aio::OutputStreamInstance(mergedisaoutname));
					// write sampling rate
					::libmaus2::serialize::Serialize<uint64_t>::serialize(*pmergedisa,isasamplingrate);
					::libmaus2::serialize::Serialize<uint64_t>::serialize(*pmergedisa,(fs+isasamplingrate-1)/isasamplingrate);
					std::string const mergeisatmp = mergedisaoutname + ".tmp";
					::libmaus2::util::TempFileRemovalContainer::addTempFile(mergeisatmp);
					::libmaus2::sorting::PairFileSorting::sortPairFile(
						isatempfilenames,mergeisatmp,
						false /* second comp */,
						false /* keep first */,
						true /* keep second */,
						*pmergedisa /* output stream */,
						blockmem/2/*par*/,
						numthreads /* parallel */,
						true /* delete input */,
						logstr
					);
					if ( logstr )
						(*logstr) << "done." << std::endl;
					libmaus2::aio::FileRemoval::removeFile(mergeisatmp.c_str());

					#if 0
					// check sampled suffix array by pairwise comparison on text
					// this can take quadratic time in fs
					checkSampledSA(fn,fs,mergedsaname,sasamplingrate,numthreads,lfblockmult);
					#endif
				}


				static uint64_t getDefaultBlockSize(uint64_t const mem, uint64_t const threads, uint64_t const fs)
				{
					uint64_t const memblocksize = std::max(static_cast<uint64_t>(0.95 * mem / ( 5 * threads )),static_cast<uint64_t>(1));
					uint64_t const fsblocksize = (fs + threads - 1)/threads;
					return std::min(memblocksize,fsblocksize);
				}


				static std::map<int64_t,uint64_t> mergeMaps(
					std::map<int64_t,uint64_t> const & A,
					std::map<int64_t,uint64_t> const & B)
				{
					std::map<int64_t,uint64_t>::const_iterator aita = A.begin(), aite = A.end();
					std::map<int64_t,uint64_t>::const_iterator bita = B.begin(), bite = B.end();
					std::map<int64_t,uint64_t> C;

					while ( aita != aite && bita != bite )
					{
						if ( aita->first < bita->first )
						{
							C[aita->first] = aita->second;
							++aita;
						}
						else if ( bita->first < aita->first )
						{
							C[bita->first] = bita->second;
							++bita;
						}
						else
						{
							C[aita->first] = aita->second + bita->second;
							++aita; ++bita;
						}
					}

					while ( aita != aite )
					{
						C[aita->first] = aita->second;
						++aita;
					}

					while ( bita != bite )
					{
						C[bita->first] = bita->second;
						++bita;
					}

					return C;
				}

				struct HashHistogram
				{
					libmaus2::autoarray::AutoArray<uint64_t> L;
					libmaus2::util::ExtendingSimpleCountingHash<uint64_t,uint64_t> H;
					libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > P;
					uint64_t p;

					HashHistogram(uint64_t const lowsize = 256, uint64_t const bigsize = (1ull<<16) ) : L(lowsize), H(libmaus2::math::nextTwoPow(bigsize)), P(64ull*1024ull), p(0) {}

					void clear()
					{
						std::fill(L.begin(),L.end(),0ull);
						H.clear();
						p = 0;
					}

					void checkSize()
					{
						H.checkExtend();
					}

					void increment(uint64_t const k)
					{
						if ( k < L.size() )
							__sync_fetch_and_add(L.begin()+k,1);
						else
							H.insert(k);
					}

					void ensureSize(uint64_t const s)
					{
						while ( H.getTableSize() < s )
							H.extendInternal();
					}

					void extract()
					{
						uint64_t nonzero = 0;
						for ( uint64_t i = 0; i < L.size(); ++i )
							if ( L[i] )
								nonzero++;
						for ( libmaus2::util::ExtendingSimpleCountingHash<uint64_t,uint64_t>::key_type const * k = H.begin(); k != H.end(); ++k )
							if ( *k != libmaus2::util::ExtendingSimpleCountingHash<uint64_t,uint64_t>::unused() )
								nonzero++;

						if ( P.size() < nonzero )
							P.resize(nonzero);

						p = 0;

						for ( uint64_t i = 0; i < L.size(); ++i )
							if ( L[i] )
								P[p++] = std::pair<uint64_t,uint64_t>(i,L[i]);

						for ( libmaus2::util::ExtendingSimpleCountingHash<uint64_t,uint64_t>::key_type const * k = H.begin(); k != H.end(); ++k )
							if ( *k != libmaus2::util::ExtendingSimpleCountingHash<uint64_t,uint64_t>::unused() )
								P[p++] = std::pair<uint64_t,uint64_t>(*k,H.cntbegin() [ (k-H.begin()) ] );

						std::sort(P.begin(),P.begin()+p);
					}
				};

				static void getBlockSymFreqsHash(std::string const fn, uint64_t const glow, uint64_t ghigh, HashHistogram & H, uint64_t const numthreads)
				{
					typedef typename input_types_type::linear_wrapper stream_type;
					typedef typename input_types_type::base_input_stream::char_type char_type;
					typedef typename ::libmaus2::util::UnsignedCharVariant<char_type>::type unsigned_char_type;

					uint64_t const fs = ghigh-glow;

					uint64_t const loopsize = 16ull*1024ull;
					uint64_t const elperthread = 8*loopsize;
					H.ensureSize(elperthread * numthreads);
					H.clear();

					uint64_t const symsperfrag = (fs + numthreads - 1)/numthreads;
					// uint64_t const numfrags = (fs + symsperfrag - 1)/symsperfrag;
					uint64_t const loops = (symsperfrag + loopsize -1)/loopsize;

					::libmaus2::autoarray::AutoArray<unsigned_char_type> GB(loopsize*numthreads,false);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(numthreads); ++t )
					{
						uint64_t const low  = std::min(glow + t * symsperfrag,ghigh);
						uint64_t const high = std::min(low+symsperfrag       ,ghigh);
						uint64_t const size = high-low;
						uint64_t todo = size;

						unsigned_char_type * const B = GB.begin() + t * loopsize;

						stream_type CIS(fn);
						CIS.seekg(low);

						for ( uint64_t i = 0; i < loops; ++i )
						{
							uint64_t const toread = std::min(todo,loopsize);

							if ( toread )
							{
								CIS.read(reinterpret_cast<char_type *>(B),toread);
								assert ( CIS.gcount() == static_cast<int64_t>(toread) );
							}

							for ( uint64_t i = 0; i < toread; ++i )
								H.increment(B[i]);

							todo -= toread;

							H.checkSize();
						}
					}

					H.extract();
				}

				static std::map<int64_t,uint64_t> getBlockSymFreqs(std::string const fn, uint64_t const glow, uint64_t ghigh, uint64_t const numthreads)
				{
					typedef typename input_types_type::linear_wrapper stream_type;
					typedef typename input_types_type::base_input_stream::char_type char_type;
					typedef typename ::libmaus2::util::UnsignedCharVariant<char_type>::type unsigned_char_type;

					uint64_t const fs = ghigh-glow;

					uint64_t const symsperfrag = (fs + numthreads - 1)/numthreads;
					uint64_t const numfrags = (fs + symsperfrag - 1)/symsperfrag;

					libmaus2::util::HistogramSet HS(numfrags,256);

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t t = 0; t < static_cast<int64_t>(numfrags); ++t )
					{
						uint64_t const low = glow + t * symsperfrag;
						uint64_t const high = std::min(low+symsperfrag,ghigh);
						uint64_t const size = high-low;
						uint64_t todo = size;

						stream_type CIS(fn);
						CIS.seekg(low);

						::libmaus2::autoarray::AutoArray<unsigned_char_type> B(16*1024,false);
						libmaus2::util::Histogram & H = HS[t];

						while ( todo )
						{
							uint64_t const toread = std::min(todo,B.size());
							CIS.read(reinterpret_cast<char_type *>(B.begin()),toread);
							assert ( CIS.gcount() == static_cast<int64_t>(toread) );
							for ( uint64_t i = 0; i < toread; ++i )
								H (B[i]);
							todo -= toread;
						}
					}

					::libmaus2::util::Histogram::unique_ptr_type PH(HS.merge());
					::libmaus2::util::Histogram & H(*PH);

					return H.getByType<int64_t>();
				}

				static uint64_t getBlockStart(uint64_t const b, uint64_t const blocksize, uint64_t const fullblocks)
				{
					return (b < fullblocks) ? (b*blocksize) : (fullblocks * blocksize + (b-fullblocks) * (blocksize-1));
				}

				static uint64_t getBlockSize(uint64_t const b, uint64_t const blocksize, uint64_t const fullblocks)
				{
					return (b < fullblocks) ? blocksize : (blocksize-1);
				}

				static BwtMergeSortResult computeBwt(BwtMergeSortOptions const & options, std::ostream * logstr)
				{
					std::string fn = options.fn;
					libmaus2::util::TempFileNameGenerator gtmpgen(options.tmpfilenamebase+"_tmpdir",5);

					/* get file size */
					uint64_t const fs = input_types_type::linear_wrapper::getFileSize(fn);
					// target block size
					uint64_t const tblocksize = std::max(static_cast<uint64_t>(1),std::min(options.maxblocksize,getDefaultBlockSize(options.mem,options.numthreads,fs)));

					libmaus2::timing::RealTimeClock bwtclock;
					bwtclock.start();

					#if defined(BWTB3M_MEMORY_DEBUG)
					uint64_t mcnt = 0;
					#endif

					::libmaus2::util::TempFileRemovalContainer::setup();
					uint64_t const rlencoderblocksize = 16*1024;
					::libmaus2::parallel::OMPLock cerrlock;


					// check whether file exists
					if ( ! ::libmaus2::util::GetFileSize::fileExists(fn) )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "File " << fn << " does not exist or cannot be opened." << std::endl;
						se.finish();
						throw se;
					}


					// file name of serialised character histogram
					std::string const chistfilename = gtmpgen.getFileName() + ".chist";
					// file name of serialised huffman tree
					std::string const huftreefilename = gtmpgen.getFileName() + ".huftree";
					::libmaus2::util::TempFileRemovalContainer::addTempFile(chistfilename);
					::libmaus2::util::TempFileRemovalContainer::addTempFile(huftreefilename);

					libmaus2::aio::ArrayFile<char const *>::unique_ptr_type COAF;
					libmaus2::autoarray::AutoArray<char> COA;

					if ( options.copyinputtomemory )
					{
						if ( input_types_type::utf8Wavelet() )
						{
							std::string const nfn = "mem://copied_" + fn;
							libmaus2::aio::InputStreamInstance ininst(fn);
							libmaus2::aio::OutputStreamInstance outinst(nfn);
							uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(ininst);
							libmaus2::util::GetFileSize::copy(ininst,outinst,fs);
							if ( logstr )
								(*logstr) << "[V] copied " << fn << " to " << nfn << std::endl;

							{
								libmaus2::aio::InputStreamInstance ininst(fn + ".idx");
								libmaus2::aio::OutputStreamInstance outinst(nfn + ".idx");
								uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(ininst);
								libmaus2::util::GetFileSize::copy(ininst,outinst,fs);
								if ( logstr )
									*logstr << "[V] copied " << fn+".idx" << " to " << nfn+".idx" << std::endl;
							}

							fn = nfn;
						}
						else
						{
							uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(fn);
							COA = libmaus2::autoarray::AutoArray<char>(fs,false);

							#if defined(_OPENMP)
							uint64_t const bs = 1024ull * 1024ull;
							uint64_t const numthreads = options.numthreads;
							uint64_t const numblocks = (fs + bs - 1)/bs;

							if ( logstr )
								(*logstr) << "[V] copying " << fn << " using " << numthreads << " threads, stripe size " << bs << std::endl;

							#pragma omp parallel num_threads(numthreads)
							{
								uint64_t const tid = omp_get_thread_num();
								libmaus2::aio::InputStreamInstance ininst(fn);

								uint64_t bid = tid;

								while ( bid < numblocks )
								{
									uint64_t const low = bid * bs;
									uint64_t const high = std::min(low+bs,fs);
									assert ( high > low );

									ininst.clear();
									ininst.seekg(low);

									ininst.read(COA.begin() + low,high-low);

									assert ( ininst && (ininst.gcount() == static_cast<int64_t>(high-low)) );

									bid += numthreads;
								}
							}
							#else
							{
								libmaus2::aio::InputStreamInstance ininst(fn);
								ininst.read(COA.begin(),fs);
								assert ( ininst && (ininst.gcount() == static_cast<int64_t>(fs)) );
							}
							#endif

							libmaus2::aio::ArrayFile<char const *>::unique_ptr_type tCOAF(
								new libmaus2::aio::ArrayFile<char const *>(COA.begin(),COA.end())
							);
							COAF = UNIQUE_PTR_MOVE(tCOAF);
							std::string const nfn = COAF->getURL();
							if ( logstr )
								*logstr << "[V] copied " << fn << " to " << nfn << std::endl;

							#if 0
							{
								libmaus2::aio::InputStreamInstance ISI0(fn);
								libmaus2::aio::InputStreamInstance ISI1(nfn);
								for ( uint64_t i = 0; i < fs; ++i )
								{
									int const c0 = ISI0.get();
									int const c1 = ISI1.get();
									assert ( c0 != std::istream::traits_type::eof() );
									assert ( c0 == c1 );
								}
							}
							#endif

							fn = nfn;
						}
					}


					/* check that file is not empty */
					if ( ! fs )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "File " << fn << " is empty." << std::endl;
						se.finish();
						throw se;
					}

					// number of blocks
					uint64_t const numblocks = (fs + tblocksize - 1) / tblocksize;
					// final block size
					uint64_t const blocksize = (fs + numblocks - 1)/ numblocks;
					// full block product
					uint64_t const fullblockprod = numblocks * blocksize;
					// extraneous
					uint64_t const extrasyms = fullblockprod - fs;
					// check
					assert ( extrasyms < numblocks );
					// full blocks
					uint64_t const fullblocks = numblocks - extrasyms;
					// reduced blocks
					uint64_t const redblocks = numblocks - fullblocks;
					// check
					assert ( fullblocks * blocksize + redblocks * (blocksize-1) == fs );

					// next power of two
					uint64_t const blocksizenexttwo = ::libmaus2::math::nextTwoPow(blocksize);
					// prev power of two
					uint64_t const blocksizeprevtwo = (blocksize == blocksizenexttwo) ? blocksize : (blocksizenexttwo / 2);

					// ISA sampling rate during block merging
					uint64_t const preisasamplingrate = std::min(options.maxpreisasamplingrate,blocksizeprevtwo);

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					if ( logstr )
					{
						*logstr << "[V] sorting file " << fn << " of size " << fs << " with block size " << blocksize << " (" << numblocks << " blocks)" << " and " << options.numthreads << " threads" << std::endl;
						*logstr << "[V] full blocks " << fullblocks << " reduced blocks " << redblocks << std::endl;
					}

					// there should be at least one block
					assert ( numblocks );

					#if 0
					std::vector<int64_t> minblockperiods(numblocks);
					// compute periods
					libmaus2::parallel::OMPLock block;
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(options.numthreads)
					#endif
					for ( uint64_t bb = 0; bb < numblocks; ++bb )
					{
						// block id
						uint64_t const b = numblocks-bb-1;
						// start of block in file
						uint64_t const blockstart = getBlockStart(b,blocksize,fullblocks);

						typedef typename input_types_type::base_input_stream base_input_stream;
						typedef typename base_input_stream::char_type char_type;
						typedef typename input_types_type::circular_wrapper circular_wrapper;

						uint64_t const readlen = 3 * blocksize;
						circular_wrapper textstr(fn,blockstart);
						libmaus2::autoarray::AutoArray<char_type> A(readlen,false);
						textstr.read(&A[0],A.size());

						block.lock();
						if ( logstr )
							*logstr << "\r" << std::string(80,' ') << "\r[Checking " << (bb+1) << "/" << numblocks << "]\r";
						block.unlock();

						libmaus2::util::BorderArray<uint32_t> SBA(A.begin(),A.size());

						uint64_t minper = std::numeric_limits<uint64_t>::max();
						for ( uint64_t i = (blocksize-1); i < A.size(); ++i )
						{
							uint64_t const period = (i+1)-SBA[i];

							// if period divides length of the prefix
							if (
								( ( (i+1) % period ) == 0 ) && ((i+1) / period > 1)
								&&
								(period < minper)
							)
							{
								block.lock();
								if ( logstr )
									*logstr << "\nlen " << (i+1) << " block " << b << " period " << period << std::endl;
								block.unlock();
								minper = period;
							}
						}

						if ( minper == std::numeric_limits<uint64_t>::max() )
							minblockperiods[b] = -1;
						else
							minblockperiods[b] = minper;

					}
					if ( logstr )
						*logstr << std::endl;

					exit(0);
					#endif

					if ( logstr )
						*logstr << "[V] computing LCP between block suffixes and the following block start: ";
					std::vector<uint64_t> largelcpblocks;
					libmaus2::parallel::OMPLock largelcpblockslock;
					// uint64_t const largelcpthres = 16*1024;
					std::vector<uint64_t> boundedlcpblockvalues(numblocks);
					libmaus2::parallel::SynchronousCounter<uint64_t> largelcpblockscomputed(0);
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(options.numthreads)
					#endif
					for ( uint64_t bb = 0; bb < numblocks; ++bb )
					{
						// block id
						uint64_t const b = numblocks-bb-1;
						// start of block in file
						uint64_t const blockstart = getBlockStart(b,blocksize,fullblocks);
						// size of this block
						uint64_t const cblocksize = getBlockSize(b,blocksize,fullblocks);

						// start of next block
						uint64_t const nextblockstart = (blockstart + cblocksize) % fs;

						// find bounded lcp between this block and start of next
						uint64_t const blcp = libmaus2::suffixsort::BwtMergeBlockSortRequestBase::findSplitCommonBounded<input_types_type>(fn,blockstart,cblocksize,nextblockstart,fs,options.largelcpthres);

						if ( blcp >= options.largelcpthres )
						{
							libmaus2::parallel::ScopeLock slock(largelcpblockslock);
							largelcpblocks.push_back(b);
						}

						{
						libmaus2::parallel::ScopeLock slock(largelcpblockslock);
						uint64_t const finished = ++largelcpblockscomputed;
						if ( logstr )
							*logstr << "(" << static_cast<double>(finished)/numblocks << ")";
						}

						boundedlcpblockvalues[b] = blcp;
					}
					if ( logstr )
						*logstr << "done." << std::endl;

					std::sort(largelcpblocks.begin(),largelcpblocks.end());
					for ( uint64_t ib = 0; ib < largelcpblocks.size(); ++ib )
					{
						uint64_t const b = largelcpblocks[ib];

						if ( logstr )
							*logstr << "[V] Recomputing lcp value for block " << b << "...";

						// start of block in file
						uint64_t const blockstart = getBlockStart(b,blocksize,fullblocks);
						// size of this block
						uint64_t const cblocksize = getBlockSize(b,blocksize,fullblocks);

						// start of next block
						uint64_t const nextblockstart = (blockstart + cblocksize) % fs;

						// find bounded lcp between this block and start of next
						uint64_t const blcp = libmaus2::suffixsort::BwtMergeBlockSortRequestBase::findSplitCommon<input_types_type>(fn,blockstart,cblocksize,nextblockstart,fs);

						boundedlcpblockvalues[b] = blcp;

						if ( logstr )
							*logstr << blcp << std::endl;
					}

					// exit(0);

					::libmaus2::suffixsort::BwtMergeTempFileNameSetVector blocktmpnames(gtmpgen, numblocks, options.numthreads /* bwt */, options.numthreads /* gt */);

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					if ( logstr )
						*logstr << "[V] computing symbol frequences" << std::endl;
					std::map<int64_t,uint64_t> chistnoterm;
					// std::vector< std::map<int64_t,uint64_t> > blockfreqvec(numblocks);
					libmaus2::timing::RealTimeClock rtc;
					for ( uint64_t bb = 0; bb < numblocks; ++bb )
					{
						// block id
						uint64_t const b = numblocks-bb-1;
						// start of block in file
						uint64_t const blockstart = getBlockStart(b,blocksize,fullblocks);
						// size of this block
						uint64_t const cblocksize = getBlockSize(b,blocksize,fullblocks);

						std::map<int64_t,uint64_t> const blockfreqs =
							getBlockSymFreqs(fn,blockstart,blockstart+cblocksize,options.numthreads);

						std::string const freqstmpfilename = blocktmpnames[b].getHist() + ".freqs";
						libmaus2::util::TempFileRemovalContainer::addTempFile(freqstmpfilename);
						{
						libmaus2::aio::OutputStreamInstance freqCOS(freqstmpfilename);
						libmaus2::util::NumberMapSerialisation::serialiseMap(freqCOS,blockfreqs);
						freqCOS.flush();
						//freqCOS.close();
						}

						#if 0
						rtc.start();
						std::map<int64_t,uint64_t> const blockfreqs =
							getBlockSymFreqs(fn,blockstart,blockstart+cblocksize,options.numthreads);
						double const t0 = rtc.getElapsedSeconds();

						HashHistogram HH;
						rtc.start();
						getBlockSymFreqsHash(fn,blockstart,blockstart+cblocksize,HH,options.numthreads);
						double const t1 = rtc.getElapsedSeconds();

						if ( logstr )
						{
							*logstr << "(" << blockfreqs.size() << "," << HH.p << ")";
							*logstr << "[" << t0 << "," << t1 << "]";
						}
						#endif

						chistnoterm = mergeMaps(chistnoterm,blockfreqs);

						// blockfreqvec[b] = blockfreqs;
					}

					#if 0
					exit(0);
					#endif

					/* get symbol count histogram */
					int64_t const bwtterm = chistnoterm.size() ? (chistnoterm.rbegin()->first+1) : 0;
					::std::map<int64_t,uint64_t> chist = chistnoterm;
					chist[bwtterm] = 1;
					uint64_t const maxsym = chist.size() ? chist.rbegin()->first : 0;

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					libmaus2::aio::OutputStreamInstance::unique_ptr_type chistCOS(new libmaus2::aio::OutputStreamInstance(chistfilename));
					(*chistCOS) << ::libmaus2::util::NumberMapSerialisation::serialiseMap(chist);
					chistCOS->flush();
					// chistCOS->close();
					chistCOS.reset();

					if ( logstr )
					{
						*logstr << "[V] computed symbol frequences, input alphabet size is " << chistnoterm.size() << std::endl;
						*logstr << "[V] bwtterm=" << bwtterm << std::endl;
					}

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					libmaus2::huffman::HuffmanTree::unique_ptr_type uhnode(new libmaus2::huffman::HuffmanTree(chist.begin(),chist.size(),false,true,true));

					libmaus2::aio::OutputStreamInstance::unique_ptr_type huftreeCOS(new libmaus2::aio::OutputStreamInstance(huftreefilename));
					uhnode->serialise(*huftreeCOS);
					huftreeCOS->flush();
					// huftreeCOS->close();
					huftreeCOS.reset();

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					::libmaus2::huffman::HuffmanTree::EncodeTable::unique_ptr_type EC(
						new ::libmaus2::huffman::HuffmanTree::EncodeTable(*uhnode));

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					std::vector < libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type > stratleafs(numblocks);

					for ( uint64_t bb = 0; bb < numblocks; ++bb )
					{
						uint64_t const b = numblocks-bb-1;

						// start of block in file
						uint64_t const blockstart = getBlockStart(b,blocksize,fullblocks);
						// size of this block
						uint64_t const cblocksize = getBlockSize(b,blocksize,fullblocks);

						// symbol frequency map
						// std::map<int64_t,uint64_t> const & blockfreqs = blockfreqvec[b];
						std::string const freqstmpfilename = blocktmpnames[b].getHist() + ".freqs";
						libmaus2::aio::InputStreamInstance freqCIS(freqstmpfilename);
						std::map<int64_t,uint64_t> const blockfreqs =
							libmaus2::util::NumberMapSerialisation::deserialiseMap<libmaus2::aio::InputStreamInstance,int64_t,uint64_t>(freqCIS);
						// freqCIS.close();
						libmaus2::aio::FileRemoval::removeFile(freqstmpfilename.c_str());
						//
						uint64_t const sourcelengthbits = input_types_type::getSourceLengthBits(fn,blockstart,blockstart+cblocksize,blockfreqs);
						//
						uint64_t const sourcelengthbytes = input_types_type::getSourceLengthBytes(fn,blockstart,blockstart+cblocksize,blockfreqs);
						//
						uint64_t const sourcetextindexbits = input_types_type::getSourceTextIndexBits(fn,blockstart,blockstart+cblocksize,blockfreqs);

						libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type PMSB = libmaus2::suffixsort::bwtb3m::MergeStrategyBaseBlock::construct(
							blockstart,blockstart+cblocksize,
							*EC,
							blockfreqs,
							sourcelengthbits,sourcelengthbytes,
							sourcetextindexbits
						);

						/* set up and register sort request */
						::libmaus2::suffixsort::BwtMergeZBlockRequestVector zreqvec;
						dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyBaseBlock *>(PMSB.get())->sortreq =
							libmaus2::suffixsort::BwtMergeBlockSortRequest(
								input_types_type::getType(),
								fn,fs,
								chistfilename,
								huftreefilename,
								bwtterm,
								maxsym,
								blocktmpnames[b].serialise(),
								options.tmpfilenamebase /* only used if computeTermSymbolHwt == true */,
								rlencoderblocksize,
								preisasamplingrate,
								blockstart,cblocksize,
								zreqvec,
								options.computeTermSymbolHwt,
								boundedlcpblockvalues[b],
								options.numthreads, /* down stream threads */
								options.verbose
							);

						// if ( logstr ) { *logstr << *PMSB; }

						stratleafs[b] = PMSB;

						#if defined(BWTB3M_MEMORY_DEBUG)
						if ( logstr )
							*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						#endif
					}

					uhnode.reset();
					EC.reset();

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					if ( logstr )
						*logstr << "[V] constructing merge tree" << std::endl;

					// construct merge tree and register z requests
					libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type mergetree = libmaus2::suffixsort::bwtb3m::MergeStrategyConstruction::constructMergeTree(stratleafs,options.mem,options.numthreads,options.wordsperthread,logstr);

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						*logstr << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					// inner node queue
					std::deque<libmaus2::suffixsort::bwtb3m::MergeStrategyBlock *> itodo;

					if ( logstr )
						(*logstr) << "[V] sorting blocks" << std::endl;

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						(*logstr) << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					// sort single blocks
					libmaus2::suffixsort::bwtb3m::BaseBlockSorting::unique_ptr_type BBS(new libmaus2::suffixsort::bwtb3m::BaseBlockSorting(stratleafs,options.mem,options.numthreads,itodo,logstr,options.verbose));
					BBS->start();
					BBS->join();
					BBS.reset();

					if ( logstr )
						(*logstr) << "[V] sorted blocks" << std::endl;

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						(*logstr) << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					#if 0
					uint64_t maxhwtsize = 0;
					for ( uint64_t i = 0; i < stratleafs.size(); ++i )
						maxhwtsize = std::max(maxhwtsize,::libmaus2::util::GetFileSize::getFileSize(stratleafs[i]->sortresult.getFiles().getHWT()));
					#endif

					if ( logstr )
						(*logstr) << "[V] filling gap request objects" << std::endl;
					mergetree->fillGapRequestObjects(options.numthreads);

					#if defined(BWTB3M_MEMORY_DEBUG)
					if ( logstr )
						(*logstr) << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
					#endif

					if ( logstr )
					{
						(*logstr) << "[V]" << std::string(80,'-') << std::endl;
						(*logstr) << *mergetree;
						(*logstr) << "[V]" << std::string(80,'-') << std::endl;
					}

					uint64_t mtmpid = 0;
					uint64_t const lfblockmult = 1;

					/**
					 * process merge tree
					 **/
					while ( itodo.size() )
					{
						libmaus2::suffixsort::bwtb3m::MergeStrategyBlock * p = itodo.front();
						itodo.pop_front();

						if ( logstr )
						{
							(*logstr) << "[V] processing ";
							p->printBase((*logstr));
							(*logstr) << std::endl;
						}

						#if 0
						std::ostringstream tmpstr;
						tmpstr << options.tmpfilenamebase << "_" << std::setfill('0') << std::setw(6) << (mtmpid++);
						#endif

						std::ostringstream sparsetmpstr;
						sparsetmpstr << options.sparsetmpfilenamebase << "_" << std::setfill('0') << std::setw(6) << (mtmpid++);

						if ( dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalBlock *>(p) )
						{
							mergeBlocks(
								gtmpgen,
								*(dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalBlock *>(p)),
								fn,fs,
								// tmpstr.str(),
								rlencoderblocksize,
								lfblockmult,
								options.numthreads,
								chist,bwtterm,
								huftreefilename,
								logstr,
								options.verbose
							);
						}
						else if ( dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalSmallBlock *>(p) )
						{
							mergeBlocks(
								gtmpgen,
								*(dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalSmallBlock *>(p)),
								fn,fs,
								// tmpstr.str(),
								rlencoderblocksize,
								lfblockmult,
								options.numthreads,
								chist,bwtterm,
								huftreefilename,
								logstr,
								options.verbose
							);
						}
						else if ( dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyMergeExternalBlock *>(p) )
						{
							mergeBlocks(
								gtmpgen,
								*(dynamic_cast<libmaus2::suffixsort::bwtb3m::MergeStrategyMergeExternalBlock *>(p)),
								fn,fs,
								//tmpstr.str(),
								sparsetmpstr.str(),
								rlencoderblocksize,
								lfblockmult,
								options.numthreads,
								chist,bwtterm,
								options.mem,
								huftreefilename,
								logstr,
								options.verbose
							);
						}

						bool const pfinished = p->parent && p->parent->childFinished();

						if ( pfinished )
							itodo.push_back(p->parent);

						#if defined(BWTB3M_MEMORY_DEBUG)
						if ( logstr )
							(*logstr) << "[M"<< (mcnt++) << "] " << libmaus2::util::MemUsage() << " " << libmaus2::autoarray::AutoArrayMemUsage() << std::endl;
						#endif
					}

					uint64_t const memperthread = (options.mem + options.numthreads-1)/options.numthreads;

					::libmaus2::suffixsort::BwtMergeBlockSortResult const mergeresult = mergetree->sortresult;

					rl_encoder::concatenate(mergeresult.getFiles().getBWT(),options.outfn,true /* removeinput */);
					// libmaus2::aio::OutputStreamFactoryContainer::rename ( mergeresult.getFiles().getBWT().c_str(), outfn.c_str() );

					if ( logstr )
						(*logstr) << "[V] BWT computed in time " << bwtclock.formatTime(bwtclock.getElapsedSeconds()) << std::endl;

					// serialise character histogram
					std::string const outhist = ::libmaus2::util::OutputFileNameTools::clipOff(options.outfn,".bwt") + ".hist";
					::libmaus2::aio::OutputStreamInstance::unique_ptr_type Phistout(new ::libmaus2::aio::OutputStreamInstance(outhist));
					::libmaus2::util::NumberMapSerialisation::serialiseMap(*Phistout,chistnoterm);
					Phistout->flush();
					// Phistout->close();
					Phistout.reset();

					// remove hwt request for term symbol hwt
					libmaus2::aio::FileRemoval::removeFile ( mergeresult.getFiles().getHWTReq().c_str() );

					// remove hwt (null op)
					std::string const debhwt = ::libmaus2::util::OutputFileNameTools::clipOff(options.outfn,".bwt") + ".hwt" + ".deb";
					if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(mergeresult.getFiles().getHWT()) )
					{
						libmaus2::aio::OutputStreamFactoryContainer::rename ( mergeresult.getFiles().getHWT().c_str(), debhwt.c_str() );
						libmaus2::aio::FileRemoval::removeFile ( debhwt.c_str() );
					}

					// remove gt files
					for ( uint64_t i = 0; i < mergeresult.getFiles().getGT().size(); ++i )
						libmaus2::aio::FileRemoval::removeFile ( mergeresult.getFiles().getGT()[i].c_str() );

					BwtMergeSortResult result;

					if ( options.bwtonly )
					{
						std::vector<std::string> const Vmergedisaname = mergeresult.getFiles().getSampledISAVector();

						std::string const outisa = ::libmaus2::util::OutputFileNameTools::clipOff(options.outfn,".bwt") + ".preisa";

						if ( Vmergedisaname.size() == 1)
						{
							libmaus2::aio::OutputStreamFactoryContainer::rename(Vmergedisaname[0].c_str(),outisa.c_str());
						}
						else
						{
							libmaus2::aio::OutputStreamInstance OSI(outisa);

							for ( uint64_t i = 0; i < Vmergedisaname.size(); ++i )
							{
								libmaus2::aio::InputStreamInstance::unique_ptr_type ISI(new libmaus2::aio::InputStreamInstance(Vmergedisaname[i]));
								uint64_t const s = libmaus2::util::GetFileSize::getFileSize(*ISI);
								libmaus2::util::GetFileSize::copy(*ISI,OSI,s);
								ISI.reset();
								libmaus2::aio::FileRemoval::removeFile(Vmergedisaname[i]);
							}
						}

						std::string const outisameta = outisa + ".meta";
						libmaus2::aio::OutputStreamInstance outisametaOSI(outisameta);
						libmaus2::util::NumberSerialisation::serialiseNumber(outisametaOSI,preisasamplingrate);

						result = BwtMergeSortResult::setupBwtOnly(
							fn,
							options.outfn,
							outhist,
							outisameta,
							outisa
						);
					}
					else
					{
						if ( logstr )
							(*logstr) << "[V] computing Huffman shaped wavelet tree of final BWT...";
						std::string const outhwt = ::libmaus2::util::OutputFileNameTools::clipOff(options.outfn,".bwt") + ".hwt";
						libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type pICHWT;
						if ( input_types_type::utf8Wavelet() )
						{
							libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(
								libmaus2::wavelet::RlToHwtBase<true,rl_decoder>::rlToHwt(options.outfn, outhwt, options.tmpfilenamebase+"_finalhwttmp",options.numthreads)
							);
							pICHWT = UNIQUE_PTR_MOVE(tICHWT);
						}
						else
						{
							libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tICHWT(
								libmaus2::wavelet::RlToHwtBase<false,rl_decoder>::rlToHwt(options.outfn, outhwt, options.tmpfilenamebase+"_finalhwttmp",options.numthreads)
							);
							pICHWT = UNIQUE_PTR_MOVE(tICHWT);
						}
						if ( logstr )
							(*logstr) << "done, " << std::endl;

						if ( logstr )
							(*logstr) << "[V] loading Huffman shaped wavelet tree of final BWT...";
						::libmaus2::lf::ImpCompactHuffmanWaveletLF IHWT(pICHWT);
						if ( logstr )
							(*logstr) << "done." << std::endl;

						// sort the sampled isa file
						uint64_t const blockmem = memperthread; // memory per thread
						// sort pre isa files by second component (position)
						std::string const mergedisaname = sortIsaFile(gtmpgen,mergeresult.getFiles().getSampledISAVector(),blockmem,options.numthreads,logstr);

						// compute sampled suffix array and sampled inverse suffix array
						computeSampledSA(
							gtmpgen,fn,fs,IHWT,mergedisaname,options.outfn,
							options.numthreads,lfblockmult,options.sasamplingrate,options.isasamplingrate,
							options.mem,
							logstr
						);

						// std::cerr << "[V] mergeresult.blockp0rank=" << mergeresult.blockp0rank << std::endl;

						result = BwtMergeSortResult::setupBwtSa(
							fn,
							options.outfn,
							outhist,
							outhwt,
							::libmaus2::util::OutputFileNameTools::clipOff(options.outfn,".bwt") + ".sa",
							::libmaus2::util::OutputFileNameTools::clipOff(options.outfn,".bwt") + ".isa"
						);

						libmaus2::aio::FileRemoval::removeFile(mergedisaname);

						for ( uint64_t i = 0; i < mergeresult.getFiles().getSampledISAVector().size(); ++i )
							libmaus2::aio::FileRemoval::removeFile(mergeresult.getFiles().getSampledISAVector()[i]);
					}

					::libmaus2::aio::FileRemoval::removeFile(chistfilename);
					::libmaus2::aio::FileRemoval::removeFile(huftreefilename);
					libmaus2::aio::FileRemoval::removeFile(mergeresult.getFiles().getHist());

					#if 0
					std::ostringstream comstr;
					comstr << "ls -lR " << (options.tmpfilenamebase+"_tmpdir");
					system(comstr.str().c_str());
					#endif

					return result;
				}
			};
		}
	}
}
#endif
