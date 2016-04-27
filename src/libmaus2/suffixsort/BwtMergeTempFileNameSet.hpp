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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTMERGETEMPFILENAMESET_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTMERGETEMPFILENAMESET_HPP

#include <string>
#include <iomanip>
#include <sstream>

#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct BwtMergeTempFileNameSet
		{
			private:
			static std::string constructFileName(
				//std::string const & tmpfilenamebase,
				libmaus2::util::TempFileNameGenerator & gtmpgen,
				uint64_t const id,
				std::string const & suffix
			)
			{
				std::ostringstream hwtnamestr;
				hwtnamestr << gtmpgen.getFileName() << "_" << std::setw(4) << std::setfill('0') << id << std::setw(0) << suffix;
				std::string const hwtname = hwtnamestr.str();
				::libmaus2::util::TempFileRemovalContainer::addTempFile(hwtname);

				return hwtname;
			}

			static std::vector<std::string> constructFileNameVector(
				//std::string const & tmpfilenamebase,
				libmaus2::util::TempFileNameGenerator & gtmpgen,
				uint64_t const id,
				std::string const & suffix,
				uint64_t const num
			)
			{
				std::vector<std::string> V;

				for ( uint64_t i = 0; i < num; ++i )
				{
					std::ostringstream hwtnamestr;
					hwtnamestr << gtmpgen.getFileName()
						<< '_'
						<< std::setw(4) << std::setfill('0') << id  << std::setw(0)
						<< '_'
						<< std::setw(4) << std::setfill('0') << i  << std::setw(0)
						<< suffix;
					std::string const hwtname = hwtnamestr.str();
					::libmaus2::util::TempFileRemovalContainer::addTempFile(hwtname);
					V.push_back(hwtname);
				}

				return V;
			}

			std::vector<std::string> gt;
			std::vector<std::string> bwt;
			std::string hwtreq;
			std::string hwt;
			std::string hist;
			std::vector<std::string> sampledisa;

			public:
			std::vector<std::string> const & getGT() const { return gt; }
			std::vector<std::string> const & getBWT() const { return bwt; }
			std::string const & getHWTReq() const { return hwtreq; }
			std::string const & getHWT() const { return hwt; }
			std::string const & getHist() const { return hist; }
			std::string const & getSampledISA() const { assert ( sampledisa.size() ); return sampledisa[0]; }
			std::vector<std::string> const & getSampledISAVector() const { return sampledisa; }

			void setGT(std::vector<std::string> const & rgt) { gt = rgt; }
			void setBWT(std::vector<std::string> const & rbwt) { bwt = rbwt; }
			void setHWTReq(std::string const & rhwtreq) { hwtreq = rhwtreq; }
			void setHWT(std::string const & rhwt) { hwt = rhwt; }
			void setHist(std::string const & rhist) { hist = rhist; }
			void setSampledISA(std::string const & rsampledisa) { sampledisa = std::vector<std::string>(1,rsampledisa); }
			void setSampledISA(std::vector<std::string> const & rsampledisa) { sampledisa = rsampledisa; }

			void setPrefix(libmaus2::util::TempFileNameGenerator & gtmpgen, uint64_t const numbwt, uint64_t const numgt, uint64_t const numisa)
			{
				std::vector<std::string> gtfilenames(numgt);
				for ( uint64_t i = 0; i < numbwt; ++i )
				{
					std::ostringstream ostr;
					ostr << gtmpgen.getFileName() << '_'
						<< std::setw(4) << std::setfill('0') << i << std::setw(0)
						<< ".gt";
					gtfilenames[i] = ostr.str();
				}
				setGT(gtfilenames);

				std::vector<std::string> bwtfilenames(numbwt);
				for ( uint64_t i = 0; i < numbwt; ++i )
				{
					std::ostringstream ostr;
					ostr << gtmpgen.getFileName() << '_'
						<< std::setw(4) << std::setfill('0') << i << std::setw(0)
						<< ".bwt";
					bwtfilenames[i] = ostr.str();
				}

				setBWT(bwtfilenames);
				setHWTReq(gtmpgen.getFileName()+".hwtreq");
				setHWT(gtmpgen.getFileName()+".hwt");
				setHist(gtmpgen.getFileName()+".hist");

				std::vector<std::string> isafilenames(numisa);
				for ( uint64_t i = 0; i < numisa; ++i )
				{
					std::ostringstream ostr;
					ostr << gtmpgen.getFileName() << '_'
						<< std::setw(4) << std::setfill('0') << i << std::setw(0)
						<< ".sampledisa";
					isafilenames[i] = ostr.str();
				}
				setSampledISA(isafilenames);
			}

			void setPrefixAndRegisterAsTemp(libmaus2::util::TempFileNameGenerator & gtmpgen, uint64_t const numbwt, uint64_t const numgt, uint64_t const numisa)
			{
				setPrefix(gtmpgen, numbwt, numgt, numisa);
				for ( uint64_t i = 0; i < getGT().size(); ++i )
					::libmaus2::util::TempFileRemovalContainer::addTempFile(getGT()[i]);
				for ( uint64_t i = 0; i < getBWT().size(); ++i )
					::libmaus2::util::TempFileRemovalContainer::addTempFile(getBWT()[i]);
				::libmaus2::util::TempFileRemovalContainer::addTempFile(getHWT());
				::libmaus2::util::TempFileRemovalContainer::addTempFile(getHist());
				for ( uint64_t i = 0; i < getSampledISAVector().size(); ++i )
					::libmaus2::util::TempFileRemovalContainer::addTempFile(getSampledISAVector()[i]);
			}

			void removeGtFiles() const
			{
				for ( uint64_t i = 0; i < gt.size(); ++i )
					if ( gt[i].size() )
						libmaus2::aio::FileRemoval::removeFile ( gt[i] );
			}

			void removeHwtReqFiles() const
			{
				if ( hwtreq.size() )
					libmaus2::aio::FileRemoval::removeFile ( hwtreq );
			}

			void removeHwtFiles() const
			{
				if ( hwt.size() )
					libmaus2::aio::FileRemoval::removeFile ( hwt );
			}

			void removeHistFiles() const
			{
				if ( hist.size() )
					libmaus2::aio::FileRemoval::removeFile ( hist );
			}

			void removeSampledIsaFiles() const
			{
				for ( uint64_t i = 0; i < sampledisa.size(); ++i )
					if ( sampledisa[i].size() )
						libmaus2::aio::FileRemoval::removeFile ( sampledisa[i] );
			}

			void removeBwtFiles() const
			{
				for ( uint64_t i = 0; i < bwt.size(); ++i )
					if ( bwt[i].size() )
						libmaus2::aio::FileRemoval::removeFile ( bwt[i] );
			}

			void removeFilesButBwtAndGt() const
			{
				removeHwtReqFiles();
				removeHwtFiles();
				removeHistFiles();
				removeSampledIsaFiles();
			}

			void removeFilesButBwt() const
			{
				removeGtFiles();
				removeFilesButBwtAndGt();
			}

			void removeFiles() const
			{
				removeBwtFiles();
				removeFilesButBwt();
			}


			BwtMergeTempFileNameSet()
			{}

			BwtMergeTempFileNameSet(libmaus2::util::TempFileNameGenerator & gtmpgen, uint64_t const id, uint64_t const numbwtfiles, uint64_t const numgtfiles)
			:
				gt(constructFileNameVector(gtmpgen,id,".gt",numgtfiles)),
				bwt(constructFileNameVector(gtmpgen,id,".bwt",numbwtfiles)),
				hwtreq(constructFileName(gtmpgen,id,".hwtreq")),
				hwt(constructFileName(gtmpgen,id,".hwt")),
				hist(constructFileName(gtmpgen,id,".hist")),
				sampledisa(constructFileNameVector(gtmpgen,id,".sampledisa",1))
			{

			}

			template<typename stream_type>
			BwtMergeTempFileNameSet(stream_type & in)
			:
				gt(::libmaus2::util::StringSerialisation::deserialiseStringVector(in)),
				bwt(::libmaus2::util::StringSerialisation::deserialiseStringVector(in)),
				hwtreq(::libmaus2::util::StringSerialisation::deserialiseString(in)),
				hwt(::libmaus2::util::StringSerialisation::deserialiseString(in)),
				hist(::libmaus2::util::StringSerialisation::deserialiseString(in)),
				sampledisa(::libmaus2::util::StringSerialisation::deserialiseStringVector(in))
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
				::libmaus2::util::StringSerialisation::serialiseStringVector(out,gt);
				::libmaus2::util::StringSerialisation::serialiseStringVector(out,bwt);
				::libmaus2::util::StringSerialisation::serialiseString(out,hwtreq);
				::libmaus2::util::StringSerialisation::serialiseString(out,hwt);
				::libmaus2::util::StringSerialisation::serialiseString(out,hist);
				::libmaus2::util::StringSerialisation::serialiseStringVector(out,sampledisa);
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
