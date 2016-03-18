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
#if !defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYCONSTRUCTION_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYCONSTRUCTION_HPP

#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeExternalBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalSmallBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequest.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBaseBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequestQueryObject.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyConstruction
			{
				static MergeStrategyBlock::shared_ptr_type constructMergeTreeRec(
					std::vector<MergeStrategyBlock::shared_ptr_type> nodes,
					uint64_t const mem,
					uint64_t const numthreads,
					uint64_t const exwordsperthread
				)
				{
					if ( !nodes.size() )
						return MergeStrategyBlock::shared_ptr_type();
					else if ( nodes.size() == 1 )
						return nodes.at(0);

					/**
					 * see if we can do a pairwise merge
					 **/
					std::vector<MergeStrategyBlock::shared_ptr_type> outnodes;
					uint64_t const outnodessize = (nodes.size()+1)/2;
					bool pairok = true;

					for ( uint64_t i = 0; pairok && i < outnodessize; ++i )
					{
						uint64_t const low = 2*i;
						uint64_t const high = std::min(low+2,static_cast<uint64_t>(nodes.size()));

						MergeStrategyBlock::shared_ptr_type node;

						if ( high-low < 2 )
						{
							node = nodes[low];
						}
						if ( (!node) && (nodes[low]->getMergeSpaceInternalGap() <= mem) )
						{
							node = MergeStrategyMergeInternalBlock::construct(std::vector<MergeStrategyBlock::shared_ptr_type>(nodes.begin()+low,nodes.begin()+high));
						}
						if ( (!node) && (nodes[low]->getMergeSpaceInternalSmallGap() <= mem) )
						{
							node = MergeStrategyMergeInternalSmallBlock::construct(std::vector<MergeStrategyBlock::shared_ptr_type>(nodes.begin()+low,nodes.begin()+high));
						}
						if ( (!node) && (nodes[low]->getMergeSpaceExternalGap(numthreads,exwordsperthread) <= mem) )
						{
							node = MergeStrategyMergeExternalBlock::construct(std::vector<MergeStrategyBlock::shared_ptr_type>(nodes.begin()+low,nodes.begin()+high));
						}

						if ( node )
							outnodes.push_back(node);
						else
							pairok = false;
					}

					MergeStrategyBlock::shared_ptr_type node;

					if ( pairok )
						node = constructMergeTreeRec(outnodes,mem,numthreads,exwordsperthread);

					/*
					 * if pairwise merge does not work, try multi-way merge without recursion
					 */
					if ( ! node )
					{
						bool internalok = true;
						for ( uint64_t i = 0; internalok && i+1 < nodes.size(); ++i )
							if ( nodes.at(i)->getMergeSpaceInternalGap() > mem )
								internalok = false;

						if ( internalok )
							node = MergeStrategyMergeInternalBlock::construct(
								std::vector<MergeStrategyBlock::shared_ptr_type>(nodes.begin(),nodes.end())
							);
					}

					if ( ! node )
					{
						bool internalsmallok = true;
						for ( uint64_t i = 0; internalsmallok && i+1 < nodes.size(); ++i )
							if ( nodes.at(i)->getMergeSpaceInternalSmallGap() > mem )
								internalsmallok = false;

						if ( internalsmallok )
							node = MergeStrategyMergeInternalSmallBlock::construct(
								std::vector<MergeStrategyBlock::shared_ptr_type>(nodes.begin(),nodes.end())
							);
					}

					if ( ! node )
					{
						bool externalok = true;
						for ( uint64_t i = 0; externalok && i+1 < nodes.size(); ++i )
							if ( nodes.at(i)->getMergeSpaceExternalGap(numthreads,exwordsperthread) > mem )
								externalok = false;

						if ( externalok )
							node = MergeStrategyMergeExternalBlock::construct(
								std::vector<MergeStrategyBlock::shared_ptr_type>(nodes.begin(),nodes.end())
							);
					}

					return node;
				}

				static MergeStrategyBlock::shared_ptr_type constructMergeTree(
					std::vector<MergeStrategyBlock::shared_ptr_type> nodes,
					uint64_t const mem,
					uint64_t const numthreads,
					uint64_t const exwordsperthread,
					std::ostream * logstr
				)
				{
					if ( logstr )
						*logstr << "[V] Number of input leaf nodes for merge tree construction is " << nodes.size() << std::endl;

					MergeStrategyBlock::shared_ptr_type node = constructMergeTreeRec(nodes,mem,numthreads,exwordsperthread);

					if ( ! node )
					{
						libmaus2::exception::LibMausException se;
						se.getStream() << "There is no way to merge with these settings." << std::endl;
						se.finish();
						throw se;
					}

					node->fillNodeId(0);
					node->fillNodeDepth(0);
					node->fillQueryPositions(numthreads);
					node->setParent(0);

					return node;
				}
			};
		}
	}
}
#endif
