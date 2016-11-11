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

#if ! defined(LIBMAUS2_BAMBAM_CIGARSTRINGPARSER_HPP)
#define LIBMAUS2_BAMBAM_CIGARSTRINGPARSER_HPP

#include <libmaus2/bambam/CigarOperation.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/bambam/BamFlagBase.hpp>
#include <vector>
#include <string>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * class for parsing cigar strings to cigar operation vectors
		 **/
		struct CigarStringParser
		{
			/**
			 * parse cigar string
			 *
			 * @param cigar SAM type string representation of cigar
			 * @return encoded cigar operation vector (pairs of operation and length)
			 **/
			static std::vector<cigar_operation> parseCigarString(std::string const & cigar);
			/**
			 * parse cigar string
			 *
			 * @param c pointer to C string storing cigar string in string form
			 * @param Aop array for storing parsed vector, will be extended if necessary
			 * @return length of cigar operation vector stored in Aop
			 **/
			static size_t parseCigarString(char const * c, libmaus2::autoarray::AutoArray<cigar_operation> & Aop);
			/**
			 * convert cigar operation vector to alignment trace
			 **/
			template<typename iterator>
			static void cigarToTrace(iterator ita, iterator ite, libmaus2::lcs::AlignmentTraceContainer & ATC, bool ignoreUnknown = true)
			{
				std::vector < libmaus2::lcs::BaseConstants::step_type > ATCV;

				for ( iterator itc = ita; itc != ite; ++itc )
					switch ( itc->first )
					{
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CMATCH:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CREF_SKIP:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP:
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CPAD:
							if ( !ignoreUnknown )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "CigarStringParser::cigarToTrace(): unsupported CIGAR operation " << itc->first << std::endl;
								lme.finish();
								throw lme;
							}
							break;
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS:
							for ( int64_t i = 0; i < static_cast<int64_t>(itc->second); ++i )
								ATCV.push_back(libmaus2::lcs::BaseConstants::STEP_INS);
							break;
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL:
							for ( int64_t i = 0; i < static_cast<int64_t>(itc->second); ++i )
								ATCV.push_back(libmaus2::lcs::BaseConstants::STEP_DEL);
							break;
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL:
							for ( int64_t i = 0; i < static_cast<int64_t>(itc->second); ++i )
								ATCV.push_back(libmaus2::lcs::BaseConstants::STEP_MATCH);
							break;
						case libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF:
							for ( int64_t i = 0; i < static_cast<int64_t>(itc->second); ++i )
								ATCV.push_back(libmaus2::lcs::BaseConstants::STEP_MISMATCH);
							break;
					}

				ATC.resize(ATCV.size());
				for ( uint64_t i = 0; i < ATCV.size(); ++i )
					*(--ATC.ta) = ATCV[ATCV.size()-i-1];
			}

			template<typename iterator>
			static void cigarBlocksToString(std::ostream & out, iterator it, uint64_t n)
			{
				static char const C[] = "MIDNSHP=X";
				static uint32_t const bound = sizeof(C)/sizeof(C[0])-1;

				while ( n-- )
				{
					libmaus2::bambam::cigar_operation const op = *(it++);
					out << op.second;
					if ( op.first < bound )
						out.put(C[op.first]);
					else
						out.put('?');
				}
			}

			template<typename iterator>
			static std::string cigarBlocksToString(iterator it, uint64_t n)
			{
				std::ostringstream ostr;
				cigarBlocksToString(ostr,it,n);
				return ostr.str();
			}

			/**
			 * convert trace stored in ATC to cigar sequence. Add clipping on left and right as noted in (hard|soft)_clip_(left|right)
			 * Aopblocks is used as a temporary array
			 * Aop is used to store the result, it is extended as needed
			 **/
			static uint64_t traceToCigar(
				libmaus2::lcs::AlignmentTraceContainer const & ATC,
				libmaus2::autoarray::AutoArray< std::pair<libmaus2::lcs::AlignmentTraceContainer::step_type,uint64_t> > & Aopblocks,
				libmaus2::autoarray::AutoArray<cigar_operation> & Aop,
				uint64_t hard_clip_left,
				uint64_t soft_clip_left,
				uint64_t soft_clip_right,
				uint64_t hard_clip_right
			)
			{
				uint64_t const clipops = (hard_clip_left!=0)+(soft_clip_left!=0)+(soft_clip_right!=0)+(hard_clip_right!=0);
				uint64_t const nonclipops = ATC.getOpBlocks(Aopblocks);
				uint64_t const totalops = clipops+nonclipops;
				if ( totalops > Aop.size() )
					Aop.resize(totalops);
				cigar_operation * out = Aop.begin();

				if ( hard_clip_left )
					*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP,hard_clip_left);
				if ( soft_clip_left )
					*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP,soft_clip_left);

				for ( uint64_t i = 0; i < nonclipops; ++i )
					switch ( Aopblocks[i].first )
					{
						case libmaus2::lcs::BaseConstants::STEP_MATCH:
							*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CEQUAL,Aopblocks[i].second);
							break;
						case libmaus2::lcs::BaseConstants::STEP_MISMATCH:
							*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDIFF,Aopblocks[i].second);
							break;
						case libmaus2::lcs::BaseConstants::STEP_INS:
							*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CINS,Aopblocks[i].second);
							break;
						case libmaus2::lcs::BaseConstants::STEP_DEL:
							*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CDEL,Aopblocks[i].second);
							break;
						default:
							break;
					}

				if ( soft_clip_right )
					*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CSOFT_CLIP,soft_clip_right);
				if ( hard_clip_right )
					*(out++) = cigar_operation(libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_CHARD_CLIP,hard_clip_right);

				return totalops;
			}
		};
	}
}
#endif
