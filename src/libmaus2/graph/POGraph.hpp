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
			node_id_type edgeid;

			POHashGraphKey() {}
			POHashGraphKey(node_id_type const rfrom, node_id_type const redgeid)
			: from(rfrom), edgeid(redgeid) {}

			bool operator==(POHashGraphKey<node_id_type> const & O) const
			{
				return from == O.from && edgeid == O.edgeid;
			}
			bool operator!=(POHashGraphKey<node_id_type> const & O) const
			{
				return ! (*this == O);
			}
		};

		template<typename _node_id_type>
		std::ostream & operator<<(std::ostream & out, POHashGraphKey<_node_id_type> const & P)
		{
			return out << "POHashGraphKey(from=" << P.from << ",edgeid=" << P.edgeid << ")";
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
				uint64_t V[2] = { static_cast<uint64_t>(v.from), static_cast<uint64_t>(v.edgeid) };
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
				return (v.from & 0xFFFFULL) ^ (v.edgeid);
			}
		};

		struct POHashGraph
		{
			typedef POHashGraph this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::unique_ptr<this_type>::type shared_ptr_type;

			typedef int32_t node_id_type;
			typedef uint32_t score_type;

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

			libmaus2::autoarray::AutoArray<char> nodelabels;
			uint64_t numnodelabels;

			// used for align()
			libmaus2::autoarray::AutoArray<uint64_t> unfinished;
			libmaus2::util::SimpleQueue<uint64_t> Q;

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

			libmaus2::autoarray::AutoArray2d<TraceNode> S;

			POHashGraph() :
				forwardedges(0), numforwardedges(), numforwardedgeso(0),
				reverseedges(0), numreverseedges(), numreverseedgeso(0),
				nodelabels(), numnodelabels(0),
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
			}

			void addNode(uint64_t const id, char const symbol)
			{
				while ( !(id < numnodelabels) )
					nodelabels.push(numnodelabels,0);
				assert ( id < numnodelabels );
				nodelabels[id] = symbol;
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

			POGraphEdge<node_id_type> getForwardEdge(uint64_t const from, uint64_t const edgeid) const
			{
				assert ( from < numforwardedgeso );
				assert ( edgeid < numforwardedges[from] );

				POGraphEdge<node_id_type> edge;
				bool const ok = forwardedges.contains(POHashGraphKey<node_id_type>(from,edgeid), edge);
				assert ( ok );

				return edge;
			}

			POGraphEdge<node_id_type> getReverseEdge(uint64_t const to, uint64_t const edgeid) const
			{
				assert ( to < numreverseedgeso );
				assert ( edgeid < numreverseedges[to] );

				POGraphEdge<node_id_type> edge;
				bool const ok = reverseedges.contains(POHashGraphKey<node_id_type>(to,edgeid), edge);
				assert ( ok );

				return edge;
			}

			void insertForwardEdge(uint64_t const from, uint64_t const to)
			{
				while ( !(from < numforwardedgeso) )
					numforwardedges.push(numforwardedgeso,0);
				assert ( from < numforwardedgeso );

				uint64_t const edgeid = numforwardedges[from]++;

				forwardedges.insertNonSyncExtend(
					POHashGraphKey<node_id_type>(from,edgeid),
					POGraphEdge<node_id_type>(to),
					0.8 /* extend load */
				);
			}

			void insertReverseEdge(uint64_t const from, uint64_t const to)
			{
				while ( !(to < numreverseedgeso) )
					numreverseedges.push(numreverseedgeso,0);
				assert ( to < numreverseedgeso );

				uint64_t const edgeid = numreverseedges[to]++;

				reverseedges.insertNonSyncExtend(
					POHashGraphKey<node_id_type>(to,edgeid),
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
						out << "\tnode_" << i << " [label=\"" << nodelabels[i] << "\"];\n";
					else
						out << "\tnode_" << i << " [label=\"" << i << "\"];\n";

				for ( int64_t i = 0; i <= maxfrom; ++i )
					for ( uint64_t j = 0; j < getNumForwardEdges(i); ++j )
						out << "\tnode_" << i << " -> " << "node_" << getForwardEdge(i,j).to << ";\n";

				out << "}\n";

				return out;
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
					char const g = nodelabels[q];

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

				while ( curi != -1 || curj != -1 )
				{
					int64_t const score = (curi>=0&&curj>=0) ? S(curi,curj).s : 0;
					libmaus2::lcs::BaseConstants::step_type t;

					if ( curi != -1 && curj != -1 )
						t = S(curi,curj).t;
					else if ( curi == -1 )
						t = libmaus2::lcs::BaseConstants::step_type::STEP_DEL;
					else
						t = libmaus2::lcs::BaseConstants::step_type::STEP_INS;

					std::cerr << "i=" << curi << " j=" << curj << " t=" << t << std::endl;

					if ( curi == -1 )
					{
						curj -= 1;
					}
					else if ( curj == -1 )
					{
						curi -= 1;
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
