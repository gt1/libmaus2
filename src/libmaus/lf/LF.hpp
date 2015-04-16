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
#if ! defined(LIBMAUS_LF_LF_HPP)
#define LIBMAUS_LF_LF_HPP

#include <libmaus/lf/LFBase.hpp>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/wavelet/WaveletTree.hpp>

#if 0
#include <memory>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/bitio/CompactArray.hpp>

#include <libmaus/wavelet/toWaveletTreeBits.hpp>
#include <libmaus/wavelet/WaveletTree.hpp>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/math/bitsPerNum.hpp>
#include <libmaus/rank/CacheLineRank.hpp>
#include <libmaus/rank/ImpCacheLineRank.hpp>
#include <libmaus/wavelet/ExternalWaveletGenerator.hpp>
#include <libmaus/huffman/RLDecoder.hpp>
#include <libmaus/wavelet/ImpWaveletTree.hpp>
#include <libmaus/wavelet/ImpExternalWaveletGenerator.hpp>
#include <libmaus/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus/rl/RLIndex.hpp>
#endif

namespace libmaus
{
	namespace lf
	{
		struct LF : LFBase< ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t > >
		{
			typedef LFBase< ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t > > base_type;
			
			typedef base_type::wt_type wt_type;
			typedef base_type::wt_ptr_type wt_ptr_type;
			typedef LF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			LF( std::istream & istr ) : base_type ( istr ) {}
			LF( std::istream & istr, uint64_t & s ) : base_type(istr,s) {}
			LF( wt_ptr_type & rW ) : base_type(rW) {}
			LF( bitio::CompactArray::unique_ptr_type & ABWT ) : base_type(ABWT) {}
			LF( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode )
			: base_type(ABWT,ahnode) {}			
		};
	}
}
#endif
