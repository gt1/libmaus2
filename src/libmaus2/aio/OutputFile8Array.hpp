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


#if ! defined(OUTPUTFILE8ARRAY_HPP)
#define OUTPUTFILE8ARRAY_HPP

#include <libmaus2/util/IntervalTree.hpp>
#include <libmaus2/aio/OutputBuffer8.hpp>
#include <vector>
#include <stdexcept>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * array of 64 bit async output streams
		 **/
		struct OutputFile8Array
		{
			//! buffer type
			typedef libmaus2::aio::OutputBuffer8 buffer_type;
			//! buffer pointer type
			typedef ::libmaus2::util::unique_ptr<buffer_type>::type buffer_ptr_type;
			//! this type
			typedef OutputFile8Array this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//! hash intervals
			::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t > > const & HI;
			//! output buffers
			::libmaus2::autoarray::AutoArray<buffer_ptr_type> buffers;
			//! output file names
			std::vector < std::string > filenames;
			//! interval tree for hash intervals
			::libmaus2::util::IntervalTree IT;

			public:
			/**
			 * constructor
			 *
			 * @param rHI hash intervals
			 * @param fileprefix prefix for output files
			 **/
			OutputFile8Array(::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t > > const & rHI, std::string const & fileprefix)
			: HI(rHI), buffers(HI.size()), IT(HI,0,HI.size())
			{
				for ( uint64_t i = 0; i < HI.size(); ++i )
				{
					std::ostringstream ostr;
					ostr << fileprefix << "_" << i;
					filenames.push_back(ostr.str());
				}
				for ( uint64_t i = 0; i < HI.size(); ++i )
				{
					buffers[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(filenames[i],64*1024)));
				}
			}
			/**
			 * destructor: flush buffers
			 **/
			~OutputFile8Array()
			{
				flush();
			}
			/**
			 * flush buffers
			 **/
			void flush()
			{
				for ( uint64_t i = 0; i < buffers.getN(); ++i )
					if ( buffers[i].get() )
					{
						std::cerr << "(" << (i+1) << "/" << buffers.getN();
						buffers[i]->flush();
						buffers[i].reset();
						std::cerr << ")";
					}
			}
			/**
			 * get buffer for hash value hashval
			 *
			 * @param hashval hash value
			 * @return buffer for hashval
			 **/
			buffer_type & getBuffer(uint64_t const hashval)
			{
				uint64_t const i = IT.find(hashval);
				assert ( i < HI.size() );
				assert ( hashval >= HI[i].first );
				assert ( hashval < HI[i].second );
				return *(buffers[i]);
			}
			/**
			 * get buffer by id
			 *
			 * @param id
			 * @return buffer for id
			 **/
			buffer_type & getBufferById(uint64_t const id)
			{
				return *(buffers[id]);
			}
		};
	}
}
#endif
