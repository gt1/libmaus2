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

				enum merge_block_type
				{
					merge_block_type_internal = 0,
					merge_block_type_internal_small = 1,
					merge_block_type_external = 2,
					merge_block_type_base = 3
				};

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

				bool operator==(MergeStrategyBlock const & O) const
				{
					if ( low != O.low )
						return false;
					if ( high != O.high )
						return false;
					if ( sourcelengthbits != O.sourcelengthbits )
						return false;
					if ( sourcelengthbytes != O.sourcelengthbytes )
						return false;
					if ( codedlength != O.codedlength )
						return false;
					if ( sourcetextindexbits != O.sourcetextindexbits )
						return false;
					if ( nodeid != O.nodeid )
						return false;
					if ( nodedepth != O.nodedepth )
						return false;
					if ( !(sortresult == O.sortresult) )
						return false;
					if ( (parent==0) != (O.parent==0) )
						return false;
					if ( parent )
					{
						assert ( O.parent );

						if ( parent->nodeid != O.parent->nodeid )
							return false;
					}
					return true;
				}

				bool operator!=(MergeStrategyBlock const & O) const
				{
					return !operator==(O);
				}

				MergeStrategyBlock()
				: low(0), high(0), sourcelengthbits(0), sourcelengthbytes(0), codedlength(0), sourcetextindexbits(0), nodeid(0), nodedepth(0), parent(0) {}
				MergeStrategyBlock(uint64_t const rlow, uint64_t const rhigh, uint64_t const rsourcelengthbits, uint64_t const rsourcelengthbytes, uint64_t const rsourcetextindexbits)
				: low(rlow), high(rhigh), sourcelengthbits(rsourcelengthbits), sourcelengthbytes(rsourcelengthbytes), codedlength(0), sourcetextindexbits(rsourcetextindexbits), nodeid(0), nodedepth(0), parent(0) {}
				MergeStrategyBlock(std::istream & in) : parent(0)
				{
					low = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					high = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					sourcelengthbits = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					sourcelengthbytes = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					codedlength = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					sourcetextindexbits = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					nodeid = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					nodedepth = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					sortresult.deserialise(in);
				}

				virtual ~MergeStrategyBlock() {}
				virtual std::ostream & print(std::ostream & out, uint64_t const indent) const = 0;
				virtual void vserialise(std::ostream & out) const = 0;

				void serialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,low);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,high);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,sourcelengthbits);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,sourcelengthbytes);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,codedlength);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,sourcetextindexbits);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,nodeid);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,nodedepth);
					sortresult.serialise(out);
				}

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
				virtual bool equal(MergeStrategyBlock const & O) const = 0;
			};

			inline std::ostream & operator<<(std::ostream & out, MergeStrategyBlock const & MSB)
			{
				return MSB.print(out,0);
			}
		}
	}
}
#endif
