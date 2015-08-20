/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_APPROXIMATERUN_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_APPROXIMATERUN_HPP

#include <cassert>
#include <set>
#include <vector>
#include <utility>
#include <libmaus2/dazzler/db/InputBase.hpp>
#include <libmaus2/dazzler/db/OutputBase.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct ApproximateRun
			{
				int64_t a_readid;
				int64_t b_readid;
				std::pair<int64_t,int64_t> full;
				
				enum approximate_run_alignment_type
				{
					approximate_run_alignment_left,
					approximate_run_alignment_right
				};
				
				approximate_run_alignment_type approximate_run_alignment;
				
				std::vector<int64_t> other;
				
				uint64_t serialise(std::ostream & out) const
				{
					uint64_t offset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(out,a_readid,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(out,b_readid,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,full.first,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,full.second,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(out,static_cast<int>(approximate_run_alignment),offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(out,other.size(),offset);
					for ( std::vector<int64_t>::const_iterator ita = other.begin(); ita != other.end(); ++ita )
						libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,*ita,offset);
					return offset;
				}
				
				void deserialise(std::istream & in)
				{
					uint64_t offset = 0;
					a_readid = libmaus2::dazzler::db::InputBase::getLittleEndianInteger4(in,offset);
					b_readid = libmaus2::dazzler::db::InputBase::getLittleEndianInteger4(in,offset);
					full.first = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					full.second = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					
					int32_t const align_v = libmaus2::dazzler::db::InputBase::getLittleEndianInteger4(in,offset);
					
					switch ( align_v )
					{
						case approximate_run_alignment_left:
							approximate_run_alignment = approximate_run_alignment_left;
							break;
						case approximate_run_alignment_right:
							approximate_run_alignment = approximate_run_alignment_right;
							break;
						default:
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "ApproximateRun::deserialise(): unknown value for approximate_run_alignment" << std::endl;
							lme.finish();
							throw lme;
							break;
						}
					}
					uint64_t const other_size = libmaus2::dazzler::db::InputBase::getLittleEndianInteger4(in,offset);
					other.resize(other_size);
					for ( uint64_t i = 0; i < other_size; ++i )
						other[i] = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
				}
				
				bool operator<(ApproximateRun const & A) const
				{
					if ( a_readid != A.a_readid )
						return a_readid < A.a_readid;
					else if ( full.first != A.full.first )
						return full.first < A.full.first;
					else if ( full.second != A.full.second )
						return full.second < A.full.second;
					else if ( approximate_run_alignment != A.approximate_run_alignment )
						return approximate_run_alignment < A.approximate_run_alignment;
					else if ( b_readid != A.b_readid )
						return b_readid < A.b_readid;
					else
					{
						uint64_t const m = std::min(other.size(),A.other.size());
						for ( uint64_t i = 0; i < m; ++i )
							if ( other[i] != A.other[i] )
								return other[i] < A.other[i];
						
						if ( other.size() != A.other.size() )
							return other.size() < A.other.size();
							
						return false;
					}
				}
				
				bool operator==(ApproximateRun const & A) const
				{
					return
						(!(*this < A))
						&&
						(!(A < *this));
				}
			};

			std::ostream & operator<<(std::ostream & out, ApproximateRun const & AR);
		}
	}
}
#endif
