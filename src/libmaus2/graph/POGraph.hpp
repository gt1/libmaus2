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
#include <libmaus2/aio/FileRemoval.hpp>

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
			POGraphEdge(std::istream & in)
			{
				deserialise(in);
			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,to);
				return out;
			}

			std::istream & deserialise(std::istream & in)
			{
				to = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				return in;
			}
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
			POHashGraphKey(std::istream & in)
			{
				deserialise(in);
			}

			bool operator==(POHashGraphKey<node_id_type> const & O) const
			{
				return from == O.from && index == O.index;
			}
			bool operator!=(POHashGraphKey<node_id_type> const & O) const
			{
				return ! (*this == O);
			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,from);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,index);
				return out;
			}

			void deserialise(std::istream & in)
			{
				from = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				index = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
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
			NodeInfo(std::istream & in)
			{
				deserialise(in);
			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,label);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,numalignedto);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,usecnt);
				return out;
			}

			void deserialise(std::istream & in)
			{
				label = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				numalignedto = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				usecnt = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			bool operator==(NodeInfo const & O) const
			{
				return label == O.label && numalignedto == O.numalignedto && usecnt == O.usecnt;
			}
		};

		struct NodeInfoLabelComparator
		{
			bool operator()(NodeInfo const & A, NodeInfo const & B) const
			{
				return A.label < B.label;
			}
		};

		struct NodeInfoLabelIndexComparator
		{
			NodeInfo const * I;

			NodeInfoLabelIndexComparator(NodeInfo const * rI)
			: I(rI)
			{

			}

			bool operator()(uint64_t const i, uint64_t const j) const
			{
				return I[i].label < I[j].label;
			}
		};

		template<typename object_type>
		struct POHashGraphIOHelper
		{
			std::ostream & serialise(std::ostream & out, object_type const & k)
			{
				k.serialise(out);
				return out;
			}

			std::istream & deserialise(std::istream & in, object_type & k)
			{
				k.deserialise(in);
				return in;
			}
		};

		struct POHashGraph
		{
			typedef POHashGraph this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

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

			std::vector < NodeInfo > getNodeLabels() const
			{
				return std::vector < NodeInfo >(nodelabels.begin(),nodelabels.begin()+numnodelabels);
			}

			std::vector < std::pair<uint64_t,uint64_t> > getForwardEdges() const
			{
				std::vector < std::pair<uint64_t,uint64_t> > V;
				for ( uint64_t i = 0; i < numforwardedgeso; ++i )
				{
					uint64_t const ne = getNumForwardEdges(i);
					for ( uint64_t j = 0; j < ne; ++j )
						V.push_back(
							std::pair<uint64_t,uint64_t>(i,getForwardEdge(i,j).to)
						);
				}
				return V;
			}

			std::vector < std::pair<uint64_t,uint64_t> > getSortedForwardEdges() const
			{
				std::vector < std::pair<uint64_t,uint64_t> > V;
				for ( uint64_t i = 0; i < numforwardedgeso; ++i )
				{
					uint64_t const ne = getNumForwardEdges(i);
					for ( uint64_t j = 0; j < ne; ++j )
						V.push_back(
							std::pair<uint64_t,uint64_t>(i,getForwardEdge(i,j).to)
						);
				}
				std::sort(V.begin(),V.end());
				return V;
			}

			std::vector < std::pair<uint64_t,uint64_t> > getReverseEdges() const
			{
				std::vector < std::pair<uint64_t,uint64_t> > V;
				for ( uint64_t i = 0; i < numreverseedgeso; ++i )
				{
					uint64_t const ne = getNumReverseEdges(i);
					for ( uint64_t j = 0; j < ne; ++j )
						V.push_back(
							std::pair<uint64_t,uint64_t>(i,getReverseEdge(i,j).to)
						);
				}
				return V;
			}

			std::vector < std::pair<uint64_t,uint64_t> > getSortedReverseEdges() const
			{
				std::vector < std::pair<uint64_t,uint64_t> > V;
				for ( uint64_t i = 0; i < numreverseedgeso; ++i )
				{
					uint64_t const ne = getNumReverseEdges(i);
					for ( uint64_t j = 0; j < ne; ++j )
						V.push_back(
							std::pair<uint64_t,uint64_t>(i,getReverseEdge(i,j).to)
						);
				}
				std::sort(V.begin(),V.end());
				return V;
			}

			std::vector < std::pair<uint64_t,uint64_t> > getAlignedToVector() const
			{
				std::vector < std::pair<uint64_t,uint64_t> > V;
				for ( uint64_t i = 0; i < numalignedtoo; ++i )
				{
					uint64_t const ne = numalignedto[i];
					for ( uint64_t j = 0; j < ne; ++j )
						V.push_back(
							std::pair<uint64_t,uint64_t>(i,getAlignedTo(i,j))
						);
				}
				return V;
			}

			std::ostream & serialise(std::ostream & out) const
			{
				assert ( checkConsistency() );

				POHashGraphIOHelper< POHashGraphKey<node_id_type> > edge_key_helper;
				POHashGraphIOHelper< POGraphEdge<node_id_type> > edge_value_helper;

				// edges
				forwardedges.serialiseKV(out,edge_key_helper,edge_value_helper);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,numforwardedgeso);
				numforwardedges.serialize(out);

				reverseedges.serialiseKV(out,edge_key_helper,edge_value_helper);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,numreverseedgeso);
				numreverseedges.serialize(out);

				// node labels
				libmaus2::util::NumberSerialisation::serialiseNumber(out,numnodelabels);
				for ( uint64_t i = 0; i < numnodelabels; ++i )
					nodelabels[i].serialise(out);

				// aligned to
				libmaus2::util::NumberSerialisation::serialiseNumber(out,numalignedtoo);
				for ( uint64_t i = 0; i < numalignedtoo; ++i )
				{
					uint64_t const na = getNumAlignedTo(i);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,na);
					for ( uint64_t j = 0; j < na; ++j )
					{
						uint64_t const tgt = getAlignedTo(i,j);
						libmaus2::util::NumberSerialisation::serialiseNumber(out,tgt);
					}
				}

				return out;
			}

			void serialise(std::string const & fn) const
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				serialise(OSI);
			}

			void deserialise(std::istream & in)
			{
				clear();

				POHashGraphIOHelper< POHashGraphKey<node_id_type> > edge_key_helper;
				POHashGraphIOHelper< POGraphEdge<node_id_type> > edge_value_helper;

				forwardedges.deserialiseKV(in,edge_key_helper,edge_value_helper);
				numforwardedgeso = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				numforwardedges.deserialize(in);

				reverseedges.deserialiseKV(in,edge_key_helper,edge_value_helper);
				numreverseedgeso = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				numreverseedges.deserialize(in);

				numnodelabels = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				nodelabels.ensureSize(numnodelabels);
				for ( uint64_t i = 0; i < numnodelabels; ++i )
					nodelabels[i].deserialise(in);

				numalignedtoo = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				numalignedto.ensureSize(numalignedtoo);
				std::fill(numalignedto.begin(),numalignedto.end(),0ull);
				for ( uint64_t i = 0; i < numalignedtoo; ++i )
				{
					uint64_t const na = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					for ( uint64_t j = 0; j < na; ++j )
					{
						uint64_t const tgt = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
						addAlignedTo(i,tgt);
					}
				}

				assert ( checkConsistency() );
			}

			void deserialise(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				deserialise(ISI);
			}

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

			bool isAlignedTo(uint64_t const src, uint64_t const tgt) const
			{
				uint64_t const na = getNumAlignedTo(src);
				for ( uint64_t i = 0; i < na; ++i )
					if ( getAlignedTo(src,i) == tgt )
						return true;
				return false;
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

			void putAlignedTo(uint64_t const * v, uint64_t const na)
			{
				for ( uint64_t i = 0; i < na; ++i )
				{
					uint64_t const src = v[i];

					while ( ! (src < numalignedtoo) )
						numalignedto.push(numalignedtoo,0);
					assert ( src < numalignedtoo );

					numalignedto[src] = 0;

					for ( uint64_t j = 0; j < na; ++j )
						if ( j != i )
						{
							uint64_t const index = numalignedto[src]++;

							alignedto.insertNonSyncExtend(
								POHashGraphKey<node_id_type>(src,index),
								POGraphEdge<node_id_type>(v[j]),
								0.8 /* extend load */
							);
						}
				}
			}

			uint64_t getNumAlignedTo(uint64_t const id) const
			{
				if ( id < numalignedtoo )
					return numalignedto[id];
				else
					return 0;
			}

			bool checkConsistency() const
			{
				return
					checkAlignedToConsistency() &&
					checkEdgeConsistency();
			}

			bool checkAlignedToConsistency() const
			{
				int64_t const maxnode = getMaxNode();
				uint64_t const nodethres = maxnode+1;

				bool ok = true;
				for ( uint64_t i = 0; i < nodethres; ++i )
				{
					uint64_t const na = getNumAlignedTo(i);

					for ( uint64_t j = 0; j < na; ++j )
					{
						uint64_t const to = getAlignedTo(i,j);
						ok = ok && isAlignedTo(to,i);
					}
				}

				return ok;
			}

			uint64_t getAlignedTo(uint64_t const id, uint64_t const aid) const
			{
				assert ( id < numalignedtoo );
				assert ( aid < numalignedto[id] );

				POGraphEdge<node_id_type> edge;
				#if ! defined(NDEBUG)
				bool const ok =
				#endif
					alignedto.contains(POHashGraphKey<node_id_type>(id,aid), edge);
				#if ! defined(NDEBUG)
				assert ( ok );
				#endif

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

			uint64_t addNode(uint64_t const id, char const symbol)
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

				return id;
			}

			uint64_t addNode(char const symbol)
			{
				uint64_t const nodeid = numnodelabels;
				return addNode(nodeid,symbol);
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

			bool checkEdgeConsistency() const
			{
				bool ok = true;

				for ( uint64_t i = 0; i < numforwardedgeso; ++i )
					for ( uint64_t j = 0; j < numforwardedges[i]; ++j )
					{
						uint64_t src = i;
						uint64_t tgt = getForwardEdge(src,j).to;
						ok = ok && haveReverseEdge(tgt,src);
					}

				for ( uint64_t i = 0; i < numreverseedgeso; ++i )
					for ( uint64_t j = 0; j < numreverseedges[i]; ++j )
					{
						uint64_t src = i;
						uint64_t tgt = getReverseEdge(src,j).to;
						ok = ok && haveForwardEdge(tgt,src);
					}

				return ok;
			}

			POGraphEdge<node_id_type> getForwardEdge(uint64_t const from, uint64_t const index) const
			{
				assert ( from < numforwardedgeso );
				assert ( index < numforwardedges[from] );

				POGraphEdge<node_id_type> edge;
				#if ! defined(NDEBUG)
				bool const ok =
				#endif
					forwardedges.contains(POHashGraphKey<node_id_type>(from,index), edge);
				#if ! defined(NDEBUG)
				assert ( ok );
				#endif

				return edge;
			}

			POGraphEdge<node_id_type> getReverseEdge(uint64_t const to, uint64_t const index) const
			{
				assert ( to < numreverseedgeso );
				assert ( index < numreverseedges[to] );

				POGraphEdge<node_id_type> edge;
				#if ! defined(NDEBUG)
				bool const ok =
				#endif
					reverseedges.contains(POHashGraphKey<node_id_type>(to,index), edge);
				#if ! defined(NDEBUG)
				assert ( ok );
				#endif

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
						#if ! defined(NDEBUG)
						bool const ok =
						#endif
							forwardedges.contains(POHashGraphKey<node_id_type>(from,i), edge);
						#if ! defined(NDEBUG)
						assert ( ok );
						#endif
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

			bool haveReverseEdge(uint64_t const from, int64_t const to) const
			{
				if ( from < numreverseedgeso )
				{
					uint64_t const num = numreverseedges[from];

					for ( uint64_t i = 0; i < num; ++i )
					{
						POGraphEdge<node_id_type> edge;

						#if ! defined(NDEBUG)
						bool const ok =
						#endif
							reverseedges.contains(POHashGraphKey<node_id_type>(from,i), edge);
						#if ! defined(NDEBUG)
						assert ( ok );
						#endif
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

			int64_t getMaxNode() const
			{
				return std::max(getMaxFrom(),getMaxTo());
			}

			template<typename iterator>
			void setupSingle(iterator it, uint64_t const n)
			{
				clear();

				if ( n )
				{
					uint64_t const nodeid = addNode(*(it++));
					nodelabels[nodeid].usecnt = 1;
					assert ( nodeid == 0 );
				}

				for ( uint64_t i = 1; i < n; ++i )
				{
					uint64_t const nodeid = addNode(*(it++));
					assert ( nodeid == i );
					insertEdge(i-1,i);
					nodelabels[nodeid].usecnt = 1;
				}
			}

			void toDot(std::string const & fn) const
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				toDot(OSI);
			}

			void toSvg(std::string const & fn) const
			{
				std::string const dotfn = fn + ".dot";
				std::string const svgfn = fn + ".svg";
				toDot(dotfn);
				std::ostringstream comstr;
				comstr << "dot -Tsvg <" << dotfn << " >" << svgfn;
				int const r = system(comstr.str().c_str());

				if ( r != EXIT_SUCCESS )
				{
					int const error = errno;
					std::cerr << "failed: " << comstr.str() << ":" << strerror(error) << std::endl;
				}

				libmaus2::aio::FileRemoval::removeFile(dotfn);
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
						out << "\tnode_" << i << " [label=\"" << nodelabels[i].label << "," << nodelabels[i].usecnt << "," << i;

						#if 1
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

			struct AlignContext
			{
				typedef AlignContext this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::autoarray::AutoArray2d<TraceNode> S;
				libmaus2::autoarray::AutoArray2d<uint64_t> U;
				libmaus2::util::SimpleQueue< std::pair<uint64_t,uint64_t> > Q;
				libmaus2::autoarray::AutoArray<int64_t> BM;
				libmaus2::autoarray::AutoArray<uint64_t> Atmp;
				libmaus2::autoarray::AutoArray<uint64_t> Btmp;
				libmaus2::autoarray::AutoArray2d<uint64_t> C;
				libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > AM;
				libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > AU;

				AlignContext() {}
			};

			/*
			 * PO/PO alignment
			 */
			static void align(POHashGraph & C, POHashGraph const & A, POHashGraph const & B, AlignContext & context)
			{
				assert ( A.checkEdgeConsistency() );
				assert ( B.checkEdgeConsistency() );
				assert ( A.checkAlignedToConsistency() );
				assert ( B.checkAlignedToConsistency() );

				libmaus2::autoarray::AutoArray2d<TraceNode> & context_S = context.S;
				libmaus2::autoarray::AutoArray2d<uint64_t> & context_U = context.U;
				libmaus2::util::SimpleQueue< std::pair<uint64_t,uint64_t> > & context_Q = context.Q;
				libmaus2::autoarray::AutoArray<int64_t> & context_BM = context.BM;
				libmaus2::autoarray::AutoArray<uint64_t> & context_Atmp = context.Atmp;
				libmaus2::autoarray::AutoArray<uint64_t> & context_Btmp = context.Btmp;
				libmaus2::autoarray::AutoArray2d<uint64_t> & context_C = context.C;
				libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > context_AM = context.AM;
				libmaus2::autoarray::AutoArray < std::pair<uint64_t,uint64_t> > context_AU = context.AU;

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

				for ( uint64_t i = 0; i < nodelimit_A; ++i )
				{
					uint64_t na = A.getAllAlignedTo(i,context_Atmp);
					for ( uint64_t z = 0; z < na; ++z )
						assert ( A.isAlignedTo(i,context_Atmp[z]) );
				}
				for ( uint64_t j = 0; j < nodelimit_B; ++j )
				{
					uint64_t nb = B.getAllAlignedTo(j,context_Btmp);
					for ( uint64_t z = 0; z < nb; ++z )
						assert ( B.isAlignedTo(j,context_Btmp[z]) );
				}

				context_S.setup(nodelimit_A,nodelimit_B);
				context_U.setup(nodelimit_A,nodelimit_B);

				context_C.setup(nodelimit_A,nodelimit_B);
				for ( uint64_t i = 0; i < nodelimit_A; ++i )
					for ( uint64_t j = 0; j < nodelimit_B; ++j )
						context_C(i,j) = 0;

				// compute number of predecessors for each node
				// put nodes with no predecessors in queue
				for ( uint64_t i = 0; i < nodelimit_A; ++i )
				{
					uint64_t const va = A.getNumReverseEdges(i);
					for ( uint64_t j = 0; j < nodelimit_B; ++j )
					{
						uint64_t const vb = B.getNumReverseEdges(j);
						uint64_t const vc = va * vb;
						context_U(i,j) =  va + vb + vc;
						if ( ! context_U(i,j) )
						{
							context_Q.push_back(std::pair<uint64_t,uint64_t>(i,j));
						}
					}
				}

				int64_t mscore = std::numeric_limits<int64_t>::min();
				int64_t mscorei = std::numeric_limits<int64_t>::min();
				int64_t mscorej = std::numeric_limits<int64_t>::min();

				// fill dynamic programming matrix
				while ( ! context_Q.empty() )
				{
					std::pair<uint64_t,uint64_t> const P = context_Q.pop_front();
					uint64_t const i = P.first;
					uint64_t const j = P.second;
					int64_t previ = i;
					int64_t prevj = j;

					// std::cerr << "processing " << i << "," << j << std::endl;

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
								// std::cerr << "accessing " << revi << "," << revj << " for match/mismatch" << std::endl;
								assert ( context_C(revi,revj) );
								int64_t const lscore = context_S(revi,revj).s + (match?1:0);
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
						// std::cerr << "accessing " << revi << "," << j << " for del" << std::endl;
						assert ( context_C(revi,j) );
						int64_t const lscore = context_S(revi,j).s;
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
						// std::cerr << "accessing " << i << "," << revj << " for ins" << std::endl;
						assert ( context_C(i,revj) );
						int64_t const lscore = context_S(i,revj).s;
						if ( lscore > score )
						{
							score = lscore;
							previ = i;
							prevj = revj;
							t = libmaus2::lcs::BaseConstants::STEP_INS;
						}
					}

					context_S(i,j) = TraceNode(previ,prevj,score,t);
					context_C(i,j) = 1;

					if ( score >= mscore )
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
							assert ( context_U(forw_A,forw_B) );
							uint64_t const u = --context_U(forw_A,forw_B);
							if ( !u )
								context_Q.push_back(std::pair<uint64_t,uint64_t>(forw_A,forw_B));
						}
					}

					for ( uint64_t ri = 0; ri < next_A; ++ri )
					{
						uint64_t const forw_A = A.getForwardEdge(i,ri).to;
						assert ( context_U(forw_A,j) );
						uint64_t const u = --context_U(forw_A,j);
						if ( ! u )
							context_Q.push_back(std::pair<uint64_t,uint64_t>(forw_A,j));
					}

					for ( uint64_t rj = 0; rj < next_B; ++rj )
					{
						uint64_t const forw_B = B.getForwardEdge(j,rj).to;
						assert ( context_U(i,forw_B) );
						uint64_t const u = --context_U(i,forw_B);
						if ( ! u )
							context_Q.push_back(std::pair<uint64_t,uint64_t>(i,forw_B));
					}
				}

				#if 0
				for ( uint64_t j = 0; j < nodelimit_B; ++j )
				{
					for ( uint64_t i = 0; i < nodelimit_A; ++i )
						std::cerr << static_cast<int>(context_S(i,j).s) << ((i+1<nodelimit_A)?" ":"");
					std::cerr << std::endl;
				}
				#endif

				// B mapping
				context_BM.ensureSize(nodelimit_B);
				std::fill(context_BM.begin(), context_BM.end() ,std::numeric_limits<int64_t>::min());

				// copy A to C
				C = A;

				uint64_t amo = 0;

				// trace back from maximum score
				while ( mscorei >= 0 && mscorej >= 0 )
				{
					TraceNode const  T = context_S(mscorei,mscorej);

					// std::cerr << "trace " << mscorei << "," << mscorej << "," << T.t << std::endl;

					// push aligned pair
					if (
						T.t == libmaus2::lcs::BaseConstants::STEP_MATCH
						||
						T.t == libmaus2::lcs::BaseConstants::STEP_MISMATCH
					)
						// put on fuse list
						context_AM.push(amo,std::pair<uint64_t,uint64_t>(mscorei,mscorej));

					mscorei = T.i;
					mscorej = T.j;
				}

				// compute representants for fuse lists
				uint64_t auo = 0;

				for ( uint64_t z = 0; z < amo; ++z )
				{
					uint64_t const mscorei = context_AM[z].first;
					uint64_t const mscorej = context_AM[z].second;

					// get all nodes aligned to mscorei
					uint64_t na = A.getAllAlignedTo(mscorei,context_Atmp);
					context_Atmp.push(na,mscorei);

					// get all nodes aligned to mscorej
					uint64_t nb = B.getAllAlignedTo(mscorej,context_Btmp);
					context_Btmp.push(nb,mscorej);

					// sort by id
					std::sort(context_Atmp.begin(),context_Atmp.begin()+na);
					std::sort(context_Btmp.begin(),context_Btmp.begin()+nb);

					libmaus2::autoarray::AutoArray<uint64_t> C_Atmp;
					libmaus2::autoarray::AutoArray<uint64_t> C_Btmp;
					uint64_t c_na = A.getAllAlignedTo(context_Atmp[0],C_Atmp);
					C_Atmp.push(c_na,context_Atmp[0]);
					uint64_t c_nb = B.getAllAlignedTo(context_Btmp[0],C_Btmp);
					C_Btmp.push(c_nb,context_Btmp[0]);

					std::sort(C_Atmp.begin(),C_Atmp.begin()+c_na);
					std::sort(C_Btmp.begin(),C_Btmp.begin()+c_nb);

					assert ( na == c_na );
					assert ( nb == c_nb );

					assert ( std::equal(context_Atmp.begin(),context_Atmp.begin()+na,C_Atmp.begin()));
					assert ( std::equal(context_Btmp.begin(),context_Btmp.begin()+nb,C_Btmp.begin()));

					context_AU.push(auo,std::pair<uint64_t,uint64_t>(context_Atmp[0],context_Btmp[0]));
				}

				// sort representants
				std::sort(context_AU.begin(),context_AU.begin()+auo);
				// filter out duplicates
				auo = std::unique(context_AU.begin(),context_AU.begin()+auo) - context_AU.begin();

				// iterate over representants
				for ( uint64_t z = 0; z < auo; ++z )
				{
					uint64_t const mscorei = context_AM[z].first;
					uint64_t const mscorej = context_AM[z].second;

					// get all nodes aligned to representant in A and B
					uint64_t na = A.getAllAlignedTo(mscorei,context_Atmp);
					context_Atmp.push(na,mscorei);
					uint64_t nb = B.getAllAlignedTo(mscorej,context_Btmp);
					context_Btmp.push(nb,mscorej);

					// sort by label
					std::sort(context_Atmp.begin(),context_Atmp.begin()+na,NodeInfoLabelIndexComparator(A.nodelabels.begin()));
					for ( uint64_t i = 1; i < na; ++i )
						assert (
							A.nodelabels[context_Atmp[i-1]].label
							<
							A.nodelabels[context_Atmp[  i]].label
						);
					std::sort(context_Btmp.begin(),context_Btmp.begin()+nb,NodeInfoLabelIndexComparator(B.nodelabels.begin()));
					for ( uint64_t i = 1; i < nb; ++i )
						assert (
							B.nodelabels[context_Btmp[i-1]].label
							<
							B.nodelabels[context_Btmp[  i]].label
						);

					// merge
					uint64_t ia = 0, ib = 0;
					while ( ia < na && ib < nb )
					{
						uint64_t const aidx = context_Atmp[ia];
						uint64_t const bidx = context_Btmp[ib];
						char const la = A.nodelabels[aidx].label;
						char const lb = B.nodelabels[bidx].label;

						if ( la < lb )
							++ia;
						else if ( lb < la )
							++ib;
						// same label
						else
						{
							// fuse nodes
							context_BM  [bidx]         =              aidx;
							C.nodelabels[aidx].usecnt += B.nodelabels[bidx].usecnt;

							++ia;
							++ib;

							assert ( ia == na || A.nodelabels[context_Atmp[ia]].label != la );
							assert ( ib == nb || B.nodelabels[context_Btmp[ib]].label != lb );
						}
					}
				}

				// assign new node ids for unaligned nodes in B
				uint64_t nextnodeid = nodelimit_A;
				for ( uint64_t i = 0; i < nodelimit_B; ++i )
					if ( context_BM[i] < 0 )
					{
						context_BM[i] = nextnodeid++;
						// assert ( context_BM[i] >= nodelimit_A );
						uint64_t const cnodeid = C.addNode(context_BM[i],B.nodelabels[i].label);
						assert ( static_cast<int64_t>(cnodeid) == context_BM[i] );
						C.nodelabels[cnodeid].usecnt = B.nodelabels[i].usecnt;
					}
				// copy aligned to info from B to C for nodes which are not aligned to nodes in A
				for ( uint64_t i = 0; i < nodelimit_B; ++i )
					if ( context_BM[i] >= static_cast<int64_t>(nodelimit_A) )
					{
						uint64_t nb = B.getAllAlignedTo(i,context_Btmp);
						for ( uint64_t j = 0; j < nb; ++j )
							C.addAlignedTo(context_BM[i],context_BM[context_Btmp[j]]);
					}

				// iterate over representants
				for ( uint64_t z = 0; z < auo; ++z )
				{
					uint64_t const mscorei = context_AM[z].first;
					uint64_t const mscorej = context_AM[z].second;

					// get all nodes aligned to representant in A and B
					uint64_t na = A.getAllAlignedTo(mscorei,context_Atmp);
					context_Atmp.push(na,mscorei);
					uint64_t nb = B.getAllAlignedTo(mscorej,context_Btmp);
					context_Btmp.push(nb,mscorej);

					// map to C graph ids
					for ( uint64_t i = 0; i < nb; ++i )
						context_Btmp[i] = context_BM[context_Btmp[i]];

					// append B nodes to A nodes to obtain list of C nodes
					for ( uint64_t i = 0; i < nb; ++i )
						context_Atmp.push(na,context_Btmp[i]);

					// sort
					std::sort(context_Atmp.begin(),context_Atmp.begin()+na);
					// uniquify
					na = std::unique(context_Atmp.begin(),context_Atmp.begin()+na)-context_Atmp.begin();

					// inject align to data
					C.putAlignedTo(context_Atmp.begin(),na);
				}

				// copy non redundant edges from B
				for ( uint64_t i = 0; i < nodelimit_B; ++i )
					for ( uint64_t j = 0; j < B.getNumForwardEdges(i); ++j )
					{
						uint64_t const btgt = B.getForwardEdge(i,j).to;

						uint64_t const src = context_BM[i];
						uint64_t const tgt = context_BM[btgt];

						if ( !C.haveForwardEdge(src,tgt) )
							C.insertEdge(src,tgt);
					}

				for ( uint64_t i = 0; i < nodelimit_A; ++i )
				{
					uint64_t na = A.getAllAlignedTo(i,context_Atmp);
					for ( uint64_t z = 0; z < na; ++z )
						assert ( C.isAlignedTo(i,context_Atmp[z]) );
				}
				for ( uint64_t j = 0; j < nodelimit_B; ++j )
				{
					uint64_t nb = B.getAllAlignedTo(j,context_Btmp);
					for ( uint64_t z = 0; z < nb; ++z )
						assert ( C.isAlignedTo(context_BM[j],context_BM[context_Btmp[z]]) );
				}
				for ( uint64_t z = 0; z < amo; ++z )
				{
					uint64_t const mscorei = context_AM[z].first;
					uint64_t const mscorej = context_AM[z].second;
					bool const ok =
					(
						static_cast<int64_t>(mscorei) == context_BM[mscorej]
						||
						C.isAlignedTo(
							mscorei,
							context_BM[mscorej]
						)
					);

					if ( ! ok )
					{
						std::cerr << "missing aligned to " << mscorei << " " << mscorej << ":" << context_BM[mscorej] << std::endl;
					}

					assert ( ok );
				}

				assert ( C.checkEdgeConsistency() );
				assert ( C.checkAlignedToConsistency() );
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
						t = libmaus2::lcs::BaseConstants::STEP_INS;
					else
						t = libmaus2::lcs::BaseConstants::STEP_DEL;

					// std::cerr << "i=" << curi << " j=" << curj << " t=" << t << std::endl;

					switch ( t )
					{
						case libmaus2::lcs::BaseConstants::STEP_MATCH:
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
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
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
						case libmaus2::lcs::BaseConstants::STEP_INS:
						{
							uint64_t const id = addNode(it[curj]);
							if ( prevnode != -1 )
								insertEdge(id,prevnode);
							prevnode = id;
							break;
						}
						case libmaus2::lcs::BaseConstants::STEP_DEL:
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
