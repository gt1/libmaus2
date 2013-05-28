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

#if ! defined(SERIALIZE_HPP)
#define SERIALIZE_HPP

#include <ostream>
#include <istream>
#include <map>

#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace serialize
	{
		template<typename N>
		struct Serialize
		{
			private:
			static uint64_t serialize(std::ostream & out, N const & c);
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n);
			
			static uint64_t deserialize(std::istream & in, N * p);
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n);

			static uint64_t ignore(std::istream & in);
			static uint64_t ignoreArray(std::istream & in, uint64_t const n);

			static uint64_t skip(std::istream & in);
			static uint64_t skipArray(std::istream & in, uint64_t const n);
		};

		template<typename N>
		struct BuiltinLocalSerializer
		{
			static uint64_t serialize(std::ostream & out, N const & c)
			{
				out.write( reinterpret_cast<char const *>(&c) , sizeof(N) ); return sizeof(N);
			}
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n)
			{
				out.write( reinterpret_cast<char const *>(A) , n * sizeof(N) ); return n * sizeof(N);
			}
			static uint64_t deserialize(std::istream & in, N * p)
			{
				in.read ( reinterpret_cast<char *>(p) , sizeof(N) ); return sizeof(N);
			}
			static uint64_t deserializeArray(
				std::istream & in, N * p, uint64_t const n
			)
			{
				static uint64_t const bs = (64*1024 + sizeof(N) - 1) / sizeof(N);
			
				uint64_t const full = n / bs;
				uint64_t const rest = n - full*bs;
			
				for ( uint64_t i = 0; i < full; ++i )
					in.read ( reinterpret_cast<char *>(p+i*bs) , bs * sizeof(N) ); 
				if ( rest )
					in.read ( reinterpret_cast<char *>(p+full*bs) , rest * sizeof(N) ); 
				
				return n*sizeof(N);
			}
			static uint64_t ignore(std::istream & in)
			{
				in.ignore ( sizeof(N) ); return sizeof(N);
			}
			static uint64_t ignoreArray(std::istream & in, uint64_t const n)
			{
				in.ignore ( n * sizeof(N) ); return n*sizeof(N);
			}
			static uint64_t skip(std::istream & in)
			{
				in.seekg(sizeof(N),std::ios::cur); return sizeof(N);
			}
			static uint64_t skipArray(std::istream & in, uint64_t const n)
			{
				in.seekg (n * sizeof(N), std::ios::cur ); return n*sizeof(N);
			}
		};

		template<>
		struct Serialize<unsigned char>
		{
			typedef unsigned char N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<unsigned short>
		{
			typedef unsigned short N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<unsigned int>
		{
			typedef unsigned int N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<unsigned long>
		{
			typedef unsigned long N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<unsigned long long>
		{
			typedef unsigned long long N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<char>
		{
			typedef char N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<short>
		{
			typedef short N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<int>
		{
			typedef int N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<long>
		{
			typedef long N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize<long long>
		{
			typedef long long N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		#if 0
		template<>
		struct Serialize<size_t>
		{
			typedef size_t N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};
		#endif

		template<>
		struct Serialize< std::pair <uint32_t,uint32_t> >
		{
			typedef std::pair <uint32_t,uint32_t> N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};

		template<>
		struct Serialize< std::pair <uint64_t,uint64_t> >
		{
			typedef std::pair <uint64_t,uint64_t> N;
			static uint64_t serialize(std::ostream & out, N const & c) { return BuiltinLocalSerializer<N>::serialize(out,c); }
			static uint64_t serializeArray(std::ostream & out, N const * A, uint64_t const n) { return BuiltinLocalSerializer<N>::serializeArray(out,A,n); }
			static uint64_t deserialize(std::istream & in, N * p) { return BuiltinLocalSerializer<N>::deserialize(in,p); }
			static uint64_t deserializeArray(std::istream & in, N * p, uint64_t const n) { return BuiltinLocalSerializer<N>::deserializeArray(in,p,n); }
			static uint64_t ignore(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t ignoreArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
			static uint64_t skip(std::istream & in) { return BuiltinLocalSerializer<N>::ignore(in); }
			static uint64_t skipArray(std::istream & in, uint64_t const n) { return BuiltinLocalSerializer<N>::ignoreArray(in,n); }
		};
	}
}

#endif
