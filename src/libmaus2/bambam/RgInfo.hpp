/*
    libmaus2
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_RGINFO_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_RGINFO_HPP

#include <libmaus2/util/ArgInfo.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct RgInfo
		{
			std::string ID;
			std::string CN;
			std::string DS;
			std::string DT;
			std::string FO;
			std::string KS;
			std::string LB;
			std::string PG; // = fastqtobam
			std::string PI;
			std::string PL;
			std::string PU;
			std::string SM;
			
			RgInfo() {}
			RgInfo(libmaus2::util::ArgInfo const & arginfo)
			:
				ID(arginfo.getUnparsedValue("RGID","")),
				CN(arginfo.getUnparsedValue("RGCN","")),
				DS(arginfo.getUnparsedValue("RGDS","")),
				DT(arginfo.getUnparsedValue("RGDT","")),
				FO(arginfo.getUnparsedValue("RGFO","")),
				KS(arginfo.getUnparsedValue("RGKS","")),
				LB(arginfo.getUnparsedValue("RGLB","")),
				PG(arginfo.getUnparsedValue("RGPG","fastqtobam")),
				PI(arginfo.getUnparsedValue("RGPI","")),
				PL(arginfo.getUnparsedValue("RGPL","")),
				PU(arginfo.getUnparsedValue("RGPU","")),
				SM(arginfo.getUnparsedValue("RGSM",""))
			{
				
			}
			
			std::string toString() const
			{
				std::ostringstream ostr;
				
				if ( ID.size() )
				{
					ostr << "@RG\tID:" << ID;
					
					if ( CN.size() ) ostr << "\tCN:" << CN;
					if ( DS.size() ) ostr << "\tDS:" << DS;
					if ( DT.size() ) ostr << "\tDT:" << DT;
					if ( FO.size() ) ostr << "\tFO:" << FO;
					if ( KS.size() ) ostr << "\tKS:" << KS;
					if ( LB.size() ) ostr << "\tLB:" << LB;
					if ( PG.size() ) ostr << "\tPG:" << PG;
					if ( PI.size() ) ostr << "\tPI:" << PI;
					if ( PL.size() ) ostr << "\tPL:" << PL;
					if ( PU.size() ) ostr << "\tPU:" << PU;
					if ( SM.size() ) ostr << "\tSM:" << SM;
					
					ostr << "\n";
				}
				
				return ostr.str();
			}
		};
	}
}
#endif
