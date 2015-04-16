/*
    libmaus
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
#if ! defined(LIBMAUS_LF_QUICKLF_HPP)
#define LIBMAUS_LF_QUICKLF_HPP

#include <libmaus/lf/LFBase.hpp>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/wavelet/WaveletTree.hpp>

namespace libmaus
{
	namespace lf
	{
		struct QuickLF : LFBase< ::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, uint64_t > >
		{
			typedef LFBase< ::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, uint64_t > > base_type;

			typedef base_type::wt_type wt_type;
			typedef base_type::wt_ptr_type wt_ptr_type;
			typedef QuickLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
						
			QuickLF( std::istream & istr ) : base_type ( istr ) {}
			QuickLF( std::istream & istr, uint64_t & s ) : base_type(istr,s) {}
			QuickLF( wt_ptr_type & rW ) : base_type(rW) {}
			QuickLF( bitio::CompactArray::unique_ptr_type & ABWT ) : base_type(ABWT) {}
			QuickLF( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode )
			: base_type(ABWT,ahnode) {}			
		};
	}
}
#endif
