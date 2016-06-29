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
#if ! defined(LIBMAUS2_BAMBAM_BAMFLAGBASE)
#define LIBMAUS2_BAMBAM_BAMFLAGBASE

#include <ostream>
#include <deque>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/stringFunctions.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * base class for BAM flags
		 **/
		struct BamFlagBase
		{
			//! alignment flags
			enum bam_flags
			{
				LIBMAUS2_BAMBAM_FPAIRED = (1u << 0),
				LIBMAUS2_BAMBAM_FPROPER_PAIR = (1u << 1),
				LIBMAUS2_BAMBAM_FUNMAP = (1u << 2),
				LIBMAUS2_BAMBAM_FMUNMAP = (1u << 3),
				LIBMAUS2_BAMBAM_FREVERSE = (1u << 4),
				LIBMAUS2_BAMBAM_FMREVERSE = (1u << 5),
				LIBMAUS2_BAMBAM_FREAD1 = (1u << 6),
				LIBMAUS2_BAMBAM_FREAD2 = (1u << 7),
				LIBMAUS2_BAMBAM_FSECONDARY = (1u << 8),
				LIBMAUS2_BAMBAM_FQCFAIL = (1u << 9),
				LIBMAUS2_BAMBAM_FDUP = (1u << 10),
				LIBMAUS2_BAMBAM_FSUPPLEMENTARY = (1u << 11),
				LIBMAUS2_BAMBAM_FCIGAR32 = (1u << 15)
			};

			//! cigar operator codes
			enum bam_cigar_ops
			{
				LIBMAUS2_BAMBAM_CMATCH = 0,
				LIBMAUS2_BAMBAM_CINS = 1,
				LIBMAUS2_BAMBAM_CDEL = 2,
				LIBMAUS2_BAMBAM_CREF_SKIP = 3,
				LIBMAUS2_BAMBAM_CSOFT_CLIP = 4,
				LIBMAUS2_BAMBAM_CHARD_CLIP = 5,
				LIBMAUS2_BAMBAM_CPAD = 6,
				LIBMAUS2_BAMBAM_CEQUAL = 7,
				LIBMAUS2_BAMBAM_CDIFF = 8,
				LIBMAUS2_BAMBAM_CTHRES = 16
			};

			/**
			 * convert a string to an alignment flag bit
			 *
			 * @param s string representation of flag (e.g. PAIRED,PROPER_PAIR,...)
			 * @return numeric value for flag s
			 **/
			static uint64_t stringToFlag(std::string const & s)
			{
				if ( s == "PAIRED" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED;
				else if ( s == "PROPER_PAIR" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPROPER_PAIR;
				else if ( s == "UNMAP" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FUNMAP;
				else if ( s == "MUNMAP" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP;
				else if ( s == "REVERSE" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;
				else if ( s == "MREVERSE" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMREVERSE;
				else if ( s == "READ1" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1;
				else if ( s == "READ2" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2;
				else if ( s == "SECONDARY" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY;
				else if ( s == "QCFAIL" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FQCFAIL;
				else if ( s == "DUP" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FDUP;
				else if ( s == "SUPPLEMENTARY" )
					return ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY;
				else
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Unknown flag " << s << std::endl;
					se.finish();
					throw se;
				}
			}

			/**
			 * convert comma separated flags in string representation to numeric flag mask
			 *
			 * @param s flag string
			 * @return numeric flags bit mask
			 **/
			static uint64_t stringToFlags(std::string const & s)
			{
				std::deque<std::string> const tokens = ::libmaus2::util::stringFunctions::tokenize(s,std::string(","));
				uint64_t flags = 0;

				for ( uint64_t i = 0; i < tokens.size(); ++i )
					flags |= stringToFlag(tokens[i]);

				return flags;
			}
		};

		inline std::ostream & operator<<(std::ostream & out, BamFlagBase::bam_cigar_ops const & v)
		{
			switch ( v )
			{
				case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH: return out << 'M';
				case BamFlagBase::LIBMAUS2_BAMBAM_CINS: return out << 'I';
				case BamFlagBase::LIBMAUS2_BAMBAM_CDEL: return out << 'D';
				case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP: return out << 'N';
				case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP: return out << 'S';
				case BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP: return out << 'H';
				case BamFlagBase::LIBMAUS2_BAMBAM_CPAD: return out << 'P';
				case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL: return out << '=';
				case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF: return out << 'X';
				default: return out;
			}
		}

	}
}
#endif
