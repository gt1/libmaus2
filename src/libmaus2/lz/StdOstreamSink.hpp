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
#if ! defined(LIBMAUS2_LZ_STDOSTREAMSINK)
#define LIBMAUS2_LZ_STDOSTREAMSINK

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_HAVE_SNAPPY)
#include <snappy-sinksource.h>

namespace libmaus2
{
	namespace lz
	{
		template<typename out_type>
		struct StdOstreamSink : public ::snappy::Sink
		{
			typedef StdOstreamSink<out_type> this_type;

			out_type & out;
			uint64_t written;

			StdOstreamSink(out_type & rout) : out(rout), written(0) {}
			virtual ~StdOstreamSink() { out.flush(); }

			void Append(const char* bytes, size_t n)
			{
				out.write(bytes,n);
				written += n;
			}
		};
	}
}
#endif
#endif
