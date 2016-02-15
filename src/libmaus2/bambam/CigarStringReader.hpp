/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_BAMBAM_CIGARSTRINGREADER_HPP)
#define LIBMAUS2_BAMBAM_CIGARSTRINGREADER_HPP

#include <libmaus2/bambam/BamFlagBase.hpp>

namespace libmaus2
{
	namespace bambam
	{
		template<typename iterator>
		struct CigarStringReader
		{
			iterator it_c;
			iterator it_e;
			cigar_operation current;

			::libmaus2::bambam::BamFlagBase::bam_cigar_ops peekslot;
			bool peekslotactive;

			CigarStringReader()
			: it_c(iterator()), it_e(iterator()), current(), peekslot(), peekslotactive(false)
			{

			}

			CigarStringReader(iterator rit_c, iterator rit_e)
			: it_c(rit_c), it_e(rit_e), current(cigar_operation(0,0)), peekslot(), peekslotactive(false)
			{

			}

			bool getNext(::libmaus2::bambam::BamFlagBase::bam_cigar_ops & op)
			{
				if ( peekslotactive )
				{
					op = peekslot;
					peekslotactive = false;
					return true;
				}

				while ( (! current.second) && (it_c != it_e) )
					current = *(it_c++);

				if ( current.second )
				{
					current.second -= 1;
					op = static_cast<::libmaus2::bambam::BamFlagBase::bam_cigar_ops>(current.first);
					return true;
				}
				else
				{
					return false;
				}
			}

			bool peekNext(::libmaus2::bambam::BamFlagBase::bam_cigar_ops & op)
			{
				if ( peekslotactive )
				{
					op = peekslot;
					return true;
				}
				else
				{
					peekslotactive = getNext(peekslot);
					op = peekslot;
					return peekslotactive;
				}
			}
		};
	}
}
#endif
