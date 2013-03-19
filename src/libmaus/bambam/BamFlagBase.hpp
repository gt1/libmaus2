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
#if ! defined(LIBMAUS_BAMBAM_BAMFLAGBASE)
#define LIBMAUS_BAMBAM_BAMFLAGBASE

#include <ostream>

namespace libmaus
{
	namespace bambam
	{
		struct BamFlagBase
		{
			enum bam_flags
			{
				LIBMAUS_BAMBAM_FPAIRED = (1u << 0),
				LIBMAUS_BAMBAM_FPROPER = (1u << 1),
				LIBMAUS_BAMBAM_FUNMAP = (1u << 2),
				LIBMAUS_BAMBAM_FMUNMAP = (1u << 3),
				LIBMAUS_BAMBAM_FREVERSE = (1u << 4),
				LIBMAUS_BAMBAM_FMREVERSE = (1u << 5),
				LIBMAUS_BAMBAM_FREAD1 = (1u << 6),
				LIBMAUS_BAMBAM_FREAD2 = (1u << 7),
				LIBMAUS_BAMBAM_FSECONDARY = (1u << 8),
				LIBMAUS_BAMBAM_FQCFAIL = (1u << 9),
				LIBMAUS_BAMBAM_FDUP = (1u << 10)
			};
			
			enum bam_cigar_ops
			{
				LIBMAUS_BAMBAM_CMATCH = 0,
				LIBMAUS_BAMBAM_CINS = 1,
				LIBMAUS_BAMBAM_CDEL = 2,
				LIBMAUS_BAMBAM_CREF_SKIP = 3,
				LIBMAUS_BAMBAM_CSOFT_CLIP = 4,
				LIBMAUS_BAMBAM_CHARD_CLIP = 5,
				LIBMAUS_BAMBAM_CPAD = 6,
				LIBMAUS_BAMBAM_CEQUAL = 7,
				LIBMAUS_BAMBAM_CDIFF = 8
			};			
		};
		
		inline std::ostream & operator<<(std::ostream & out, BamFlagBase::bam_flags const f)
		{
			switch ( f )
			{
				case BamFlagBase::LIBMAUS_BAMBAM_FPAIRED: out << "LIBMAUS_BAMBAM_FPAIRED"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FPROPER: out << "LIBMAUS_BAMBAM_FPROPER"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FUNMAP: out << "LIBMAUS_BAMBAM_FUNMAP"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FMUNMAP: out << "LIBMAUS_BAMBAM_FMUNMAP"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FREVERSE: out << "LIBMAUS_BAMBAM_FREVERSE"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FMREVERSE: out << "LIBMAUS_BAMBAM_FMREVERSE"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FREAD1: out << "LIBMAUS_BAMBAM_FREAD1"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FREAD2: out << "LIBMAUS_BAMBAM_FREAD2"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FSECONDARY: out << "LIBMAUS_BAMBAM_FSECONDARY"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FQCFAIL: out << "LIBMAUS_BAMBAM_FQCFAIL"; break;
				case BamFlagBase::LIBMAUS_BAMBAM_FDUP: out << "LIBMAUS_BAMBAM_FDUP"; break;
			}
			
			return out;
		}
	}
}
#endif
