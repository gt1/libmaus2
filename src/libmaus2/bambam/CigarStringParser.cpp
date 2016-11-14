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

#include <libmaus2/bambam/CigarStringParser.hpp>
#include <libmaus2/bambam/BamFlagBase.hpp>
#include <cassert>
#include <sstream>

std::vector< libmaus2::bambam::cigar_operation > libmaus2::bambam::CigarStringParser::parseCigarString(std::string const & cigar)
{
	std::vector<cigar_operation> ops;
	std::string::const_iterator ita = cigar.begin();
	std::string::const_iterator const ite = cigar.end();

	while ( ita != ite )
	{
		assert ( isdigit(ita[0]) );

		uint64_t numlen = 0;
		while ( ita+numlen != ite && isdigit(ita[numlen]) )
			numlen++;

		std::string const nums(ita,ita+numlen);
		std::istringstream istr(nums);
		uint64_t num = 0;
		istr >> num;

		ita += numlen;

		uint32_t op = 0;

		assert ( ita != ite );

		switch ( ita[0] )
		{
			case 'M':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH;
				break;
			case 'I':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS;
				break;
			case 'D':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL;
				break;
			case 'N':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP;
				break;
			case 'S':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP;
				break;
			case 'H':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP;
				break;
			case 'P':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CPAD;
				break;
			case '=':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL;
				break;
			case 'X':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF;
				break;
			default:
				op = 9;
				break;
		}

		ita += 1;

		ops.push_back(cigar_operation(op,num));
	}

	return ops;
}

size_t libmaus2::bambam::CigarStringParser::parseCigarString(char const * c, libmaus2::autoarray::AutoArray<libmaus2::bambam::cigar_operation> & Aop)
{
	size_t nc = 0;

	while ( *c )
	{
		if ( ! isdigit(*c) )
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "CigarStringParser::parseCigarString: unexpected symbol " << *c << std::endl;
			lme.finish();
			throw lme;
		}

		uint64_t n = 0;
		while ( isdigit(*c) )
		{
			n *= 10;
			n += *(c++) - '0';
		}

		uint32_t op = 0;

		switch ( *(c++) )
		{
			case 'M':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH;
				break;
			case 'I':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS;
				break;
			case 'D':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL;
				break;
			case 'N':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP;
				break;
			case 'S':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP;
				break;
			case 'H':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP;
				break;
			case 'P':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CPAD;
				break;
			case '=':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL;
				break;
			case 'X':
				op = libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF;
				break;
			case 0:
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "CigarStringParser::parseCigarString: unexpected end of string" << std::endl;
				lme.finish();
				throw lme;
			}
			default:
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "CigarStringParser::parseCigarString: unknown/invalid symbol " << *c << std::endl;
				lme.finish();
				throw lme;
			}
		}

		while ( nc >= Aop.size() )
		{
			if ( ! Aop.size() )
				Aop.resize(1);
			else
				Aop.resize(2*Aop.size());
		}

		Aop[nc++] = libmaus2::bambam::cigar_operation(op,n);
	}

	return nc;
}
