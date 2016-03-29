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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEBLOCK_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYMERGEBLOCK_HPP

#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequest.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyMergeBlock : public MergeStrategyBlock
			{
				std::vector<MergeStrategyBlock::shared_ptr_type> children;
				std::vector<MergeStrategyMergeGapRequest::shared_ptr_type> gaprequests;
				uint64_t unfinishedChildren;

				MergeStrategyMergeBlock()
				: MergeStrategyBlock()
				{
				}

				void releaseChildren()
				{
					gaprequests.resize(0);
					children.resize(0);
				}

				virtual std::ostream & print(std::ostream & out, uint64_t const indent) const = 0;

				//void fillQueryObjects(std::vector<MergeStrategyMergeGapRequestQueryObject> & VV)
				void fillQueryObjects(libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject> & VV)
				{
					for ( uint64_t i = 0; i < children.size(); ++i )
						children[i]->fillQueryObjects(VV);
				}

				void fillGapRequestObjects(uint64_t const t)
				{
					for ( uint64_t i = 0; i < gaprequests.size(); ++i )
					{
						// std::vector < MergeStrategyMergeGapRequestQueryObject > VV;
						// get gap query objects
						libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject> VV = gaprequests[i]->getQueryPositionObjects(/* VV, */t);
						std::sort(VV.begin(),VV.end());
						// fill the objects (add up ranks for each query position)
						children[gaprequests[i]->into]->fillQueryObjects(VV);
						// size of vector
						uint64_t const vn = VV.size();

						// push z blocks back to front
						for ( uint64_t i = 0; i < VV.size(); ++i )
						{
							uint64_t const ii = vn-i-1;
							VV[i].o->zblocks.push_back(::libmaus2::suffixsort::BwtMergeZBlock(VV[ii].p,VV[ii].r));
						}
					}

					for ( uint64_t i = 0; i < children.size(); ++i )
						children[i]->fillGapRequestObjects(t);
				}

				std::ostream & print(std::ostream & out, uint64_t const indent, std::string const & name) const
				{
					out << "[V]" << std::string(indent,' ')<< name << "(";
					printBase(out);
					out << ")" << std::endl;

					for ( uint64_t i = 0; i < gaprequests.size(); ++i )
						out << "[V]" << std::string(indent+1,' ') << (*(gaprequests[i])) << std::endl;

					for ( uint64_t i = 0; i < children.size(); ++i )
						children[i]->print(out,indent+1);

					return out;
				}

				static void construct(MergeStrategyMergeBlock * pobj, std::vector<MergeStrategyBlock::shared_ptr_type> const children)
				{
					pobj->children = children;

					uint64_t low = children.size() ? children[0]->low : 0;
					uint64_t high = children.size() ? children[0]->high : 0;
					uint64_t sourcelengthbits = 0;
					uint64_t sourcelengthbytes = 0;
					uint64_t codedlength = 0;
					uint64_t sourcetextindexbits = 0;

					for ( uint64_t i = 0; i < children.size(); ++i )
					{
						low = std::min(low,children[i]->low);
						high = std::max(high,children[i]->high);
						sourcelengthbits += children[i]->sourcelengthbits;
						sourcelengthbytes += children[i]->sourcelengthbytes;
						codedlength += children[i]->codedlength;
						sourcetextindexbits += children[i]->sourcetextindexbits;
					}

					pobj->low = low;
					pobj->high = high;
					pobj->sourcelengthbits = sourcelengthbits;
					pobj->sourcelengthbytes = sourcelengthbytes;
					pobj->codedlength = codedlength;
					pobj->sourcetextindexbits = sourcetextindexbits;
					pobj->unfinishedChildren = children.size();

					for ( uint64_t i = 0; i+1 < children.size(); ++i )
					{
						pobj->gaprequests.push_back(
							MergeStrategyMergeGapRequest::shared_ptr_type(
								new MergeStrategyMergeGapRequest(&(pobj->children),i))
						);
					}
				}

				uint64_t fillNodeId(uint64_t i)
				{
					MergeStrategyBlock::nodeid = i++;

					for ( uint64_t j = 0; j < children.size(); ++j )
						i = children[j]->fillNodeId(i);

					return i;
				}

				void fillNodeDepth(uint64_t const i)
				{
					MergeStrategyBlock::nodedepth = i;

					for ( uint64_t j = 0; j < children.size(); ++j )
						children[j]->fillNodeDepth(i+1);
				}

				virtual void registerQueryPositions(std::vector<uint64_t> const & V)
				{
					for ( uint64_t i = 0; i < children.size(); ++ i )
						children[i]->registerQueryPositions(V);
				}

				virtual void fillQueryPositions(uint64_t const t)
				{
					for ( uint64_t i = 0; i < gaprequests.size(); ++i )
					{
						// get query position from gap request
						std::vector<uint64_t> const Q = gaprequests[i]->getQueryPositions(t);
						// register these positions in the leafs
						children[i]->registerQueryPositions(Q);
					}
					// recursive call for children
					for ( uint64_t i = 0; i < children.size(); ++i )
						children[i]->fillQueryPositions(t);
				}
				virtual bool isLeaf() const
				{
					return false;
				}

				void setParent(MergeStrategyBlock * rparent)
				{
					parent = rparent;

					for ( uint64_t i = 0; i < children.size(); ++i )
						children[i]->setParent(this);
				}
				bool childFinished()
				{
					return --unfinishedChildren == 0;
				}
			};
		}
	}
}
#endif
