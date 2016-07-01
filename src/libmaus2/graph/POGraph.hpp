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
#include <libmaus2/util/SimpleHashMap.hpp>

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
				uint64_t V[2] = { v.from, v.edgeid };
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

			typedef uint64_t node_id_type;

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

			POHashGraph() :
				forwardedges(0), numforwardedges(), numforwardedgeso(0),
				reverseedges(0), numreverseedges(), numreverseedgeso(0)
			{

			}

			void clear()
			{
				numforwardedgeso = 0;
				numreverseedgeso = 0;
				forwardedges.clearFast();
				reverseedges.clearFast();
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

			bool haveForwardEdge(uint64_t const from, uint64_t const to) const
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
