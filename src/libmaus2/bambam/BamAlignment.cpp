/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/bambam/BamAlignment.hpp>

std::string libmaus2::bambam::BamAlignment::getReferenceRegionViaMd() const
{
	if ( ! isMapped() )
	{
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "BamAlignment::getReferenceRegionViaMd(): read is not aligned" << std::endl;
		lme.finish();
		throw lme;
	}

	char const * md = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(D.begin(),blocksize,"MD");

	if ( ! md )
	{
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "BamAlignment::getReferenceRegionViaMd(): MD aux field is missing" << std::endl;
		lme.finish();
		throw lme;
	}

	std::vector<char> MDOP;
	while ( *md )
	{
		if ( *md >= '0' && *md <= '9' )
		{
			uint64_t n = 0;

			while ( *md && *md >= '0' && *md <= '9' )
			{
				n *= 10;
				n += *md - '0';
				++md;
			}

			for ( uint64_t i = 0; i < n; ++i )
				MDOP.push_back(0);
		}
		else if ( *md >= 'A' && *md <= 'Z' )
		{
			MDOP.push_back(*md);
			++md;
		}
		else if ( *md == '^' )
		{
			// nop
			++md;
		}
		else
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "BamAlignment::getReferenceRegionViaMd(): invalid symbol " << *md << " in MD aux field" << std::endl;
			lme.finish();
			throw lme;
		}
	}

	libmaus2::autoarray::AutoArray<cigar_operation> Acigop, Bcigop;
	uint64_t const numciga = getCigarOperations(Acigop);

	{
		libmaus2::bambam::CigarStringReader<cigar_operation const *> RA(Acigop.begin(),Acigop.begin()+numciga);
		::libmaus2::bambam::BamFlagBase::bam_cigar_ops opA;
		for ( uint64_t i = 0; i < numciga; ++i )
		{
			for ( int64_t j = 0; j < Acigop[i].second; ++j )
			{
				bool const ok = RA.getNext(opA);
				assert ( ok );
				assert ( opA == Acigop[i].first );
			}
		}
	}

	libmaus2::bambam::CigarStringReader<cigar_operation const *> RA(Acigop.begin(),Acigop.begin()+numciga);
	::libmaus2::bambam::BamFlagBase::bam_cigar_ops opA;
	std::string const R = getRead();

	int64_t readpos = 0;
	int64_t refpos = 0;

	std::ostringstream ostr;
	std::vector<char>::const_iterator it = MDOP.begin();
	while ( RA.getNext(opA) )
	{
		switch ( opA )
		{
			case BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
			{
				assert ( it < MDOP.end() );
				char const c = *(it++);
				char const d = c ? c : R[readpos];
				ostr.put(d);
				readpos++;
				refpos++;
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
			{
				assert ( it < MDOP.end() );
				char const c = *(it++);
				assert ( c == 0 );
				ostr.put(R[readpos]);
				readpos++;
				refpos++;
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
			{
				assert ( it < MDOP.end() );
				char const c = *(it++);
				assert ( c != 0 );
				ostr.put(c);
				readpos++;
				refpos++;
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CINS:
			{
				readpos += 1;
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
			{
				readpos += 1;
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
			{
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
			{
				assert ( it < MDOP.end() );
				char const c = *(it++);
				assert ( c != 0 );
				ostr.put(c);
				refpos++;
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
			{
				break;
			}
			case BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
			{
				break;
			}
			default:
			{
				break;
			}
		}
	}

	assert ( readpos == static_cast<int64_t>(R.size()) );

	return ostr.str();
}
