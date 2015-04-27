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

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/stringFunctions.hpp>
#include <limits>
#include <cstdlib>
#include <sstream>
#include <iostream>

#if defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
libmaus2::parallel::PosixSpinLock libmaus2::autoarray::AutoArray_lock;
#elif defined(_OPENMP)
libmaus2::parallel::OMPLock libmaus2::autoarray::AutoArray_lock;
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

uint64_t volatile libmaus2::autoarray::AutoArray_memusage = 0;
uint64_t volatile libmaus2::autoarray::AutoArray_peakmemusage = 0;
uint64_t volatile libmaus2::autoarray::AutoArray_maxmem = getMaxMem();

#if defined(AUTOARRAY_TRACE)
std::vector< libmaus2::autoarray::AutoArrayBackTrace<AUTOARRAY_TRACE> > libmaus2::autoarray::tracevector;
libmaus2::parallel::PosixSpinLock libmaus2::autoarray::backtracelock;
libmaus2::parallel::PosixSpinLock libmaus2::autoarray::tracelock;
#endif

#include <libmaus2/rank/CodeBase.hpp>

typedef ::libmaus2::rank::ChooseCache choose_cache_type;
choose_cache_type libmaus2::rank::CodeBase::CC64(64);

#include <libmaus2/rank/ERankBase.hpp>

typedef ::libmaus2::rank::EncodeCache<16,uint16_t> encode_cache_type;
encode_cache_type libmaus2::rank::ERankBase::EC16;

typedef ::libmaus2::rank::DecodeCache<16,uint16_t> decode_cache_type;
decode_cache_type libmaus2::rank::ERankBase::DC16; 

#include <libmaus2/rank/RankTable.hpp>

#if defined(RANKTABLES)
typedef ::libmaus2::rank::RankTable rank_table_type;
typedef ::libmaus2::rank::SimpleRankTable simple_rank_table_type;
const rank_table_type libmaus2::rank::ERankBase::R;
const simple_rank_table_type libmaus2::rank::ERankBase::S;
#endif  

#include <libmaus2/lcs/HashContainer.hpp>

::libmaus2::autoarray::AutoArray<uint8_t> const ::libmaus2::lcs::HashContainer::S = HashContainer::createSymMap();
::libmaus2::autoarray::AutoArray<unsigned int> const ::libmaus2::lcs::HashContainer::E = HashContainer::createErrorMap();

#include <libmaus2/lcs/HashContainer2.hpp>

::libmaus2::autoarray::AutoArray<uint8_t> const ::libmaus2::lcs::HashContainer2::S = HashContainer2::createSymMap();
::libmaus2::autoarray::AutoArray<unsigned int> const ::libmaus2::lcs::HashContainer2::E = HashContainer2::createErrorMap();

#include <libmaus2/util/SaturatingCounter.hpp>

unsigned int const ::libmaus2::util::SaturatingCounter::shift[4] = { 6,4,2,0 };
unsigned char const ::libmaus2::util::SaturatingCounter::mask[4] = { 
		static_cast<uint8_t>(~(3 << 6)),
		static_cast<uint8_t>(~(3 << 4)),
		static_cast<uint8_t>(~(3 << 2)),
		static_cast<uint8_t>(~(3 << 0))
};

#include <libmaus2/lz/RAZFConstants.hpp>

unsigned int const libmaus2::lz::RAZFConstants::razf_window_bits = 15;
uint64_t const libmaus2::lz::RAZFConstants::razf_block_size = 1ull << razf_window_bits;
uint64_t const libmaus2::lz::RAZFConstants::razf_bin_size = (1ull << 32) / razf_block_size;

#include <libmaus2/network/CurlInit.hpp>

uint64_t libmaus2::network::CurlInit::initcomplete = 0;
libmaus2::parallel::PosixSpinLock libmaus2::network::CurlInit::lock;

#include <libmaus2/network/OpenSSLInit.hpp>

uint64_t libmaus2::network::OpenSSLInit::initcomplete = 0;
libmaus2::parallel::PosixSpinLock libmaus2::network::OpenSSLInit::lock;

#include <libmaus2/util/AlphaDigitTable.hpp>

libmaus2::util::AlphaDigitTable::AlphaDigitTable()
{
	memset(&A[0],0,sizeof(A));
	
	A[static_cast<int>('0')] = 1;
	A[static_cast<int>('1')] = 1;
	A[static_cast<int>('2')] = 1;
	A[static_cast<int>('3')] = 1;
	A[static_cast<int>('4')] = 1;
	A[static_cast<int>('5')] = 1;
	A[static_cast<int>('6')] = 1;
	A[static_cast<int>('7')] = 1;
	A[static_cast<int>('8')] = 1;
	A[static_cast<int>('9')] = 1;
	
	for ( int i = 'a'; i <= 'z'; ++i )
		A[i] = 1;
	for ( int i = 'A'; i <= 'Z'; ++i )
		A[i] = 1;
}

#include <libmaus2/util/AlphaTable.hpp>

libmaus2::util::AlphaTable::AlphaTable()
{
	memset(&A[0],0,sizeof(A));
	
	for ( int i = 'a'; i <= 'z'; ++i )
		A[i] = 1;
	for ( int i = 'A'; i <= 'Z'; ++i )
		A[i] = 1;
}

#include <libmaus2/util/DigitTable.hpp>

libmaus2::util::DigitTable::DigitTable()
{
	memset(&A[0],0,sizeof(A));
	A[static_cast<int>('0')] = 1;
	A[static_cast<int>('1')] = 1;
	A[static_cast<int>('2')] = 1;
	A[static_cast<int>('3')] = 1;
	A[static_cast<int>('4')] = 1;
	A[static_cast<int>('5')] = 1;
	A[static_cast<int>('6')] = 1;
	A[static_cast<int>('7')] = 1;
	A[static_cast<int>('8')] = 1;
	A[static_cast<int>('9')] = 1;
}

#include <libmaus2/bambam/SamPrintableTable.hpp>

libmaus2::bambam::SamPrintableTable::SamPrintableTable()
{
	memset(&A[0],0,sizeof(A));
	
	for ( int i = '!'; i <= '~'; ++i )
		A[i] = 1;
}

#include <libmaus2/bambam/SamZPrintableTable.hpp>

libmaus2::bambam::SamZPrintableTable::SamZPrintableTable()
{
	memset(&A[0],0,sizeof(A));
	A[static_cast<int>(' ')] = 1;
	
	for ( int i = '!'; i <= '~'; ++i )
		A[i] = 1;
}

#include <libmaus2/bambam/SamInfoBase.hpp>

libmaus2::util::DigitTable const libmaus2::bambam::SamInfoBase::DT;
libmaus2::util::AlphaDigitTable const libmaus2::bambam::SamInfoBase::ADT;
libmaus2::util::AlphaTable const libmaus2::bambam::SamInfoBase::AT;
libmaus2::bambam::SamPrintableTable const libmaus2::bambam::SamInfoBase::SPT;
libmaus2::bambam::SamZPrintableTable const libmaus2::bambam::SamInfoBase::SZPT;
libmaus2::math::DecimalNumberParser const libmaus2::bambam::SamInfoBase::DNP;

#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

std::map<std::string,libmaus2::aio::InputStreamFactory::shared_ptr_type> libmaus2::aio::InputStreamFactoryContainer::factories =
	libmaus2::aio::InputStreamFactoryContainer::setupFactories();

#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>

std::map<std::string,libmaus2::aio::OutputStreamFactory::shared_ptr_type> libmaus2::aio::OutputStreamFactoryContainer::factories =
	libmaus2::aio::OutputStreamFactoryContainer::setupFactories();

#include <libmaus2/aio/InputOutputStreamFactoryContainer.hpp>

std::map<std::string,libmaus2::aio::InputOutputStreamFactory::shared_ptr_type> libmaus2::aio::InputOutputStreamFactoryContainer::factories =
	libmaus2::aio::InputOutputStreamFactoryContainer::setupFactories();

#include <libmaus2/bambam/ScramInputContainer.hpp>

std::map<void *, libmaus2::util::shared_ptr<scram_cram_io_input_t>::type > libmaus2::bambam::ScramInputContainer::Mcontrol;	
std::map<void *, libmaus2::aio::InputStream::shared_ptr_type> libmaus2::bambam::ScramInputContainer::Mstream;
std::map<void *, libmaus2::aio::InputStream::shared_ptr_type> libmaus2::bambam::ScramInputContainer::Mcompstream;
libmaus2::parallel::PosixMutex libmaus2::bambam::ScramInputContainer::Mlock;

#include <libmaus2/digest/DigestFactoryContainer.hpp>

std::map< std::string, libmaus2::digest::DigestFactoryInterface::shared_ptr_type > libmaus2::digest::DigestFactoryContainer::factories = 
	libmaus2::digest::DigestFactoryContainer::setupFactories();

#include <libmaus2/util/NotDigitOrTermTable.hpp>
char const libmaus2::util::NotDigitOrTermTable::table[256] = {
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

#include <libmaus2/aio/MemoryFileContainer.hpp>
#include <libmaus2/util/ArgInfoParseBase.hpp>

libmaus2::parallel::PosixMutex libmaus2::aio::MemoryFileContainer::lock;
std::map < std::string, libmaus2::aio::MemoryFile::shared_ptr_type > libmaus2::aio::MemoryFileContainer::M;

static uint64_t getMemoryFileMaxBlockSize()
{
	char const * mem = getenv("MEMORYFILEMAXBLOCKSIZE");

	if ( ! mem )
		// return std::numeric_limits<uint64_t>::max();
		return 16ull*1024ull*1024ull;
	else
	{
		uint64_t const v = libmaus2::util::ArgInfoParseBase::parseValueUnsignedNumeric<uint64_t>("MEMORYFILEMAXBLOCKSIZE",mem);
		
		std::cerr << "[D] using value " << v << " (parsed from " << mem << ") for MEMORYFILEMAXBLOCKSIZE" << std::endl;
		
		return v;
	}
}

uint64_t libmaus2::aio::MemoryFile::maxblocksize = getMemoryFileMaxBlockSize();

#include <libmaus2/util/PosixExecute.hpp>

libmaus2::parallel::PosixMutex libmaus2::util::PosixExecute::lock;
