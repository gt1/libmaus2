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
#if ! defined(LIBMAUS2_LCS_CHAINNODEINFOSET_HPP)
#define LIBMAUS2_LCS_CHAINNODEINFOSET_HPP

#include <libmaus2/geometry/RangeSet.hpp>
#include <libmaus2/lcs/NNPTraceContainer.hpp>
#include <libmaus2/lcs/NNP.hpp>
#include <libmaus2/util/SimpleQueue.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <iomanip>
#include <set>
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSortResult.hpp>
#include <libmaus2/rank/DNARankGetPosition.hpp>
#include <libmaus2/util/SplayTree.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>
#include <libmaus2/fastx/DNAIndexMetaDataBigBandBiDir.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct ChainNodeInfoSet
		{
			libmaus2::lcs::ChainNodeInfo * CNI;
			uint64_t o;
			uint64_t const minscore;
			uint64_t n;
			char const * text;

			libmaus2::fastx::DNAIndexMetaDataBigBandBiDir const & meta;

			libmaus2::autoarray::AutoArray<uint64_t> Anumchildren;
			libmaus2::autoarray::AutoArray<uint64_t> Aforward;

			libmaus2::autoarray::AutoArray < std::pair < libmaus2::lcs::ChainNodeInfo * , uint64_t > > Asort;
			libmaus2::autoarray::AutoArray < uint64_t > Amergechildlist;

			libmaus2::geometry::RangeSet<libmaus2::lcs::NNPTraceContainer::DiagStrip> RS;
			libmaus2::util::SimpleQueue< libmaus2::geometry::RangeSet<libmaus2::lcs::NNPTraceContainer::DiagStrip>::search_q_element > RSQ;

			libmaus2::autoarray::AutoArray < libmaus2::lcs::ChainAlignment > Aalgn;
			uint64_t aalgno;
			libmaus2::lcs::NNP nnp;
			libmaus2::lcs::NNPTraceContainer nnptrace;

			libmaus2::autoarray::AutoArray<libmaus2::lcs::NNPTraceContainer::DiagStrip> D;

			libmaus2::util::SimpleQueue<uint64_t> simpQ;

			libmaus2::util::SimpleQueue<uint64_t> calctodo;
			libmaus2::util::SimpleQueue<uint64_t> calcRtodo;

			libmaus2::util::SimpleQueue<uint64_t> aligntodoQ;
			libmaus2::util::SimpleQueue<uint64_t> alignVtodo;
			libmaus2::util::SimpleQueue<uint64_t> alignVtodoout;

			libmaus2::autoarray::AutoArray < libmaus2::lcs::ChainAlignment > Aalgnfilter;

			uint64_t fracmul;
			uint64_t fracdiv;

			#if defined(CHAINNODEINFOSET_DOT)
			uint64_t chaindot;
			#endif

			void reset()
			{
				aalgno = 0;
			}

			struct ChainNodeInfoSortComparator
			{
				bool operator()(std::pair < libmaus2::lcs::ChainNodeInfo * , uint64_t > const & A, std::pair < libmaus2::lcs::ChainNodeInfo * , uint64_t > const & B) const
				{
					int64_t const diagA = A.first->diag();
					int64_t const diagB = B.first->diag();

					if ( diagA != diagB )
						return diagA < diagB;
					else if ( A.first->xleft != B.first->xleft )
						return A.first->xleft < B.first->xleft;
					else if ( A.first->xright != B.first->xright )
						return A.first->xright > B.first->xright;
					else
						return A.second < B.second;
				}
			};

			ChainNodeInfoSet(
				uint64_t const rn,
				char const * rtext,
				libmaus2::fastx::DNAIndexMetaDataBigBandBiDir const & rmeta,
				uint64_t const rminscore,
				uint64_t const rfracmul,
				uint64_t const rfracdiv
			) : minscore(rminscore), n(rn), text(rtext), meta(rmeta), RS(n), fracmul(rfracmul), fracdiv(rfracdiv)
				#if defined(CHAINNODEINFOSET_DOT)
				,chaindot(0)
				#endif
			{

			}

			void computeNumChildren()
			{
				std::fill(Anumchildren.begin(),Anumchildren.begin()+o+1,0ull);

				for ( uint64_t i = 0; i < o; ++i )
					if ( CNI[i].parentsubid < o && CNI[i].xright-CNI[i].xleft )
						Anumchildren[CNI[i].parentsubid]++;

				libmaus2::util::PrefixSums::prefixSums(Anumchildren.begin(),Anumchildren.begin()+o+1);
			}

			void computeForwardLinks()
			{
				computeNumChildren();
				Aforward.ensureSize(Anumchildren[o]);

				for ( uint64_t i = 0; i < o; ++i )
					if ( CNI[i].parentsubid < o && CNI[i].xright-CNI[i].xleft )
						Aforward [ Anumchildren[CNI[i].parentsubid] ++ ] = i;

				computeNumChildren();
			}

			uint64_t getNumChildren(uint64_t const i) const
			{
				return Anumchildren[i+1]-Anumchildren[i];
			}

			uint64_t getChild(uint64_t const i, uint64_t const j) const
			{
				return Aforward [ Anumchildren[i] + j ];
			}

			libmaus2::autoarray::AutoArray<uint64_t> C;

			void compactify()
			{
				// ensure rename array is large enough
				C.ensureSize(o);

				// compute names of non empty intervals
				uint64_t ni = 0;
				for ( uint64_t i = 0; i < o; ++i )
					// non empty?
					if ( CNI[i].xright != CNI[i].xleft )
						C[i] = ni++;
					// empty
					else
						C[i] = std::numeric_limits<uint64_t>::max();

				// update parent pointers for compacted array
				for ( uint64_t i = 0; i < o; ++i )
					if ( CNI[i].parentsubid < o )
						CNI[i].parentsubid = C[CNI[i].parentsubid];

				ni = 0;
				for ( uint64_t i = 0; i < o; ++i )
					if ( CNI[i].xright != CNI[i].xleft )
						CNI[ni++] = CNI[i];

				o = ni;
			}

			void simplify()
			{
				bool changed = true;

				while ( changed )
				{
					changed = false;

					simpQ.clear();

					if ( o )
						simpQ.push_back(0);
						// todo.push_back(0);

					while ( ! simpQ.empty() )
					{
						uint64_t const cur = simpQ.front();
						simpQ.pop_front();

						uint64_t const nc = getNumChildren(cur);
						Asort.ensureSize(nc);

						uint64_t o = 0;
						for ( uint64_t i = 0; i < getNumChildren(cur); ++i )
						{
							Asort [ o ++ ] = std::pair < libmaus2::lcs::ChainNodeInfo * , uint64_t > ( CNI + getChild(cur,i), i );
							simpQ.push_back(getChild(cur,i));
						}

						// sort by diagonal, xleft, xright, child id
						std::sort(Asort.begin(),Asort.begin()+o,ChainNodeInfoSortComparator());

						uint64_t low = 0;
						while ( low < o )
						{
							// look for end of diagonal
							uint64_t high = low+1;
							while ( high < o && Asort[low].first->diag() == Asort[high].first->diag() )
								++high;

							if ( high-low > 1 )
							{
								uint64_t clow = low;
								while ( clow < high )
								{
									uint64_t chigh = clow+1;
									uint64_t xend = Asort[clow].first->xright;
									while ( chigh < high && Asort[chigh].first->xleft < xend )
									{
										xend = std::max(xend,Asort[chigh].first->xright);
										++chigh;
									}

									if ( chigh-clow > 1 )
									{
										uint64_t xmin = Asort[clow].first->xleft;
										uint64_t xmax = Asort[clow].first->xright;
										uint64_t ymin = Asort[clow].first->yleft;
										uint64_t ymax = Asort[clow].first->yright;

										//std::cerr << "MERGE" << std::endl;
										// std::cerr << (*(Asort[clow].first)) << std::endl;

										for ( uint64_t i = clow+1; i < chigh; ++i )
										{
											xmin = std::min(xmin,Asort[i].first->xleft);
											xmax = std::max(xmax,Asort[i].first->xright);
											ymin = std::min(ymin,Asort[i].first->yleft);
											ymax = std::max(ymax,Asort[i].first->yright);

											//std::cerr << (*(Asort[i].first)) << std::endl;
										}

										// get all children
										uint64_t oc = 0;
										for ( uint64_t i = clow; i < chigh; ++i )
										{
											uint64_t const sibid = getChild(cur,Asort[i].second);

											for ( uint64_t j = 0; j < getNumChildren(sibid); ++j )
												Amergechildlist.push(oc,getChild(sibid,j));
										}

										// first sibling
										uint64_t const firstsib = getChild(cur,Asort[clow].second);

										// set parent for all children
										for ( uint64_t i = 0; i < oc; ++i )
											CNI[ Amergechildlist[i] ] . parentsubid = firstsib;

										// erase all but first
										for ( uint64_t i = clow+1; i < chigh; ++i )
											Asort[i].first->xleft = Asort[i].first->xright =
											Asort[i].first->yleft = Asort[i].first->yright = 0;

										// reset range of first sibling
										Asort[clow].first->xleft = xmin;
										Asort[clow].first->xright = xmax;
										Asort[clow].first->yleft = ymin;
										Asort[clow].first->yright = ymax;

										changed = true;

										// std::cerr << "xmin=" << xmin << " xmax=" << xmax << " ymin=" << ymin << " ymax=" << ymax << std::endl;
									}

									clow = chigh;
								}

							}

							low = high;
						}
					}

					if ( changed )
						computeForwardLinks();
				}

				compactify();
				computeForwardLinks();
			}

			void calculateScore()
			{
				if ( o )
					calctodo.push_back(0);
					//todo.push_back(0);

				while ( ! calctodo.empty() )
				{
					uint64_t const cur = calctodo.front();
					calctodo.pop_front();
					calcRtodo.push_back(cur);

					CNI[cur].chainscore = CNI[cur].xright-CNI[cur].xleft;

					for ( uint64_t i = 0; i < getNumChildren(cur); ++i )
						calctodo.push_back(getChild(cur,i));
				}

				calcRtodo.reverse();

				while ( ! calcRtodo.empty() )
				{
					uint64_t const cur = calcRtodo.front();
					calcRtodo.pop_front();

					libmaus2::lcs::ChainNodeInfo & node = CNI[cur];

					uint64_t childscore = node.chainscore;

					for ( uint64_t k = 0; k < getNumChildren(cur); ++k )
					{
						libmaus2::lcs::ChainNodeInfo & childnode = CNI[getChild(cur,k)];

						// no overlap
						if ( node.xright <= childnode.xleft )
						{
							childscore = std::max(childscore,childnode.chainscore+static_cast<uint64_t>(node.chainscore));
						}
						// overlap
						else
						{
							uint64_t const thisend = childnode.xleft;

							if ( thisend >= node.xleft )
							{
								assert ( node.xright >= childnode.xleft );

								childscore =
									std::max(
										childscore,
										childnode.chainscore + static_cast<uint64_t>(node.chainscore) - (node.xright-childnode.xleft)
									);
							}
							else
							{
								libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
								std::cerr << "[W] node " << node << " child " << childnode << std::endl;
							}
						}
					}

					node.chainscore = childscore;
				}
			}

			void print()
			{
				std::deque<uint64_t> todo;
				if ( o )
					todo.push_back(0);

				while ( todo.size() )
				{
					uint64_t const cur = todo.front();
					todo.pop_front();

					for ( uint64_t i = 0; i < getNumChildren(cur); ++i )
					{
						std::cerr << CNI[cur] << " -> " << CNI[getChild(cur,i)] << std::endl;
						todo.push_back(getChild(cur,i));
					}
				}
			}

			std::string label(libmaus2::lcs::ChainNodeInfo const & C) const
			{
				#if 0
				std::pair<uint64_t,uint64_t> const & Pl = meta.mapCoordinates(C.yleft);
				std::pair<uint64_t,uint64_t> const & Pr = meta.mapCoordinates(C.yright);
				#endif

				libmaus2::fastx::DNAIndexMetaDataBigBandBiDir::Coordinates const CO = meta.mapCoordinatePair(C.yleft,C.yright);

				std::ostringstream ostr;
				ostr << "x=[" << C.xleft << "," << C.xright << "),y=[(" << CO.seq << "," << CO.rc << "," << CO.left << "," << CO.left+CO.length << ")";
				return ostr.str();
			}

			void dot(std::ostream & out) const
			{
				out << "digraph chain {\n";

				std::deque<uint64_t> todo;
				if ( o )
					todo.push_back(0);

				while ( todo.size() )
				{
					uint64_t const cur = todo.front();
					todo.pop_front();

					for ( uint64_t i = 0; i < getNumChildren(cur); ++i )
					{
						std::ostringstream sourcelabelstr;
						std::ostringstream destlabelstr;

						sourcelabelstr << label(CNI[cur]);
						destlabelstr << label(CNI[getChild(cur,i)]);

						// std::cerr << CNI[cur] << " -> " << CNI[getChild(cur,i)] << std::endl;
						todo.push_back(getChild(cur,i));

						out << "\t\"" << sourcelabelstr.str() << "\" -> \"" << destlabelstr.str() << "\"\n";
					}
				}

				out << "}\n";
			}

			void dot(std::string const & fn) const
			{
				{
					libmaus2::aio::OutputStreamInstance OSI(fn);
					dot(OSI);
				}

				std::string const svgfn = fn + ".svg";
				std::ostringstream comstr;
				comstr << "dot -Tsvg <" << fn << " >" << svgfn;
				int const r = system(comstr.str().c_str());
				if ( r < 0 )
				{
					std::cerr << "dot failed";
				}
			}

			void dot(uint64_t id, std::string const & dottype = std::string()) const
			{
				std::ostringstream fnostr;
				fnostr << "chain_" << std::setw(6) << std::setfill('0') << id << std::setw(0) << dottype << ".dot";
				dot(fnostr.str());
			}

			void setup(libmaus2::lcs::ChainNodeInfo * rCNI, uint64_t ro)
			{
				CNI = rCNI;
				o = ro;
				assert ( o );
				Anumchildren.ensureSize(o+1);
				computeForwardLinks();

				#if defined(CHAINNODEINFOSET_DOT)
				if ( o > 1 )
					dot(chaindot,"_unsimplified");
				#endif

				assert ( o );
				simplify();
				assert ( o );
				calculateScore();
				assert ( o );


				#if defined(CHAINNODEINFOSET_DOT)
				if ( o > 1 )
				{
					std::cerr << "[V] produced chain of score " << getChainScore() << " valid " << isValid() << std::endl;
					dot(chaindot++,"_simplified");
				}
				#endif
			}

			uint64_t getChainScore() const
			{
				if ( o )
					return CNI[0].chainscore;
				else
					return 0;
			}

			bool isValid() const
			{
				return o && getChainScore() >= minscore;
			}

			void align(char const * query, uint64_t const querysize)
			{
				if ( o && CNI[0].chainscore >= minscore )
				{
					// std::cerr << "[V] running align for chain of score " << CNI[0].chainscore << std::endl;

					aligntodoQ.clear();
					alignVtodo.clear();
					alignVtodoout.clear();

					if ( o )
						aligntodoQ.push_back(0);

					while ( !aligntodoQ.empty() )
					{
						uint64_t const cur = aligntodoQ.front();
						aligntodoQ.pop_front();

						std::pair<uint64_t,uint64_t> const & Pl = meta.mapCoordinates(CNI[cur].yleft);
						std::pair<uint64_t,uint64_t> const & Pr = meta.mapCoordinates(CNI[cur].yright-1);

						if ( Pl.first == Pr.first )
							alignVtodo.push_back(cur);

						for ( uint64_t i = 0; i < getNumChildren(cur); ++i )
							aligntodoQ.push_back(getChild(cur,i));
					}

					while ( !alignVtodo.empty() )
					{
						// std::cerr << "[V] todo " << alignVtodo.size() << std::endl;

						uint64_t const cur = alignVtodo.front(); // Vtodo.front();

						assert ( CNI[cur].xright > CNI[cur].xleft );

						std::pair<uint64_t,uint64_t> const & Pl = meta.mapCoordinates(CNI[cur].yleft);
						uint64_t const refleft = meta.L [ Pl.first ];
						uint64_t const refright = meta.L [ Pl.first + 1 ];

						libmaus2::lcs::NNPAlignResult res = nnp.align(
							query,
							query+querysize,
							CNI[cur].xleft,
							text+refleft,
							text+refright,
							CNI[cur].yleft - refleft,
							nnptrace,
							true /* self checking */
						);

						//std::cerr << "refleft=" << refleft << " refright=" << refright << " res=" << res << std::endl;

						res.bbpos += refleft;
						res.bepos += refleft;

						assert ( res.bbpos >= refleft );
						assert ( res.bbpos < refright );

						#if 0
						{
							libmaus2::fastx::DNAIndexMetaDataBigBandBiDir::Coordinates CO = meta.mapCoordinatePair(res.bbpos,res.bepos);
							std::cerr
								<< res
								<< " seq=" << CO.seq << " rc=" << CO.rc << " left=" << CO.left << " length=" << CO.length
								<< std::endl;
						}
						#endif

						#if 0
						std::cerr << res << std::endl;
						nnptrace.printTraceLines(
							std::cerr,
							text+res.abpos,
							text+res.bbpos,
							80 /* linelength */,
							std::string() /* indent */,std::string() /* linesep */,libmaus2::fastx::remapChar);
						#endif

						Aalgn.push(aalgno,libmaus2::lcs::ChainAlignment(res,CNI[cur].xleft,CNI[cur].yleft,CNI[cur].xright-CNI[cur].xleft));

						uint64_t const d_o = nnptrace.getDiagStrips(res.abpos,res.bbpos,D);

						#if 0
						std::cerr << "Number of diag strips is " << d_o << std::endl;
						#endif

						alignVtodoout.clear();

						uint64_t l_o = 0;
						uint64_t l_v = 0;
						while ( l_o < d_o && l_v < alignVtodo.size() )
						{
							int64_t const diag_strip = D[l_o].d;
							int64_t const diag_vcomp = CNI[alignVtodo[l_v]].diag();

							if ( diag_strip < diag_vcomp )
								++l_o;
							else if ( diag_vcomp < diag_strip )
							{
								// copy if not current
								if ( alignVtodo[l_v] != cur )
									alignVtodoout.push_back(alignVtodo[l_v]);

								++l_v;
							}
							else
							{
								RS.clear();

								uint64_t h_o = l_o;
								while ( h_o < d_o && D[h_o].d == diag_strip )
									RS.insert(D[h_o++]);

								uint64_t h_v = l_v;
								while ( h_v < alignVtodo.size() && CNI[alignVtodo[h_v]].diag() == diag_vcomp )
								{
									uint64_t const low = CNI[alignVtodo[h_v]].antiLow();
									uint64_t const high = CNI[alignVtodo[h_v]].antiHigh();

									// VR.resize(0);
									libmaus2::lcs::NNPTraceContainer::DiagStrip Q;
									Q.d = diag_strip;
									Q.l = low;
									Q.h = high;
									uint64_t const vrsize = RS.search(Q,RSQ);

									if ( ! vrsize )
									{
										#if 0
										std::cerr << "not matched E[]=" << Anodes[Asize[Vcomponent[h_v].first]+Vcomponent[h_v].second]
											<< " " << EC.getDiag(Vcomponent[h_v])
											<< " " << EC.getAntiLow(Vcomponent[h_v])
											<< " " << EC.getAntiHigh(Vcomponent[h_v])
											<< std::endl;
										#endif

										if ( alignVtodo[h_v] != cur )
											alignVtodoout.push_back(alignVtodo[h_v]);
									}

									++h_v;
								}

								l_o = h_o;
								l_v = h_v;
							}
						}

						assert ( alignVtodoout.size() < alignVtodo.size() );

						alignVtodo.swap(alignVtodoout);
					}
				}
			}

			void cleanup()
			{
				// sort alignments
				std::sort(Aalgn.begin(),Aalgn.begin()+aalgno);
				// filter out duplicates (identical left and right ends)
				aalgno = std::unique(Aalgn.begin(),Aalgn.begin()+aalgno)-Aalgn.begin();

				// filter out covered alignments
				uint64_t aalignfilto = 0;
				std::set < ChainAlignment, ChainAlignmentAEndComparator > active;
				for ( uint64_t i = 0; i < aalgno; ++i )
				{
					uint64_t const abpos = Aalgn[i].res.abpos;
					uint64_t const aepos = Aalgn[i].res.aepos;
					// uint64_t const alen = aepos-abpos;
					libmaus2::math::IntegerInterval<int64_t> Ii(abpos,aepos-1);

					while ( (! active.empty()) && active.begin()->res.aepos < abpos )
					{
						Aalgnfilter.push(aalignfilto,*active.begin());
						active.erase(active.begin());
					}

					std::set < ChainAlignment > activekill;
					bool covered = false;
					for ( std::set < ChainAlignment, ChainAlignmentAEndComparator >::const_iterator ita =
						active.begin(); ita != active.end(); ++ita )
					{
						uint64_t const back_abpos = ita->res.abpos;
						uint64_t const back_aepos = ita->res.aepos;
						// uint64_t const backlen = back_aepos - back_abpos;

						libmaus2::math::IntegerInterval<int64_t> Ia(back_abpos,back_aepos-1);
						libmaus2::math::IntegerInterval<int64_t> Ic = Ii.intersection(Ia);

						if (
							Ic.diameter() >= static_cast<int64_t>((fracmul * Ii.diameter()) / fracdiv)
							&&
							Ia.diameter() >= 2 * Ii.diameter()
							&&
							ita->getScore() >= 2 * Aalgn[i].getScore()
						)
						{
							//std::cerr << "killing " << Aalgn[i].res << " in favour of " << ita->res << std::endl;
							covered = true;
						}
						else if (
							Ic.diameter() >= static_cast<int64_t>((fracmul * Ia.diameter()) / fracdiv)
							&&
							Ii.diameter() >= 2 * Ia.diameter()
							&&
							Aalgn[i].getScore() >= 2 * ita->getScore()
						)
						{
							//std::cerr << "killing " << ita->res << " in favour of " << Aalgn[i].res << std::endl;
							activekill.insert(*ita);
						}
					}

					if ( ! covered )
					{
						active.insert(Aalgn[i]);
					}
					for ( std::set < ChainAlignment >::iterator ita = activekill.begin(); ita != activekill.end(); ++ita )
						active.erase(*ita);
				}

				for ( std::set < ChainAlignment, ChainAlignmentAEndComparator >::const_iterator ita =
					active.begin(); ita != active.end(); ++ita )
					Aalgnfilter.push(aalignfilto,*ita);

				Aalgn.swap(Aalgnfilter);
				std::swap(aalgno,aalignfilto);

				// sort remaining alignments by score
				std::sort(Aalgn.begin(),Aalgn.begin()+aalgno,ChainAlignmentScoreComparator());
			}

			void copyback(
				libmaus2::util::ContainerElementFreeList<libmaus2::lcs::ChainNode> & chainnodefreelist,
				libmaus2::lcs::Chain & chain,
				uint64_t const chainid
			)
			{
				uint64_t rightmost = 0;

				for ( uint64_t i = 0; i < o; ++i )
				{
					libmaus2::lcs::ChainNodeInfo const & CI = CNI[i];

					libmaus2::lcs::ChainNode CN(
						chainid,
						i,
						CI.parentsubid,
						CI.xleft,
						CI.xright,
						CI.yleft,
						CI.yright
					);
					CN.chainscore = CI.chainscore;

					chain.push(chainnodefreelist,CN);

					rightmost = std::max(rightmost,CI.xright);
				}

				chain.rightmost = rightmost;

				#if defined(CHAINNODEINFOSET_DOT)
				std::cerr << "[V] copied back chain of score " << chain.getChainScore(chainnodefreelist) << " range " << chain.getRange(chainnodefreelist) << std::endl;
				dot(chainid, "_copyback");
				#endif
			}

			void printAlignments(uint64_t const minlength = 0)
			{
				for ( uint64_t i = 0; i < aalgno; ++i )
				{
					if ( Aalgn[i].res.aepos-Aalgn[i].res.abpos >= minlength )
					{
						libmaus2::math::IntegerInterval<int64_t> IA(Aalgn[i].res.abpos,Aalgn[i].res.aepos-1);
						libmaus2::math::IntegerInterval<int64_t> IB(Aalgn[i].res.bbpos,Aalgn[i].res.bepos-1);
						libmaus2::math::IntegerInterval<int64_t> IC = IA.intersection(IB);
						bool const self = ! IC.isEmpty();

						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::coutlock);
						std::cout << Aalgn[i].res << "\t" << (self?"self":"distinct") << std::endl;
					}
				}
			}
		};
	}
}
#endif
