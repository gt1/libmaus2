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
#if ! defined(LIBMAUS2_BAMBAM_BAMHEADERUPDATE_HPP)
#define LIBMAUS2_BAMBAM_BAMHEADERUPDATE_HPP

#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/bambam/BamParallelRewrite.hpp>
#include <libmaus2/bambam/ProgramHeaderLineSet.hpp>
#include <libmaus2/util/ArgInfo.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamHeaderUpdate : public libmaus2::bambam::BamHeaderRewriteCallback
		{
			libmaus2::util::ArgInfo const & arginfo;
			std::string const id;
			std::string const packageversion;

			BamHeaderUpdate(libmaus2::util::ArgInfo const & rarginfo, std::string const & rid, std::string const & rpackageversion)
			: arginfo(rarginfo), id(rid), packageversion(rpackageversion)
			{
			
			}

			static ::libmaus2::bambam::BamHeader::unique_ptr_type updateHeader(
				::libmaus2::util::ArgInfo const & arginfo,
				::libmaus2::bambam::BamHeader const & header,
				std::string const & id,
				std::string const & packageversion
			)
			{
				std::string const headertext(header.text);

				// add PG line to header
				std::string const upheadtext = ::libmaus2::bambam::ProgramHeaderLineSet::addProgramLine(
					headertext,
					id, // ID
					id, // PN
					arginfo.commandline, // CL
					::libmaus2::bambam::ProgramHeaderLineSet(headertext).getLastIdInChain(), // PP
					packageversion //std::string(PACKAGE_VERSION) // VN			
				);
				// construct new header
				::libmaus2::bambam::BamHeader::unique_ptr_type uphead(new ::libmaus2::bambam::BamHeader(upheadtext));
				
				return UNIQUE_PTR_MOVE(uphead);
			}

			::libmaus2::bambam::BamHeader::unique_ptr_type operator()(::libmaus2::bambam::BamHeader const & header)  const
			{
				::libmaus2::bambam::BamHeader::unique_ptr_type ptr(updateHeader(arginfo,header,id,packageversion));
				return UNIQUE_PTR_MOVE(ptr);
			}
		};
	}
}
#endif
