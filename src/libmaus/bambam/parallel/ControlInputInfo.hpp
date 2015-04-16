/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
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
*/
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_INPUTINFO_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_INPUTINFO_HPP

#include <libmaus/bambam/parallel/InputBlock.hpp>
#include <libmaus/parallel/LockedFreeList.hpp>
#include <libmaus/parallel/LockedBool.hpp>
#include <libmaus/bambam/parallel/InputBlockAllocator.hpp>
#include <libmaus/bambam/parallel/InputBlockTypeInfo.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			/**
			 * Input state for parallel BAM decoding.
			 *
			 **/
			struct ControlInputInfo
			{
				typedef InputBlock input_block_type;
			
				// lock for everything below
				libmaus::parallel::PosixSpinLock readLock;
				// input stream reference
				std::istream & istr;
				// returns true if end of file has been observed
				libmaus::parallel::LockedBool eof;
				// id of this input stream
				uint64_t volatile streamid;
				// next input block id
				uint64_t volatile blockid;
				// list of free input blocks
				libmaus::parallel::LockedFreeList<
					input_block_type,
					InputBlockAllocator,
					InputBlockTypeInfo
				> inputBlockFreeList;
	
				ControlInputInfo(
					std::istream & ristr, 
					uint64_t const rstreamid,
					uint64_t const rfreelistsize
				) : istr(ristr), eof(false), streamid(rstreamid), blockid(0), inputBlockFreeList(rfreelistsize) {}
				
				bool getEOF()
				{
					return eof.get();
				}
				
				void setEOF(bool v)
				{
					eof.set(v);
				}
			};
		}
	}
}
#endif
