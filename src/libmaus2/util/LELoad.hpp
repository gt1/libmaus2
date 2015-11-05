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

#if !defined(LELOAD_HPP)
#define LELOAD_HPP

#include <libmaus2/types/types.hpp>
#include <cstdlib>
#include <cassert>

namespace libmaus2
{
	namespace util
	{
		/*
		 * little endian unaligned number loading
		 */
		inline uint64_t loadValueLE1(uint8_t const * p)
		{
			return static_cast<uint64_t>(*p);
		}
		template<unsigned int mod>
		inline uint64_t loadValueLE2Template(uint8_t const * p)
		{
			if ( mod )
				return static_cast<uint64_t>(p[0]) | ( static_cast<uint64_t>(p[1]) << 8) ;
			else
				return static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p)));
		}
		inline uint64_t loadValueLE2(uint8_t const * p)
		{
			if ( reinterpret_cast<size_t>(p) & 1 )
				return loadValueLE2Template<1>(p);
			else
				return loadValueLE2Template<0>(p);
		}
		// tested
		template<unsigned int mod>
		inline uint64_t loadValueLE3Template(uint8_t const * p)
		{
			if ( mod )
				return
					static_cast<uint64_t>(p[0]) |
					(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p+1))) << 8);
			else
				return
					(static_cast<uint64_t>(p[2]) << 16) |
					(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p))) << 0);
		}
		inline uint64_t loadValueLE3(uint8_t const * p)
		{
			if ( reinterpret_cast<size_t>(p) & 1 )
				return loadValueLE3Template<1>(p);
			else
				return loadValueLE3Template<0>(p);
		}
		template<unsigned int mod>
		inline uint64_t loadValueLE4Template(uint8_t const * p)
		{
			switch ( mod )
			{
				case 0:
					return static_cast<uint64_t>(*(reinterpret_cast<uint32_t const *>(p)));
				case 2:
					return
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p))) << 0) |
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p+2))) << 16);
				default: // 1,3
					return
						static_cast<uint64_t>(*p) |
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p+1))) << 8) |
						(static_cast<uint64_t>(p[3]) << 24);
			}
		}
		inline uint64_t loadValueLE4(uint8_t const * p)
		{
			switch ( reinterpret_cast<size_t>(p) & 3 )
			{
				case 0:  return loadValueLE4Template<0>(p);
				case 1:  return loadValueLE4Template<1>(p);
				case 2:  return loadValueLE4Template<2>(p);
				default: return loadValueLE4Template<3>(p);
			}
		}
		template<unsigned int mod>
		inline uint64_t loadValueLE5Template(uint8_t const * p)
		{
			switch ( mod )
			{
				case 0:
					return
						static_cast<uint64_t>(*(reinterpret_cast<uint32_t const *>(p))) |
						(static_cast<uint64_t>(p[4]) << 32);
				case 1:
					return
						static_cast<uint64_t>(*p) |
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p+1))) << 8) |
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p+3))) << 24);
				case 2:
					return
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p))) << 0) |
						(static_cast<uint64_t>(*(reinterpret_cast<uint16_t const *>(p+2))) << 16) |
						(static_cast<uint64_t>(p[4]) << 32);
				default: // case 3:
					return
						(static_cast<uint64_t>(*p)) |
						(static_cast<uint64_t>(
							*(
								reinterpret_cast<uint32_t const *>(p+1)
							)
						) << 8);
			}
		}
		inline uint64_t loadValueLE5(uint8_t const * p)
		{
			switch ( reinterpret_cast<size_t>(p) & 3 )
			{
				case 0:  return loadValueLE5Template<0>(p);
				case 1:  return loadValueLE5Template<1>(p);
				case 2:  return loadValueLE5Template<2>(p);
				default: return loadValueLE5Template<3>(p);
			}
		}

		template<typename iterator_A, unsigned int numlen, unsigned int mod, unsigned int index, unsigned int cnt>
		struct LoadMultipleValuesNMC
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A)
			{
				// std::cerr << "numlen=" << numlen << " mod=" << mod << " cnt=" << cnt << std::endl;

				switch ( numlen )
				{
					case 1: A[index] = loadValueLE1(p); break;
					case 2: A[index] = loadValueLE2Template<mod % 2>(p); break;
					case 3: A[index] = loadValueLE3Template<mod % 2>(p); break;
					case 4: A[index] = loadValueLE4Template<mod % 4>(p); break;
					case 5: A[index] = loadValueLE5Template<mod % 4>(p); break;
					default: assert(0);
				}

				LoadMultipleValuesNMC<iterator_A,numlen,mod+numlen,index+1,cnt-1>::loadMultipleValues(p+numlen,A);
			}
		};

		template<typename iterator_A, unsigned int numlen, unsigned int mod, unsigned int index>
		struct LoadMultipleValuesNMC<iterator_A,numlen,mod,index,0>
		{
			static inline void loadMultipleValues(uint8_t const *, iterator_A &)
			{
			}
		};

		template<typename iterator_A, unsigned int numlen, unsigned int cnt>
		struct LoadMultipleValuesNC
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A);
		};

		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,0>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,0>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,1>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,1>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,2>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,2>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,3>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,3>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,4>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,4>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,5>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,5>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNC<iterator_A,numlen,6>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A) { LoadMultipleValuesNMC<iterator_A,numlen,0,0,6>::loadMultipleValues(p,A); }
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesNCCall
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A, unsigned int const cnt)
			{
				switch ( cnt )
				{
					case 0: LoadMultipleValuesNC<iterator_A,numlen,0>::loadMultipleValues(p,A); break;
					case 1: LoadMultipleValuesNC<iterator_A,numlen,1>::loadMultipleValues(p,A); break;
					case 2: LoadMultipleValuesNC<iterator_A,numlen,2>::loadMultipleValues(p,A); break;
					case 3: LoadMultipleValuesNC<iterator_A,numlen,3>::loadMultipleValues(p,A); break;
					case 4: LoadMultipleValuesNC<iterator_A,numlen,4>::loadMultipleValues(p,A); break;
					case 5: LoadMultipleValuesNC<iterator_A,numlen,5>::loadMultipleValues(p,A); break;
					case 6: LoadMultipleValuesNC<iterator_A,numlen,6>::loadMultipleValues(p,A); break;
				}
			}
		};
		template<typename iterator_A, unsigned int numlen>
		struct LoadMultipleValuesN
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A, unsigned int const cnt);
		};
		template<typename iterator_A>
		struct LoadMultipleValuesN<iterator_A,1>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A, unsigned int const cnt)
			{
				LoadMultipleValuesNCCall<iterator_A,1>::loadMultipleValues(p,A,cnt);
			}
		};
		template<typename iterator_A>
		struct LoadMultipleValuesN<iterator_A,2>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A, unsigned int const cnt)
			{
				LoadMultipleValuesNCCall<iterator_A,2>::loadMultipleValues(p,A,cnt);
			}
		};
		template<typename iterator_A>
		struct LoadMultipleValuesN<iterator_A,3>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A, unsigned int const cnt)
			{
				LoadMultipleValuesNCCall<iterator_A,3>::loadMultipleValues(p,A,cnt);
			}
		};
		template<typename iterator_A>
		struct LoadMultipleValuesN<iterator_A,4>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A,  unsigned int const cnt)
			{
				LoadMultipleValuesNCCall<iterator_A,4>::loadMultipleValues(p,A,cnt);
			}
		};
		template<typename iterator_A>
		struct LoadMultipleValuesN<iterator_A,5>
		{
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A,  unsigned int const cnt)
			{
				LoadMultipleValuesNCCall<iterator_A,5>::loadMultipleValues(p,A,cnt);
			}
		};
		struct LoadMultipleValuesCall
		{
			template<typename iterator_A>
			static inline void loadMultipleValues(uint8_t const * p, iterator_A & A,  unsigned int const cnt, unsigned int const numlen)
			{
				switch ( numlen )
				{
					case 1: LoadMultipleValuesN<iterator_A,1>::loadMultipleValues(p,A,cnt); break;
					case 2: LoadMultipleValuesN<iterator_A,2>::loadMultipleValues(p,A,cnt); break;
					case 3: LoadMultipleValuesN<iterator_A,3>::loadMultipleValues(p,A,cnt); break;
					case 4: LoadMultipleValuesN<iterator_A,4>::loadMultipleValues(p,A,cnt); break;
					case 5: LoadMultipleValuesN<iterator_A,5>::loadMultipleValues(p,A,cnt); break;
					default: assert(false);
				}
			}
		};
	}
}
#endif
