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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYBLOCK_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTB3M_MERGESTRATEGYBLOCK_HPP

#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeGapRequestQueryObject.hpp>
#include <libmaus2/suffixsort/BwtMergeBlockSortResult.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		namespace bwtb3m
		{
			struct MergeStrategyBlock
			{
				typedef MergeStrategyBlock this_type;
				typedef libmaus2::util::shared_ptr<MergeStrategyBlock>::type shared_ptr_type;

				//! low symbol offset in file
				uint64_t low;
				//! high symbol offset in file
				uint64_t high;
				//! length of the source block (on disk) in bits
				uint64_t sourcelengthbits;
				//! length of the source block in bytes (for byte oriented suffix sorting)
				uint64_t sourcelengthbytes;
				//! length of sequence in Huffman code
				uint64_t codedlength;
				//! length of index for random access in text
				uint64_t sourcetextindexbits;
				//! node id
				uint64_t nodeid;
				//! node depth
				uint64_t nodedepth;
				//! sorting result
				::libmaus2::suffixsort::BwtMergeBlockSortResult sortresult;
				//! parent node
				MergeStrategyBlock * parent;

				MergeStrategyBlock()
				: low(0), high(0), sourcelengthbits(0), sourcelengthbytes(0), codedlength(0), sourcetextindexbits(0), nodeid(0), nodedepth(0), parent(0) {}
				MergeStrategyBlock(uint64_t const rlow, uint64_t const rhigh, uint64_t const rsourcelengthbits, uint64_t const rsourcelengthbytes, uint64_t const rsourcetextindexbits)
				: low(rlow), high(rhigh), sourcelengthbits(rsourcelengthbits), sourcelengthbytes(rsourcelengthbytes), codedlength(0), sourcetextindexbits(rsourcetextindexbits), nodeid(0), nodedepth(0), parent(0) {}

				virtual ~MergeStrategyBlock() {}
				virtual std::ostream & print(std::ostream & out, uint64_t const indent) const = 0;

				/**
				 * get estimate for space used by Huffman shaped wavelet tree in bits
				 **/
				uint64_t getIHWTSpaceBits() const
				{
					return (codedlength * 4+2)/3;
				}

				/**
				 * get estimate for space used by Huffman shaped wavelet tree in bytes
				 **/
				uint64_t getIHWTSpaceBytes() const
				{
					return (getIHWTSpaceBits()+7)/8;
				}

				/**
				 * get estimate for number of bytes required to merge into this block using an internal memory gap array
				 **/
				uint64_t getMergeSpaceInternalGap() const
				{
					return
						getIHWTSpaceBytes() + (high-low) * sizeof(uint32_t);
				}
				/**
				 * get estimate for number of bytes required to merge into this block using an internal memory small gap array (1 byte + overflow)
				 **/
				uint64_t getMergeSpaceInternalSmallGap() const
				{
					return
						getIHWTSpaceBytes() + (high-low) * sizeof(uint8_t);
				}

				/**
				 * get estimate for number of bytes required to merge into this block using an external memory gap array
				 **/
				uint64_t getMergeSpaceExternalGap(uint64_t const threads, uint64_t wordsperthread) const
				{
					return
						getIHWTSpaceBytes() + threads * wordsperthread * sizeof(uint64_t);
				}

				/**
				 * @return space required in bytes for sorting block in internal memory using divsufsort
				 **/
				virtual uint64_t directSortSpace() const
				{
					if ( (high-low) < (1ull << 31) )
					{
						return
							(high-low)*(sizeof(uint32_t)) + // suffix array
							(high-low+7)/8 + // GT bit vector
							(sourcetextindexbits+7)/8 +
							sourcelengthbytes;
					}
					else
					{
						return (high-low)*(sizeof(uint64_t)) + // suffix array
							(high-low+7)/8 + // GT bit vector
							(sourcetextindexbits+7)/8 +
							sourcelengthbytes;
					}
				}

				std::ostream & printBase(std::ostream & out) const
				{
					out
						<< "id=" << nodeid
						<< ",d=" << nodedepth
						<< ",[" << low << "," << high << "),sourcelengthbits=" << sourcelengthbits << ",sourcelengthbytes=" << sourcelengthbytes << ",codedlength=" << codedlength << ",directsortspace=" << directSortSpace() << ",sourcetextindexbits=" << sourcetextindexbits;
					return out;
				}

				virtual uint64_t fillNodeId(uint64_t i) = 0;
				virtual void fillNodeDepth(uint64_t const i) = 0;
				// register the given query positions in this leaf or all leafs under this inner node
				virtual void registerQueryPositions(std::vector<uint64_t> const & V) = 0;
				// fill query positions for t thread for each gap request in the tree
				virtual void fillQueryPositions(uint64_t const t) = 0;

				// fill query objects for inner nodes given finished leaf node information
				virtual void fillQueryObjects(libmaus2::autoarray::AutoArray<MergeStrategyMergeGapRequestQueryObject> & VV) = 0;
				// fill zblock ranks from finished base blocks for t threads
				virtual void fillGapRequestObjects(uint64_t const t) = 0;

				virtual bool isLeaf() const = 0;
				virtual void setParent(MergeStrategyBlock * rparent) = 0;
				virtual bool childFinished() = 0;
			};
		}
	}
}
#endif
