/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTSNAPPYINPUT_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTSNAPPYINPUT_HPP

#include <libmaus/lz/SnappyFileInputStream.hpp>
#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentSnappyInput
		{
			::libmaus::lz::SnappyFileInputStream GZ;
			::libmaus::bambam::BamAlignment alignment;
			
			BamAlignmentSnappyInput(std::string const & filename)
			: GZ(filename)
			{
			
			}
			
			::libmaus::bambam::BamAlignment & getAlignment()
			{
				return alignment;
			}

			::libmaus::bambam::BamAlignment const & getAlignment() const
			{
				return alignment;
			}
			
			bool readAlignment()
			{
				/* read alignment block size */
				int64_t const bs0 = GZ.get();
				int64_t const bs1 = GZ.get();
				int64_t const bs2 = GZ.get();
				int64_t const bs3 = GZ.get();
				if ( bs3 < 0 )
					// reached end of file
					return false;
				
				/* assemble block size as LE integer */
				alignment.blocksize = (bs0 << 0) | (bs1 << 8) | (bs2 << 16) | (bs3 << 24) ;

				/* read alignment block */
				if ( alignment.blocksize > alignment.D.size() )
					alignment.D = ::libmaus::bambam::BamAlignment::D_array_type(alignment.blocksize);
				GZ.read(reinterpret_cast<char *>(alignment.D.begin()),alignment.blocksize);
				// assert ( static_cast<int64_t>(GZ.gcount()) == static_cast<int64_t>(alignment.blocksize) );
			
				return true;
			}
		};
	}
}
#endif
