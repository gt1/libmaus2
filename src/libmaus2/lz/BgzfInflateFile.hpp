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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATEFILE_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEFILE_HPP

#include <libmaus2/lz/BgzfInflateStream.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateFileBase
		{
			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			std::istream & in;

			BgzfInflateFileBase(std::istream & rin, uint64_t const fileoff = 0) : in(rin)
			{
				in.clear();
				in.seekg(fileoff,std::ios::beg);
			}
			BgzfInflateFileBase(std::string const & fn, uint64_t const fileoff = 0) : PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI)
			{
				in.clear();
				in.seekg(fileoff,std::ios::beg);
			}
		};

		struct BgzfInflateFile : public BgzfInflateFileBase, public BgzfInflateStream
		{
			typedef BgzfInflateFile this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			BgzfInflateFile(std::istream & in, uint64_t const fileoff = 0, uint64_t const blockoff = 0)
			: BgzfInflateFileBase(in,fileoff), BgzfInflateStream(BgzfInflateFileBase::in)
			{
				if ( blockoff )
					BgzfInflateStream::ignore(blockoff);
			}
			BgzfInflateFile(std::string const & fn, uint64_t const fileoff = 0, uint64_t const blockoff = 0)
			: BgzfInflateFileBase(fn,fileoff), BgzfInflateStream(BgzfInflateFileBase::in)
			{
				if ( blockoff )
					BgzfInflateStream::ignore(blockoff);
			}
		};
	}
}
#endif
