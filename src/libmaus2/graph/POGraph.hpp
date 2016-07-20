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
#if ! defined(LIBMAUS2_GRAPH_POGRAPH_HPP)
#define LIBMAUS2_GRAPH_POGRAPH_HPP

#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/autoarray/AutoArray2d.hpp>
#include <libmaus2/util/SimpleHashMap.hpp>
#include <libmaus2/util/SimpleQueue.hpp>
#include <libmaus2/lcs/BaseConstants.hpp>

namespace libmaus2
{
	namespace graph
	{
		template<typename _node_id_type>
		struct POGraphEdge
		{
			typedef _node_id_type node_id_type;
			typedef POGraphEdge<node_id_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type shared_ptr_type;

			node_id_type to;

			POGraphEdge() {}
			POGraphEdge(node_id_type rto) : to(rto) {}
		};

		template<typename node_id_type>
		std::ostream & operator<<(std::ostream & out, POGraphEdge<node_id_type> const & O)
		{
			return out << "POGraphEdge(to=" << O.to << ")";
		}

		template<typename _node_id_type>
		struct POHashGraphKey
		{
			typedef _node_id_type node_id_type;

			node_id_type from;
			node_id_type index;

			POHashGraphKey() {}
			POHashGraphKey(node_id_type const rfrom, node_id_type const rindex)
			: from(rfrom), index(rindex) {}

			bool operator==(POHashGraphKey<node_id_type> const & O) const
			{
				return from == O.from && index == O.index;
			}
			bool operator!=(POHashGraphKey<node_id_type> const & O) const
			{
				return ! (*this == O);
			}
		};

		template<typename _node_id_type>
		std::ostream & operator<<(std::ostream & out, POHashGraphKey<_node_id_type> const & P)
		{
			return out << "POHashGraphKey(from=" << P.from << ",index=" << P.index << ")";
		}

		template<typename _node_id_type>
		struct POHashGraphKeyConstantsKeyPrint
		{
			static std::ostream & printKey(std::ostream & out, POHashGraphKey<_node_id_type> const & P)
			{
				return (out << P);
			}
		};

		template<typename _node_id_type>
		struct POHashGraphKeyConstants
		{
			typedef _node_id_type node_id_type;
			typedef POHashGraphKey<node_id_type> key_type;

			static key_type const unused()
			{
				return key_type(
					std::numeric_limits<node_id_type>::max(),
					std::numeric_limits<node_id_type>::max()
				);
			}

			static key_type const deleted()
			{
				return key_type(
					std::numeric_limits<node_id_type>::max()-1,
					std::numeric_limits<node_id_type>::max()-1
				);
			}

			// unused or deleted
			static bool isFree(key_type const & v)
			{
				return
					v == unused()
					||
					v == deleted();
			}

			// in use (not unused or deleted)
			static bool isInUse(key_type const & v)
			{
				return !isFree(v);
			}

			virtual ~POHashGraphKeyConstants() {}
		};

		template<typename _node_id_type>
		struct POHashGraphKeyHashCompute
		{
			typedef _node_id_type node_id_type;
			typedef POHashGraphKey<node_id_type> key_type;

			inline static uint64_t hash(key_type const & v)
			{
				uint64_t V[2] = { static_cast<uint64_t>(v.from), static_cast<uint64_t>(v.index) };
				return libmaus2::hashing::EvaHash::hash642(&V[0],2);
			}
		};

		template<typename _node_id_type>
		struct POHashGraphKeyDisplaceMaskFunction
		{
			typedef _node_id_type node_id_type;
			typedef POHashGraphKey<node_id_type> key_type;

			static uint64_t displaceMask(key_type const & v)
			{
				return (v.from & 0xFFFFULL) ^ (v.index);
			}
		};

		struct NodeInfo
		{
			char label;
			uint64_t numalignedto;
			uint64_t usecnt;

			NodeInfo() : label(0), numalignedto(0), usecnt(0)
			{

			}
			NodeInfo(char const rlabel) : label(rlabel), numalignedto(0), usecnt(0)
			{

			}
		};

		struct POHashGraph
		{
			typedef POHashGraph this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::unique_ptr<this_type>::type shared_ptr_type;

			typedef int32_t node_id_type;
			typedef uint32_t score_type;

			// forward edges
			libmaus2::util::SimpleHashMap<
				POHashGraphKey<node_id_type>,
				POGraphEdge<node_id_type>,
				POHashGraphKeyConstants<node_id_type>,
				POHashGraphKeyHashCompute<node_id_type>,
				POHashGraphKeyDisplaceMaskFunction<node_id_type>,
				POHashGraphKeyConstantsKeyPrint<node_id_type>
			> forwardedges;
			libmaus2::autoarray::AutoArray<uint64_t> numforwardedges;
			uint64_t numforwardedgeso;

			// reverse edges
			libmaus2::util::SimpleHashMap<
				POHashGraphKey<node_id_type>,
				POGraphEdge<node_id_type>,
				POHashGraphKeyConstants<node_id_type>,
				POHashGraphKeyHashCompute<node_id_type>,
				POHashGraphKeyDisplaceMaskFunction<node_id_type>,
				POHashGraphKeyConstantsKeyPrint<node_id_type>
			> reverseedges;
			libmaus2::autoarray::AutoArray<uint64_t> numreverseedges;
			uint64_t numreverseedgeso;

			// node labels
			libmaus2::autoarray::AutoArray<NodeInfo> nodelabels;
			uint64_t numnodelabels;

			// node aligned to lists
			libmaus2::util::SimpleHashMap<
				POHashGraphKey<node_id_type>,
				POGraphEdge<node_id_type>,
				POHashGraphKeyConstants<node_id_type>,
				POHashGraphKeyHashCompute<node_id_type>,
				POHashGraphKeyDisplaceMaskFunction<node_id_type>,
				POHashGraphKeyConstantsKeyPrint<node_id_type>
			> alignedto;
			libmaus2::autoarray::AutoArray<uint64_t> numalignedto;
			uint64_t numalignedtoo;

			// used for align()
			struct TraceNode
			{
				node_id_type i;
				node_id_type j;
				score_type s;
				libmaus2::lcs::BaseConstants::step_type t;

				TraceNode() {}
				TraceNode(
					node_id_type const ri,
					node_id_type const rj,
					score_type const rs,
					libmaus2::lcs::BaseConstants::step_type const rt
				) : i(ri), j(rj), s(rs), t(rt) {}
			};

			libmaus2::autoarray::AutoArray<uint64_t> unfinished;
			libmaus2::util::SimpleQueue<uint64_t> Q;
			libmaus2::autoarray::AutoArray2d<TraceNode> S;
			libmaus2::autoarray::AutoArray<uint64_t> Atmp;

			POHashGraph & operator=(POHashGraph const & O)
			{
				if ( this != &O )
				{
					// copy graph data
					forwardedges = O.forwardedges;
					numforwardedges = O.numforwardedges.clone();
					numforwardedgeso = O.numforwardedgeso;

					reverseedges = O.reverseedges;
					numreverseedges = O.numreverseedges.clone();
					numreverseedgeso = O.numreverseedgeso;

					nodelabels = O.nodelabels.clone();
					numnodelabels = O.numnodelabels;

					alignedto = O.alignedto;
					numalignedto = O.numalignedto.clone();
					numalignedtoo = O.numalignedtoo;

					// we do not copy unfinished,Q,S,Atmp as they are tmp variables for align
				}
				return *this;
			}

			POHashGraph() :
				forwardedges(8), numforwardedges(), numforwardedgeso(0),
				reverseedges(8), numreverseedges(), numreverseedgeso(0),
				nodelabels(), numnodelabels(0),
				alignedto(8),
				numalignedto(),
				numalignedtoo(0),
				unfinished(),
				Q(),
				S()
			{

			}

			void clear()
			{
				numforwardedgeso = 0;
				numreverseedgeso = 0;
				forwardedges.clearFast();
				reverseedges.clearFast();
				numnodelabels = 0;
				alignedto.clearFast();
				numalignedtoo = 0;
			}

			void addAlignedTo(uint64_t const id, uint64_t const aid)
			{
				while ( ! (id < numalignedtoo) )
					numalignedto.push(numalignedtoo,0);
				assert ( id < numalignedtoo );

				uint64_t const index = numalignedto[id]++;

				alignedto.insertNonSyncExtend(
					POHashGraphKey<node_id_type>(id,index),
					POGraphEdge<node_id_type>(aid),
					0.8 /* extend load */
				);

				assert ( alignedto.contains(POHashGraphKey<node_id_type>(id,index)) );
			}

			uint64_t getNumAlignedTo(uint64_t const id) const
			{
				if ( id < numalignedtoo )
					return numalignedto[id];
				else
					return 0;
			}

			uint64_t getAlignedTo(uint64_t const id, uint64_t const aid) const
			{
				assert ( id < numalignedtoo );
				assert ( aid < numalignedto[id] );

				POGraphEdge<node_id_type> edge;
				bool const ok = alignedto.contains(POHashGraphKey<node_id_type>(id,aid), edge);
				assert ( ok );

				return edge.to;
			}

			uint64_t getAllAlignedTo(uint64_t const id, libmaus2::autoarray::AutoArray<uint64_t> & Atmp) const
			{
				uint64_t const n = getNumAlignedTo(id);
				Atmp.ensureSize(n);
				for ( uint64_t i = 0; i < n; ++i )
					Atmp[i] = getAlignedTo(id,i);
				return n;
			}

			void addNode(uint64_t const id, char const symbol)
			{
				while ( !(id < numnodelabels) )
					nodelabels.push(numnodelabels,NodeInfo());
				assert ( id < numnodelabels );

				#if 0
				while ( !(id < numalignedtoo) )
					numalignedto.push(numalignedtoo,0);
				assert ( id < numalignedtoo );
				#endif

				nodelabels[id] = NodeInfo(symbol);
				//numalignedto[id] = 0;
			}

			uint64_t addNode(char const symbol)
			{
				uint64_t const nodeid = numnodelabels;
				addNode(nodeid,symbol);
				return nodeid;
			}

			uint64_t getNumForwardEdges(uint64_t const from) const
			{
				if ( from < numforwardedgeso )
					return numforwardedges[from];
				else
					return 0;
			}

			uint64_t getNumReverseEdges(uint64_t const to) const
			{
				if ( to < numreverseedgeso )
					return numreverseedges[to];
				else
					return 0;
			}

			POGraphEdge<node_id_type> getForwardEdge(uint64_t const from, uint64_t const index) const
			{
				assert ( from < numforwardedgeso );
				assert ( index < numforwardedges[from] );

				POGraphEdge<node_id_type> edge;
				bool const ok = forwardedges.contains(POHashGraphKey<node_id_type>(from,index), edge);
				assert ( ok );

				return edge;
			}

			POGraphEdge<node_id_type> getReverseEdge(uint64_t const to, uint64_t const index) const
			{
				assert ( to < numreverseedgeso );
				assert ( index < numreverseedges[to] );

				POGraphEdge<node_id_type> edge;
				bool const ok = reverseedges.contains(POHashGraphKey<node_id_type>(to,index), edge);
				assert ( ok );

				return edge;
			}

			void insertForwardEdge(uint64_t const from, uint64_t const to)
			{
				while ( !(from < numforwardedgeso) )
					numforwardedges.push(numforwardedgeso,0);
				assert ( from < numforwardedgeso );

				uint64_t const index = numforwardedges[from]++;

				forwardedges.insertNonSyncExtend(
					POHashGraphKey<node_id_type>(from,index),
					POGraphEdge<node_id_type>(to),
					0.8 /* extend load */
				);
			}

			void insertReverseEdge(uint64_t const from, uint64_t const to)
			{
				while ( !(to < numreverseedgeso) )
					numreverseedges.push(numreverseedgeso,0);
				assert ( to < numreverseedgeso );

				uint64_t const index = numreverseedges[to]++;

				reverseedges.insertNonSyncExtend(
					POHashGraphKey<node_id_type>(to,index),
					POGraphEdge<node_id_type>(from),
					0.8 /* extend load */
				);
			}

			void insertEdge(uint64_t const from, uint64_t const to)
			{
				if ( ! haveForwardEdge(from,to) )
				{
					insertForwardEdge(from,to);
					insertReverseEdge(from,to);
				}
			}

			bool haveForwardEdge(uint64_t const from, int64_t const to) const
			{
				if ( from < numforwardedgeso )
				{
					uint64_t const num = numforwardedges[from];

					for ( uint64_t i = 0; i < num; ++i )
					{
						POGraphEdge<node_id_type> edge;
						bool const ok = forwardedges.contains(POHashGraphKey<node_id_type>(from,i), edge);
						assert ( ok );
						if ( edge.to == to )
							return true;
					}

					return false;
				}
				else
				{
					return false;
				}
			}

			int64_t getMaxTo() const
			{
				for ( int64_t i = static_cast<int64_t>(numreverseedgeso)-1; i >= 0; --i )
					if ( numreverseedges[i] )
						return i;

				return -1;
			}

			int64_t getMaxFrom() const
			{
				for ( int64_t i = static_cast<int64_t>(numforwardedgeso)-1; i >= 0; --i )
					if ( numforwardedges[i] )
						return i;

				return -1;
			}

			template<typename iterator>
			void setupSingle(iterator it, uint64_t const n)
			{
				clear();

				if ( n )
					addNode(*(it++));

				for ( uint64_t i = 1; i < n; ++i )
				{
					addNode(*(it++));
					insertEdge(i-1,i);
				}
			}

			std::ostream & toDot(std::ostream & out, std::string const & name = "pograph") const
			{
				out << "digraph " << name << " {\n";

				out << "\trankdir=LR;\n";

				int64_t const maxfrom = getMaxFrom();
				int64_t const maxto = getMaxTo();
				int64_t const maxnode = std::max(maxfrom,maxto);

				for ( int64_t i = 0; i <= maxnode; ++i )
					if ( i < static_cast<int64_t>(numnodelabels) )
					{
						out << "\tnode_" << i << " [label=\"" << nodelabels[i].label << "," << nodelabels[i].usecnt;

						#if 0
						for ( uint64_t j = 0; j < getNumAlignedTo(i); ++j )
						{
							out << ";" << getAlignedTo(i,j);
						}
						#endif

						#if 0
						out << ";" << i;
						#endif

						out << "\"];\n";
					}
					else
						out << "\tnode_" << i << " [label=\"" << i << "\"];\n";

				for ( int64_t i = 0; i <= maxfrom; ++i )
					for ( uint64_t j = 0; j < getNumForwardEdges(i); ++j )
						out << "\tnode_" << i << " -> " << "node_" << getForwardEdge(i,j).to << ";\n";

				out << "}\n";

				return out;
			}

			/*
			 * PO/PO alignment
			 */
			static void align(POHashGraph & C, POHashGraph const & A, POHashGraph const & B)
			{
				// we currenctly do not support having one of the graphs as parameter and result
				assert ( &C != &A );
				assert ( &C != &B );

				int64_t const maxfrom_A = A.getMaxFrom();
				int64_t const maxto_A = A.getMaxTo();
				int64_t const maxnode_A = std::max(maxfrom_A,maxto_A);
				uint64_t const nodelimit_A = maxnode_A+1;

				int64_t const maxfrom_B = B.getMaxFrom();
				int64_t const maxto_B = B.getMaxTo();
				int64_t const maxnode_B = std::max(maxfrom_B,maxto_B);
				uint64_t const nodelimit_B = maxnode_B+1;

				libmaus2::autoarray::AutoArray2d<TraceNode> S(nodelimit_A,nodelimit_B);
				libmaus2::autoarray::AutoArray2d<uint64_t> U(nodelimit_A,nodelimit_B);
				libmaus2::util::SimpleQueue< std::pair<uint64_t,uint64_t> > Q;

				// compute number of predecessors for each node
				// put nodes with no predecessors in queue
				for ( uint64_t i = 0; i < nodelimit_A; ++i )
				{
					uint64_t const va = A.getNumReverseEdges(i);
					for ( uint64_t j = 0; j < nodelimit_B; ++j )
					{
						uint64_t const vb = B.getNumReverseEdges(j);
						uint64_t const vc = va * vb;
						U(i,j) =  vc;
						if ( ! vc )
						{
							Q.push_back(std::pair<uint64_t,uint64_t>(i,j));
						}
					}
				}

				int64_t mscore = std::numeric_limits<int64_t>::min();
				int64_t mscorei = std::numeric_limits<int64_t>::min();
				int64_t mscorej = std::numeric_limits<int64_t>::min();

				// fill dynamic programming matrix
				while ( ! Q.empty() )
				{
					std::pair<uint64_t,uint64_t> const P = Q.pop_front();
					uint64_t const i = P.first;
					uint64_t const j = P.second;
					int64_t previ = i;
					int64_t prevj = j;

					uint64_t const pre_A = A.getNumReverseEdges(i);
					uint64_t const pre_B = B.getNumReverseEdges(j);
					uint64_t const pre_C = pre_A * pre_B;

					char const c_a = A.nodelabels[i].label;
					char const c_b = B.nodelabels[j].label;
					bool const match = (c_a == c_b);

					int64_t score = std::numeric_limits<int64_t>::min();
					libmaus2::lcs::BaseConstants::step_type t = libmaus2::lcs::BaseConstants::STEP_RESET;

					/*
					 * match/mismatch
					 */
					if ( ! pre_C )
					{
						int64_t lscore = match ? 1 : 0;
						if ( lscore > score )
						{
							score = lscore;
							t = match ? libmaus2::lcs::BaseConstants::STEP_MATCH : libmaus2::lcs::BaseConstants::STEP_MISMATCH;

							if ( ! pre_A && ! pre_B )
							{
								previ = -1;
								prevj = -1;
							}
							else if ( pre_A )
							{
								previ = A.getReverseEdge(i,0).to;
								prevj = -1;
							}
							else
							{
								assert ( pre_B );
								previ = -1;
								prevj = B.getReverseEdge(j,0).to;
							}
						}
					}
					else
					{
						assert ( pre_C );
						assert ( pre_A );
						assert ( pre_B );

						for ( uint64_t ri = 0; ri < pre_A; ++ri )
						{
							uint64_t const revi = A.getReverseEdge(i,ri).to;

							for ( uint64_t rj = 0; rj < pre_B; ++rj )
							{
								uint64_t const revj = B.getReverseEdge(j,rj).to;
								int64_t const lscore = S(revi,revj).s + (match?1:0);
								if ( lscore > score )
								{
									score = lscore;
									t = match ? libmaus2::lcs::BaseConstants::STEP_MATCH : libmaus2::lcs::BaseConstants::STEP_MISMATCH;
									previ = revi;
									prevj = revj;
								}
							}
						}
					}
					/*
					 * deletion (move on A but not on B)
					 */
					for ( uint64_t ri = 0; ri < pre_A; ++ri )
					{
						uint64_t const revi = A.getReverseEdge(i,ri).to;
						int64_t const lscore = S(revi,j).s;
						if ( lscore > score )
						{
							score = lscore;
							previ = revi;
							prevj = j;
							t = libmaus2::lcs::BaseConstants::STEP_DEL;
						}
					}
					/*
					 * insertion (move on B but not on A)
					 */
					for ( uint64_t rj = 0; rj < pre_B; ++rj )
					{
						uint64_t const revj = B.getReverseEdge(j,rj).to;
						int64_t const lscore = S(i,revj).s;
						if ( lscore > score )
						{
							score = lscore;
							previ = i;
							prevj = revj;
							t = libmaus2::lcs::BaseConstants::STEP_INS;
						}
					}

					S(i,j) = TraceNode(previ,prevj,score,t);

					if ( score > mscore )
					{
						mscore = score;
						mscorei = i;
						mscorej = j;
					}

					uint64_t const next_A = A.getNumForwardEdges(i);
					uint64_t const next_B = B.getNumForwardEdges(j);

					for ( uint64_t ri = 0; ri < next_A; ++ri )
					{
						uint64_t const forw_A = A.getForwardEdge(i,ri).to;

						for ( uint64_t rj = 0; rj < next_B; ++rj )
						{
							uint64_t const forw_B = B.getForwardEdge(j,rj).to;
							assert ( U(forw_A,forw_B) );
							uint64_t const u = --U(forw_A,forw_B);
							if ( !u )
								Q.push_back(std::pair<uint64_t,uint64_t>(forw_A,forw_B));
						}
					}
				}

				// B mapping
				libmaus2::autoarray::AutoArray<int64_t> BM(nodelimit_B,false);
				libmaus2::autoarray::AutoArray<int64_t> BMA(nodelimit_B,false);
				std::fill(BM.begin(), BM.end() ,std::numeric_limits<int64_t>::min());
				std::fill(BMA.begin(),BMA.end(),std::numeric_limits<int64_t>::min());

				// copy A to C
				C = A;

				// trace back from maximum score
				while ( mscorei >= 0 && mscorej >= 0 )
				{
					TraceNode const & T = S(mscorei,mscorej);

					char const c_a = A.nodelabels[mscorei].label;
					char const c_b = B.nodelabels[mscorej].label;

					if (
						T.t == libmaus2::lcs::BaseConstants::STEP_MATCH
						||
						T.t == libmaus2::lcs::BaseConstants::STEP_MISMATCH
					)
					{
						// matching, fuse nodes
						if ( c_a == c_b )
						{
							BM[mscorej] = mscorei;
							C.nodelabels[mscorei].usecnt += 1;
						}
						// not matching
						else
						{
							// check whether there is a node aligned to mscorei labeled with c_b
							int64_t mscorei_alt = std::numeric_limits<int64_t>::min();

							for ( uint64_t z = 0; z < A.getNumAlignedTo(mscorei); ++z )
							{
								uint64_t const lmscorei_alt = A.getAlignedTo(mscorei,z);

								if ( A.nodelabels[lmscorei_alt].label == c_b )
									mscorei_alt = lmscorei_alt;
							}

							// got one? fuse to it
							if ( mscorei_alt >= 0 )
							{
								BM[mscorej] = mscorei_alt;
							}
							// no, align new (existing in B) node to mscorei
							else
							{
								BMA[mscorej] = mscorei;
							}
						}
					}

					mscorei = T.i;
					mscorej = T.j;
				}

				// assign new node ids for unaligned nodes in B
				uint64_t nextnodeid = nodelimit_A;
				for ( uint64_t i = 0; i < BM.size(); ++i )
					if ( BM[i] < 0 )
					{
						BM[i] = nextnodeid++;
						// assert ( BM[i] >= nodelimit_A );
						C.addNode(BM[i],B.nodelabels[i].label);
					}

				// insert alignedto entries
				libmaus2::autoarray::AutoArray<uint64_t> Atmp;
				for ( uint64_t i = 0; i < BMA.size(); ++i )
					if ( BMA[i] >= 0 )
					{
						// node id of B node
						uint64_t const nodeid = BM[i];
						// node aligned to in A
						uint64_t const alignto = BMA[i];

						uint64_t na = A.getAllAlignedTo(alignto,Atmp);
						Atmp.push(na,alignto);

						for ( uint64_t j = 0; j < na; ++j )
						{
							C.addAlignedTo(nodeid,Atmp[j]);
							C.addAlignedTo(Atmp[j],nodeid);
						}
					}

				// copy non redundant edges from B
				for ( uint64_t i = 0; i < nodelimit_B; ++i )
					for ( uint64_t j = 0; j < B.getNumForwardEdges(i); ++j )
					{
						uint64_t const btgt = B.getForwardEdge(i,j).to;

						uint64_t const src = BM[i];
						uint64_t const tgt = BM[btgt];

						if ( !C.haveForwardEdge(src,tgt) )
							C.insertEdge(src,tgt);
					}
			}

			template<typename iterator>
			void align(iterator it, uint64_t const n)
			{
				int64_t const maxfrom = getMaxFrom();
				int64_t const maxto = getMaxTo();
				int64_t const maxnode = std::max(maxfrom,maxto);
				uint64_t const nodelimit = maxnode+1;

				S.setup(nodelimit,n);

				unfinished.ensureSize(nodelimit);
				std::fill(unfinished.begin(),unfinished.begin()+nodelimit,0ull);

				for ( uint64_t i = 0; i < nodelimit; ++i )
				{
					unfinished[i] = getNumReverseEdges(i);
					// std::cerr << "unfinished " << i << " " << unfinished[i] << std::endl;
					if ( ! unfinished[i] )
						Q.push_back(i);
				}

				while ( ! Q.empty() )
				{
					uint64_t const q = Q.pop_front();
					char const g = nodelabels[q].label;

					// std::cerr << "processing " << q << " " << g << std::endl;

					for ( uint64_t i = 0; i < n; ++i )
					{
						int64_t score = std::numeric_limits<int64_t>::min();
						int64_t previ = q;
						int64_t prevj = i;
						libmaus2::lcs::BaseConstants::step_type t = libmaus2::lcs::BaseConstants::STEP_RESET;

						if (
							getNumReverseEdges(q) == 0 || i == 0
						)
						{
							bool const match = it[i] == g;
							int64_t lscore = match ? 1 : 0;

							if ( lscore > score )
							{
								score = lscore;
								t = match ? libmaus2::lcs::BaseConstants::STEP_MATCH : libmaus2::lcs::BaseConstants::STEP_MISMATCH;

								if ( i )
								{
									previ = -1;
									prevj = i-1;
								}
								else if ( getNumReverseEdges(q) )
								{
									previ = getReverseEdge(q,0).to;
									prevj = -1;
								}
								else
								{
									previ = -1;
									prevj = -1;
								}
							}
						}
						else
						{
							for ( uint64_t j = 0; j < getNumReverseEdges(q); ++j )
							{
								uint64_t const p = getReverseEdge(q,j).to;

								// assert ( S.find(std::pair<uint64_t,uint64_t>(p,i-1)) != S.end() );

								bool const match = (it[i] == g);
								int64_t const lscore = S(p,i-1).s + (match?1:0);

								if ( lscore > score )
								{
									score = lscore;
									previ = p;
									prevj = i-1;
									// op = match/mismatch
									t = match ? libmaus2::lcs::BaseConstants::STEP_MATCH : libmaus2::lcs::BaseConstants::STEP_MISMATCH;
								}
							}
						}

						if ( i )
						{
							// assert ( S.find(std::pair<uint64_t,uint64_t>(q,i-1)) != S.end() );
							int64_t const lscore = S(q,i-1).s;
							if ( lscore > score )
							{
								score = lscore;
								previ = q,
								prevj = i-1;
								// move on query but not on graph
								// op = ins
								t = libmaus2::lcs::BaseConstants::STEP_INS;
							}
						}
						for ( uint64_t j = 0; j < getNumReverseEdges(q); ++j )
						{
							uint64_t const p = getReverseEdge(q,j).to;

							//assert ( S.find(std::pair<uint64_t,uint64_t>(p,i)) != S.end() );
							int64_t const lscore = S(p,i).s;

							if ( lscore > score )
							{
								score = lscore;
								previ = p;
								prevj = i;
								// move on graph but not on query
								// op = del
								t = libmaus2::lcs::BaseConstants::STEP_DEL;
							}
						}

						S(q,i) = TraceNode(previ,prevj,score,t);
					}

					for ( uint64_t i = 0; i < getNumForwardEdges(q); ++i )
					{
						node_id_type t = getForwardEdge(q,i).to;

						if ( ! --unfinished[t] )
							Q.push_back(t);
					}
				}

				// std::cerr << "left process loop" << std::endl;

				#if 0
				for ( uint64_t i = 0; i < nodelimit; ++i )
				{
					for ( uint64_t j = 0; j < n; ++j )
					{
						std::cerr << S(i,j).s << " ";
					}
					std::cerr << std::endl;
				}
				#endif

				int64_t maxscore = std::numeric_limits<int64_t>::min();
				int64_t maxscorenode = -1;
				for ( uint64_t i = 0; i < nodelimit; ++i )
					if ( getNumForwardEdges(i) == 0 )
					{
						int64_t const lscore = S(i,n-1).s;

						// std::cerr << "end node " << i << " score " << lscore << std::endl;

						if ( lscore > maxscore )
						{
							maxscore = lscore;
							maxscorenode = i;
						}
					}

				// std::cerr << "maxscore=" << maxscore << " at node " << maxscorenode << std::endl;

				int64_t curi = maxscorenode;
				int64_t curj = static_cast<int64_t>(n)-1;
				int64_t prevnode = -1;

				while ( curi != -1 || curj != -1 )
				{
					// int64_t const score = (curi>=0&&curj>=0) ? S(curi,curj).s : 0;
					libmaus2::lcs::BaseConstants::step_type t;

					if ( curi != -1 && curj != -1 )
						t = S(curi,curj).t;
					else if ( curi == -1 )
						t = libmaus2::lcs::BaseConstants::step_type::STEP_INS;
					else
						t = libmaus2::lcs::BaseConstants::step_type::STEP_DEL;

					// std::cerr << "i=" << curi << " j=" << curj << " t=" << t << std::endl;

					switch ( t )
					{
						case libmaus2::lcs::BaseConstants::step_type::STEP_MATCH:
						{
							// std::cerr << "match for " << it[curj] << std::endl;

							// std::cerr << "prevnode=" << prevnode << std::endl;
							nodelabels[curi].usecnt += 1;

							if  (
								prevnode != -1
								&&
								(! haveForwardEdge(curi,prevnode))
							)
							{
								// std::cerr << "inserting edge " << curi << " " << prevnode << std::endl;
								insertEdge(curi,prevnode);
							}

							prevnode = curi;
							break;
						}
						case libmaus2::lcs::BaseConstants::step_type::STEP_MISMATCH:
						{
							assert ( curi >= 0 );
							assert ( curj >= 0 );

							// std::cerr << "mismatch " << nodelabels[curi] << " != " << it[curj] << std::endl;

							// do we have a match in an aligned to
							bool havematch = false;
							// prevnode for next round
							int64_t nextprevnode = curi;

							for ( uint64_t j = 0; j < getNumAlignedTo(curi); ++j )
							{
								// std::cerr << "checking for aligned to " << getAlignedTo(curi,j) << " label " << nodelabels[getAlignedTo(curi,j)] << std::endl;
								bool const lhavematch = (nodelabels[getAlignedTo(curi,j)].label == it[curj]);
								havematch = havematch || lhavematch;
								if ( lhavematch )
									nextprevnode = getAlignedTo(curi,j);
							}

							// we already have match, set edge to next node if it does not exists it
							if ( havematch )
							{
								//std::cerr << "have match" << std::endl;

								if (
									prevnode != -1
									&&
									(! haveForwardEdge(nextprevnode,prevnode))
								)
									insertEdge(nextprevnode,prevnode);

								// std::cerr << "setting prevnode to " << nextprevnode << std::endl;

								prevnode = nextprevnode;
							}
							// no match, insert new node
							else
							{
								uint64_t const id = addNode(it[curj]);
								uint64_t const n = getAllAlignedTo(curi, Atmp);

								for ( uint64_t i = 0; i < n; ++i )
								{
									addAlignedTo(id,Atmp[i]);
									addAlignedTo(Atmp[i],id);
								}

								addAlignedTo(curi,id);
								addAlignedTo(id,curi);

								// connect up new node
								if ( prevnode != -1 )
									insertEdge(id,prevnode);

								prevnode = id;
							}

							break;
						}
						case libmaus2::lcs::BaseConstants::step_type::STEP_INS:
						{
							uint64_t const id = addNode(it[curj]);
							if ( prevnode != -1 )
								insertEdge(id,prevnode);
							prevnode = id;
							break;
						}
						case libmaus2::lcs::BaseConstants::step_type::STEP_DEL:
						{
							break;
						}
						default:
						{
							std::cerr << "unknown op" << std::endl;
							break;
						}
					}

					if ( curi == -1 )
					{
						curj -= 1;
					}
					else if ( curj == -1 )
					{
						if ( getNumReverseEdges(curi) )
							curi = getReverseEdge(curi,0).to;
						else
							curi = -1;
					}
					else
					{
						TraceNode const & T = S(curi,curj);
						curi = T.i;
						curj = T.j;
					}
				}
			}
		};

		inline std::ostream & operator<<(std::ostream & out, POHashGraph const & P)
		{
			for ( uint64_t i = 0; i < P.numforwardedgeso; ++i )
				for ( uint64_t j = 0; j < P.getNumForwardEdges(i); ++j )
					out << "F\t" << i << "->" << P.getForwardEdge(i,j) << std::endl;
			for ( uint64_t i = 0; i < P.numreverseedgeso; ++i )
				for ( uint64_t j = 0; j < P.getNumReverseEdges(i); ++j )
					out << "R\t" << i << "->" << P.getReverseEdge(i,j) << std::endl;
			return out;
		}
	}
}
#endif
