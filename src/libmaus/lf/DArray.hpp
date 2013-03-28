/**
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
**/
#if ! defined(LIBMAUS_LF_DARRAY_HPP)
#define LIBMAUS_LF_DARRAY_HPP

#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>

namespace libmaus
{
	namespace lf
	{
		struct DArray
		{
			::libmaus::autoarray::AutoArray<uint64_t> D;
			
			template<typename stream_type>
			static ::libmaus::autoarray::AutoArray<uint64_t> loadArray(stream_type & CIS)
			{
				uint64_t const s = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				::libmaus::autoarray::AutoArray<uint64_t> D(s,false);
				for ( uint64_t i = 0; i < s; ++i )
					D[i] = ::libmaus::util::NumberSerialisation::deserialiseNumber(CIS);
				return D;
			}

			static ::libmaus::autoarray::AutoArray<uint64_t> loadArray(std::string const & filename)
			{
				::libmaus::aio::CheckedInputStream CIS(filename);	
				return loadArray(CIS);
			}
			
			DArray() : D()
			{
				
			}
			template<typename stream_type>
			DArray(stream_type & stream)
			: D(loadArray(stream))
			{
				
			}
			DArray(std::map<int64_t,uint64_t> const & M, uint64_t const bwtterm)
			: D(bwtterm+1)
			{
				for ( std::map<int64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					D [ ita->first ] += ita->second;
				D.prefixSums();
			}
			DArray(std::string const & fn)
			: D(loadArray(fn))
			{
			}
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{	
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,D.size());
				
				for ( uint64_t i = 0; i < D.size(); ++i )
					::libmaus::util::NumberSerialisation::serialiseNumber(stream,D[i]);
			}
			void serialise(std::string const & filename) const
			{
				::libmaus::aio::CheckedOutputStream COS(filename);
				serialise(COS);
			}
			
			void merge(DArray const & o)
			{
				if ( o.D.size() != D.size() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "DArray::merge(): array sizes are not compatible." << std::endl;
					se.finish();
					throw se;
				}
				
				for ( uint64_t i = 0; i < D.size(); ++i )
					D[i] != o.D[i];
			}
		};
	}
}
#endif
