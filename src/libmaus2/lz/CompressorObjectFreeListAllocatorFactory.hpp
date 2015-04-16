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
#if ! defined(LIBMAUS_LZ_COMPRESSOROBJECTFREELISTALLOCATORFACTORY_HPP)
#define LIBMAUS_LZ_COMPRESSOROBJECTFREELISTALLOCATORFACTORY_HPP

#include <libmaus2/lz/SnappyCompressorObjectFreeListAllocator.hpp>
#include <libmaus2/lz/ZlibCompressorObjectFreeListAllocator.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct CompressorObjectFreeListAllocatorFactory
		{
			typedef CompressorObjectFreeListAllocatorFactory this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			static bool startsWith(std::string const & s, std::string const & prefix)
			{
				return
					s.size() >= prefix.size() &&
					s.substr(0,prefix.size()) == prefix;
			}
		
			static libmaus2::lz::CompressorObjectFreeListAllocator::unique_ptr_type construct(std::string const & desc)
			{
				static char const * zlibprefix = "zlib:";
			
				if ( desc == "snappy" )
				{
					libmaus2::lz::SnappyCompressorObjectFreeListAllocator::unique_ptr_type tptr(
						new libmaus2::lz::SnappyCompressorObjectFreeListAllocator
					);
					return UNIQUE_PTR_MOVE(tptr);
				}
				else if ( startsWith(desc,std::string(zlibprefix)) )
				{
					std::string slevel = desc.substr(std::string(zlibprefix).size());
					std::istringstream istr(slevel);
					int64_t ilevel;
					istr >> ilevel;
					
					if ( ! istr )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::lz::CompressorObjectFreeListAllocatorFactory: Cannot parse zlib compression level " << slevel << "\n";
						lme.finish();
						throw lme;
					}
					
					switch ( ilevel )
					{
						case Z_DEFAULT_COMPRESSION:
						case Z_NO_COMPRESSION:
						case Z_BEST_SPEED:
						case Z_BEST_COMPRESSION:
						{
							libmaus2::lz::ZlibCompressorObjectFreeListAllocator::unique_ptr_type tptr(
								new libmaus2::lz::ZlibCompressorObjectFreeListAllocator(ilevel)
							);
							return UNIQUE_PTR_MOVE(tptr);
						}
						break;
						default:
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "libmaus2::lz::CompressorObjectFreeListAllocatorFactory: unknown zlib compression level " << ilevel << "; please choose from {" <<
								Z_DEFAULT_COMPRESSION << "," <<
								Z_NO_COMPRESSION << "," <<
								Z_BEST_SPEED << "," <<
								Z_BEST_COMPRESSION << "}" << "\n";
							lme.finish();
							throw lme;
						}
						break;
					}
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::lz::CompressorObjectFreeListAllocatorFactory: cannot parse compression setting " << desc << "\n";
					lme.finish();
					throw lme;
				}
			}
		};
	}
}
#endif
