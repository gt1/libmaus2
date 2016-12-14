/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_SMEMPROCESSOR_HPP)
#define LIBMAUS2_LCS_SMEMPROCESSOR_HPP

#include <libmaus2/lcs/ChainLeftMostComparator.hpp>
#include <libmaus2/lcs/ChainAddQueueElementHeapComparator.hpp>
#include <libmaus2/lcs/ChainAddQueueElement.hpp>
#include <libmaus2/lcs/ChainAlignment.hpp>
#include <libmaus2/lcs/Chain.hpp>
#include <libmaus2/lcs/ChainNode.hpp>
#include <libmaus2/lcs/ChainNodeInfo.hpp>
#include <libmaus2/lcs/ChainNodeInfoSet.hpp>
#include <libmaus2/lcs/ChainRemoveQueueElementComparator.hpp>
#include <libmaus2/lcs/ChainRemoveQueueElement.hpp>
#include <libmaus2/lcs/ChainSplayTreeNodeComparator.hpp>
#include <libmaus2/lcs/ChainSplayTreeNode.hpp>
#include <libmaus2/rank/DNARankSMEMComputation.hpp>
#include <libmaus2/fastx/FastAIndex.hpp>
#include <libmaus2/fastx/CoordinateCacheBiDir.hpp>

namespace libmaus2
{
	namespace lcs
	{
		template<typename _ssa_type = libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::BareSimpleSampledSuffixArray>
		struct SMEMProcessor
		{
			typedef _ssa_type ssa_type;
			typedef SMEMProcessor<ssa_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus2::fastx::DNAIndexMetaDataBigBandBiDir const & meta;
			libmaus2::fastx::CoordinateCacheBiDir const & cocache;
			libmaus2::rank::DNARank const & Prank;
			ssa_type const & BSSSA;
			char const * text;
			uint64_t const maxxdist;
			libmaus2::rank::DNARankGetPosition<ssa_type> GP;
			uint64_t const n;

			libmaus2::util::SplayTree<libmaus2::lcs::ChainSplayTreeNode,libmaus2::lcs::ChainSplayTreeNodeComparator> ST;
			libmaus2::util::FiniteSizeHeap<libmaus2::lcs::ChainAddQueueElement,libmaus2::lcs::ChainAddQueueElementHeapComparator> addQ;
			libmaus2::util::FiniteSizeHeap<libmaus2::lcs::ChainRemoveQueueElement,libmaus2::lcs::ChainRemoveQueueElementComparator> remQ;

			// rightmost,chainid pairs
			std::set< std::pair<uint64_t,uint64_t> > chainend;
			// chains
			std::map < uint64_t, libmaus2::lcs::Chain > chains;
			// chain left end
			std::map < uint64_t, uint64_t > chainleftend;

			libmaus2::lcs::ChainNodeInfoSet CNIS;
			libmaus2::util::ContainerElementFreeList<libmaus2::lcs::ChainNode> chainnodefreelist;
			libmaus2::autoarray::AutoArray<libmaus2::lcs::ChainNodeInfo> ACH;

			libmaus2::lcs::ChainLeftMostComparator CLMC;
			libmaus2::util::FiniteSizeHeap<libmaus2::lcs::Chain,libmaus2::lcs::ChainLeftMostComparator> chainQ;
			libmaus2::util::FiniteSizeHeap<uint64_t> chainleftpending;

			std::map < uint64_t, libmaus2::lcs::Chain > activechains;
			uint64_t const activemax;
			libmaus2::autoarray::AutoArray < uint64_t > activepop;

			uint64_t chainleftcheck;

			uint64_t fracmul;
			uint64_t fracdiv;

			bool selfcheck;

			uint64_t maxocc;

			uint64_t chaindommul;
			uint64_t chaindomdiv;

			bool domsameref;

			bool idle() const
			{
				return
					ST.idle()
					&&
					addQ.empty()
					&&
					remQ.empty()
					&&
					chainend.empty()
					&&
					chains.empty()
					&&
					chainleftend.empty()
					&&
					CNIS.idle()
					&&
					chainnodefreelist.idle()
					&&
					chainQ.empty()
					&&
					chainleftpending.empty()
					&&
					activechains.empty();
			}

			SMEMProcessor(
				//libmaus2::fastx::FastAIndex const & rfaindex,
				libmaus2::fastx::DNAIndexMetaDataBigBandBiDir const & rmeta,
				libmaus2::fastx::CoordinateCacheBiDir const & rcocache,
				libmaus2::rank::DNARank const & rPrank,
				ssa_type const & rBSSSA,
				char const * rtext,
				uint64_t const rmaxxdist,
				uint64_t const ractivemax,
				uint64_t const rfracmul,
				uint64_t const rfracdiv,
				bool const rselfcheck,
				uint64_t const rchainminscore,
				uint64_t const rmaxocc,
                                uint64_t const ralgndommul,
                                uint64_t const ralgndomdiv,
                                uint64_t const rchaindommul,
                                uint64_t const rchaindomdiv,
                                uint64_t const rmaxwerr,
                                uint64_t const rmaxback,
                                bool const rdomsameref
			) : // faindex(rfaindex),
			    meta(rmeta), cocache(rcocache), Prank(rPrank), BSSSA(rBSSSA), text(rtext), maxxdist(rmaxxdist), GP(Prank,BSSSA), n(Prank.size()), ST(), addQ(16*1024), remQ(16*1024),
			    //chainrightmost(),
			    chainend(), CNIS(n,text,meta,rchainminscore,rfracmul,rfracdiv,ralgndommul,ralgndomdiv,rmaxwerr,rmaxback,rdomsameref), ACH(),
			    // chainmeta(), chainmetao(0),
			    CLMC(&chainnodefreelist),
			    chainQ(16*1024,CLMC),
			    chainleftpending(16*1024),
			    activemax(ractivemax),
			    fracmul(rfracmul),
			    fracdiv(rfracdiv),
			    selfcheck(rselfcheck),
			    maxocc(rmaxocc),
			    chaindommul(rchaindommul),
			    chaindomdiv(rchaindomdiv),
			    domsameref(rdomsameref)
			{

			}

			void handleChainQ(char const * query, uint64_t const querysize, uint64_t const minlength, double const maxerr)
			{
				libmaus2::lcs::Chain chain = chainQ.pop();
				uint64_t const id = chain.getId(chainnodefreelist);

				// check order
				assert ( chain.getLeftMost(chainnodefreelist) >= chainleftcheck );
				chainleftcheck = chain.getLeftMost(chainnodefreelist);

				// find chains which are too far back from active list
				uint64_t activepopo = 0;
				for ( std::map<uint64_t,libmaus2::lcs::Chain>::const_iterator ita = activechains.begin(); ita != activechains.end(); ++ita )
					if ( ita->second.rightmost <= chainleftcheck )
						activepop.push(activepopo,ita->first);
				// process chains which we erase from active list
				for ( uint64_t i = 0; i < activepopo; ++i )
				{
					libmaus2::lcs::Chain backchain = activechains.find(activepop[i])->second;

					uint64_t const len = backchain.getInfo(chainnodefreelist, ACH);
					backchain.returnNodes(chainnodefreelist);

					CNIS.setup(ACH.get(),len);
					// std::cerr << "align " << backchain.getRefID() << std::endl;
					CNIS.align(query,querysize,minlength,maxerr,backchain.getRefID());

					activechains.erase(activepop[i]);
				}

				uint64_t covered = 0;
				activepopo = 0;

				libmaus2::math::IntegerInterval<int64_t> Icur(
					chain.getLeftMost(chainnodefreelist),
					static_cast<int64_t>(chain.rightmost-1)
				);

				// iterate over active chains
				for ( std::map<uint64_t,libmaus2::lcs::Chain>::iterator ita = activechains.begin(); ita != activechains.end(); ++ita )
				{
					libmaus2::lcs::Chain & backchain = ita->second;

					// interval for chain in active list
					libmaus2::math::IntegerInterval<int64_t> Iback(backchain.getLeftMost(chainnodefreelist),static_cast<int64_t>(backchain.rightmost-1));
					// intersect with interval of new chain
					libmaus2::math::IntegerInterval<int64_t> Isec = Icur.intersection(Iback);

					if (
						((!domsameref) || (ita->second.getRefID() == chain.getRefID()) )
						&&
						Isec.diameter() >= static_cast<int64_t>((Icur.diameter() * fracmul)/fracdiv)
						&&
						(chaindommul * backchain.getRange(chainnodefreelist))/chaindomdiv > chain.getRange(chainnodefreelist)
						&&
						(chaindommul * backchain.getChainScore(chainnodefreelist))/chaindomdiv > chain.getChainScore(chainnodefreelist)
					)
					{
						//std::cerr << "chain " << chain << " covered by " << backchain << std::endl;
						covered++;
					}
					else if (
						((!domsameref) || (ita->second.getRefID() == chain.getRefID()) )
						&&
						Isec.diameter() >= static_cast<int64_t>((Iback.diameter() * fracmul)/fracdiv)
						&&
						(chaindommul * chain.getRange(chainnodefreelist))/chaindomdiv > backchain.getRange(chainnodefreelist)
						&&
						(chaindommul * chain.getChainScore(chainnodefreelist))/chaindomdiv > backchain.getChainScore(chainnodefreelist)
					)
					{
						// std::cerr << "oldchain " << backchain << " covered by " << chain << std::endl;
						backchain.covercount += 1;
						if ( backchain.covercount >= activemax )
							activepop.push(activepopo,ita->first);
					}
				}

				chain.covercount = covered;

				if ( chain.covercount < activemax )
					activechains [ id ] = chain;
				else
					chain.returnNodes(chainnodefreelist);

				for ( uint64_t i = 0; i < activepopo; ++i )
				{
					libmaus2::lcs::Chain backchain = activechains.find(activepop[i])->second;
					backchain.returnNodes(chainnodefreelist);
					activechains.erase(activepop[i]);
				}
			}

			void handleChainEnd(char const * query, uint64_t const querysize, uint64_t const minlength, double const maxerr)
			{
				uint64_t const rightmost = chainend.begin()->first;
				uint64_t const chainid = chainend.begin()->second;
				chainend.erase(std::pair<uint64_t,uint64_t>(rightmost,chainid));

				assert ( chains.find(chainid) != chains.end() );
				libmaus2::lcs::Chain const & chain = chains.find(chainid)->second;
				uint64_t const chainleftmost = chain.getLeftMost(chainnodefreelist);
				uint64_t const refid = chain.getRefID();

				uint64_t const len = chain.getInfo(chainnodefreelist, ACH);
				assert ( len == chain.length );
				assert ( len );
				chains.find(chainid)->second.returnNodes(chainnodefreelist);
				chains.erase(chainid);

				CNIS.setup(ACH.get(),len);

				if ( CNIS.isValid() )
				{
					libmaus2::lcs::Chain NC;
					NC.refid = refid;
					CNIS.copyback(chainnodefreelist,NC,chainid);
					chainQ.pushBump(NC);
				}

				{
					assert ( chainleftend.find(chainleftmost) != chainleftend.end() );
					chainleftend[chainleftmost]--;
					if (
						chainleftend.find(chainleftmost)->second == 0
					)
					{
						chainleftend.erase(chainleftmost);
						chainleftpending.pushBump(chainleftmost);

						while (
							(!chainleftpending.empty())
							&&
							(
								chainleftend.empty()
								||
								chainleftpending.top() < chainleftend.begin()->first
							)
						)
						{
							uint64_t const chainproc = chainleftpending.pop();

							while ( ! chainQ.empty() && chainQ.top().getLeftMost(chainnodefreelist) < chainproc )
								handleChainQ(query,querysize,minlength,maxerr);
						}
					}

					// std::cerr << "end of chain " << chainid << " rightmost " << rightmost << std::endl;
					#if 0
					if ( count > 1 )
					{
						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
						std::cerr << "[V] end of chain " << chainid << " of count " << count << std::endl;
					}
					#endif
				}

			}

			uint64_t mapSeq(uint64_t const id) const
			{
				if ( id < meta.S.size() / 2 )
					return id;
				else
					return meta.S.size() - id - 1;
			}

			template<typename enumerator_type>
			void process(
				enumerator_type & senum,
				char const * query,
				uint64_t const querysize,
				uint64_t const minlength = 0,
				double const maxerr = std::numeric_limits<double>::max()
			)
			{
				assert ( idle() );
				// uint64_t const maxocc = 500;

				ST.clear();
				addQ.clear();
				remQ.clear();
				chainend.clear();
				chains.clear();
				CNIS.reset();
				chainleftpending.clear();
				activechains.clear();
				chainleftcheck = 0;

				uint64_t nextchainid = 0;
				libmaus2::rank::DNARankMEM smem;
				for ( uint64_t z = 0; (senum.getNext(smem)); ++z )
				{
					#if 0
					if ( z % (16*1024) == 0 )
						std::cerr << "smem " << smem << " addQ=" << addQ.f << " remQ=" << remQ.f << std::endl;
					#endif

					// std::cerr << "proc smem " << smem << std::endl;

					uint64_t const xleft = smem.left;
					uint64_t const xright = smem.right;

					// process add queue
					while ( (!addQ.empty()) && (addQ.top().xright <= xleft) )
					{
						// get element
						libmaus2::lcs::ChainAddQueueElement const & Q = addQ.top();
						// compute element length
						uint64_t const addsmemlen = Q.xright-Q.xleft;

						// get position
						uint64_t const yleft = Q.yleft;
						uint64_t const yright = yleft + addsmemlen;

						// assert ( yleft != Q.xleft );

						// add into splay tree
						ST.insert(libmaus2::lcs::ChainSplayTreeNode(Q.xright,yright,Q.chainid,Q.chainsubid));

						// add to remove queue
						libmaus2::lcs::ChainRemoveQueueElement RQE(
							Q.xright /* right end on x */,
							yright /* right end on y */,
							Q.chainid,Q.chainsubid
						);
						//std::cerr << "RQE " << RQE << std::endl;
						remQ.pushBump(RQE);

						addQ.popvoid();
					}

					// process remove queue
					while ( (!remQ.empty()) && (remQ.top().xright + maxxdist < xleft) )
					{
						libmaus2::lcs::ChainRemoveQueueElement const & R = remQ.top();

						// std::cerr << "removing " << R << std::endl;

						libmaus2::util::SplayTree<libmaus2::lcs::ChainSplayTreeNode,libmaus2::lcs::ChainSplayTreeNodeComparator>::node_id_type s_id =
							ST.find(libmaus2::lcs::ChainSplayTreeNode(R.xright,R.yright,R.chainid,R.chainsubid));

						if ( s_id >= 0 )
						{
							ST.erase(libmaus2::lcs::ChainSplayTreeNode(R.xright,R.yright,R.chainid,R.chainsubid));
						}
						else
						{
							std::cerr << "[E] failed to find " << R << " in SplayTree" << std::endl;
							assert ( s_id >= 0 );
						}

						remQ.popvoid();
					}

					while ( (!chainend.empty()) && (chainend.begin()->first+maxxdist < xleft) )
						handleChainEnd(query,querysize,minlength,maxerr);

					uint64_t const inc = (smem.intv.size < maxocc) ? 1 : ((smem.intv.size+maxocc-1)/maxocc);
					for ( uint64_t i = 0; i < smem.intv.size; i += inc )
					{
						// get (Y) position
						uint64_t const p = GP.getPosition(smem.intv.forw + i);

						std::pair<uint64_t,uint64_t> const coL = cocache[p];
						std::pair<uint64_t,uint64_t> const coR = cocache[((p+(xright-xleft))+n-1)%n];

						if (
							// do not consider seeds spanning over a refseq boundary
							(coL.first == coR.first)
							&&
							((!selfcheck) || (p != smem.left))
						)
						{
							uint64_t chainid;
							uint64_t chainsubid;
							uint64_t parentsubid;
							// std::cerr << "[V] trying to link up " << libmaus2::lcs::ChainAddQueueElement(xright,p,z /* mem id */,i /* seed id */,0 /* chain id*/,0/*chain sub id*/) << std::endl;

							// find largest entry suitable for p
							libmaus2::util::SplayTree<libmaus2::lcs::ChainSplayTreeNode,libmaus2::lcs::ChainSplayTreeNodeComparator>::node_id_type s_init =
								ST.chain(libmaus2::lcs::ChainSplayTreeNode(std::numeric_limits<uint64_t>::max(),p,0,0));
							int64_t s_id = -1;

							assert ( s_init == -1 || ST.getKey(s_init).yright <= p );

							#if defined(LIBMAUS2_CHAIN_LINK_DEBUG)
							{
								libmaus2::fastx::DNAIndexMetaDataBigBandBiDir::Coordinates CO =
									meta.mapCoordinatePair(p,p+(xright-xleft)-1);

								std::cerr
									<< "trying to link up x=[" << xleft << "," << xright << ") y="
									<< "([" << CO.seq << "," << CO.rc << "," << CO.left << "," << CO.left+CO.length+1 << ")"
									<< std::endl;
							}
							#endif

							if ( s_init >= 0 )
							{
								libmaus2::lcs::ChainSplayTreeNode const & K = ST.getKey(s_init);


								assert ( p >= K.yright );
								assert ( xleft >= K.xright );

								// maximum available y still too low?
								if ( p - K.yright > maxxdist )
									s_init = -1;
								else
								{
									uint64_t mindif = std::numeric_limits<uint64_t>::max();

									for ( int64_t s_cur = s_init;
										s_cur >= 0 &&
										p - ST.getKey(s_cur).yright <= maxxdist;
										s_cur = ST.getPrev(s_cur)
									)
									{
										libmaus2::lcs::ChainSplayTreeNode const & K = ST.getKey(s_cur);

										assert ( xleft >= K.xright );
										uint64_t const xdif = xleft - K.xright;
										assert ( p >= K.yright );
										uint64_t const ydif = p - K.yright;

										uint64_t const ddif = std::max(xdif,ydif) - std::min(xdif,ydif);

										std::pair < uint64_t, uint64_t> const coP = cocache[(K.yright+n-1)%n];

										if (
											coP.first == coL.first
											&&
											ddif < mindif
										)
										{
											s_id = s_cur;
											mindif = ddif;
										}

										#if defined(LIBMAUS2_CHAIN_LINK_DEBUG)
										{
											libmaus2::fastx::DNAIndexMetaDataBigBandBiDir::Coordinates CO =
												meta.mapCoordinatePair((K.yright+n-1)%n,K.yright);

											std::cerr
												<< "could try to xright=" << K.xright << " yright="
												<< "[" << CO.seq << "," << CO.rc << "," << CO.left+1 << ")"
												<< " ddif=" << ddif
												<< std::endl;
										}
										#endif
									}
								}
							}

							#if defined(LIBMAUS2_CHAIN_LINK_DEBUG)
							if ( s_id >= 0 )
							{
								libmaus2::lcs::ChainSplayTreeNode const & K = ST.getKey(s_id);

								libmaus2::fastx::DNAIndexMetaDataBigBandBiDir::Coordinates CO =
									meta.mapCoordinatePair((K.yright+n-1)%n,K.yright);

								std::cerr
									<< "linking to xright=" << K.xright << " yright="
									<< "[" << CO.seq
									<< "," << CO.rc
									<< "," << CO.left+1 << ")"
									<< std::endl;
							}
							#endif

							// if any
							if ( s_id >= 0 )
							{
								// get key
								libmaus2::lcs::ChainSplayTreeNode const & K = ST.getKey(s_id);

								assert ( p >= K.yright );
								assert ( xleft >= K.xright );

								chainid = K.chainid;

								assert ( chains.find(chainid) != chains.end() );

								chainsubid = chains.find(chainid)->second.length;
								parentsubid = K.chainsubid;

								uint64_t const oldrightmost = chains.find(chainid)->second.rightmost;
								assert ( chainend.find(std::pair<uint64_t,uint64_t>(oldrightmost,chainid)) != chainend.end() );
								chainend.erase(std::pair<uint64_t,uint64_t>(oldrightmost,chainid));

								uint64_t const newrightmost = std::max(smem.right,oldrightmost);
								chainend.insert(std::pair<uint64_t,uint64_t>(newrightmost,chainid));

								chains [ chainid ] .rightmost = newrightmost;
							}
							else
							{
								chainid = nextchainid++;
								chainsubid = 0;
								parentsubid = std::numeric_limits<uint64_t>::max();

								assert ( chains.find(chainid) == chains.end() );

								libmaus2::lcs::Chain nchain;
								nchain.rightmost = smem.right;
								nchain.refid = coL.first;

								chains [ chainid ] = nchain;
								chainleftend [ smem.left ] ++;
								chainend.insert(std::pair<uint64_t,uint64_t>(smem.right,chainid));
							}

							libmaus2::lcs::ChainNode CN(chainid,chainsubid,parentsubid,smem.left,smem.right,p,p+(smem.right-smem.left));
							chains[chainid].push(chainnodefreelist,CN);

							libmaus2::lcs::ChainAddQueueElement AQE(xleft,xright,p,chainid,chainsubid);
							// put element in add queue
							addQ.pushBump(AQE);

							// std::cerr << "added " << AQE << std::endl;
						}
					}
				}

				// process add queue
				while ( !addQ.empty() )
				{
					// get element
					libmaus2::lcs::ChainAddQueueElement const & Q = addQ.top();
					// compute element length
					uint64_t const addsmemlen = Q.xright-Q.xleft;

					// get position
					uint64_t const yleft = Q.yleft;
					uint64_t const yright = yleft + addsmemlen;

					// assert ( yleft != Q.xleft );

					// add into splay tree
					ST.insert(libmaus2::lcs::ChainSplayTreeNode(Q.xright,yright,Q.chainid,Q.chainsubid));

					// add to remove queue
					libmaus2::lcs::ChainRemoveQueueElement RQE(
						Q.xright /* right end on x */,
						yright /* right end on y */,
						Q.chainid,Q.chainsubid
					);
					//std::cerr << "RQE " << RQE << std::endl;
					remQ.pushBump(RQE);

					addQ.popvoid();
				}

				// process remove queue
				while ( !remQ.empty() )
				{
					libmaus2::lcs::ChainRemoveQueueElement const & R = remQ.top();

					// std::cerr << "removing " << R << std::endl;

					libmaus2::util::SplayTree<libmaus2::lcs::ChainSplayTreeNode,libmaus2::lcs::ChainSplayTreeNodeComparator>::node_id_type s_id =
						ST.find(libmaus2::lcs::ChainSplayTreeNode(R.xright,R.yright,R.chainid,R.chainsubid));

					if ( s_id >= 0 )
					{
						ST.erase(libmaus2::lcs::ChainSplayTreeNode(R.xright,R.yright,R.chainid,R.chainsubid));
					}
					else
					{
						std::cerr << "[E] failed to find " << R << " in SplayTree" << std::endl;
						assert ( s_id >= 0 );
					}
					remQ.popvoid();
				}

				while ( !chainend.empty() )
					handleChainEnd(query,querysize,minlength,maxerr);
				while ( ! chainQ.empty() )
					handleChainQ(query,querysize,minlength,maxerr);

				for ( std::map<uint64_t,libmaus2::lcs::Chain>::const_iterator ita = activechains.begin(); ita != activechains.end(); ++ita )
				{
					libmaus2::lcs::Chain backchain = ita->second;

					uint64_t const len = backchain.getInfo(chainnodefreelist, ACH);
					backchain.returnNodes(chainnodefreelist);

					CNIS.setup(ACH.get(),len);
					CNIS.align(query,querysize,minlength,maxerr,backchain.refid);
				}

				activechains.clear();
				CNIS.cleanup();

				assert ( idle() );
			}

			void printAlignments(uint64_t const minlength = 0)
			{
				CNIS.printAlignments(minlength);
			}
		};
	}
}
#endif
