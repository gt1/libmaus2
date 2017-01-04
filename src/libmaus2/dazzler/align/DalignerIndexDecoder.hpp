/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_DALIGNERINDEXDECODER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_DALIGNERINDEXDECODER_HPP

#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct DalignerIndexDecoder
			{
				typedef DalignerIndexDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::string const lasfn;
				std::string const indexfn;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & ISI;
				uint64_t const fs;
				uint64_t const pren;
				uint64_t const n;
				int64_t const minaread;
				uint64_t const lassize;
				int64_t const tspace;

				libmaus2::parallel::PosixSpinLock lock;

				static std::string getDalignerIndexName(std::string const & aligns)
				{
					std::string::size_type const p = aligns.find_last_of('/');

					if ( p == std::string::npos )
						return std::string(".") + aligns + std::string(".idx");
					else
					{
						std::string const prefix = aligns.substr(0,p);
						std::string const suffix = aligns.substr(p+1);
						return prefix + "/." + suffix + ".idx";
					}
				}

				int64_t getMinARead() const
				{
					libmaus2::aio::InputStreamInstance AISI(lasfn);
					libmaus2::dazzler::align::AlignmentFile AF(AISI);
					libmaus2::dazzler::align::Overlap OVL;
					if ( AF.getNextOverlap(AISI,OVL) )
						return OVL.aread;
					else
						return -1;
				}

				uint64_t operator[](uint64_t const i)
				{
					if ( minaread < 0 )
						return libmaus2::dazzler::align::AlignmentFile::getSerialisedHeaderSize();
					else if ( static_cast<int64_t>(i) < minaread )
						return libmaus2::dazzler::align::AlignmentFile::getSerialisedHeaderSize();
					else if ( static_cast<int64_t>(i) >= static_cast<int64_t>(minaread + n) )
						return lassize;
					else
					{
						libmaus2::parallel::ScopePosixSpinLock slock(lock);

						uint64_t o = (4 + (i-minaread))*sizeof(uint64_t);

						ISI.clear();
						ISI.seekg(o,std::ios::beg);

						return libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(ISI,o);
					}
				}

				DalignerIndexDecoder(std::string const & rlasfn, std::string const & rindexfn = std::string())
				: lasfn(rlasfn),
				  indexfn(rindexfn.size() ? rindexfn : getDalignerIndexName(lasfn)),
				  PISI(new libmaus2::aio::InputStreamInstance(indexfn)), ISI(*PISI),
				  fs(libmaus2::util::GetFileSize::getFileSize(ISI)),
				  pren(fs/sizeof(uint64_t)),
				  n(fs - 5),
				  minaread(getMinARead()),
				  lassize(libmaus2::util::GetFileSize::getFileSize(lasfn)),
				  tspace(libmaus2::dazzler::align::AlignmentFile::getTSpace(lasfn))
				{
					if ( fs % sizeof(uint64_t) != 0 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DalignerIndexDecoder: size of file " << indexfn << " is not a multiple of " << sizeof(uint64_t) << std::endl;
						lme.finish();
						throw lme;
					}
					if ( pren < 5 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DalignerIndexDecoder: size of file " << indexfn << " is too short" << std::endl;
						lme.finish();
						throw lme;
					}
				}
			};
		}
	}
}
#endif
