/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_LF_QUICKLF_HPP)
#define LIBMAUS2_LF_QUICKLF_HPP

#include <libmaus2/lf/LFBase.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/wavelet/WaveletTree.hpp>

namespace libmaus2
{
	namespace lf
	{
		struct QuickLF : LFBase< ::libmaus2::wavelet::QuickWaveletTree< ::libmaus2::rank::ERank222B, uint64_t > >
		{
			typedef LFBase< ::libmaus2::wavelet::QuickWaveletTree< ::libmaus2::rank::ERank222B, uint64_t > > base_type;

			typedef base_type::wt_type wt_type;
			typedef base_type::wt_ptr_type wt_ptr_type;
			typedef QuickLF this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			QuickLF( std::istream & istr ) : base_type ( istr ) {}
			QuickLF( std::istream & istr, uint64_t & s ) : base_type(istr,s) {}
			QuickLF( wt_ptr_type & rW ) : base_type(rW) {}
			QuickLF( bitio::CompactArray::unique_ptr_type & ABWT ) : base_type(ABWT) {}
			QuickLF( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus2::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode )
			: base_type(ABWT,ahnode) {}
		};
	}
}
#endif
