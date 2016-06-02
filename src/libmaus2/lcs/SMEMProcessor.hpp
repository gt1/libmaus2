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

namespace libmaus2
{
	namespace lcs
	{
		struct SMEMProcessor
		{
			typedef SMEMProcessor this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			libmaus2::rank::DNARank const & Prank;
			libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::BareSimpleSampledSuffixArray const & BSSSA;
			char const * text;
			uint64_t const maxxdist;
			libmaus2::rank::DNARankGetPosition GP;
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

			SMEMProcessor(
				libmaus2::rank::DNARank const & rPrank,
				libmaus2::suffixsort::bwtb3m::BwtMergeSortResult::BareSimpleSampledSuffixArray const & rBSSSA,
				char const * rtext,
				uint64_t const rmaxxdist,
				uint64_t const ractivemax,
				uint64_t const rfracmul,
				uint64_t const rfracdiv,
				bool const rselfcheck
			) : Prank(rPrank), BSSSA(rBSSSA), text(rtext), maxxdist(rmaxxdist), GP(Prank,BSSSA), n(Prank.size()), ST(), addQ(16*1024), remQ(16*1024),
			    //chainrightmost(),
			    chainend(), CNIS(n,text), ACH(),
			    // chainmeta(), chainmetao(0),
			    CLMC(&chainnodefreelist),
			    chainQ(16*1024,CLMC),
			    chainleftpending(16*1024),
			    activemax(ractivemax),
			    fracmul(rfracmul),
			    fracdiv(rfracdiv),
			    selfcheck(rselfcheck)
			{

			}

			void handleChainQ()
			{
				libmaus2::lcs::Chain chain = chainQ.pop();
				uint64_t const id = chain.getId(chainnodefreelist);

				// check order
				assert ( chain.getLeftMost(chainnodefreelist) >= chainleftcheck );
				chainleftcheck = chain.getLeftMost(chainnodefreelist);

				// erase chains which are too far back from active list
				uint64_t activepopo = 0;
				for ( std::map<uint64_t,libmaus2::lcs::Chain>::const_iterator ita = activechains.begin(); ita != activechains.end(); ++ita )
					if ( ita->second.rightmost <= chainleftcheck )
						activepop.push(activepopo,ita->first);
				//
				for ( uint64_t i = 0; i < activepopo; ++i )
				{
					libmaus2::lcs::Chain backchain = activechains.find(activepop[i])->second;

					uint64_t const len = backchain.getInfo(chainnodefreelist, ACH);
					backchain.returnNodes(chainnodefreelist);

					CNIS.setup(ACH.get(),len);
					CNIS.align();

					activechains.erase(activepop[i]);
				}

				uint64_t covered = 0;
				activepopo = 0;

				libmaus2::math::IntegerInterval<int64_t> Icur(
					chain.getLeftMost(chainnodefreelist),
					static_cast<int64_t>(chain.rightmost-1)
				);

				for ( std::map<uint64_t,libmaus2::lcs::Chain>::iterator ita = activechains.begin(); ita != activechains.end(); ++ita )
				{
					libmaus2::lcs::Chain & backchain = ita->second;

					libmaus2::math::IntegerInterval<int64_t> Iback(
						backchain.getLeftMost(chainnodefreelist),
						static_cast<int64_t>(backchain.rightmost-1)
					);

					libmaus2::math::IntegerInterval<int64_t> Isec = Icur.intersection(Iback);

					if (
						Isec.diameter() >= static_cast<int64_t>((Icur.diameter() * fracmul)/fracdiv)
						&&
						backchain.getChainScore(chainnodefreelist) > chain.getChainScore(chainnodefreelist)
					)
					{
						//std::cerr << "chain " << chain << " covered by " << backchain << std::endl;
						covered++;
					}
					else if (
						Isec.diameter() >= static_cast<int64_t>((Iback.diameter() * fracmul)/fracdiv)
						&&
						chain.getChainScore(chainnodefreelist) > backchain.getChainScore(chainnodefreelist)
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

			void handleChainEnd()
			{
				uint64_t const rightmost = chainend.begin()->first;
				uint64_t const chainid = chainend.begin()->second;
				chainend.erase(std::pair<uint64_t,uint64_t>(rightmost,chainid));

				assert ( chains.find(chainid) != chains.end() );
				libmaus2::lcs::Chain const & chain = chains.find(chainid)->second;
				uint64_t const chainleftmost = chain.getLeftMost(chainnodefreelist);

				uint64_t const len = chain.getInfo(chainnodefreelist, ACH);
				assert ( len == chain.length );
				assert ( len );
				chains.find(chainid)->second.returnNodes(chainnodefreelist);
				chains.erase(chainid);

				CNIS.setup(ACH.get(),len);

				if ( CNIS.isValid() )
				{
					libmaus2::lcs::Chain NC;
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
								handleChainQ();
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

			void process(libmaus2::rank::DNARankSMEMComputation::SMEMEnumerator<char const *> & senum)
			{
				uint64_t const maxocc = 500;

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
						handleChainEnd();

					uint64_t const inc = (smem.intv.size < maxocc) ? 1 : ((smem.intv.size+maxocc-1)/maxocc);
					for ( uint64_t i = 0; i < smem.intv.size; i += inc )
					{
						// get (Y) position
						uint64_t const p = GP.getPosition(smem.intv.forw + i);

						if ( (!selfcheck) || (p != smem.left) )
						{
							uint64_t chainid;
							uint64_t chainsubid;
							uint64_t parentsubid;
							// std::cerr << "[V] trying to link up " << libmaus2::lcs::ChainAddQueueElement(xright,p,z /* mem id */,i /* seed id */,0 /* chain id*/,0/*chain sub id*/) << std::endl;

							// find largest entry suitable for p
							libmaus2::util::SplayTree<libmaus2::lcs::ChainSplayTreeNode,libmaus2::lcs::ChainSplayTreeNodeComparator>::node_id_type s_id = ST.chain(libmaus2::lcs::ChainSplayTreeNode(std::numeric_limits<uint64_t>::max(),p,0,0));

							assert ( s_id == -1 || ST.getKey(s_id).yright <= p );

							if ( s_id >= 0 )
							{
								libmaus2::lcs::ChainSplayTreeNode const & K = ST.getKey(s_id);

								assert ( p >= K.yright );
								assert ( xleft >= K.xright );

								if ( p - K.yright > maxxdist )
									s_id = -1;
							}

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

								chains [ chainid ] = libmaus2::lcs::Chain();
								chainleftend [ smem.left ] ++;
								chains [ chainid ].rightmost = smem.right;
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

				while ( !chainend.empty() )
					handleChainEnd();
				while ( ! chainQ.empty() )
					handleChainQ();

				for ( std::map<uint64_t,libmaus2::lcs::Chain>::const_iterator ita = activechains.begin(); ita != activechains.end(); ++ita )
				{
					libmaus2::lcs::Chain backchain = ita->second;

					uint64_t const len = backchain.getInfo(chainnodefreelist, ACH);
					backchain.returnNodes(chainnodefreelist);

					CNIS.setup(ACH.get(),len);
					CNIS.align();
				}

				activechains.clear();

				CNIS.cleanup();

				assert ( chainleftend.size() == 0 );
			}

			void printAlignments()
			{
				CNIS.printAlignments();
			}
		};
	}
}
#endif
