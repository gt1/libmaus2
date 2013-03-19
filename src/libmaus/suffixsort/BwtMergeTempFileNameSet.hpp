/**
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
**/
#if ! defined(LIBMAUS_SUFFIXSORT_BWTMERGETEMPFILENAMESET_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGETEMPFILENAMESET_HPP

#include <string>
#include <iomanip>
#include <sstream>

#include <libmaus/util/StringSerialisation.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct BwtMergeTempFileNameSet
		{
			std::string gt;
			std::string bwt;
			std::string hwt;
			std::string hist;
			std::string sampledisa;
			
			void removeFilesButBwt() const
			{
				if ( gt.size() )
					remove ( gt.c_str() );
				if ( hwt.size() )
					remove ( hwt.c_str() );
				if ( hist.size() )
					remove ( hist.c_str() );
				if ( sampledisa.size() )
					remove ( sampledisa.c_str() );
			}

			void removeFiles() const
			{
				if ( gt.size() )
					remove ( gt.c_str() );
				if ( bwt.size() )
					remove ( bwt.c_str() );
				if ( hwt.size() )
					remove ( hwt.c_str() );
				if ( hist.size() )
					remove ( hist.c_str() );
				if ( sampledisa.size() )
					remove ( sampledisa.c_str() );
			}

			static std::string constructFileName(
				std::string const & tmpfilenamebase,
				uint64_t const id,
				std::string const & suffix
			)
			{
				std::ostringstream hwtnamestr;			
				hwtnamestr << tmpfilenamebase << "_" << std::setw(4) << std::setfill('0') << id << std::setw(0) << suffix;
				std::string const hwtname = hwtnamestr.str();
				::libmaus::util::TempFileRemovalContainer::addTempFile(hwtname);

				return hwtname;
			}
			
			BwtMergeTempFileNameSet()
			{}
			
			BwtMergeTempFileNameSet(std::string const & tmpfilenamebase, uint64_t const id)
			: 
				gt(constructFileName(tmpfilenamebase,id,".gt")),
				bwt(constructFileName(tmpfilenamebase,id,".bwt")),
				hwt(constructFileName(tmpfilenamebase,id,".hwt")),
				hist(constructFileName(tmpfilenamebase,id,".hist")),
				sampledisa(constructFileName(tmpfilenamebase,id,".sampledisa"))
			{
				
			}
			
			template<typename stream_type>
			BwtMergeTempFileNameSet(stream_type & in)
			: 
				gt(::libmaus::util::StringSerialisation::deserialiseString(in)),
				bwt(::libmaus::util::StringSerialisation::deserialiseString(in)),
				hwt(::libmaus::util::StringSerialisation::deserialiseString(in)),
				hist(::libmaus::util::StringSerialisation::deserialiseString(in)),
				sampledisa(::libmaus::util::StringSerialisation::deserialiseString(in))
			{
				
			}
			
			static BwtMergeTempFileNameSet load(std::string const & serialised)
			{
				std::istringstream istr(serialised);
				return BwtMergeTempFileNameSet(istr);
			}
			
			template<typename stream_type>
			void serialise(stream_type & out) const
			{
				::libmaus::util::StringSerialisation::serialiseString(out,gt);
				::libmaus::util::StringSerialisation::serialiseString(out,bwt);
				::libmaus::util::StringSerialisation::serialiseString(out,hwt);
				::libmaus::util::StringSerialisation::serialiseString(out,hist);
				::libmaus::util::StringSerialisation::serialiseString(out,sampledisa);
			}
			
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
		};
	}
}
#endif
