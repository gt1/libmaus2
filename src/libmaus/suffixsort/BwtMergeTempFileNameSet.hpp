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
			private:
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

			std::string gt;
			std::string bwt;
			std::string hwtreq;
			std::string hwt;
			std::string hist;
			std::string sampledisa;
			
			public:
			std::string const & getGT() const { return gt; }
			std::string const & getBWT() const { return bwt; }
			std::string const & getHWTReq() const { return hwtreq; }
			std::string const & getHWT() const { return hwt; }
			std::string const & getHist() const { return hist; }
			std::string const & getSampledISA() const { return sampledisa; }
			
			void setGT(std::string const & rgt) { gt = rgt; }
			void setBWT(std::string const & rbwt) { bwt = rbwt; }
			void setHWTReq(std::string const & rhwtreq) { hwtreq = rhwtreq; }
			void setHWT(std::string const & rhwt) { hwt = rhwt; }
			void setHist(std::string const & rhist) { hist = rhist; }
			void setSampledISA(std::string const & rsampledisa) { sampledisa = rsampledisa; }
			
			void setPrefix(std::string const & prefix)
			{
				setGT(prefix+".gt");
				setBWT(prefix+".bwt");
				setHWTReq(prefix+".hwtreq");
				setHWT(prefix+".hwt");
				setHist(prefix+".hist");
				setSampledISA(prefix+".sampledisa");
			}
			
			void setPrefixAndRegisterAsTemp(std::string const & prefix)
			{
				setPrefix(prefix);
				::libmaus::util::TempFileRemovalContainer::addTempFile(getGT());
				::libmaus::util::TempFileRemovalContainer::addTempFile(getBWT());
				::libmaus::util::TempFileRemovalContainer::addTempFile(getHWT());
				::libmaus::util::TempFileRemovalContainer::addTempFile(getHist());
				::libmaus::util::TempFileRemovalContainer::addTempFile(getSampledISA());			
			}
			
			void removeFilesButBwt() const
			{
				if ( gt.size() )
					remove ( gt.c_str() );
				if ( hwtreq.size() )
					remove ( hwtreq.c_str() );
				if ( hwt.size() )
					remove ( hwt.c_str() );
				if ( hist.size() )
					remove ( hist.c_str() );
				if ( sampledisa.size() )
					remove ( sampledisa.c_str() );
			}

			void removeFiles() const
			{
				removeFilesButBwt();
				if ( bwt.size() )
					remove ( bwt.c_str() );
			}

			
			BwtMergeTempFileNameSet()
			{}
			
			BwtMergeTempFileNameSet(std::string const & tmpfilenamebase, uint64_t const id)
			: 
				gt(constructFileName(tmpfilenamebase,id,".gt")),
				bwt(constructFileName(tmpfilenamebase,id,".bwt")),
				hwtreq(constructFileName(tmpfilenamebase,id,".hwtreq")),
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
				hwtreq(::libmaus::util::StringSerialisation::deserialiseString(in)),
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
				::libmaus::util::StringSerialisation::serialiseString(out,hwtreq);
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
