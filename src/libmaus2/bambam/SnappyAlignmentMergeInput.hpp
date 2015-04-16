/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_SNAPPYALIGNMENTMERGEINPUT_HPP)
#define LIBMAUS2_BAMBAM_SNAPPYALIGNMENTMERGEINPUT_HPP

#include <libmaus2/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus2/bambam/BamAlignmentHeapComparator.hpp>
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/lz/SnappyOffsetFileInputStream.hpp>
#include <queue>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * merging read back class for snappy encoded, sorted alignment blocks
		 **/
		struct SnappyAlignmentMergeInput
		{
			//! this type
			typedef SnappyAlignmentMergeInput this_type;
			//! unique pointer type
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			//! file stream
			libmaus2::aio::CheckedInputStream::unique_ptr_type Psingle;
			//! block index
			std::vector < std::pair < uint64_t, uint64_t > > index;
			//! snappy decoder array
			libmaus2::autoarray::AutoArray < libmaus2::lz::SnappyOffsetFileInputStream::unique_ptr_type > streams;
			//! alignments
			libmaus2::autoarray::AutoArray < libmaus2::bambam::BamAlignment > data;
			//! alignment name comparator
			libmaus2::bambam::BamAlignmentNameComparator const namecomp;
			//! heap comparator
			libmaus2::bambam::BamAlignmentHeapComparator < libmaus2::bambam::BamAlignmentNameComparator > const heapcomp;
			//! heap
			std::priority_queue<uint64_t,std::vector<uint64_t>,libmaus2::bambam::BamAlignmentHeapComparator < libmaus2::bambam::BamAlignmentNameComparator > > Q;
			
			public:
			/**
			 * construct decoder
			 *
			 * @param rindex block index
			 * @param fn file name
			 * @return decoder object
			 **/
			static unique_ptr_type construct(std::vector < std::pair < uint64_t, uint64_t > > const & rindex, std::string const & fn)
			{
				unique_ptr_type ptr(new this_type(rindex,fn));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			/**
			 * constructor
			 *
			 * @param rindex block index
			 * @param fn file name
			 **/
			SnappyAlignmentMergeInput(
				std::vector < std::pair < uint64_t, uint64_t > > const & rindex,
				std::string const & fn)
			: index(rindex), streams(index.size()), data(index.size()), namecomp(static_cast<uint8_t const *>(0)), heapcomp(namecomp,data.begin()), Q(heapcomp)
			{
				bool openok = true;
			
				try
				{
					for ( uint64_t i = 0; i < index.size(); ++i )
						if ( index[i].second )
						{
							libmaus2::lz::SnappyOffsetFileInputStream::unique_ptr_type tstreamsi(
                                	                                        new libmaus2::lz::SnappyOffsetFileInputStream(fn,index[i].first)
                                        	                        );
							streams [ i ] = UNIQUE_PTR_MOVE(tstreamsi);
						}
				}
				catch(std::exception const & ex)
				{
					openok = false;
				}
				
				if ( ! openok )
				{
					std::cerr << "[V] failed to open a file handle for each single collation block, trying to merge through a single file handle" << std::endl;

					for ( uint64_t i = 0; i < index.size(); ++i )
						if ( index[i].second )
							streams[i].reset();
					
					libmaus2::aio::CheckedInputStream::unique_ptr_type TCIS(new libmaus2::aio::CheckedInputStream(fn));
					Psingle	= UNIQUE_PTR_MOVE(TCIS);

					for ( uint64_t i = 0; i < index.size(); ++i )
						if ( index[i].second )
						{
							libmaus2::lz::SnappyOffsetFileInputStream::unique_ptr_type tstreamsi
							(
                                	                	new libmaus2::lz::SnappyOffsetFileInputStream(*Psingle,index[i].first)
                                        	        );
							streams [ i ] = UNIQUE_PTR_MOVE(tstreamsi);
						}
				}
					
				for ( uint64_t i = 0; i < index.size(); ++i )
					if ( index[i].second )
					{
						index[i].second -= 1;

						#if !defined(NDEBUG)
						bool const alok = 
						#endif
						        libmaus2::bambam::BamDecoder::readAlignmentGz(*(streams[i]),data[i],0,false);
						        
						#if !defined(NDEBUG)
						assert ( alok );
						#endif
						
						Q.push(i);
					}
			}
			
			/**
			 * decode next alignment
			 *
			 * @param algn reference to alignment object to be filled
			 * @return true iff next alignment could be read, false when no more alignments are available
			 **/
			bool readAlignment(libmaus2::bambam::BamAlignment & algn)
			{
				if ( Q.empty() )
					return false;
				
				uint64_t const t = Q.top();
				Q.pop();
				
				libmaus2::bambam::BamAlignment::D_array_type T = algn.D;
				
				algn.D = data[t].D;
				algn.blocksize = data[t].blocksize;
				
				data[t].D = T;
				data[t].blocksize = 0;
				
				if ( index[t].second-- )
				{
					#if !defined(NDEBUG)
					bool const alok = 
					#endif
					        libmaus2::bambam::BamDecoder::readAlignmentGz(*(streams[t]),data[t],0,false);
					        
					#if !defined(NDEBUG)
					assert ( alok );
					#endif
					
					Q.push(t);
				}
					
				return true;
			}
		};
	}
}
#endif
