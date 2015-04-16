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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILE_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILE_HPP

#include <string>
#include <ostream>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct SparseGammaGapFile
		{
			std::string fn;
			uint64_t level;

			SparseGammaGapFile() : fn(), level(0) {}
			SparseGammaGapFile(std::string const & rfn, uint64_t const rlevel)
			: fn(rfn), level(rlevel) {}
			
			bool operator<(SparseGammaGapFile const & o) const
			{
				return level > o.level;
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, SparseGammaGapFile const & SGGF)
		{
			out << "SparseGammaGapFile(fn=" << SGGF.fn << ",level=" << SGGF.level << ")";
			return out;
		}
	}
}
#endif
