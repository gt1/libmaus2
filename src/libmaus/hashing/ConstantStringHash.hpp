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
#if ! defined(LIBMAUS_HASHING_CONSTANTSTRINGHASH_HPP)
#define LIBMAUS_HASHING_CONSTANTSTRINGHASH_HPP

#include <libmaus/hashing/hash.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace hashing
	{
		struct ConstantStringHash
		{
			typedef ConstantStringHash this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			unsigned int k;
			uint64_t n;
			uint64_t m;			
			libmaus::autoarray::AutoArray<int64_t> H;
			
			ConstantStringHash(ConstantStringHash const & O)
			: k(O.k), n(O.n), m(O.m), H(O.H.size(),false)
			{
				std::copy(O.H.begin(),O.H.end(),H.begin());
			} 

			public:
			int64_t operator[](uint64_t const i) const
			{
				return H[i & m];
			}
			
			unique_ptr_type uclone() const
			{
				unique_ptr_type ptr(new this_type(*this));
				return UNIQUE_PTR_MOVE(ptr);
			}

			shared_ptr_type sclone() const
			{
				return shared_ptr_type(new this_type(*this));
			}
			
			template<typename iterator>
			static unique_ptr_type construct(iterator ita, iterator ite, uint64_t const maxn = 64*1024)
			{
				try
				{
					unique_ptr_type u(new this_type(ita,ite,maxn));
					return UNIQUE_PTR_MOVE(u);
				}			
				catch(...)
				{
					unique_ptr_type ptr;
					return UNIQUE_PTR_MOVE(ptr);
				}
			}

			template<typename iterator>
			static shared_ptr_type constructShared(iterator ita, iterator ite, uint64_t const maxn = 64*1024)
			{
				try
				{
					shared_ptr_type s(new this_type(ita,ite,maxn));
					return s;
				}			
				catch(...)
				{
					return shared_ptr_type();
				}
			}

			template<typename iterator>
			ConstantStringHash(iterator ita, iterator ite, uint64_t const maxn = 64*1024)
			{
				k = 0;
				n = (1 << k);
				m = 0;
				bool ok = false;
				
				for ( ; (! ok) && n <= maxn; ++k, n <<= 1, m = (m << 1)|1 )
				{
					libmaus::autoarray::AutoArray<uint64_t> C(n);

					for ( iterator it = ita; it != ite; ++it )
					{
						assert ( (it->hash() & m) < n );
						C [ it->hash() & m ] ++;
					}
						
					ok = true;
					for ( uint64_t i = 0; i < n; ++i )
						ok = ok && C[i] <= 1;
				}
				
				/*
				 * if we did not find an n <= maxn such that the hash is perfect
				 * then check whether the keys are unique and print warning if
				 * they are not
				 */
				if ( ! ok )
				{
					std::set<uint64_t> S;
					
					for ( iterator it = ita; it != ite; ++it )
					{
						uint64_t const h = it->hash();
						
						if ( S.find(h) == S.end() )
						{
							S.insert(h);
						}
						else
						{
							std::cerr << "[E] ConstantStringHash hash value " << h << " is not unique." << std::endl;
						}
					}
				}
				
				/*
				 * throw exception if we cannot construct the hash. Do not translate
				 * the stack trace, as this may not be a critical error
				 */
				if ( ! ok )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "Cannot create perfect hash of size <= " << maxn << " for " << ite-ita << " elements" << std::endl;
					se.finish(false /* do not translate stack trace */);
					throw se;
				}
				
				H = libmaus::autoarray::AutoArray<int64_t>(n);
				std::fill(H.begin(),H.end(),-1);
				
				for ( iterator it = ita; it != ite; ++it )
					H [ it->hash() & m ] = it-ita;

				for ( iterator it = ita; it != ite; ++it )
					assert ( H [ it->hash() & m ] == it-ita );
			}
		};
	}
}
#endif
