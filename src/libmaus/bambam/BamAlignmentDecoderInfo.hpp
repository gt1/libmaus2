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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTDECODERINFO_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTDECODERINFO_HPP

#include <string>
#include <libmaus/types/types.hpp>
#include <ostream>
#include <vector>
#include <libmaus/util/ArgInfo.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentDecoderInfo
		{
			std::string inputfilename;
			std::string inputformat;
			uint64_t inputthreads;
			std::string reference;
			bool putrank;
			std::ostream * copystr;
			std::string range;

			static std::vector<libmaus::bambam::BamAlignmentDecoderInfo> filenameToInfo(
				libmaus::util::ArgInfo const & arginfo, 
				std::vector<std::string> const & filenames,
				std::string const & putrank = std::string("0")
			)
			{
				std::vector<libmaus::bambam::BamAlignmentDecoderInfo> V;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					V.push_back(libmaus::bambam::BamAlignmentDecoderInfo(
						arginfo,
						filenames[i],
						std::string(), /* format */
						std::string(), /* threads */
						std::string(), /* reference */
						putrank        /* put rank */
						)
					);

				return V;
			}

			static std::vector<libmaus::bambam::BamAlignmentDecoderInfo> filenameToInfo(
				std::vector<std::string> const & filenames
			)
			{
				std::vector<libmaus::bambam::BamAlignmentDecoderInfo> V;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					V.push_back(libmaus::bambam::BamAlignmentDecoderInfo(filenames[i]));
				return V;
			}

			static std::string getDefaultInputFileName()
			{
				return "-";
			}
			
			static std::string getDefaultInputFormat()
			{
				return "bam";
			}

			static uint64_t getDefaultThreads()
			{
				return 1;
			}
			
			static std::string getDefaultReference()
			{
				return "";
			}
			
			static bool getDefaultPutRank()
			{
				return false;
			}
			
			static std::ostream * getDefaultCopyStr()
			{
				return 0;
			}
			
			static std::string getDefaultRange()
			{
				return "";
			}
			
			static uint64_t parseNumber(std::string const & number)
			{
				std::istringstream istr(number);
				uint64_t u;
				istr >> u;
				if ( ! istr )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "BamAlignmentDecoderInfo::parseNumber: cannot parse " << number << " as number" << "\n";
					lme.finish();
					throw lme;
				}
				return u;
			}
			
			BamAlignmentDecoderInfo(BamAlignmentDecoderInfo const & o)
			: inputfilename(o.inputfilename), inputformat(o.inputformat), inputthreads(o.inputthreads),
			  reference(o.reference), putrank(o.putrank), copystr(o.copystr), range(o.range) {}
			BamAlignmentDecoderInfo(
				std::string rinputfilename = getDefaultInputFileName(),
				std::string rinputformat = getDefaultInputFormat(),
				uint64_t rinputthreads = getDefaultThreads(),
				std::string rreference = getDefaultReference(),
				bool rputrank = getDefaultPutRank(),
				std::ostream * rcopystr = getDefaultCopyStr(),
				std::string const rrange = getDefaultRange()
			)
			: inputfilename(rinputfilename), inputformat(rinputformat), inputthreads(rinputthreads),
			  reference(rreference), putrank(rputrank), copystr(rcopystr), range(rrange) {}
			BamAlignmentDecoderInfo(
				libmaus::util::ArgInfo const & arginfo,
				std::string rinputfilename = std::string(),
				std::string rinputformat   = std::string(),
				std::string rinputthreads  = std::string(),
				std::string rreference     = std::string(),
				std::string rputrank       = std::string(),
				std::ostream * rcopystr    = NULL,
				std::string const rrange   = std::string()
			)
			: inputfilename(getDefaultInputFileName()), 
			  inputformat(getDefaultInputFormat()), 
			  inputthreads(getDefaultThreads()),
			  reference(getDefaultReference()), 
			  putrank(getDefaultPutRank()), 
			  copystr(getDefaultCopyStr()), 
			  range(getDefaultRange()) 
			{
				inputfilename = rinputfilename.size() ? rinputfilename             : arginfo.getUnparsedValue("I",          inputfilename);
				inputformat   = rinputformat.size()   ? rinputformat               : arginfo.getUnparsedValue("inputformat",inputformat);
				inputthreads  = rinputthreads.size()  ? parseNumber(rinputthreads) : arginfo.getValue<unsigned int>("inputthreads",getDefaultThreads());
				reference     = rreference.size()     ? rreference                 : arginfo.getUnparsedValue("reference",reference);
				putrank       = rputrank.size()       ? parseNumber(rputrank)      : arginfo.getValue<unsigned int>("putrank",inputthreads);
				copystr       = rcopystr              ? rcopystr                   : copystr;
				range         = rrange.size()         ? rrange                     : arginfo.getUnparsedValue("range",range);
				range         = range.size()          ? range                      : arginfo.getUnparsedValue("ranges",range);
			}
			  
			BamAlignmentDecoderInfo & operator=(BamAlignmentDecoderInfo const & o)
			{
				if ( this != &o )
				{
					inputfilename = o.inputfilename;
					inputformat = o.inputformat;
					inputthreads = o.inputthreads;
					reference = o.reference;
					putrank = o.putrank;
					copystr = o.copystr;
					range = o.range;
				}
				return *this;
			}

			static libmaus::bambam::BamAlignmentDecoderInfo constructInfo(
				libmaus::util::ArgInfo const & arginfo,
				std::string const & filename,
				bool const putrank = false, 
				std::ostream * copystr = 0
			)
			{
				std::string const inputformat = arginfo.getValue<std::string>("inputformat",libmaus::bambam::BamAlignmentDecoderInfo::getDefaultInputFormat());
				uint64_t const inputthreads = arginfo.getValue<uint64_t>("inputthreads",libmaus::bambam::BamAlignmentDecoderInfo::getDefaultThreads());
				std::string const reference = arginfo.getUnparsedValue("reference",libmaus::bambam::BamAlignmentDecoderInfo::getDefaultReference());
				std::string const prange = arginfo.getUnparsedValue("range",libmaus::bambam::BamAlignmentDecoderInfo::getDefaultRange());
				std::string const pranges = arginfo.getUnparsedValue("ranges",std::string(""));
				std::string const range = pranges.size() ? pranges : prange;
			
				return libmaus::bambam::BamAlignmentDecoderInfo(
					filename,
					inputformat,
					inputthreads,
					reference,
					putrank,
					copystr,
					range
				);
			}
		};
		
		std::ostream & operator<<(std::ostream & out, libmaus::bambam::BamAlignmentDecoderInfo const & o);
	}
}
#endif
