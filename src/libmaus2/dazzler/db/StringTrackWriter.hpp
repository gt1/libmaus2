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
#if ! defined(LIBMAUS2_DAZZLER_DB_STRINGTRACKWRITER_HPP)
#define LIBMAUS2_DAZZLER_DB_STRINGTRACKWRITER_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/dazzler/db/OutputBase.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct StringTrackWriter
			{
				typedef StringTrackWriter this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				libmaus2::aio::OutputStreamInstance::unique_ptr_type annoOSI;
				libmaus2::aio::OutputStreamInstance::unique_ptr_type dataOSI;
				uint64_t p;
				uint64_t r;

				StringTrackWriter(std::string const & annofilename, std::string const & datafilename)
				:
					annoOSI(new libmaus2::aio::OutputStreamInstance(annofilename)),
					dataOSI(new libmaus2::aio::OutputStreamInstance(datafilename)),
					p(0),
					r(0)
				{
					uint64_t cigannooff = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(*annoOSI,0,cigannooff); // number of reads
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(*annoOSI,8,cigannooff); // size of anno entries
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*annoOSI,0,cigannooff); // first offset
				}

				~StringTrackWriter()
				{
					flush();
				}

				void flush()
				{
					if ( annoOSI )
					{
						annoOSI->seekp(0,std::ios::beg);
						uint64_t cigannooff = 0;
						libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(*annoOSI,r,cigannooff); // number of reads

						annoOSI.reset();
						dataOSI.reset();
					}
				}

				void put(char const * c, uint64_t const n)
				{
					dataOSI->write(c,n);
					p += n;
					r += 1;

					uint64_t cigannooff = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*annoOSI,p,cigannooff); // first offset
				}

				void put(std::string const & s)
				{
					put(s.c_str(),s.size());
				}
			};
		}
	}
}
#endif
