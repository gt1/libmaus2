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
#if ! defined(LIBMAUS2_LCS_CHAIN_HPP)
#define LIBMAUS2_LCS_CHAIN_HPP

#include <libmaus2/lcs/ChainNodeInfo.hpp>
#include <libmaus2/lcs/ChainNode.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/types/types.hpp>
#include <libmaus2/util/ContainerElementFreeList.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		struct Chain
		{
			typedef Chain this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t head;
			uint64_t last;
			uint64_t length;
			uint64_t rightmost;
			uint64_t covercount;
			uint64_t refid;

			Chain()
			: head(std::numeric_limits<uint64_t>::max()), last(std::numeric_limits<uint64_t>::max()), length(0), rightmost(0), covercount(0), refid(0)
			{

			}

			uint64_t getRefID() const
			{
				return refid;
			}

			uint64_t getRange(libmaus2::util::ContainerElementFreeList<ChainNode> & nodes) const
			{
				return rightmost - getLeftMost(nodes);
			}

			uint64_t getLeftMost(libmaus2::util::ContainerElementFreeList<ChainNode> & nodes) const
			{
				assert ( head != std::numeric_limits<uint64_t>::max() );
				return nodes.getElementsArray()[head].xleft;
			}

			uint64_t getId(libmaus2::util::ContainerElementFreeList<ChainNode> & nodes) const
			{
				assert ( head != std::numeric_limits<uint64_t>::max() );
				return nodes.getElementsArray()[head].chainid;
			}

			uint64_t getChainScore(libmaus2::util::ContainerElementFreeList<ChainNode> & nodes) const
			{
				assert ( head != std::numeric_limits<uint64_t>::max() );
				return nodes.getElementsArray()[head].chainscore;
			}

			void push(libmaus2::util::ContainerElementFreeList<ChainNode> & nodes, ChainNode const & CN)
			{
				uint64_t const node = nodes.getNewNode();
				nodes.getElementsArray()[node] = CN;

				if ( ! length )
				{
					head = last = node;
					length = 1;
				}
				else
				{
					nodes.getElementsArray()[last].next = node;
					last = node;
					length += 1;
				}
			}

			uint64_t getInfo(libmaus2::util::ContainerElementFreeList<ChainNode> & chainnodefreelist, libmaus2::autoarray::AutoArray<ChainNodeInfo> & A) const
			{
				uint64_t o = 0;

				for ( uint64_t cur = head; cur != std::numeric_limits<uint64_t>::max(); cur = chainnodefreelist.getElementsArray()[cur].next )
				{
					ChainNode const & CN = chainnodefreelist.getElementsArray()[cur];
					A.push(o,ChainNodeInfo(CN.parentsubid,CN.xleft,CN.xright,CN.yleft,CN.yright));
				}

				return o;
			}

			void returnNodes(libmaus2::util::ContainerElementFreeList<ChainNode> & chainnodefreelist)
			{
				for ( uint64_t cur = head; cur != std::numeric_limits<uint64_t>::max(); cur = chainnodefreelist.getElementsArray()[cur].next )
					chainnodefreelist.deleteNode(cur);

				head = last = std::numeric_limits<uint64_t>::max();
				length = 0;
			}
		};

		std::ostream & operator<<(std::ostream & out, Chain const & C);
	}
}
#endif
