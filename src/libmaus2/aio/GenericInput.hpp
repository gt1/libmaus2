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
#if ! defined(GENERICINPUT_HPP)
#define GENERICINPUT_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <libmaus2/types/types.hpp>
#include <libmaus2/aio/AsynchronousBufferReader.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/Demangle.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/ConcatRequest.hpp>
#include <libmaus2/aio/FileFragment.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <limits>
#include <set>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * generic input class asynchronously reading a vector of elements of type input_type
		 **/
		template < typename input_type >
		struct GenericInput
		{
			//! value type
			typedef input_type value_type;
			//! this type
			typedef GenericInput<input_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//! buffer size
			uint64_t const bufsize;
			//! asynchronous reader class
			::libmaus2::aio::AsynchronousBufferReader ABR;
			//! elements left in current buffer
			uint64_t curbufleft;
			//! current buffer
			std::pair < char const *, ssize_t > curbuf;
			//! current word
			input_type const * curword;

			//! total words
			uint64_t const totalwords;
			//! total words read
			uint64_t totalwordsread;
			
			public:
			/**
			 * read an array of type input_type from the file named inputfilename
			 *
			 * @param inputfilename file name
			 * @return array
			 **/
			static ::libmaus2::autoarray::AutoArray<input_type> readArray(std::string const & inputfilename)
			{
				uint64_t const fs =
					::libmaus2::util::GetFileSize::getFileSize(inputfilename);
				assert ( fs % sizeof(input_type) == 0 );
				uint64_t const n = fs/sizeof(input_type);

				::libmaus2::aio::GenericInput<input_type> in(inputfilename,64*1024);
				::libmaus2::autoarray::AutoArray<input_type> A(n,false);
			
				for ( uint64_t i = 0; i < n; ++i )
				{
					input_type v;
					bool ok = in.getNext(v);
					assert ( ok );
					A[i] = v;
				}
				
				return A;
			}

			/**
			 * constructor
			 *
			 * @param filename input file name
			 * @param rbufsize buffer size in elements
			 * @param roffset offset in file in elements
			 **/
			GenericInput(std::string const & filename, uint64_t const rbufsize, uint64_t const roffset = 0)
			: bufsize(rbufsize), 
				ABR(filename, 16, bufsize*sizeof(input_type), roffset * sizeof(input_type)), 
				curbufleft(0),
				curbuf(reinterpret_cast<char const *>(0),0),
				curword(0),
				totalwords ( ::libmaus2::util::GetFileSize::getFileSize(filename) / sizeof(input_type) - roffset ),
				totalwordsread(0)
			{

			}
			/**
			 * destructor
			 **/
			~GenericInput()
			{
				ABR.flush();
			}
			/**
			 * get next element
			 *
			 * @param word reference for storing the next element
			 * @return true if a value was deposited in word
			 **/
			bool getNext(input_type & word)
			{
				if ( ! curbufleft )
				{
					if ( curbuf.first )
					{
						ABR.returnBuffer();
						curbuf.first = reinterpret_cast<char const *>(0);
						curbuf.second = 0;
					}
					if ( ! ABR.getBuffer(curbuf) )
						return false;
					if ( ! curbuf.second )
						return false;
					assert ( curbuf.second % sizeof(input_type) == 0 );
					
					curbufleft = curbuf.second / sizeof(input_type);
					assert ( curbufleft );
					curword = reinterpret_cast<input_type const *>(curbuf.first);
				}

				word = *(curword++);
				curbufleft -= 1;
				totalwordsread++;

				return true;
			}
		};
	}
}
#endif
