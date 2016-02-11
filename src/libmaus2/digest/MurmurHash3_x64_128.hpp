/**
   -----------------------------------------------------------------------------
   The following code is an adaption of Austin Appleby's MurmurHash3 for block
   updates by German Tischler. Like MurmurHash3 it is placed in the public
   domain. The authors hereby disclaim copyright to this source code.
 **/
#if ! defined(LIBMAUS2_DIGEST_MURMURHASH3_X64_128_CONTEXT_HPP)
#define LIBMAUS2_DIGEST_MURMURHASH3_X64_128_CONTEXT_HPP

#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <utility>
#include <libmaus2/digest/DigestBase.hpp>

namespace libmaus2
{
	namespace digest
	{
		struct MurmurHash3_x64_128 : public ::libmaus2::digest::DigestBase<16 /* digest length */, 7 /* block size shift */, 0 /* need padding */, 0 /* number length */, false>
		{
			private:
			uint64_t h1;
			uint64_t h2;
			uint8_t B[16];
			size_t f;
			uint64_t numblocks;

			static inline uint64_t rotl64 ( uint64_t x, int8_t r )
			{
				return (x << r) | (x >> (64 - r));
			}

			static inline uint64_t fmix64 ( uint64_t k )
			{
				k ^= k >> 33;
				k *= 0xff51afd7ed558ccdLLU;
				k ^= k >> 33;
				k *= 0xc4ceb9fe1a85ec53LLU;
				k ^= k >> 33;
				return k;
			}

			#if defined(LIBMAUS2_HAVE_x86_64)
			static inline uint64_t getblock64 ( const uint64_t * p, int i )
			{
			  return p[i];
			}
			#else
			static inline uint64_t getblock64 ( const uint64_t * p, int i )
			{
				uint8_t const * u = reinterpret_cast<uint8_t const *>(p+i);

				#if defined(LIBMAUS2_BYTE_ORDER_LITTLE_ENDIAN)
				return
					(static_cast<uint64_t>(u[0]) << 0) |
					(static_cast<uint64_t>(u[1]) << 8) |
					(static_cast<uint64_t>(u[2]) << 16) |
					(static_cast<uint64_t>(u[3]) << 24) |
					(static_cast<uint64_t>(u[4]) << 32) |
					(static_cast<uint64_t>(u[5]) << 40) |
					(static_cast<uint64_t>(u[6]) << 48) |
					(static_cast<uint64_t>(u[7]) << 56)
					;
				#elif defined(LIBMAUS2_BYTE_ORDER_BIG_ENDIAN)
				return
					(static_cast<uint64_t>(u[7]) << 0) |
					(static_cast<uint64_t>(u[6]) << 8) |
					(static_cast<uint64_t>(u[5]) << 16) |
					(static_cast<uint64_t>(u[4]) << 24) |
					(static_cast<uint64_t>(u[3]) << 32) |
					(static_cast<uint64_t>(u[2]) << 40) |
					(static_cast<uint64_t>(u[1]) << 48) |
					(static_cast<uint64_t>(u[0]) << 56)
					;
				#else
				#error "Cannot handle non little/big endian architecture"
				#endif
			}
			#endif

			void finish()
			{
				uint8_t const * tail = &B[0];

				uint64_t k1 = 0;
				uint64_t k2 = 0;

				size_t const len = f + numblocks*sizeof(B)/sizeof(B[0]);

				uint64_t const c1 = 0x87c37b91114253d5LLU;
				uint64_t const c2 = 0x4cf5ad432745937fLLU;

				switch(len & 15)
				{
					case 15: k2 ^= ((uint64_t)tail[14]) << 48;
					case 14: k2 ^= ((uint64_t)tail[13]) << 40;
					case 13: k2 ^= ((uint64_t)tail[12]) << 32;
					case 12: k2 ^= ((uint64_t)tail[11]) << 24;
					case 11: k2 ^= ((uint64_t)tail[10]) << 16;
					case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
					case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
						k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;
					case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
					case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
					case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
					case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
					case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
					case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
					case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
					case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
					k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
				};

				//----------
				// finalization

				h1 ^= len; h2 ^= len;

				h1 += h2;
				h2 += h1;

				h1 = fmix64(h1);
				h2 = fmix64(h2);

				h1 += h2;
				h2 += h1;
			}

			public:
			static uint32_t getDefaultSeed()
			{
				return 0xb979379eul;
			}

			static void wordsToDigest(uint64_t const h1, uint64_t const h2, uint8_t * d)
			{
				size_t o = 0;
				for ( unsigned int i = 0; i < sizeof(h1); ++i )
					d[o++] = (h1 >> (8*(sizeof(h1)-i-1))) & 0xFFu;
				for ( unsigned int i = 0; i < sizeof(h2); ++i )
					d[o++] = (h2 >> (8*(sizeof(h2)-i-1))) & 0xFFu;
			}

			MurmurHash3_x64_128()
			{

			}

			MurmurHash3_x64_128(uint32_t const seed)
			: h1(seed), h2(seed), f(0), numblocks(0)
			{
			}

			void init(uint32_t const seed = getDefaultSeed())
			{
				h1 = seed;
				h2 = seed;
				f = 0;
				numblocks = 0;
			}

			void update(uint8_t const * p, size_t l)
			{
				// while we have data and are not block aligned
				while ( l && f )
				{
					// space left
					size_t space = sizeof(B)/sizeof(B[0]) - f;
					// number of octets to copy
					size_t tocopy = std::min(l,space);
					// copy
					std::copy(p,p+tocopy,&B[0]+f);

					// update
					f += tocopy;
					p += tocopy;
					l -= tocopy;

					// handle block if complete
					if ( f == sizeof(B)/sizeof(B[0]) )
					{
						uint64_t const c1 = 0x87c37b91114253d5LLU;
						uint64_t const c2 = 0x4cf5ad432745937fLLU;

						uint64_t k1 = getblock64(reinterpret_cast<uint64_t const *>(&B[0]),0);
						uint64_t k2 = getblock64(reinterpret_cast<uint64_t const *>(&B[0]),1);

						k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
						h1 = rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

						k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;
						h2 = rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;

						numblocks += 1;
						f = 0;
					}
				}

				assert ( (!f) || (!l) );

				while ( l >= 16 )
				{
					assert ( ! f );

					uint64_t const c1 = 0x87c37b91114253d5LLU;
					uint64_t const c2 = 0x4cf5ad432745937fLLU;

					uint64_t k1 = getblock64(reinterpret_cast<uint64_t const *>(p),0);
					uint64_t k2 = getblock64(reinterpret_cast<uint64_t const *>(p),1);

					k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
					h1 = rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

					k2 *= c2; k2  = rotl64(k2,33); k2 *= c1; h2 ^= k2;
					h2 = rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;

					numblocks += 1;

					p += 16;
					l -= 16;
				}

				if ( l )
				{
					assert ( l < 16 );
					assert ( ! f );
					std::copy(p,p+l,&B[0]);

					f += l;
					p += l;
					l -= l;
				}

				assert ( ! l );
			}

			void digest(uint8_t * d)
			{
				finish();
				wordsToDigest(h1,h2,d);
			}

			void copyFrom(MurmurHash3_x64_128 const & O)
			{
				h1 = O.h1;
				h2 = O.h2;
				std::copy(&(O.B[0]),&(O.B[sizeof(B)/sizeof(B[0])]),&B[0]);
				f = O.f;
				numblocks = O.numblocks;
			}

			static size_t getDigestLength() { return digestlength; }

			void vinit()
			{
				init();
			}

			void vupdate(uint8_t const * u, size_t l)
			{
				update(u,l);
			}
		};
	}
}
#endif
