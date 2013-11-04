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

			static std::vector<libmaus::bambam::BamAlignmentDecoderInfo> filenameToInfo(std::vector<std::string> const & filenames)
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
			
			BamAlignmentDecoderInfo(BamAlignmentDecoderInfo const & o)
			: inputfilename(o.inputfilename), inputformat(o.inputformat), inputthreads(o.inputthreads),
			  reference(o.reference), putrank(o.putrank), copystr(o.copystr) {}
			BamAlignmentDecoderInfo(
				std::string rinputfilename = getDefaultInputFileName(),
				std::string rinputformat = getDefaultInputFormat(),
				uint64_t rinputthreads = getDefaultThreads(),
				std::string rreference = getDefaultReference(),
				bool rputrank = getDefaultPutRank(),
				std::ostream * rcopystr = getDefaultCopyStr()
			)
			: inputfilename(rinputfilename), inputformat(rinputformat), inputthreads(rinputthreads),
			  reference(rreference), putrank(rputrank), copystr(rcopystr) {}
			  
			BamAlignmentDecoderInfo & operator==(BamAlignmentDecoderInfo const & o)
			{
				if ( this != &o )
				{
					inputfilename = o.inputfilename;
					inputformat = o.inputformat;
					inputthreads = o.inputthreads;
					reference = o.reference;
					putrank = o.putrank;
					copystr = o.copystr;
				}
				return *this;
			}
		};
	}
}
#endif
