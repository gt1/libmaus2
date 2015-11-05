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

#if ! defined(COMPACTARRAY_HPP)
#define COMPACTARRAY_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/CompactArrayBase.hpp>
#include <libmaus2/util/iterator.hpp>

namespace libmaus2
{
	namespace bitio
	{
		template<bool _synchronous>
		struct CompactArrayTemplate : public CompactArrayBase
		{
			static bool const synchronous = _synchronous;

			typedef CompactArrayTemplate<synchronous> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef uint64_t value_type;

			typedef typename ::libmaus2::util::AssignmentProxy<CompactArrayTemplate<synchronous>,value_type> proxy_type;
			typedef typename ::libmaus2::util::AssignmentProxyIterator<CompactArrayTemplate<synchronous>,value_type> iterator;
			typedef typename ::libmaus2::util::ConstIterator<CompactArrayTemplate<synchronous>,value_type> const_iterator;

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			const_iterator end() const
			{
				return const_iterator(this,size());
			}
			iterator begin()
			{
				return iterator(this,0);
			}
			iterator end()
			{
				return iterator(this,size());
			}

			uint64_t n;
			uint64_t s; // number of words

			::libmaus2::autoarray::AutoArray<uint64_t, ::libmaus2::autoarray::alloc_type_c> AD;
			uint64_t * D;

			uint64_t size() const
			{
				return n;
			}
			void resize(uint64_t rn)
			{
				uint64_t const ns = (rn*b+63)/64;
				AD.resize(ns);
				n = rn;
				s = ns;
				D = AD.get();
			}

			void erase()
			{
				for ( uint64_t i = 0; i < s; ++i )
					D[i] = 0;
			}

			// get least i such that i*b is a multiple of 64
			uint64_t getWordMod() const
			{
				return ::libmaus2::math::lcm(b,64) / b;
			}
			uint64_t getPageMod() const
			{
				assert ( (getWordMod() * b) % 64 == 0 );
				return ::libmaus2::math::lcm ( (getWordMod() * b) /* bits */, ::libmaus2::autoarray::AutoArray<uint8_t>::getpagesize()*8 /* bits */ ) / b;
			}

			static std::pair<uint64_t,uint64_t> getParamPair(std::string const & filename, uint64_t const offset)
			{
				libmaus2::aio::InputStreamInstance istr(filename);

				istr.seekg ( offset, std::ios::beg );

				uint64_t const b = deserializeNumber(istr);
				uint64_t const n = deserializeNumber(istr);

				return std::pair<uint64_t,uint64_t>(b,n);
			}

			static unique_ptr_type concat(
				std::string const & filename,
				::libmaus2::autoarray::AutoArray<uint64_t> const & offsets,
				uint64_t const li, uint64_t const ri
			)
			{
				unique_ptr_type ptr;

				if ( ri != li )
				{
					assert ( ri > li );

					uint64_t const b = getParamPair(filename,offsets[0]).first;
					uint64_t n = 0;

					for ( uint64_t i = li; i < ri; ++i )
					{
						std::pair<uint64_t,uint64_t> const p = getParamPair(filename,offsets[i]);
						assert ( b == p.first );
						n += p.second;
					}

					ptr = unique_ptr_type(new this_type(n,b));

					uint64_t totalbits = 0;

					for ( uint64_t i = li; i < ri; ++i )
					{
						libmaus2::aio::InputStreamInstance istr(filename);
						istr.seekg ( offsets[i], std::ios::beg );

						uint64_t const b = deserializeNumber(istr);
						uint64_t const n = deserializeNumber(istr);
						uint64_t const s = deserializeNumber(istr);
						uint64_t const as = deserializeNumber(istr);
						assert ( s == as );

						uint64_t const blocksize = 16*1024;
						uint64_t wordsleft = (n*b+63)/64;
						::libmaus2::autoarray::AutoArray<uint64_t> B(blocksize,false);

						unsigned int const mod = totalbits % 64;
						unsigned int const mod64 = 64-mod;
						uint64_t * D = ptr->D + (totalbits/64);

						if ( !mod )
						{
							while ( wordsleft )
							{
								uint64_t const wordstoread = std::min(wordsleft,blocksize);
								uint64_t const bytestoread = wordstoread*sizeof(uint64_t);

								istr.read ( reinterpret_cast<char *>(B.get()), bytestoread );
								assert ( istr );
								assert ( istr.gcount() == static_cast<int64_t>(bytestoread) );

								for ( uint64_t z = 0; z < wordstoread; ++z )
									*(D++) = B[z];

								wordsleft -= wordstoread;
							}
						}
						else
						{
							while ( wordsleft )
							{
								uint64_t const wordstoread = std::min(wordsleft,blocksize);
								uint64_t const bytestoread = wordstoread*sizeof(uint64_t);

								istr.read ( reinterpret_cast<char *>(B.get()), bytestoread );
								assert ( istr );
								assert ( istr.gcount() == static_cast<int64_t>(bytestoread) );

								for ( uint64_t z = 0; z < wordstoread; ++z )
								{
									*(D++) |= (B[z] >> mod);
									*D |= B[z] << (mod64);
								}

								wordsleft -= wordstoread;
							}
						}

						totalbits += n*b;
					}
				}

				return ptr;
			}

			static unique_ptr_type concat(
				std::vector < std::string > const & filenames,
				uint64_t const li, uint64_t const ri
			)
			{
				unique_ptr_type ptr;

				if ( ri != li )
				{
					assert ( ri > li );

					uint64_t const b = getParamPair(filenames[li],0).first;
					uint64_t n = 0;
					uint64_t minn = std::numeric_limits<uint64_t>::max();

					::libmaus2::autoarray::AutoArray<uint64_t> bitoffsets(filenames.size());
					;
					for ( uint64_t i = li; i < ri; ++i )
					{
						bitoffsets[i-li] = n*b;
						std::pair<uint64_t,uint64_t> const p = getParamPair(filenames[i],0);
						assert ( b == p.first );
						minn = std::min(minn,p.second);
						n += p.second;
					}

					if ( minn < 1024 )
						return concatSlow(filenames,li,ri);

					ptr = unique_ptr_type(new this_type(n,b));

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1)
					#endif
					for ( int64_t i = li; i < static_cast<int64_t>(ri); i += 2 )
					{
						libmaus2::aio::InputStreamInstance istr(filenames[i]);

						uint64_t const b = deserializeNumber(istr);
						uint64_t const n = deserializeNumber(istr);
						uint64_t const s = deserializeNumber(istr);
						uint64_t const as = deserializeNumber(istr);
						assert ( s == as );

						uint64_t const blocksize = 16*1024;
						uint64_t wordsleft = (n*b+63)/64;
						::libmaus2::autoarray::AutoArray<uint64_t> B(blocksize,false);

						uint64_t const totalbits = bitoffsets[i-li];

						unsigned int const mod = totalbits % 64;
						unsigned int const mod64 = 64-mod;
						uint64_t * D = ptr->D + (totalbits/64);

						if ( !mod )
						{
							while ( wordsleft )
							{
								uint64_t const wordstoread = std::min(wordsleft,blocksize);
								uint64_t const bytestoread = wordstoread*sizeof(uint64_t);

								istr.read ( reinterpret_cast<char *>(B.get()), bytestoread );
								assert ( istr );
								assert ( istr.gcount() == static_cast<int64_t>(bytestoread) );

								for ( uint64_t z = 0; z < wordstoread; ++z )
									*(D++) |= B[z];

								wordsleft -= wordstoread;
							}
						}
						else
						{
							while ( wordsleft )
							{
								uint64_t const wordstoread = std::min(wordsleft,blocksize);
								uint64_t const bytestoread = wordstoread*sizeof(uint64_t);

								istr.read ( reinterpret_cast<char *>(B.get()), bytestoread );
								assert ( istr );
								assert ( istr.gcount() == static_cast<int64_t>(bytestoread) );

								for ( uint64_t z = 0; z < wordstoread; ++z )
								{
									*(D++) |= (B[z] >> mod);
									*D |= B[z] << (mod64);
								}

								wordsleft -= wordstoread;
							}
						}
					}
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1)
					#endif
					for ( int64_t i = li+1; i < static_cast<int64_t>(ri); i += 2 )
					{
						libmaus2::aio::InputStreamInstance istr(filenames[i]);

						uint64_t const b = deserializeNumber(istr);
						uint64_t const n = deserializeNumber(istr);
						uint64_t const s = deserializeNumber(istr);
						uint64_t const as = deserializeNumber(istr);
						assert ( s == as );

						uint64_t const blocksize = 16*1024;
						uint64_t wordsleft = (n*b+63)/64;
						::libmaus2::autoarray::AutoArray<uint64_t> B(blocksize,false);

						uint64_t const totalbits = bitoffsets[i-li];

						unsigned int const mod = totalbits % 64;
						unsigned int const mod64 = 64-mod;
						uint64_t * D = ptr->D + (totalbits/64);

						if ( !mod )
						{
							while ( wordsleft )
							{
								uint64_t const wordstoread = std::min(wordsleft,blocksize);
								uint64_t const bytestoread = wordstoread*sizeof(uint64_t);

								istr.read ( reinterpret_cast<char *>(B.get()), bytestoread );
								assert ( istr );
								assert ( istr.gcount() == static_cast<int64_t>(bytestoread) );

								for ( uint64_t z = 0; z < wordstoread; ++z )
									*(D++) |= B[z];

								wordsleft -= wordstoread;
							}
						}
						else
						{
							while ( wordsleft )
							{
								uint64_t const wordstoread = std::min(wordsleft,blocksize);
								uint64_t const bytestoread = wordstoread*sizeof(uint64_t);

								istr.read ( reinterpret_cast<char *>(B.get()), bytestoread );
								assert ( istr );
								assert ( istr.gcount() == static_cast<int64_t>(bytestoread) );

								for ( uint64_t z = 0; z < wordstoread; ++z )
								{
									*(D++) |= (B[z] >> mod);
									*D |= B[z] << (mod64);
								}

								wordsleft -= wordstoread;
							}
						}
					}
				}

				return ptr;
			}

			static unique_ptr_type concatSlow(std::string const & filename,
				::libmaus2::autoarray::AutoArray<uint64_t> const & offsets,
				uint64_t const li, uint64_t const ri)
			{
				unique_ptr_type ptr;

				if ( ri != li )
				{
					assert ( ri > li );

					uint64_t const b = getParamPair(filename,offsets[li]).first;
					uint64_t n = 0;

					for ( uint64_t i = li; i < ri; ++i )
					{
						std::pair<uint64_t,uint64_t> const p = getParamPair(filename,offsets[i]);
						assert ( b == p.first );
						n += p.second;
					}

					ptr = unique_ptr_type(new this_type(n,b));

					uint64_t p = 0;

					for ( uint64_t i = li; i < ri; ++i )
					{
						libmaus2::aio::InputStreamInstance istr(filename);
						istr.seekg ( offsets[i], std::ios::beg );

						CompactArrayTemplate<synchronous> C(istr);

						for ( uint64_t j = 0; j < C.n; ++j )
							ptr->set ( p++, C.get(j) );
					}
				}

				return ptr;
			}

			static unique_ptr_type concatSlow(std::vector<std::string> const & filenames,
				uint64_t const li, uint64_t const ri)
			{
				unique_ptr_type ptr;

				if ( ri != li )
				{
					assert ( ri > li );

					uint64_t const b = getParamPair(filenames[li],0).first;
					uint64_t n = 0;
					::libmaus2::autoarray::AutoArray<uint64_t> offsets(ri-li);

					for ( uint64_t i = li; i < ri; ++i )
					{
						offsets[i-li] = n;
						std::pair<uint64_t,uint64_t> const p = getParamPair(filenames[i],0);
						assert ( b == p.first );
						n += p.second;
					}

					ptr = unique_ptr_type(new this_type(n,b));


					for ( uint64_t i = li; i < ri; ++i )
					{
						uint64_t p = offsets[i-li];

						libmaus2::aio::InputStreamInstance istr(filenames[i]);

						CompactArrayTemplate<synchronous> C(istr);

						for ( uint64_t j = 0; j < C.n; ++j )
							ptr->set ( p++, C.get(j) );
					}
				}

				return ptr;
			}

			static unique_ptr_type concatOddEven(std::vector<std::string> const & filenames,
				uint64_t const li, uint64_t const ri)
			{
				unique_ptr_type ptr;

				if ( ri != li )
				{
					assert ( ri > li );

					uint64_t const b = getParamPair(filenames[li],0).first;
					uint64_t n = 0;
					::libmaus2::autoarray::AutoArray<uint64_t> offsets(ri-li);
					uint64_t minsize = std::numeric_limits<uint64_t>::max();

					for ( uint64_t i = li; i < ri; ++i )
					{
						offsets[i-li] = n;
						std::pair<uint64_t,uint64_t> const p = getParamPair(filenames[i],0);
						minsize = std::min(minsize,p.second);
						assert ( b == p.first );

						n += p.second;
					}

					if ( minsize < 1024 )
						return concatSlow(filenames,li,ri);

					ptr = unique_ptr_type(new this_type(n,b));

					std::cerr << "Reading parallel." << std::endl;

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1)
					#endif
					for ( int64_t i = li; i < static_cast<int64_t>(ri); i += 2 )
					{
						uint64_t p = offsets[i-li];

						libmaus2::aio::InputStreamInstance istr(filenames[i]);

						CompactArrayTemplate<synchronous> C(istr);

						for ( uint64_t j = 0; j < C.n; ++j )
							ptr->set ( p++, C.get(j) );
					}
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1)
					#endif
					for ( int64_t i = li+1; i < static_cast<int64_t>(ri); i += 2 )
					{
						uint64_t p = offsets[i-li];

						libmaus2::aio::InputStreamInstance istr(filenames[i]);

						CompactArrayTemplate<synchronous> C(istr);

						for ( uint64_t j = 0; j < C.n; ++j )
							ptr->set ( p++, C.get(j) );
					}
				}

				return ptr;
			}

			static uint64_t deserializeNumber(std::istream & in, uint64_t & t)
			{
				uint64_t n;
				t += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in, &n);
				return n;
			}
			static uint64_t deserializeNumber(std::istream & in)
			{
				uint64_t t;
				return deserializeNumber(in,t);
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c> deserializeArray(std::istream & in, uint64_t & t)
			{
				::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c> AD;
				t += AD.deserialize(in);
				return AD;
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c> deserializeArray(std::istream & in)
			{
				uint64_t t;
				return deserializeArray(in,t);
			}

			uint64_t serialize(std::ostream & out) const
			{
				uint64_t t = 0;
				t += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out, CompactArrayBase::b);
				t += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out, n);
				t += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out, s);
				t += AD.serialize(out);
				return t;
			}

			template<typename iterator>
			CompactArrayTemplate(iterator a, iterator e, uint64_t const rb, uint64_t const pad = 0)
			: CompactArrayBase(rb), n(e-a), s ( computeS(n,b) ), AD( allocate(n,b,pad) ), D(AD.get())
			{
				uint64_t i = 0;
				for ( ; a != e; ++a, ++i )
					set(i,*a);
			}
			CompactArrayTemplate(uint64_t const rn, uint64_t const rb, uint64_t const pad = 0)
			: CompactArrayBase(rb), n(rn), s ( computeS(n,b) ), AD( allocate(n,b,pad) ), D(AD.get())
			{
			}
			CompactArrayTemplate(uint64_t const rn, uint64_t const rb, uint64_t * const rD) : CompactArrayBase(rb), n(rn), s ( (n*b+63)/64 ), AD(), D(rD)
			{
			}
			CompactArrayTemplate(std::istream & in)
			: CompactArrayBase(deserializeNumber(in)), n(deserializeNumber(in)), s(deserializeNumber(in)),
			  AD(deserializeArray(in)), D(AD.get())
			{

			}
			CompactArrayTemplate(std::istream & in, uint64_t & t)
			: CompactArrayBase(deserializeNumber(in,t)), n(deserializeNumber(in,t)), s(deserializeNumber(in,t)),
			  AD(deserializeArray(in,t)), D(AD.get())
			{

			}

			uint64_t minparoffset() const
			{
				return ( b + quant() ) / b;
			}

			static uint64_t quant()
			{
				return 8 * sizeof(uint64_t);
			}

			uint64_t byteSize() const
			{
				return
					3*sizeof(uint64_t) + 1*sizeof(uint64_t *) + AD.byteSize();
			}

			static uint64_t computeS(uint64_t const n, uint64_t const b)
			{
				return (n*b+63)/64;
			}

			static ::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c> allocate(uint64_t const n, uint64_t const b, uint64_t const pad = 0)
			{
				return ::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c>( computeS(n,b) + pad );
			}

			uint64_t get(uint64_t const i) const
			{
				return getBits(i*b);
			}
			void set(uint64_t const i, uint64_t const v)
			{
				putBits(i*b, v);
			}

			uint64_t operator[](uint64_t const i) const
			{
				return get(i);
			}

			proxy_type operator[](uint64_t const i)
			{
				return proxy_type(this,i);
			}

			uint64_t postfixIncrement(uint64_t i) { uint64_t const v = get(i); set(i,v+1); return v; }

			unique_ptr_type clone() const
			{
				unique_ptr_type O( new CompactArrayTemplate<synchronous>(n,b) );
				for ( uint64_t i = 0; i < n; ++i )
					O->set( i, this->get(i) );
				return UNIQUE_PTR_MOVE(O);
			}


			// get bits from an array
			uint64_t getBits(uint64_t const offset) const
			{
				uint64_t const byteSkip = (offset >> bshf);
				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const restBits = bcnt-bitSkip;

				uint64_t const * DD = D + byteSkip;
				uint64_t v = *DD;

				// skip bits by masking them
				v &= getFirstMask[bitSkip];

				if ( b <= restBits )
				{
					return v >> (restBits - b);
				}
				else
				{
					unsigned int const numbits = b - restBits;

					v = (v<<numbits) | (( *(++DD) ) >> (bcnt-numbits));

					return v;
				}
			}

			void putBits(uint64_t const offset, uint64_t v)
			{
				#if 0
				if ( ! (( v & vmask ) == v) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "CompactArray::putBits(): value " << v << " out of range for bit width " << b << std::endl;
					se.finish();
					throw se;
				}
				#else
				assert ( ( v & vmask ) == v );
				#endif

				uint64_t * DD = D + (offset >> bshf);
				unsigned int const bitSkip = (offset & bmsk);
				unsigned int const bitsinfirstword = bitsInFirstWord[bitSkip];

				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				if ( synchronous )
				{
					__sync_fetch_and_and ( DD, firstKeepMask[bitSkip] );
					__sync_fetch_and_or ( DD, (v >> (b - bitsinfirstword)) << firstShift[bitSkip] );
				}
				else
				#endif
				{
					uint64_t t = *DD;
					t &= firstKeepMask[bitSkip];
					t |= (v >> (b - bitsinfirstword)) << firstShift[bitSkip];
					*DD = t;
				}

				if ( b - bitsinfirstword )
				{
					v &= firstValueKeepMask[bitSkip];
					DD++;

					#if defined(LIBMAUS2_HAVE_SYNC_OPS)
					if ( synchronous )
					{
						__sync_fetch_and_and (DD,lastMask[bitSkip]);
						__sync_fetch_and_or(DD,(v << lastShift[bitSkip]));
					}
					else
					#endif
					{
						uint64_t t = *DD;
						t &= lastMask[bitSkip];
						t |= (v << lastShift[bitSkip]);
						*DD = t;
					}
				}
			}
		};

		typedef CompactArrayTemplate<false> CompactArray;
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		typedef CompactArrayTemplate<true> SynchronousCompactArray;
		#endif
	}
}
#endif
