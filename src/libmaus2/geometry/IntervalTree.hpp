/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_GEOMETRY_INTERVALTREE_HPP)
#define LIBMAUS2_GEOMETRY_INTERVALTREE_HPP

#include <stack>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/math/numbits.hpp>

namespace libmaus2
{
	namespace geometry
	{
		struct IntervalNode
		{
			typedef IntervalNode this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef uint32_t ptr_type;
			ptr_type left;
			ptr_type right;
			bool full;

			static ptr_type getNullPtr()
			{
				return ::std::numeric_limits<ptr_type>::max();
			}

			void reset()
			{
				left = right = getNullPtr();
				full = false;
			}

			IntervalNode()
			{
				reset();
			}
		};

		struct IntervalTreeNodeContainer
		{
			typedef IntervalTreeNodeContainer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::autoarray::AutoArray<IntervalNode> nodes;
			IntervalNode::ptr_type nextid;

			IntervalNode::ptr_type getNode()
			{
				nodes.ensureSize(nextid+1);
				nodes[nextid].reset();
				return nextid++;
			}

			IntervalNode::ptr_type getNodeUnchecked()
			{
				nodes[nextid].reset();
				return nextid++;
			}

			void ensureSize(uint64_t const s)
			{
				if ( nodes.size() < s )
					nodes.ensureSize(s);
			}

			void reset()
			{
				nextid = 0;
			}

			void clean()
			{
				nodes = libmaus2::autoarray::AutoArray<IntervalNode>(0);
				reset();
			}

			IntervalTreeNodeContainer()
			: nodes(), nextid(0)
			{

			}
		};

		struct IntervalTree
		{
			typedef IntervalTree this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t thres;

			IntervalTreeNodeContainer::unique_ptr_type Pcont;
			IntervalTreeNodeContainer & cont;
			libmaus2::autoarray::AutoArray<IntervalNode> & nodes;

			IntervalNode::ptr_type root;

			void reset()
			{
				root = cont.getNode();
			}

			void reset(uint64_t rthres)
			{
				thres = libmaus2::math::nextTwoPow(rthres);
				reset();
			}

			void resetNTP(uint64_t rthres)
			{
				thres = rthres;
				reset();
			}

			void resetNTPUnchecked(uint64_t rthres)
			{
				thres = rthres;
				root = cont.getNodeUnchecked();
			}

			IntervalTree(uint64_t const rthres)
			: thres(libmaus2::math::nextTwoPow(rthres)), Pcont(new IntervalTreeNodeContainer()), cont(*Pcont), nodes(cont.nodes)
			{
				reset();
			}

			IntervalTree(uint64_t const rthres, IntervalTreeNodeContainer & rcont)
			: thres(libmaus2::math::nextTwoPow(rthres)), Pcont(), cont(rcont), nodes(cont.nodes)
			{
				reset();
			}

			struct InsertQueueEntry
			{
				uint64_t left;
				uint64_t right;
				uint64_t nodesize;
				IntervalNode::ptr_type node;
				uint32_t stage;

				InsertQueueEntry()
				{}
				InsertQueueEntry(uint64_t rleft, uint64_t rright, IntervalNode::ptr_type rnode, uint64_t const rnodesize, uint32_t const rstage)
				: left(rleft), right(rright), nodesize(rnodesize), node(rnode), stage(rstage) {}
			};

			struct EnumerateQueueEntry
			{
				uint64_t left;
				uint64_t nodesize;
				IntervalNode::ptr_type node;
				unsigned int stage;

				EnumerateQueueEntry()
				{}

				EnumerateQueueEntry(uint64_t const rleft, uint64_t const rnodesize, IntervalNode::ptr_type rnode, unsigned int const rstage)
				: left(rleft), nodesize(rnodesize), node(rnode), stage(rstage) {}
			};

			struct QueryQueueEntry
			{
				uint64_t left;
				uint64_t nodesize;
				IntervalNode::ptr_type node;

				QueryQueueEntry()
				{}

				QueryQueueEntry(uint64_t const rleft, uint64_t const rnodesize, IntervalNode::ptr_type rnode)
				: left(rleft), nodesize(rnodesize), node(rnode) {}
			};

			bool query(uint64_t const left, uint64_t const right)
			{
				std::stack<QueryQueueEntry> Q;
				Q.push(QueryQueueEntry(0,thres,root));

				while ( Q.size() )
				{
					QueryQueueEntry E = Q.top(); Q.pop();

					if ( right <= E.left || (E.left+E.nodesize) <= left )
						continue;

					if ( nodes[E.node].full )
					{
						return true;
					}
					else
					{
						if ( nodes[E.node].left != IntervalNode::getNullPtr() )
							Q.push(QueryQueueEntry(E.left,E.nodesize/2,nodes[E.node].left));
						if ( nodes[E.node].right != IntervalNode::getNullPtr() )
							Q.push(QueryQueueEntry(E.left+E.nodesize/2,E.nodesize/2,nodes[E.node].right));
					}
				}

				return false;
			}

			void enumerate(std::ostream & out)
			{
				std::stack<EnumerateQueueEntry> Q;
				Q.push(EnumerateQueueEntry(0,thres,root,0));
				std::pair<uint64_t,uint64_t> P(0,0);

				while ( Q.size() )
				{
					EnumerateQueueEntry E = Q.top(); Q.pop();

					if ( nodes[E.node].full )
					{
						if ( E.left == P.second )
							P.second = E.left + E.nodesize;
						else
						{
							if ( P.second-P.first )
								out << "(" << P.first << "," << P.second << ")";

							P.first = E.left;
							P.second = E.left + E.nodesize;
						}

					}
					else if ( E.stage == 0 )
					{
						Q.push(EnumerateQueueEntry(E.left,E.nodesize,E.node,1));
						if ( nodes[E.node].left != IntervalNode::getNullPtr() )
							Q.push(EnumerateQueueEntry(E.left,E.nodesize/2,nodes[E.node].left,0));
					}
					else if ( E.stage == 1 )
					{
						if ( nodes[E.node].right != IntervalNode::getNullPtr() )
							Q.push(EnumerateQueueEntry(E.left+E.nodesize/2,E.nodesize/2,nodes[E.node].right,0));
					}
				}

				if ( P.second-P.first )
					out << "(" << P.first << "," << P.second << ")";
			}

			void ensureLeftExists(IntervalNode::ptr_type const nodeid)
			{
				assert ( nodeid < nodes.size() );
				if ( nodes[nodeid].left == IntervalNode::getNullPtr() )
				{
					uint64_t const newid = cont.getNode();
					nodes[nodeid].left = newid;
				}
				assert ( nodes[nodeid].left != IntervalNode::getNullPtr() );
			}

			void ensureRightExists(IntervalNode::ptr_type const nodeid)
			{
				if ( nodes[nodeid].right == IntervalNode::getNullPtr() )
				{
					uint64_t const newid = cont.getNode();
					nodes[nodeid].right = newid;
				}
				assert ( nodes[nodeid].right != IntervalNode::getNullPtr() );
			}

			void insert(uint64_t const ileft, uint64_t const iright)
			{
				assert ( iright <= thres );

				std::deque<InsertQueueEntry> Q;
				Q.push_back(InsertQueueEntry(ileft,iright,root,thres,0));

				while ( !Q.empty() )
				{
					InsertQueueEntry const E = Q.front(); Q.pop_front();

					switch ( E.stage )
					{
						case 0:
						{
							if ( !nodes[E.node].full )
							{
								if ( E.right-E.left == E.nodesize )
								{
									assert ( E.left == 0 );
									assert ( E.right == E.nodesize );
									nodes[E.node].full = true;
									nodes[E.node].left = IntervalNode::getNullPtr();
									nodes[E.node].right = IntervalNode::getNullPtr();
								}
								else
								{
									assert ( E.nodesize > 1 );
									uint64_t const mid = (E.nodesize>>1);

									if ( E.left < mid )
									{
										if ( E.right <= mid )
										{
											ensureLeftExists(E.node);
											Q.push_back(InsertQueueEntry(E.left,E.right,nodes[E.node].left,E.nodesize/2,0));
										}
										else
										{
											assert ( E.right > mid );
											ensureLeftExists(E.node);
											ensureRightExists(E.node);
											Q.push_back(InsertQueueEntry(E.left,mid,nodes[E.node].left,E.nodesize/2,0));
											Q.push_back(InsertQueueEntry(0,E.right-mid,nodes[E.node].right,E.nodesize/2,0));
										}
									}
									else
									{
										assert ( E.left >= mid );
										ensureRightExists(E.node);
										Q.push_back(InsertQueueEntry(E.left-mid,E.right-mid,nodes[E.node].right,E.nodesize/2,0));
									}

									Q.push_back(
										InsertQueueEntry(
											E.left,E.right,E.node,E.nodesize,E.stage+1
										)
									);
								}

							}

							break;
						}
						case 1:
						{
							if (
								nodes[E.node].left != IntervalNode::getNullPtr()
								&&
								nodes[E.node].right != IntervalNode::getNullPtr()
							)
							{
								IntervalNode::ptr_type nleft = nodes[E.node].left;
								IntervalNode::ptr_type nright = nodes[E.node].right;

								if ( nodes[nleft].full && nodes[nright].full )
								{
									nodes[E.node].full = true;
									nodes[E.node].left = IntervalNode::getNullPtr();
									nodes[E.node].right = IntervalNode::getNullPtr();
								}
							}
							break;
						}
					}
				}
			}
		};
	}
}
#endif
