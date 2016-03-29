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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYBASEBLOCK_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYBASEBLOCK_HPP

#include <libmaus2/suffixsort/BwtMergeBlockSortRequest.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBlock.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyBaseBlock : public MergeStrategyBlock
			{
				libmaus2::suffixsort::BwtMergeBlockSortRequest sortreq;
				std::vector<uint64_t> querypos;

				MergeStrategyBaseBlock() : MergeStrategyBlock() {}

				MergeStrategyBaseBlock(
					uint64_t const rlow, uint64_t const rhigh,
					::libmaus2::huffman::HuffmanTree::EncodeTable & EC,
					std::map<int64_t,uint64_t> const & blockhist,
					uint64_t const rsourcelengthbits,
					uint64_t const rsourcelengthbytes,
					uint64_t const rsourcetextindexbits
				)
				: MergeStrategyBlock(rlow,rhigh,rsourcelengthbits,rsourcelengthbytes,rsourcetextindexbits)
				{
					for (
						std::map<int64_t,uint64_t>::const_iterator ita = blockhist.begin();
						ita != blockhist.end();
						++ita )
					{
						MergeStrategyBlock::codedlength += (EC).getCodeLength(ita->first) * ita->second;
					}
				}

				std::ostream & print(std::ostream & out, uint64_t const indent) const
				{
					out << "[V]" << std::string(indent,' ') << "MergeStrategyBaseBlock(";
					printBase(out);
					out << ")" << std::endl;
					out << "[V]" << std::string(indent+1,' ') << "qp={";
					for ( uint64_t i = 0; i < querypos.size(); ++i )
					{
						out << querypos[i];
						if ( i+1 < querypos.size() )
							out << ";";
					}
					out << "}" << std::endl;
					out << "[V]" << std::string(indent+1,' ') << "req=";
					out << sortreq;
					out << std::endl;
					return out;
				}

				static MergeStrategyBlock::shared_ptr_type construct(
					uint64_t const rlow, uint64_t const rhigh,
					::libmaus2::huffman::HuffmanTree::EncodeTable & EC,
					std::map<int64_t,uint64_t> const & blockhist,
					uint64_t const rsourcelengthbits,
					uint64_t const rsourcelengthbytes,
					uint64_t const rsourcetextindexbits
				)
				{
					return MergeStrategyBlock::shared_ptr_type(new MergeStrategyBaseBlock(rlow,rhigh,EC,blockhist,rsourcelengthbits,rsourcelengthbytes,rsourcetextindexbits));
				}

				uint64_t fillNodeId(uint64_t i)
				{
					MergeStrategyBlock::nodeid = i++;
					return i;
				}

				void fillNodeDepth(uint64_t const i)
				{
					MergeStrategyBlock::nodedepth = i;
				}

				virtual void registerQueryPositions(std::vector<uint64_t> const & V)
				{
					std::copy(V.begin(),V.end(),std::back_insert_iterator< std::vector<uint64_t> >(querypos));
				}

				void fillQueryPositions(uint64_t const /* t */)
				{
					::libmaus2::suffixsort::BwtMergeZBlockRequestVector zreqvec;
					zreqvec.resize(querypos.size());
					for ( uint64_t i = 0; i < querypos.size(); ++i )
						zreqvec[i] = ::libmaus2::suffixsort::BwtMergeZBlockRequest(querypos[i]);
					sortreq.zreqvec = zreqvec;
				}

				void fillQueryObjects(libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject> & VV)
				{
					libmaus2::autoarray::AutoArray < ::libmaus2::suffixsort::BwtMergeZBlock > const & Z = sortresult.getZBlocks();

					for ( uint64_t i = 0; i < Z.size(); ++i )
					{
						uint64_t const p = Z[i].getZAbsPos();
						uint64_t const r = Z[i].getZRank();

						// search objects with matching position
						typedef libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject>::iterator it;
						std::pair<it,it> range = std::equal_range(VV.begin(),VV.end(),MergeStrategyMergeGapRequestQueryObject(p));

						// add rank r to each object for position p
						for ( it ita = range.first; ita != range.second; ++ita )
							ita->r += r;
					}
				}
				void fillGapRequestObjects(uint64_t const)
				{

				}
				bool isLeaf() const
				{
					return true;
				}
				void setParent(MergeStrategyBlock * rparent)
				{
					parent = rparent;
				}
				bool childFinished()
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "childFinished called on an object of struct MergeStrategyBaseBlock" << std::endl;
					se.finish();
					throw se;
				}

				/**
				 * @return space required in bytes for sorting block in internal memory using divsufsort
				 **/
				virtual uint64_t directSortSpace() const
				{
					if ( ((high-low)+sortreq.lcpnext) < (1ull << 31) )
					{
						return
							((high-low)+sortreq.lcpnext)*(sizeof(uint32_t)) + // suffix array
							(high-low+7)/8 + // GT bit vector
							(sourcetextindexbits+7)/8 +
							sourcelengthbytes;
					}
					else
					{
						return ((high-low)+sortreq.lcpnext)*(sizeof(uint64_t)) + // suffix array
							(high-low+7)/8 + // GT bit vector
							(sourcetextindexbits+7)/8 +
							sourcelengthbytes;
					}
				}
			};
		}
	}
}
#endif
