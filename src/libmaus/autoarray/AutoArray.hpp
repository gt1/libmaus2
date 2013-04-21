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


#if ! defined(LIBMAUS_AUTOARRAY_AUTOARRAY_HPP)
#define LIBMAUS_AUTOARRAY_AUTOARRAY_HPP

#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <libmaus/serialize/Serialize.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <libmaus/util/Demangle.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/math/gcd.hpp>
#include <sstream>
#include <fstream>
#include <limits>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <libmaus/util/I386CacheLineSize.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>

#if defined(LIBMAUS_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(__FreeBSD__)
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#if defined(LIBMAUS_HAVE_WINDOWS_H)
#include <windows.h>
#endif

// #define AUTOARRAY_DEBUG

#if defined(AUTOARRAY_DEBUG)
#include <iostream>
#endif

namespace libmaus
{
	namespace autoarray
	{
		extern uint64_t AutoArray_memusage;
		extern uint64_t AutoArray_peakmemusage;
		extern uint64_t AutoArray_maxmem;
		#if defined(_OPENMP)
		extern ::libmaus::parallel::OMPLock AutoArray_lock;
		#endif

		enum alloc_type { alloc_type_cxx, alloc_type_c, alloc_type_memalign_cacheline };
		
		struct AutoArrayMemUsage
		{
			uint64_t memusage;
			uint64_t peakmemusage;
			uint64_t maxmem;
			
			AutoArrayMemUsage()
			: memusage(AutoArray_memusage), peakmemusage(AutoArray_peakmemusage),
			  maxmem(AutoArray_maxmem)
			{
			}
			AutoArrayMemUsage(AutoArrayMemUsage const & o)
			: memusage(o.memusage), peakmemusage(o.peakmemusage), maxmem(o.maxmem)
			{
			}
			
			AutoArrayMemUsage & operator=(AutoArrayMemUsage const & o)
			{
				memusage = o.memusage;
				peakmemusage = o.peakmemusage;
				maxmem = o.maxmem;
				return *this;
			}
			
			bool operator==(AutoArrayMemUsage const & o) const
			{
				return
					memusage == o.memusage
					&&
					peakmemusage == o.peakmemusage
					&&
					maxmem == o.maxmem;
			}
			bool operator!=(AutoArrayMemUsage const & o) const
			{
				return ! ((*this)==o);
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, AutoArrayMemUsage const & aamu)
		{
			#if defined(_OPENMP)
			AutoArray_lock.lock();
			#endif
			out << "AutoArrayMemUsage("
				"memusage=" << static_cast<double>(aamu.memusage)/(1024.0*1024.0) << "," <<
				"peakmemusage=" << static_cast<double>(aamu.peakmemusage)/(1024.0*1024.0) << "," <<
				"maxmem=" << static_cast<double>(aamu.maxmem)/(1024.0*1024.0) << ")";
			#if defined(_OPENMP)
			AutoArray_lock.unlock();
			#endif			
			return out;
		}

		template<typename N, alloc_type atype>
		struct AlignedAllocation
		{
			static N * alignedAllocate(uint64_t const n, uint64_t const)
			{
				return new N[n];
			}
			static void freeAligned(N * alignedp)
			{
				delete [] alignedp;
			}			
		};

		template<typename N>
		struct AlignedAllocation<N,alloc_type_memalign_cacheline>
		{
			static N * alignedAllocate(uint64_t const n, uint64_t const align)
			{
				uint64_t const allocbytes = (sizeof(N *) - 1) + align  + n * sizeof(N);
				uint8_t * const allocp = new uint8_t[ allocbytes ];
				uint8_t * const alloce = allocp + allocbytes;
				uint64_t const mod = reinterpret_cast<uint64_t>(allocp) % align;

				uint8_t * alignedp = mod ? (allocp + (align-mod)) : allocp;
				assert ( reinterpret_cast<uint64_t>(alignedp) % align == 0 );

				while ( (alignedp - allocp) < static_cast<ptrdiff_t>(sizeof(N *)) )
					alignedp += align;

				assert ( reinterpret_cast<uint64_t>(alignedp) % align == 0 );
				assert ( alloce - alignedp >= static_cast<ptrdiff_t>(n*sizeof(N)) );
				assert ( alignedp-allocp >= static_cast<ptrdiff_t>(sizeof(uint8_t *)) );
				
				(reinterpret_cast<uint8_t **>(alignedp))[-1] = allocp;
				
				return reinterpret_cast<N *>(alignedp);
			}
			
			static void freeAligned(N * alignedp)
			{
				delete [] ((uint8_t **)((alignedp)))[-1];
			}
		};

		template<typename N>
		struct ArrayErase
		{
			static void erase(N * array, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					array[i] = N();
			}
		};

		#if defined(LIBMAUS_USE_STD_UNIQUE_PTR)
		template<typename N>
		struct ArrayErase< std::unique_ptr<N> >
		{
			static void erase(std::unique_ptr<N> * array, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					array[i] = UNIQUE_PTR_MOVE(std::unique_ptr<N>());
			}			
		};
		#endif
		
		#if defined(LIBMAUS_USE_BOOST_UNIQUE_PTR)
		template<typename N>
		struct ArrayErase< typename ::boost::interprocess::unique_ptr<N,::libmaus::deleter::Deleter<N> > >
		{
			static void erase(typename ::libmaus::util::unique_ptr<N>::type * array, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					array[i] = UNIQUE_PTR_MOVE(typename ::libmaus::util::unique_ptr<N>::type());
			}
		};
		#endif

		/**
		 * automatic array
		 */
		template<typename N, alloc_type atype = alloc_type_cxx>
		struct AutoArray
		{
			public:
			typedef N value_type;
			typedef value_type symbol_type;
			typedef AutoArray<value_type,atype> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef N * iterator;
			typedef N const * const_iterator;

			private:
			mutable N * array;
			mutable uint64_t n;
						
			static void increaseTotalAllocation(uint64_t n)
			{
				#if defined(_OPENMP)
				AutoArray_lock.lock();
				#endif
				
				if ( AutoArray_memusage + n * sizeof(N) > AutoArray_maxmem )
				{
					#if defined(_OPENMP)
					AutoArray_lock.unlock();
					#endif	
					
					::libmaus::exception::LibMausException se;
					se.getStream() << "bad allocation: AutoArray mem limit of " << AutoArray_maxmem << " bytes exceeded by new allocation of " << n*sizeof(N) << " bytes.";
					se.finish();
					throw se;
				}
				
				AutoArray_memusage += n * sizeof(N);
				AutoArray_peakmemusage = std::max(AutoArray_peakmemusage, AutoArray_memusage);
				
				#if defined(_OPENMP)
				AutoArray_lock.unlock();
				#endif
			}
			static void decreaseTotalAllocation(uint64_t n)
			{
				#if defined(_OPENMP)
				AutoArray_lock.lock();
				#endif
				
				AutoArray_memusage -= n * sizeof(N);
				
				#if defined(_OPENMP)
				AutoArray_lock.unlock();
				#endif
			}
			
			public:
			
			N * begin()
			{
				return get();
			}
			N * end()
			{
				return get()+size();
			}
			N const * begin() const
			{
				return get();
			}
			N const * end() const
			{
				return get()+size();
			}
			
			this_type inverse() const
			{
				this_type I(n,false);
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
					I [ array[i] ] = i;
					
				return I;
			}
			
			static std::string getValueTypeName()
			{
				return ::libmaus::util::Demangle::demangle<value_type>();
			}
			static std::string getAllocTypeName()
			{
				switch ( atype )
				{
					case alloc_type_cxx:
						return "alloc_type_cxx";
					case alloc_type_c:
						return "alloc_type_c";
					case alloc_type_memalign_cacheline:
						return "alloc_type_memalign_cacheline";
				}
			}
			static std::string getTypeName()
			{
				return std::string("AutoArray<") + getValueTypeName() + "," + getAllocTypeName() + ">";
			}
			
			uint64_t prefixSums()
			{
				N c = N();
				
				for ( uint64_t i = 0; i < getN(); ++i )
				{
					N const t = (*this)[i];
					(*this)[i] = c;
					c += t;
				}
				
				return c;
			}

			uint64_t serialize(std::ostream & out, bool writeHeader= true) const
			{
				uint64_t s = 0;
				if ( writeHeader )
					s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				s += ::libmaus::serialize::Serialize<N>::serializeArray(out,array,n);
				return s;
			}
			
			uint64_t serialize(std::string const & filename, bool writeHeader = true) const
			{
				std::ofstream ostr(filename.c_str(),std::ios::binary);
				uint64_t const s = serialize(ostr,writeHeader);
				ostr.flush();
				if ( ! ostr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to serialize " << getTypeName() << " of size " << size() << " to file " << filename << std::endl;
					se.finish();
					throw se;
				}
				ostr.close();
				if ( ! ostr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to serialize " << getTypeName() << " of size " << size() << " to file " << filename << std::endl;
					se.finish();
					throw se;
				}
				return s;
			}
			
			uint64_t serializePrefix(std::ostream & out, uint64_t const tn) const
			{
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,tn);
				s += ::libmaus::serialize::Serialize<N>::serializeArray(out,array,tn);
				return s;
			}

			static uint64_t getWordMod()
			{
				return 1;
			}
			
			#if defined(_WIN32)
			static uint64_t getpagesize() 
			{
				SYSTEM_INFO system_info;
			        GetSystemInfo (&system_info);
			        return system_info.dwPageSize;
			}
			#else
			static uint64_t getpagesize()
			{
				return ::getpagesize();
			}
			#endif
			
			uint64_t getPageMod() const
			{
				return ::libmaus::math::lcm(getpagesize(),sizeof(uint64_t)) / sizeof(uint64_t);
			}

			
			void resize(uint64_t const rn)
			{
				switch ( atype )
				{
					case alloc_type_c:
					{
						N * newarray = reinterpret_cast<N *>(realloc(array, rn * sizeof(N) ));
						if ( (! newarray) && (rn != 0) )
						{
							bool topfailed = false;
							if ( system("top -b -n1") < 0 )
							{
								topfailed = true;
							}
							::libmaus::exception::LibMausException se;
							se.getStream() 
								<< getTypeName()
								<< "::resize() failed to allocate " << rn << " elements ("
								<< rn*sizeof(N) << " bytes)" << "\n"
								<< "current total allocation " << AutoArray_memusage
								<< (topfailed ? " (system(top -b -n1) failed)" : "") << std::endl;
								;
							se.finish();
							throw se;
						}
						decreaseTotalAllocation(n);
						n = rn;
						increaseTotalAllocation(n);
						array = newarray;
						break;
					}
					case alloc_type_cxx:
					case alloc_type_memalign_cacheline:
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << getTypeName() << "::resize() not supported for allocation type " << getAllocTypeName() << std::endl;
							se.finish();
							throw se;
						}
						break;
				}				
			}
			
			#if defined(__linux__)
			static uint64_t getCacheLineSize()
			{
				uint64_t sclinesize = sysconf (_SC_LEVEL1_DCACHE_LINESIZE);

				if ( sclinesize )
					return sclinesize;
				
				#if defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)				
				return ::libmaus::util::I386CacheLineSize::getCacheLineSize();				
				#else
				return 0;
				#endif
			}
			#elif defined(_WIN32)
			static uint64_t getCacheLineSize() 
			{
				uint64_t cachelinesize = 0;
				DWORD bufsize = 0;
				GetLogicalProcessorInformation(0, &bufsize);
				::libmaus::autoarray::AutoArray<uint8_t> Abuffer(bufsize);
	                        SYSTEM_LOGICAL_PROCESSOR_INFORMATION * const buffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION *>(Abuffer.get());
	                        GetLogicalProcessorInformation(&buffer[0], &bufsize);
	                            
	                        for (uint64_t i = 0; i != bufsize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) 
	                        {
	                        	if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
	                                	cachelinesize = buffer[i].Cache.LineSize;
	                                	break;
					}
				}
	                                                                            
				return cachelinesize;
			}
			#elif defined(__APPLE__)
			static uint64_t getCacheLineSize()
			{
				uint64_t cachelinesize = 0;
				size_t cachelinesizelen = sizeof(cachelinesize);
				int const sysctlretname = sysctlbyname("hw.cachelinesize", &cachelinesize, &cachelinesizelen, 0, 0);
				assert ( ! sysctlretname );
				return cachelinesize;
			}
			#elif defined(LIBMAUS_USE_ASSEMBLY) && defined(LIBMAUS_HAVE_i386)
			static uint64_t getCacheLineSize() 
			{
				return ::libmaus::util::I386CacheLineSize::getCacheLineSize();
			}
			#endif
			

			/**
			 * release memory (set size of array to zero)
			 **/
			void release()
			{
				decreaseTotalAllocation(n);

				switch ( atype )
				{
					case alloc_type_cxx:
						delete [] array;
						break;
					case alloc_type_c:
						free(array);
						break;
					case alloc_type_memalign_cacheline:
						#if defined(_WIN32) || defined(LIBMAUS_HAVE_POSIX_MEMALIGN)
						free(array);
						#else
						AlignedAllocation<N,alloc_type_memalign_cacheline>::freeAligned(array);
						#endif
						break;
				}
				
				array = 0;
				n = 0;
			}
			
			void allocateArray(uint64_t const n)
			{	
				try
				{
					switch ( atype )
					{
						case alloc_type_cxx:
							array = new N[n];
							break;
						case alloc_type_c:
							array = reinterpret_cast<N *>(malloc( n * sizeof(N) ));
							if ( ! array )
								throw std::bad_alloc();
							break;
						case alloc_type_memalign_cacheline:
						{
							uint64_t const cachelinesize = getCacheLineSize();
							#if defined(_WIN32)
							array = reinterpret_cast<N *>(_aligned_malloc( n * sizeof(N), cachelinesize ));
							if ( ! array )
								throw std::bad_alloc();
							#elif defined(LIBMAUS_HAVE_POSIX_MEMALIGN)
							int r = posix_memalign(reinterpret_cast<void**>(&array),cachelinesize,n * sizeof(N) );
							if ( r )
							{
								int const aerrno = r;
								std::cerr << "allocation failure: " << strerror(aerrno) << " cachelinesize=" << cachelinesize << " requested size is " << n*sizeof(N) << std::endl;
								throw std::bad_alloc();
							}
							#else
							array = AlignedAllocation<N,atype>::alignedAllocate(n,cachelinesize);
							#endif
							break;
						}
					}
				}
				catch(std::bad_alloc const & ex)
				{
					bool topfailed = false;
					if ( system("top -b -n1") < 0 )
					{
						topfailed = true;
					}
					::libmaus::exception::LibMausException se;
					se.getStream() 
						<< getTypeName()
						<< " failed to allocate " << n << " elements ("
						<< n*sizeof(N) << " bytes)" << "\n"
						<< "current total allocation " << AutoArray_memusage
						<< (topfailed ? " (system(top -b -n1) failed)" : "") << std::endl;
						;
					se.finish();
					throw se;
				}
			}
			
			uint64_t deserialize(std::istream & in)
			{
				release();
				uint64_t s = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				increaseTotalAllocation(n);
				allocateArray(n);
				s += ::libmaus::serialize::Serialize<N>::deserializeArray(in,array,n);
				return s;
			}
			static uint64_t ignore(std::istream & in)
			{
				uint64_t s = 0;
				uint64_t n;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				s += ::libmaus::serialize::Serialize<N>::ignoreArray(in,n);
				return s;
			}
			static uint64_t skip(std::istream & in)
			{
				uint64_t s = 0;
				uint64_t n;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				s += ::libmaus::serialize::Serialize<N>::skipArray(in,n);
				return s;
			}
			static uint64_t copy(std::istream & in, std::ostream & out)
			{
				uint64_t s = 0;
				uint64_t n;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);

				uint64_t const bufferbytes = 16*1024;
				uint64_t const tocopy = n * sizeof(N);
				uint64_t const fullbuffers = tocopy / bufferbytes;
				AutoArray<char> buffer(bufferbytes,false);
				
				for ( uint64_t i = 0; i < fullbuffers; ++i )
				{
					in.read ( buffer.get(), bufferbytes );
					assert ( in );
					assert ( in.gcount() == static_cast<int64_t>(bufferbytes) );
					out.write ( buffer.get(), bufferbytes );
					assert ( out );
					
					s += bufferbytes;
				}
				
				uint64_t const restbytes = tocopy % bufferbytes;
				
				if ( restbytes )
				{
					in.read ( buffer.get(), restbytes );
					assert ( in );
					assert ( in.gcount() == static_cast<int64_t>(restbytes) );
					out.write ( buffer.get(), restbytes );
					assert ( out );
					
					s += restbytes;
				}
				
				return s;
			}
			static uint64_t sampleCopy(std::istream & in, std::ostream & out, uint64_t const rate)
			{
				assert ( rate != 0 );
			
				uint64_t s = 0;

				uint64_t in_n;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&in_n);
				assert ( in );

				uint64_t const out_n = ( in_n + rate - 1 ) / rate;
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,out_n);
				
				uint64_t const maxbufferbytes = 16*1024;
				
				uint64_t const bufferelements =  
					std::max ( 
						static_cast<uint64_t>(( 
							maxbufferbytes / sizeof(N) 
						) / rate)
							, static_cast<uint64_t>(1ull)
					) * rate;
				assert ( bufferelements % rate == 0 );
				uint64_t const bufferbytes = bufferelements * sizeof(N);
				
				uint64_t const fullbuffers = in_n / bufferelements;
				AutoArray < N > buffer ( bufferelements , false );
				
				for ( uint64_t i = 0; i < fullbuffers; ++i )
				{
					in.read ( reinterpret_cast<char *>(buffer.get()), bufferbytes );
					assert ( in );
					assert ( in.gcount() == static_cast<int64_t>(bufferbytes) );
					
					uint64_t inptr = 0;
					uint64_t outptr = 0;
					for ( ; inptr != bufferelements; inptr += rate, outptr += 1 )
						buffer [ outptr ] = buffer[ inptr ];
						
					out.write ( 
						reinterpret_cast<char const *>(buffer.get()),
						bufferbytes / rate );
					assert ( out );
					
					s +=  bufferbytes / rate;					
				}
				
				uint64_t const restelements = in_n - fullbuffers * bufferelements;
				assert ( restelements < bufferelements );
				uint64_t const restbytes = restelements * sizeof(N);
				
				in.read ( reinterpret_cast<char *>(buffer.get()), restbytes );
				assert ( in );
				assert ( in.gcount() == static_cast<int64_t>(restbytes) );
				
				uint64_t const restelementsout = ( restelements + rate - 1 ) / rate;
				
				uint64_t inptr = 0;
				uint64_t outptr = 0;
				
				for ( ; inptr < restelements; inptr += rate, outptr += 1 )
					buffer[ outptr ] = buffer[ inptr ];
				
				out.write (
					reinterpret_cast<char const *>(buffer.get()),
						restelementsout * sizeof(N) );
				assert ( out );
				
				return s;
			}

			static uint64_t byteSize(uint64_t const n)
			{
				return sizeof(N*) + sizeof(uint64_t) + n*sizeof(N);
			}
		

			public:
			AutoArray<N,atype> clone() const
			{
				AutoArray<N,atype> O(n,false);
				for ( uint64_t i = 0; i < n; ++i )
					O[i] = array[i];
				return O;
			}
			
			/**
			 * @return estimated space in bytes
			 **/
			uint64_t byteSize() const
			{
				return  sizeof(uint64_t) + sizeof(N *) + n * sizeof(N);
			}
			/**
			 * @return number of elements in array
			 **/
			uint64_t getN() const
			{
				return n;
			}
			/**
			 * @return number of elements in array
			 **/
			uint64_t size() const
			{
				return n;
			}
			
			// #define AUTOARRAY_DEBUG
		
			/**
			 * constructor for empty array
			 **/
			AutoArray() : array(0), n(0) 
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(), " << this << std::endl;
				#endif
			}
			/**
			 * copy constructor. retrieves array from o and invalidates o.
			 * @param o
			 **/
			AutoArray(AutoArray<N,atype> const & o) : array(0) 
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(AutoArray &), " << this << std::endl;
				#endif
				
				array = o.array;
				n = o.n;
				o.array = 0;
				o.n = 0;

			}
			#if 1
			/**
			 * copy constructor
			 **/
			AutoArray(uint64_t rn, N const * D) : array(0), n(rn) 
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(uint64_t, value_type const *), " << this << std::endl;
				#endif

				increaseTotalAllocation(n); 
				allocateArray(n);
				
				for ( uint64_t i = 0; i < n; ++i )
					array[i] = D[i];

			}
			#endif
			/**
			 * allocates an array with rn elements
			 * @param rn number of elements
			 * @param erase if true, elements will be assigned default value of type (i.e. 0 for numbers)
			 **/
			AutoArray(uint64_t rn, bool erase = true) : array(0), n(rn) 
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(uint64_t, bool), " << this << std::endl;
				#endif
				
				increaseTotalAllocation(n); 
				allocateArray(n);
			
				if ( erase )
					ArrayErase<N>::erase(array,n);
					#if 0
					for ( uint64_t i = 0; i < n; ++i )
						array[i] = N();
					#endif

			}

			static uint64_t readNumber(std::istream & in, uint64_t & s)
			{
				uint64_t n = 0;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&n);
				return n;
			}
			static uint64_t readNumber(std::istream & in)
			{
				uint64_t s = 0;
				return readNumber(in,s);
			}

			AutoArray(std::istream & in, uint64_t & s)
			: n(readNumber(in,s))
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(std::istream,uint64_t &), " << this << std::endl;
				#endif
				
				increaseTotalAllocation(n);
				allocateArray(n);
				s += ::libmaus::serialize::Serialize<N>::deserializeArray(in,array,n);
			}
			AutoArray(std::istream & in)
			: n(readNumber(in))
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(std::istream), " << this << std::endl;
				#endif

				increaseTotalAllocation(n);
				allocateArray(n);
				::libmaus::serialize::Serialize<N>::deserializeArray(in,array,n);
			}

			/**
			 * destructor freeing resources.
			 **/
			~AutoArray() 
			{
				release();
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << "~" << getTypeName() << "(), " << this << std::endl;
				#endif
			}
			
			/**
			 * retrieve pointer
			 * @return pointer to array
			 **/
			N * get() { return array; }
			/**
			 * retrieve const pointer
			 * @return const pointer to array
			 **/
			N const * get() const { return array; }
			
			/**
			 * retrieve reference to i'th element
			 * @param i
			 * @return reference to i'th element
			 **/
			N       & operator[](uint64_t i)       { return array[i]; }
			/**
			 * retrieve const reference to i'th element
			 * @param i
			 * @return const reference to i'th element
			 **/
			N const & operator[](uint64_t i) const { return array[i]; }
			/**
			 * retrieve reference to i'th element
			 * @param i
			 * @return reference to i'th element
			 **/
			N       & get(uint64_t i)       { return array[i]; }
			/**
			 * retrieve const reference to i'th element
			 * @param i
			 * @return const reference to i'th element
			 **/
			N const & get(uint64_t i) const { return array[i]; }
		
			/**
			 * assignment. retrieves array from o and invalidates o
			 * @param o
			 * @return this array
			 **/
			AutoArray<N,atype> & operator=(AutoArray<N,atype> const & o)
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << this << "," << getTypeName() << "::operator=(" << &o << ")" << std::endl;;
				#endif
				
				if ( this != &o )
				{
					release();
					this->array = o.array;
					this->n = o.n;
					o.array = 0;
					o.n = 0;
				}
				return *this;
			}
			
			N rnvGeneric(uint64_t const l, uint64_t const r, N const k) const
			{
				N v = std::numeric_limits<N>::max();
				
				for ( uint64_t i = l; i < r; ++i )
					if ( 
						(*this)[i] >= k
						&&
						(*this)[i] < v
					)
						v = (*this)[i];
				
				return v;
			}
			uint64_t rank(N sym, uint64_t i) const
			{
				uint64_t c = 0;
				for ( uint64_t j = 0; j <= i; ++j )
					if ( (*this)[j] == sym )
						c++;
				return c;
			}
			uint64_t select(N sym, uint64_t i) const
			{
				uint64_t j = 0;
				
				while ( j < size() && (*this)[j] != sym )
					++j;
				
				while ( j < size() )
				{
					assert ( (*this)[j] == sym );
					if ( ! i )
						return j;
					i--;

					while ( j < size() && (*this)[j] != sym )
						++j;
				}
				
				return std::numeric_limits<uint64_t>::max();
			}
			
			static AutoArray<N> readFilePortion(
				std::string const & filename,
				uint64_t const offset,
				uint64_t const length)
			{
				::libmaus::aio::CheckedInputStream CIS(filename);
				CIS.seekg(offset*sizeof(N));
				AutoArray<N> A(length,false);
				CIS.read ( reinterpret_cast<char *>(A.begin()), length*sizeof(N));
				return A;
			}
			
			static AutoArray<N> readFile(std::string const & filename)
			{
				::libmaus::aio::CheckedInputStream CIS(filename);
				CIS.seekg(0, std::ios::end);
				uint64_t const filesize = CIS.tellg();
				
				if ( filesize % sizeof(N) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "AutoArray::readFile(" << filename << "): file size " << filesize << " is not a multiple of " << sizeof(N) << std::endl;
					se.finish();
					throw se;
				}
				
				uint64_t const length = filesize/sizeof(N);
				CIS.seekg(0, std::ios::beg);
				
				AutoArray<N> A(length,false);
				CIS.read ( reinterpret_cast<char *>(A.begin()), length*sizeof(N));
				return A;
			}
		};

		template<typename N, alloc_type atype>
		bool operator==(AutoArray<N,atype> const & A, AutoArray<N,atype> const & B)
		{
			if ( A.getN() != B.getN() )
				return false;
			
			uint64_t const n = A.getN();
			
			for ( uint64_t i = 0; i < n; ++i )
				if ( A[i] != B[i] )
					return false;
			
			return true;
		}
	}
}
#endif
