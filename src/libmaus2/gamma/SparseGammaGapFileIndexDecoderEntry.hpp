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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILEINDEXDECODERENTRY_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILEINDEXDECODERENTRY_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace gamma
	{
		struct SparseGammaGapFileIndexDecoderEntry
		{
			uint64_t ikey;
			uint64_t ibitoff;
			
			SparseGammaGapFileIndexDecoderEntry() : ikey(0), ibitoff(0) {}
			SparseGammaGapFileIndexDecoderEntry(uint64_t const rikey, uint64_t const ribitoff = 0) 
			: ikey(rikey), ibitoff(ribitoff) {}
			
			bool operator<(SparseGammaGapFileIndexDecoderEntry const & o) const
			{
				return ikey < o.ikey;
			}
		};

		std::ostream & operator<<(std::ostream & out, SparseGammaGapFileIndexDecoderEntry const & S);
	}
}
#endif
