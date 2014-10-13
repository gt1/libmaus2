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

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/stringFunctions.hpp>
#include <limits>
#include <cstdlib>
#include <sstream>
#include <iostream>

#if defined(LIBMAUS_HAVE_POSIX_SPINLOCKS)
libmaus::parallel::PosixSpinLock libmaus::autoarray::AutoArray_lock;
#elif defined(_OPENMP)
libmaus::parallel::OMPLock libmaus::autoarray::AutoArray_lock;
#endif

static uint64_t getMaxMem()
{
      char const * mem = getenv("AUTOARRAYMAXMEM");

      if ( ! mem )
            return std::numeric_limits<uint64_t>::max();
      else
      {
            std::string const smem = mem;
            
            std::istringstream istr(smem);
            uint64_t maxmeg;
            istr >> maxmeg;
            
            if ( ! istr )
            {
                  std::cerr << "Unable to parse AUTOARRAYMAXMEM=" << smem << " as integer number." << std::endl;
                  exit(EXIT_FAILURE);
            }
            
            uint64_t const maxmem = maxmeg * (1024*1024);
            
            std::cerr << "AutoArray_maxmem will be set to " << maxmeg << " MB = " << maxmem << " bytes." << std::endl;
            
            return maxmem;
      }      
}

uint64_t libmaus::autoarray::AutoArray_memusage = 0;
uint64_t libmaus::autoarray::AutoArray_peakmemusage = 0;
uint64_t libmaus::autoarray::AutoArray_maxmem = getMaxMem();

#include <libmaus/rank/CodeBase.hpp>

typedef ::libmaus::rank::ChooseCache choose_cache_type;
choose_cache_type libmaus::rank::CodeBase::CC64(64);

#include <libmaus/rank/ERankBase.hpp>

typedef ::libmaus::rank::EncodeCache<16,uint16_t> encode_cache_type;
encode_cache_type libmaus::rank::ERankBase::EC16;

typedef ::libmaus::rank::DecodeCache<16,uint16_t> decode_cache_type;
decode_cache_type libmaus::rank::ERankBase::DC16; 

#include <libmaus/rank/RankTable.hpp>

#if defined(RANKTABLES)
typedef ::libmaus::rank::RankTable rank_table_type;
typedef ::libmaus::rank::SimpleRankTable simple_rank_table_type;
const rank_table_type libmaus::rank::ERankBase::R;
const simple_rank_table_type libmaus::rank::ERankBase::S;
#endif  

#include <libmaus/lcs/HashContainer.hpp>

::libmaus::autoarray::AutoArray<uint8_t> const ::libmaus::lcs::HashContainer::S = HashContainer::createSymMap();
::libmaus::autoarray::AutoArray<unsigned int> const ::libmaus::lcs::HashContainer::E = HashContainer::createErrorMap();

#include <libmaus/lcs/HashContainer2.hpp>

::libmaus::autoarray::AutoArray<uint8_t> const ::libmaus::lcs::HashContainer2::S = HashContainer2::createSymMap();
::libmaus::autoarray::AutoArray<unsigned int> const ::libmaus::lcs::HashContainer2::E = HashContainer2::createErrorMap();

#include <libmaus/util/SaturatingCounter.hpp>

unsigned int const ::libmaus::util::SaturatingCounter::shift[4] = { 6,4,2,0 };
unsigned char const ::libmaus::util::SaturatingCounter::mask[4] = { 
		static_cast<uint8_t>(~(3 << 6)),
		static_cast<uint8_t>(~(3 << 4)),
		static_cast<uint8_t>(~(3 << 2)),
		static_cast<uint8_t>(~(3 << 0))
};

#include <libmaus/lz/RAZFConstants.hpp>

unsigned int const libmaus::lz::RAZFConstants::razf_window_bits = 15;
uint64_t const libmaus::lz::RAZFConstants::razf_block_size = 1ull << razf_window_bits;
uint64_t const libmaus::lz::RAZFConstants::razf_bin_size = (1ull << 32) / razf_block_size;

#include <libmaus/network/CurlInit.hpp>

uint64_t libmaus::network::CurlInit::initcomplete = 0;
libmaus::parallel::PosixSpinLock libmaus::network::CurlInit::lock;

#include <libmaus/network/OpenSSLInit.hpp>

uint64_t libmaus::network::OpenSSLInit::initcomplete = 0;
libmaus::parallel::PosixSpinLock libmaus::network::OpenSSLInit::lock;
