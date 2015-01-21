/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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

/* this code is inspired by the samtools_irods project */
#if ! defined(LIBMAUS_IRODS_IRODSFILEBASE_HPP)
#define LIBMAUS_IRODS_IRODSFILEBASE_HPP

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/irods/IRodsCommProvider.hpp>

#if defined(LIBMAUS_HAVE_IRODS)
#include <rodsClient.h>
#endif

namespace libmaus
{
	namespace irods
	{
		struct IRodsSystem;

		struct IRodsFileBase
		{
			typedef IRodsFileBase this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			friend struct IRodsSystem;

			bool fdvalid;
			long fd; /* file descriptor returned by rcDataObjCreate() */
			libmaus::irods::IRodsCommProvider::shared_ptr_type commProvider;
			
			private:
			IRodsFileBase();
			
			public:
			~IRodsFileBase();
			
			// read block of data
			uint64_t read(char * buffer, uint64_t len);
			
			// perform seek operation and return new position in stream
			uint64_t seek(long offset, int whence);

			// get size of file
			uint64_t size();
		};
	}
}
#endif
