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

#if ! defined(LIBMAUS2_AUTOARRAY_AUTOARRAY_HPP)
#define LIBMAUS2_AUTOARRAY_AUTOARRAY_HPP

#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <libmaus2/serialize/Serialize.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/util/Demangle.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/math/gcd.hpp>
#include <sstream>
#include <fstream>
#include <limits>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <libmaus2/util/I386CacheLineSize.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>

// #define AUTOARRAY_TRACE 7
#if defined(AUTOARRAY_TRACE)
#include <execinfo.h>
#endif

#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#if defined(LIBMAUS2_HAVE_WINDOWS_H)
#include <windows.h>
#endif

// #define AUTOARRAY_DEBUG

#if defined(AUTOARRAY_DEBUG)
#include <iostream>
#endif

#if defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
#include <libmaus2/parallel/PosixSpinLock.hpp>
#endif

namespace libmaus2
{
	namespace autoarray
	{
		extern uint64_t volatile AutoArray_memusage;
		extern uint64_t volatile AutoArray_peakmemusage;
		extern uint64_t volatile AutoArray_maxmem;
		#if defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
		extern ::libmaus2::parallel::PosixSpinLock AutoArray_lock;
		#elif defined(_OPENMP)
		extern ::libmaus2::parallel::OMPLock AutoArray_lock;
		#endif

		enum alloc_type { alloc_type_cxx = 0, alloc_type_c = 1, alloc_type_memalign_cacheline = 2 };
		
		#if defined(AUTOARRAY_TRACE)
		template<unsigned int n>
		struct AutoArrayBackTrace
		{
			void * volatile P[n];
			unsigned int tracelength;
			uint64_t volatile alloccnt;
			uint64_t volatile freecnt;			
			uint64_t volatile allocbytes;
			uint64_t volatile freebytes;
			std::string type;
			
			AutoArrayBackTrace() : tracelength(0), alloccnt(0), freecnt(0), allocbytes(0), freebytes(0)
			{
				for ( unsigned int i = 0; i < n; ++i )
					P[i] = NULL;
			}
			
			bool operator<(AutoArrayBackTrace<n> const & O) const
			{
				return allocbytes-freebytes < O.allocbytes-O.freebytes;
			}
		};
		#endif

		/**
		 * class for storing AutoArray memory snapshot
		 **/		
		struct AutoArrayMemUsage
		{
			//! current AutoArray memory usage
			uint64_t memusage;
			//! observed peak of AutoArray memory usage
			uint64_t peakmemusage;
			//! maximum amount of memory that can be allocated through AutoArray objects
			uint64_t maxmem;
			
			/**
			 * constructor copying current values of AutoArray memory usage
			 **/
			AutoArrayMemUsage();

			/**
			 * copy constructor
			 * @param o object copied
			 **/
			AutoArrayMemUsage(AutoArrayMemUsage const & o);
			
			/**
			 * assignment operator
			 * @param o object copied
			 * @return *this
			 **/
			AutoArrayMemUsage & operator=(AutoArrayMemUsage const & o);
			
			/**
			 * return true iff *this == o
			 * @param o object to be compared
			 * @return true iff *this == o
			 **/
			bool operator==(AutoArrayMemUsage const & o) const;

			/**
			 * return true iff *this is different from o
			 * @param o object to be compared
			 * @return true iff *this != o
			 **/
			bool operator!=(AutoArrayMemUsage const & o) const;
		};

		/**
		 * class for the allocation of memory
		 **/
		template<typename N, alloc_type atype>
		struct AlignedAllocation
		{
			/**
			 * allocate n elements of type N
			 * @param n number of elements to allocate
			 * @return pointer to allocated memory
			 **/
			static N * alignedAllocate(uint64_t const n, uint64_t const);
			/**
			 * free memory allocate through alignedAllocate
			 * @param alignedp pointer to memory to be freed
			 **/
			static void freeAligned(N * alignedp);
		};

		/**
		 * class for the allocation of memory: special version for aligned allocation
		 **/
		template<typename N>
		struct AlignedAllocation<N,alloc_type_memalign_cacheline>
		{
			/**
			 * allocate n elements of type N aligned to an address aligned to a multiple of align
			 * @param n number of elements to allocate
			 * @param align requested multiplier for address
			 * @return pointer to allocated memory
			 **/
			static N * alignedAllocate(uint64_t const n, uint64_t const align);			
			/**
			 * free memory allocate through alignedAllocate
			 * @param alignedp pointer to memory to be freed
			 **/
			static void freeAligned(N * alignedp);
		};

		/**
		 * class for erasing an array of type N
		 **/
		template<typename N>
		struct ArrayErase
		{
			/**
			 * erase an array of n elements of type N
			 * @param array memory to be erased
			 * @param n number of elements in array
			 **/
			static void erase(N * array, uint64_t const n);
		};
		
		#if defined(LIBMAUS2_USE_STD_UNIQUE_PTR)
		/**
		 * class for erasing an array of std::unique_ptr objects
		 **/
		template<typename N>
		struct ArrayErase< std::unique_ptr<N> >
		{
			/**
			 * erase an array of n elements of type std::unique_ptr<N>
			 * @param array to be erased
			 * @param n number of elements in array
			 **/
			static void erase(std::unique_ptr<N> * array, uint64_t const n);
		};
		#endif
		#if defined(LIBMAUS2_USE_BOOST_UNIQUE_PTR)
		/**
		 * class for erasing an array of boost::interprocess::unique_ptr<N,::libmaus2::deleter::Deleter<N> > objects
		 **/
		template<typename N>
		struct ArrayErase< typename ::boost::interprocess::unique_ptr<N,::libmaus2::deleter::Deleter<N> > >
		{
			/**
			 * erase an array of n elements of type boost::interprocess::unique_ptr<N,::libmaus2::deleter::Deleter<N> >
			 * @param array to be erased
			 * @param n number of elements in array
			 **/
			static void erase(typename ::libmaus2::util::unique_ptr<N>::type * array, uint64_t const n);
		};
		#endif

		#if defined(AUTOARRAY_TRACE)
		extern std::vector< AutoArrayBackTrace<AUTOARRAY_TRACE> > tracevector;
		extern libmaus2::parallel::PosixSpinLock backtracelock;
		extern libmaus2::parallel::PosixSpinLock tracelock;
		
		extern void autoArrayPrintTraces(std::ostream & out);
		#endif

		/**
		 * array with automatic deallocation
		 */
		template<typename N, alloc_type atype = alloc_type_cxx, typename erase_type = ArrayErase<N> >
		struct AutoArray
		{
			public:
			//! element type
			typedef N value_type;
			//! symbol type equals element_type
			typedef value_type symbol_type;
			//! this type
			typedef AutoArray<value_type,atype,erase_type> this_type;
			//! unique pointer object for this type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer object for this type
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			//! iterator: pointer to N
			typedef N * iterator;
			//! const_iterator: constant pointer to N
			typedef N const * const_iterator;

			private:
			//! pointer to wrapped memory
			mutable N * array;
			//! number of elements in this array
			mutable uint64_t n;

			#if defined(AUTOARRAY_TRACE)
			void * trace[AUTOARRAY_TRACE];
			unsigned int tracelength;

			void fillTrace()
			{
				libmaus2::parallel::ScopePosixSpinLock slock(backtracelock);
				void * ltrace[AUTOARRAY_TRACE+2];

				tracelength = backtrace(&ltrace[0],AUTOARRAY_TRACE+2);
				
				if ( tracelength < 2 )
				{
					tracelength = 0;
					for ( unsigned int i = tracelength; i < AUTOARRAY_TRACE; ++i )
						trace[i] = NULL;
				}
				else
				{
					for ( unsigned int i = 2; i < AUTOARRAY_TRACE+2; ++i )
						trace[i-2] = ltrace[i];
					tracelength -= 2;

					for ( unsigned int i = tracelength; i < AUTOARRAY_TRACE; ++i )
						trace[i] = NULL;
				}
			}
			
			uint64_t findTrace()
			{
				uint64_t i = 0;
				
				for ( ; i < tracevector.size() ; ++i )
				{
					bool eq = true;
					AutoArrayBackTrace<AUTOARRAY_TRACE> & A = tracevector[i];
					
					for ( unsigned int j = 0; j < AUTOARRAY_TRACE; ++j )
						if ( A.P[j] != trace[j] )
						{
							eq = false;
							break;
						}
						
					if ( eq )
						break;
				}

				return i;			
			}

			void traceIn(size_t const bytes)
			{
				fillTrace();
				
				libmaus2::parallel::ScopePosixSpinLock slock(tracelock);
				uint64_t const i = findTrace();
				
				if ( i == tracevector.size() )
				{
					AutoArrayBackTrace<AUTOARRAY_TRACE> A;
					
					for ( unsigned int j = 0; j < AUTOARRAY_TRACE; ++j )
						A.P[j] = trace[j];
					A.tracelength = tracelength;
					
					if ( bytes )
						A.alloccnt = 1;
					else
						A.alloccnt = 0;

					A.allocbytes = bytes;
					A.type = getValueTypeName();
					
					tracevector.push_back(A);
				}
				else
				{
					AutoArrayBackTrace<AUTOARRAY_TRACE> & A = tracevector[i];
					
					if ( bytes )
						A.alloccnt += 1;
					A.allocbytes += bytes;				
				}
			}

			void traceOut(size_t const bytes)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(tracelock);
				uint64_t const i = findTrace();
				
				assert ( i < tracevector.size() );
				
				AutoArrayBackTrace<AUTOARRAY_TRACE> & A = tracevector[i];

				if ( bytes )
					A.freecnt += 1;
					
				A.freebytes += bytes;				
			}
			#endif
			
			/**
			 * increase total AutoArray allocation counter by n elements of type N
			 * @param n number of elements allocated
			 **/
			void increaseTotalAllocation(uint64_t n)
			{
				#if defined(AUTOARRAY_TRACE)
				traceIn(n * sizeof(N));
				#endif
				
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				uint64_t const newmemusage = __sync_add_and_fetch(&AutoArray_memusage, n * sizeof(N));
				
				if ( newmemusage > AutoArray_maxmem )
				{
					__sync_fetch_and_sub(&AutoArray_memusage, n * sizeof(N));

					::libmaus2::exception::LibMausException se;
					se.getStream() << "bad allocation: AutoArray mem limit of " << AutoArray_maxmem << " bytes exceeded by new allocation of " << n*sizeof(N) << " bytes.";
					se.finish();
					throw se;
				
				}
				
				uint64_t peak;
				while ( newmemusage > (peak=AutoArray_peakmemusage) )
					__sync_val_compare_and_swap(&AutoArray_peakmemusage,peak,newmemusage);
				#else
			
				#if defined(_OPENMP) || defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
				AutoArray_lock.lock();
				#endif
				
				if ( AutoArray_memusage + n * sizeof(N) > AutoArray_maxmem )
				{
					#if defined(_OPENMP) || defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
					AutoArray_lock.unlock();
					#endif	
					
					::libmaus2::exception::LibMausException se;
					se.getStream() << "bad allocation: AutoArray mem limit of " << AutoArray_maxmem << " bytes exceeded by new allocation of " << n*sizeof(N) << " bytes.";
					se.finish();
					throw se;
				}
				
				AutoArray_memusage += n * sizeof(N);
				AutoArray_peakmemusage = std::max(AutoArray_peakmemusage, AutoArray_memusage);
				
				#if defined(_OPENMP) || defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
				AutoArray_lock.unlock();
				#endif
				
				#endif // LIBMAUS2_HAVE_SYNC_OPS
			}
			/**
			 * decrease total AutoArray allocation counter by n elements of type N
			 * @param n number of elements deallocated
			 **/
			void decreaseTotalAllocation(uint64_t n)
			{
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)

				__sync_fetch_and_sub(&AutoArray_memusage, n * sizeof(N));
				
				#else
				
				#if defined(_OPENMP) || defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
				AutoArray_lock.lock();
				#endif
				
				AutoArray_memusage -= n * sizeof(N);
				
				#if defined(_OPENMP) || defined(LIBMAUS2_HAVE_POSIX_SPINLOCKS)
				AutoArray_lock.unlock();
				#endif
				
				#endif

				#if defined(AUTOARRAY_TRACE)
				traceOut(n * sizeof(N));
				#endif
			}

			/**
			 * allocate an array of size n
			 * @param n size of array
			 **/
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
							#elif defined(LIBMAUS2_HAVE_POSIX_MEMALIGN)
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
					::libmaus2::exception::LibMausException se;
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
			
			public:
			/**
			 * return pointer to start of array
			 * @return pointer to start of array
			 **/
			N * begin()
			{
				return get();
			}
			/**
			 * return pointer to first element behind array
			 * @return pointer to first element behind array
			 **/
			N * end()
			{
				return get()+size();
			}
			/**
			 * return const pointer to start of array
			 * @return const pointer to start of array
			 **/
			N const * begin() const
			{
				return get();
			}
			/**
			 * return const pointer to first element behind array
			 * @return const pointer to first element behind array
			 **/
			N const * end() const
			{
				return get()+size();
			}
			
			/**
			 * return inverse array, if this is a permutation (undefined otherwise)
			 * @return inverse array, if this is a permutation
			 **/
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
			
			/**
			 * return name of value type
			 * @return name of value type
			 **/
			static std::string getValueTypeName()
			{
				return ::libmaus2::util::Demangle::demangle<value_type>();
			}
			
			/**
			 * return name of allocation type
			 * @return name of allocation type
			 **/
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
					default:
						return "alloc_type_unknown";
				}
			}
			/**
			 * return full type name of array class
			 * @return full type name of array class
			 **/
			static std::string getTypeName()
			{
				return std::string("AutoArray<") + getValueTypeName() + "," + getAllocTypeName() + ">";
			}
			
			/**
			 * compute array of prefix sums in place
			 * @return sum over all elements of the array before prefix sum computation
			 *  (i.e. first element in the prefix sums array beyond the end)
			 **/
			N prefixSums()
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
			
			/**
			 * compute prefix sums in parallel
			 * @return sum over all elements of the array before prefix sum computation
			 **/
			N prefixSumsParallel()
			{
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif
				
				uint64_t const elperthread = (getN() + numthreads-1)/numthreads;
				uint64_t const parts = (getN() + elperthread-1)/elperthread;
				
				libmaus2::autoarray::AutoArray<N> partial(parts+1,false);
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < static_cast<int64_t>(parts); ++t )
				{
					uint64_t const low = t * elperthread;
					uint64_t const high = std::min(low+elperthread,getN());
					
					N acc = N();
					for ( uint64_t i = low; i < high; ++i )
						acc += (*this)[i];
					partial[t] = acc;
				}
				
				partial.prefixSums();

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < static_cast<int64_t>(parts); ++t )
				{
					uint64_t const low = t * elperthread;
					uint64_t const high = std::min(low+elperthread,getN());
					
					N acc = partial[t];
					for ( uint64_t i = low; i < high; ++i )
					{
						N const t = (*this)[i];
						(*this)[i] = acc;
						acc += t;
					}
					partial[t] = acc;
				}
				
				return partial[partial.size()-1];
			}
			
			/**
			 * write AutoArray header
			 *
			 * @param out output stream
			 * @param n number of elements
			 * @return number of bytes written
			 **/
			static uint64_t putHeader(std::ostream & out, uint64_t const n)
			{
				return ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);
			}

			/**
			 * serialise object to an output stream
			 * @param out output stream
			 * @param writeHeader if true an AutoArray header is written (necessary for deserialisation)
			 * @return number of bytes written to out
			 **/
			uint64_t serialize(std::ostream & out, bool writeHeader= true) const
			{
				uint64_t s = 0;
				if ( writeHeader )
					s += putHeader(out,n);
				s += ::libmaus2::serialize::Serialize<N>::serializeArray(out,array,n);
				return s;
			}
			
			/**
			 * @return size of serialised object
			 **/
			uint64_t serialisedSize(bool const writeHeader = true) const
			{
				uint64_t s = 0;
				if ( writeHeader )
					s += sizeof(uint64_t);
				s += n * sizeof(N);
				return s;
			}
			
			/**
			 * serialise object to a file
			 * @param filename name of file object is serialised to
			 * @param writeHeader if true an AutoArray header is written (necessary for deserialisation)
			 * @return number of bytes written to file
			 **/
			uint64_t serialize(std::string const & filename, bool writeHeader = true) const
			{
				libmaus2::aio::OutputStreamInstance ostr(filename);
				uint64_t const s = serialize(ostr,writeHeader);
				ostr.flush();
				if ( ! ostr )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to serialize " << getTypeName() << " of size " << size() << " to file " << filename << std::endl;
					se.finish();
					throw se;
				}
				return s;
			}
			
			/**
			 * serialise prefix of length tn of this array to stream out. Function writes an AutoArray header, i.e.
			 * prefix of array can be read by using deserialize()
			 *
			 * @param out ouptut stream
			 * @param tn number of elements to write
			 * @return number of bytes written to out
			 **/
			uint64_t serializePrefix(std::ostream & out, uint64_t const tn) const
			{
				uint64_t s = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::serialize(out,tn);
				s += ::libmaus2::serialize::Serialize<N>::serializeArray(out,array,tn);
				return s;
			}

			/**
			 * get word modulus
			 * @return word modulus
			 **/
			static uint64_t getWordMod()
			{
				return 1;
			}
			
			/**
			 * return system page size
			 * @return system page size
			 **/
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
			
			/**
			 * @return page mod (smallest nonzero number of uint64_t size words to align to a page boundary)
			 **/
			uint64_t getPageMod() const
			{
				return ::libmaus2::math::lcm(getpagesize(),sizeof(uint64_t)) / sizeof(uint64_t);
			}

			/**
			 * resize array to rn elements. This only works for alloc_type_c arrays. If this method is
			 * called for other types of arrays, then an exception is thrown.
			 * @param rn new size of array
			 **/
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
							::libmaus2::exception::LibMausException se;
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
							this_type C(rn,false);
							uint64_t const copy = std::min(size(),C.size());
							std::copy(begin(),begin()+copy,C.begin());
							*this = C;
						}
						break;
				}				
			}

			/**
			 * swap two objects
			 **/
			void swap(AutoArray<N,atype> & o)
			{
				std::swap(array,o.array);
				std::swap(n,o.n);
			}
					
			private:
			/**
			 * @return size of a (level 1) cache line in bytes
			 **/
			#if defined(__linux__) && defined(_SC_LEVEL1_DCACHE_LINESIZE)
			static uint64_t getCacheLineSize()
			{
				uint64_t sclinesize = sysconf (_SC_LEVEL1_DCACHE_LINESIZE);

				if ( sclinesize )
					return sclinesize;
				
				#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)				
				return ::libmaus2::util::I386CacheLineSize::getCacheLineSize();				
				#else
				return 64ull;
				#endif
			}
			#elif defined(_WIN32)
			static uint64_t getCacheLineSize() 
			{
				uint64_t cachelinesize = 0;
				DWORD bufsize = 0;
				GetLogicalProcessorInformation(0, &bufsize);
				::libmaus2::autoarray::AutoArray<uint8_t> Abuffer(bufsize);
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
				#if defined(NDEBUG)
				sysctlbyname("hw.cachelinesize", &cachelinesize, &cachelinesizelen, 0, 0);
				#else
				int const sysctlretname = sysctlbyname("hw.cachelinesize", &cachelinesize, &cachelinesizelen, 0, 0);
				assert ( ! sysctlretname );
				#endif
				return cachelinesize;
			}
			#elif defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
			static uint64_t getCacheLineSize() 
			{
				return ::libmaus2::util::I386CacheLineSize::getCacheLineSize();
			}
			#else
			static uint64_t getCacheLineSize() 
			{
				libmaus2::exception::LibMausException se;
				se.getStream() << "AutoArray<>::getCacheLineSize(): cache line size detection not available." << std::endl;
				se.finish();
				throw se;
			}			
			#endif
			
			public:
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
						#if defined(_WIN32) || defined(LIBMAUS2_HAVE_POSIX_MEMALIGN)
						free(array);
						#else
						AlignedAllocation<N,alloc_type_memalign_cacheline>::freeAligned(array);
						#endif
						break;
				}
				
				array = 0;
				n = 0;
			}
			
			/**
			 * deserialise array from stream in. Previous content of array is discarded by calling the release method
			 * @param in input stream
			 * @return number of bytes read from in
			 **/
			uint64_t deserialize(std::istream & in)
			{
				release();
				uint64_t s = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
				increaseTotalAllocation(n);
				allocateArray(n);
				s += ::libmaus2::serialize::Serialize<N>::deserializeArray(in,array,n);
				return s;
			}
			
			/**
			 * ignore an AutoArray<N> object on stream in (read over it and discard the data). Note that this reads the data from the stream,
			 * it does not skip over the data by calling any seek type function (see skipArray for this).
			 * @param in input stream
			 * @return number of bytes ignored
			 **/
			static uint64_t ignore(std::istream & in)
			{
				uint64_t s = 0;
				uint64_t n;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
				s += ::libmaus2::serialize::Serialize<N>::ignoreArray(in,n);
				return s;
			}
			/**
			 * skip over an AutoArray<N> object in stream in
			 * @param in input stream
			 * @return size of serialised AutoArray object in bytes
			 **/
			static uint64_t skip(std::istream & in)
			{
				uint64_t s = 0;
				uint64_t n;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
				s += ::libmaus2::serialize::Serialize<N>::skipArray(in,n);
				return s;
			}
			/**
			 * copy an AutoArray<N> object from input stream in to output stream out
			 * @param in input stream
			 * @param out output stream
			 * @return number of bytes copied
			 **/
			static uint64_t copy(std::istream & in, std::ostream & out)
			{
				uint64_t s = 0;
				uint64_t n;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
				::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n);

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
			/**
			 * subsample an AutoArray<N> object from input stream in to output stream out (i.e. keep every rate's element)
			 * @param in input stream
			 * @param out output stream
			 * @param rate sampling rate
			 * @return number of bytes written for serialised subsampled AutoArray
			 **/
			static uint64_t sampleCopy(std::istream & in, std::ostream & out, uint64_t const rate)
			{
				assert ( rate != 0 );
			
				uint64_t s = 0;

				uint64_t in_n;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&in_n);
				assert ( in );

				uint64_t const out_n = ( in_n + rate - 1 ) / rate;
				::libmaus2::serialize::Serialize<uint64_t>::serialize(out,out_n);
				
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

			/**
			 * @param n number of elements
			 * @return return estimate for size of an AutoArray<N> object of size n in bytes
			 **/
			static uint64_t byteSize(uint64_t const n)
			{
				return sizeof(N*) + sizeof(uint64_t) + n*sizeof(N);
			}
		

			public:
			/**
			 * @return a clone of this array
			 **/
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
			#if defined(AUTOARRAY_TRACE)
			, tracelength(0)
			#endif
			{
				increaseTotalAllocation(0); 
				
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(), " << this << std::endl;
				#endif
			}
			/**
			 * copy constructor. retrieves array from o and invalidates o.
			 * @param o
			 **/
			AutoArray(AutoArray<N,atype> const & o) : array(0), n(0)
			#if defined(AUTOARRAY_TRACE)
			, tracelength(0)
			#endif
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(AutoArray &), " << this << std::endl;
				#endif
				
				array = o.array;
				n = o.n;
				o.array = 0;
				o.n = 0;

				#if defined(AUTOARRAY_TRACE)
				for ( unsigned int i = 0; i < AUTOARRAY_TRACE; ++i )
					trace[i] = o.trace[i];
				tracelength = o.tracelength;
				#endif
			}
			#if 1
			/**
			 * copy constructor
			 **/
			AutoArray(uint64_t rn, N const * D) : array(0), n(rn) 
			#if defined(AUTOARRAY_TRACE)
			, tracelength(0)
			#endif
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
			#if defined(AUTOARRAY_TRACE)
			, tracelength(0)
			#endif
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(uint64_t, bool), " << this << std::endl;
				#endif
				
				increaseTotalAllocation(n); 
				allocateArray(n);
			
				if ( erase )
					erase_type::erase(array,n);
					#if 0
					for ( uint64_t i = 0; i < n; ++i )
						array[i] = N();
					#endif

			}

			/**
			 * read number from stream in and add size of number (sizeof(uint64_t)=8) to s
			 * @param in input stream
			 * @param s deserialisation byte number accu
			 * @return deserialised number
			 **/
			static uint64_t readNumber(std::istream & in, uint64_t & s)
			{
				uint64_t n = 0;
				s += ::libmaus2::serialize::Serialize<uint64_t>::deserialize(in,&n);
				return n;
			}
			/**
			 * read number from stream in
			 * @param in input stream
			 * @return deserialised number
			 **/
			static uint64_t readNumber(std::istream & in)
			{
				uint64_t s = 0;
				return readNumber(in,s);
			}

			/**
			 * deserialise object from stream in. add number of bytes read from in to s
			 * @param in input stream
			 * @param s deserialisation byte number accu
			 **/
			AutoArray(std::istream & in, uint64_t & s)
			: n(readNumber(in,s))
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(std::istream,uint64_t &), " << this << std::endl;
				#endif
				
				increaseTotalAllocation(n);
				allocateArray(n);
				s += ::libmaus2::serialize::Serialize<N>::deserializeArray(in,array,n);
			}
			/**
			 * deserialise object from stream in.
			 * @param in input stream
			 **/
			AutoArray(std::istream & in)
			: n(readNumber(in))
			{
				#if defined(AUTOARRAY_DEBUG)
				std::cerr << getTypeName() << "(std::istream), " << this << std::endl;
				#endif

				increaseTotalAllocation(n);
				allocateArray(n);
				::libmaus2::serialize::Serialize<N>::deserializeArray(in,array,n);
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
			 * retrieve reference to i'th element with range checking.
			 * the methods throws an exception if i is out of range
			 * @param i
			 * @return reference to i'th element
			 **/
			N       & at(uint64_t i)       
			{
				if ( i < size() )
					return array[i]; 
				else
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "AutoArray<"<<getTypeName()<<">::at(" << i << "): index is out of bounds for array of size " << size() << std::endl;
					ex.finish();
					throw ex;
				}
			}
			/**
			 * retrieve reference to i'th element with range checking.
			 * the methods throws an exception if i is out of range
			 * @param i
			 * @return reference to i'th element
			 **/
			N const & at(uint64_t i) const
			{
				if ( i < size() )
					return array[i]; 
				else
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "AutoArray<"<<getTypeName()<<">::at(" << i << "): index is out of bounds for array of size " << size() << std::endl;
					ex.finish();
					throw ex;
				}
			}
		
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

					#if defined(AUTOARRAY_TRACE)
					for ( unsigned int i = 0; i < AUTOARRAY_TRACE; ++i )
						trace[i] = o.trace[i];
					tracelength = o.tracelength;
					#endif
				}
				return *this;
			}
			
			/**
			 * retrieve next larger value >= k in index interval [l,r)
			 * @param l left interval bound (inclusive)
			 * @param r right interval bound (exclusive)
			 * @param k reference value
			 * @return next larger value >= k in index interval [l,r) or std::numeric_limits<N>::max() if there is no such value
			 **/
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
			
			/**
			 * @param sym element
			 * @param i right bound (inclusive)
			 * @return number of elements equaling sym in index interval [0,i]
			 **/
			uint64_t rank(N sym, uint64_t i) const
			{
				uint64_t c = 0;
				for ( uint64_t j = 0; j <= i; ++j )
					if ( (*this)[j] == sym )
						c++;
				return c;
			}
			/**
			 * @param sym element
			 * @param i selector
			 * @return position of i'th element equalling sym (zero based) or std::numeric_limits<uint64_t>::max() if there is no such element
			 **/
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
			
			/**
			 * read portion of file as AutoArray. Does not deserialise a header
			 * @param filename name of file
			 * @param offset file byte position as multiple of sizeof(N)
			 * @param length number of elements to be read
			 **/
			static AutoArray<N> readFilePortion(
				std::string const & filename,
				uint64_t const offset,
				uint64_t const length)
			{
				::libmaus2::aio::InputStreamInstance CIS(filename);
				CIS.seekg(offset*sizeof(N));
				AutoArray<N> A(length,false);
				CIS.read ( reinterpret_cast<char *>(A.begin()), length*sizeof(N));
				return A;
			}
			
			/**
			 * read a complete file as an AutoArray. Does not read an AutoArray header
			 * @param filename name of file to be read
			 * @return contents of file as array of type N
			 **/
			static AutoArray<N> readFile(std::string const & filename)
			{
				::libmaus2::aio::InputStreamInstance CIS(filename);
				CIS.seekg(0, std::ios::end);
				uint64_t const filesize = CIS.tellg();
				
				if ( filesize % sizeof(N) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "AutoArray::readFile(" << filename << "): file size " << filesize << " is not a multiple of " << sizeof(N) << std::endl;
					se.finish();
					throw se;
				}
				
				uint64_t const length = filesize/sizeof(N);
				CIS.seekg(0, std::ios::beg);
				
				AutoArray<N> A(length,false);
				
				uint64_t t = length*sizeof(N);
				char * p = reinterpret_cast<char *>(A.begin());
				uint64_t const bs = 64*1024;
				while ( t )
				{
					uint64_t const toread = std::min(t,bs);
					CIS.read(p,toread);
					
					if ( CIS.gcount() != static_cast<int64_t>(toread) )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "AutoArray::readFile(" << filename << "): file size " << filesize << " input error" << std::endl;
						se.finish();
						throw se;
					}
					
					t -= toread;
					p += toread;
				}
				
				return A;
			}
		};

		template<typename N>
		void ArrayErase<N>::erase(N * array, uint64_t const n)
		{
			for ( uint64_t i = 0; i < n; ++i )
				array[i] = N();
		}
		
		#if defined(LIBMAUS2_USE_STD_UNIQUE_PTR)
		template<typename N>
		void ArrayErase< std::unique_ptr<N> >::erase(std::unique_ptr<N> * array, uint64_t const n)
		{
			for ( uint64_t i = 0; i < n; ++i )
				array[i] = UNIQUE_PTR_MOVE(std::unique_ptr<N>());
		}			
		#endif
		#if defined(LIBMAUS2_USE_BOOST_UNIQUE_PTR)
		template<typename N>
		void ArrayErase< typename ::boost::interprocess::unique_ptr<N,::libmaus2::deleter::Deleter<N> > >::erase(typename ::libmaus2::util::unique_ptr<N>::type * array, uint64_t const n)
		{
			for ( uint64_t i = 0; i < n; ++i )
			{
				typename ::libmaus2::util::unique_ptr<N>::type ptr;
				array[i] = UNIQUE_PTR_MOVE(ptr);
			}
		}
		#endif


		template<typename N, alloc_type atype>
		N * AlignedAllocation<N,atype>::alignedAllocate(uint64_t const n, uint64_t const)
		{
			return new N[n];
		}

		template<typename N, alloc_type atype>
		void AlignedAllocation<N,atype>::freeAligned(N * alignedp)
		{
			delete [] alignedp;
		}			

		template<typename N>
		N * AlignedAllocation<N,alloc_type_memalign_cacheline>::alignedAllocate(uint64_t const n, uint64_t const align)
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

		template<typename N>
		void AlignedAllocation<N,alloc_type_memalign_cacheline>::freeAligned(N * alignedp)
		{
			if ( alignedp )
				// delete [] (reinterpret_cast<uint8_t **>((alignedp)))[-1];
				delete [] (
					(uint8_t **)(alignedp)
				)[-1];
		}

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

		std::ostream & operator<<(std::ostream & out, libmaus2::autoarray::AutoArrayMemUsage const & aamu);
	}
}

#endif
