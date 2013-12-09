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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPMULTIFILE_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPMULTIFILE_HPP

#include <string>
#include <ostream>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapMultiFile
		{
			std::vector<std::string> fn;
			uint64_t level;

			SparseGammaGapMultiFile() : fn(), level(0) {}
			SparseGammaGapMultiFile(std::string const & rfn, uint64_t const rlevel)
			: fn(std::vector<std::string>(1,rfn)), level(rlevel) {}
			SparseGammaGapMultiFile(std::vector<std::string> const & rfn, uint64_t const rlevel)
			: fn(rfn), level(rlevel) {}
			
			bool operator<(SparseGammaGapMultiFile const & o) const
			{
				return level > o.level;
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, SparseGammaGapMultiFile const & SGGF)
		{
			out << "SparseGammaGapMultiFile(fn={";
			for ( uint64_t i = 0; i < SGGF.fn.size(); ++i )
			{
				out << SGGF.fn[i];
				if ( i+1 << SGGF.fn.size() )
					out << ",";
			}
			out << "},level=" << SGGF.level << ")";
			return out;
		}
	}
}
#endif
