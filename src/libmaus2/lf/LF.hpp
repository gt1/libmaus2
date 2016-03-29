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
#if ! defined(LIBMAUS2_LF_LF_HPP)
#define LIBMAUS2_LF_LF_HPP

#include <libmaus2/lf/LFBase.hpp>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/wavelet/WaveletTree.hpp>

#if 0
#include <memory>
#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/bitio/CompactArray.hpp>

#include <libmaus2/wavelet/toWaveletTreeBits.hpp>
#include <libmaus2/wavelet/WaveletTree.hpp>
#include <libmaus2/huffman/huffman.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/rank/CacheLineRank.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/wavelet/ExternalWaveletGenerator.hpp>
#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/wavelet/ImpWaveletTree.hpp>
#include <libmaus2/wavelet/ImpExternalWaveletGenerator.hpp>
#include <libmaus2/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus2/wavelet/ImpCompactHuffmanWaveletTree.hpp>
#include <libmaus2/rl/RLIndex.hpp>
#endif

namespace libmaus2
{
	namespace lf
	{
		struct LF : LFBase< ::libmaus2::wavelet::WaveletTree< ::libmaus2::rank::ERank222B, uint64_t > >
		{
			typedef LFBase< ::libmaus2::wavelet::WaveletTree< ::libmaus2::rank::ERank222B, uint64_t > > base_type;

			typedef base_type::wt_type wt_type;
			typedef base_type::wt_ptr_type wt_ptr_type;
			typedef LF this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			LF( std::istream & istr ) : base_type ( istr ) {}
			LF( std::istream & istr, uint64_t & s ) : base_type(istr,s) {}
			LF( wt_ptr_type & rW ) : base_type(rW) {}
			LF( bitio::CompactArray::unique_ptr_type & ABWT, bool const verbose, bool const parallel, uint64_t const numthreads ) : base_type(ABWT,verbose,parallel,numthreads) {}
			LF( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus2::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode, bool const verbose, bool const parallel, uint64_t const numthreads )
			: base_type(ABWT,ahnode,verbose,parallel,numthreads) {}
		};
	}
}
#endif
