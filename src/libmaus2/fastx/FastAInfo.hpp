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
#if ! defined(LIBMAUS2_FASTX_FASTAINFO_HPP)
#define LIBMAUS2_FASTX_FASTAINFO_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/utf8.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAInfo
		{
			std::string sid;
			uint64_t len;

			FastAInfo()
			{

			}
			FastAInfo(
				std::string const & rsid,
				uint64_t const rlen
			) : sid(rsid), len(rlen)
			{

			}
			FastAInfo(std::istream & in)
			{
				len = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t sidlenlen = 0;
				uint64_t const sidlen = libmaus2::util::UTF8::decodeUTF8(in,sidlenlen);
				libmaus2::autoarray::AutoArray<char> B(sidlen,false);
				in.read(B.begin(),sidlen);
				if ( in.gcount() != static_cast<int64_t>(sidlen) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastAInfo(std::istream &): unexpected EOF/IO failure" << std::endl;
					lme.finish();
					throw lme;
				}
				sid = std::string(B.begin(),B.end());

				uint64_t bytes = sizeof(uint64_t) + sidlen + sidlenlen;

				while ( bytes % sizeof(uint64_t) )
				{
					in.get();
					++bytes;
				}
			}

			private:
			template<typename stream_type>
			void serialiseInternal(stream_type & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,len);
				libmaus2::util::UTF8::encodeUTF8(sid.size(),out);
				out.write(sid.c_str(),sid.size());
			}

			public:
			template<typename stream_type>
			uint64_t serialise(stream_type & out) const
			{
				libmaus2::util::CountPutObject CPO;
				serialiseInternal(CPO);
				serialiseInternal(out);

				while ( CPO.c % sizeof(uint64_t) )
				{
					out.put(0);
					CPO.put(0);
				}

				return CPO.c;
			}
		};
	}
}
#endif
