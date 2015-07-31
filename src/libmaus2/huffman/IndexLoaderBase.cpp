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
#include <libmaus2/huffman/IndexLoaderBase.hpp>
#include <libmaus2/util/ReverseByteOrder.hpp>

/* 
 * read position of index, which is stored in the last 8 bytes of the file 
 * byte order of the number is big endian
 */
uint64_t libmaus2::huffman::IndexLoaderBase::getIndexPos(std::string const & filename)
{
	libmaus2::aio::InputStream::unique_ptr_type Pindexistr(libmaus2::aio::InputStreamFactoryContainer::constructUnique(filename));
	std::istream & indexistr = *Pindexistr;

	// read position of index (last 8 bytes of file)
	// and convert byte order if necessary
	indexistr.seekg(-8,std::ios::end);
	uint64_t v;
	indexistr.read( reinterpret_cast<char *>(&v) , 8 );
	
	#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
	#if defined(_WIN32)
	uint64_t const indexpos = _byteswap_uint64(v);
	#elif defined(__FreeBSD__)
	uint64_t const indexpos = bswap64(v);
	#elif defined(__linux__)
	uint64_t const indexpos = bswap_64(v);
	#else
	uint64_t const indexpos = ::libmaus2::util::ReverseByteOrder::reverseByteOrder<uint64_t>(v);
	#endif
	#else
	uint64_t const indexpos = v;

	#endif
	// std::cerr << "Index at position " << indexpos << " file length " << ::libmaus2::util::GetFileSize::getFileSize(filename) << std::endl;

	return indexpos;
}
