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


#if ! defined(SYNCHRONOUSOUTPUTFILE8ARRAY_HPP)
#define SYNCHRONOUSOUTPUTFILE8ARRAY_HPP

#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/aio/SynchronousOutputBuffer8.hpp>
#include <libmaus/util/TempFileNameGenerator.hpp>
#include <vector>
#include <stdexcept>

namespace libmaus
{
	namespace aio
	{
		template<typename _buffer_type>
		struct SynchronousOutputFile8ArrayTemplate
		{
			typedef _buffer_type buffer_type;
			typedef typename ::libmaus::util::unique_ptr<buffer_type>::type buffer_ptr_type;
			typedef SynchronousOutputFile8ArrayTemplate<buffer_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const * HI;
			::libmaus::autoarray::AutoArray<buffer_ptr_type> buffers;
			std::vector < std::string > filenames;
			::libmaus::util::IntervalTree::unique_ptr_type IT;
			
			void init(uint64_t const numbuf, std::string const & fileprefix)
			{
				for ( uint64_t i = 0; i < numbuf; ++i )
				{
					std::ostringstream ostr;
					ostr << fileprefix << "_" << i;
					filenames.push_back(ostr.str());
				}
				for ( uint64_t i = 0; i < numbuf; ++i )
				{
					buffers[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(filenames[i],1024)));
				}			
			}

			void init(uint64_t const numbuf, ::libmaus::util::TempFileNameGenerator & tmpgen, bool const parallel = false)
			{
				filenames.resize(numbuf);

				if ( parallel )
				{
					uint64_t created = 0;
					::libmaus::parallel::OMPLock lock;
				
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1)
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(numbuf); ++i )
					{
						filenames[i] = tmpgen.getFileName();
						buffers[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(filenames[i],1024)));
						
						lock.lock();
						created += 1;
						std::cerr << "(" << created << "/" << numbuf << ")";
						lock.unlock();
					}	
				}
				else
				{
					for ( uint64_t i = 0; i < numbuf; ++i )
					{
						filenames[i] = tmpgen.getFileName();
						buffers[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(filenames[i],1024)));
					}			
				}
			}

			void init(std::vector<std::string> const & rfilenames, bool truncate)
			{
				filenames = rfilenames;

				uint64_t const numbuf = filenames.size();

				for ( uint64_t i = 0; i < numbuf; ++i )
				{
					buffers[i] = UNIQUE_PTR_MOVE(buffer_ptr_type(new buffer_type(filenames[i],1024,truncate)));
				}			
			}

			SynchronousOutputFile8ArrayTemplate(
				::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & rHI, 
				std::string const & fileprefix
			)
			: HI(&rHI), buffers(HI->size()), IT(new ::libmaus::util::IntervalTree(*HI,0,HI->size()))
			{
				init ( HI->size(), fileprefix );
			}
			SynchronousOutputFile8ArrayTemplate(
				::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & rHI, 
				::libmaus::util::TempFileNameGenerator & tmpgen
			)
			: HI(&rHI), buffers(HI->size()), IT(new ::libmaus::util::IntervalTree(*HI,0,HI->size()))
			{
				init ( HI->size(), tmpgen );
			}
			
			SynchronousOutputFile8ArrayTemplate(std::string const & fileprefix, uint64_t const numbuf)
			: HI(0), buffers(numbuf)
			{
				init ( numbuf, fileprefix );
			}

			SynchronousOutputFile8ArrayTemplate(::libmaus::util::TempFileNameGenerator & tmpgen, uint64_t const numbuf, bool const parallel = false)
			: HI(0), buffers(numbuf)
			{
				init ( numbuf, tmpgen, parallel );
			}

			SynchronousOutputFile8ArrayTemplate(
				::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & rHI, 
				std::vector<std::string> const & filenames,
				bool const truncate
			)
			: HI(&rHI), buffers(HI->size()), IT(new ::libmaus::util::IntervalTree(*HI,0,HI->size()))
			{
				init ( filenames, truncate );
			}
			
			~SynchronousOutputFile8ArrayTemplate()
			{
				flush();
			}
			void removeFiles()
			{
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					remove ( filenames[i].c_str() );
			}
			void flush()
			{
				for ( uint64_t i = 0; i < buffers.getN(); ++i )
					if ( buffers[i].get() )
					{
						//std::cerr << "(" << (i+1) << "/" << buffers.getN();
						buffers[i]->flush();
						buffers[i].reset();
						//std::cerr << ")";
					}
			}
			buffer_type & getBuffer(uint64_t const hashval)
			{
				uint64_t const i = IT->find(hashval);
				return *(buffers[i]);
			}
			buffer_type & getBufferById(uint64_t const id)
			{
				return *(buffers[id]);
			}
		};
	}
}

namespace libmaus
{
	namespace aio
	{
		typedef SynchronousOutputFile8ArrayTemplate<libmaus::aio::SynchronousOutputBuffer8> SynchronousOutputFile8Array;
	}
}

#if ! defined(_WIN32)
#include <libmaus/aio/SynchronousOutputBuffer8Posix.hpp>
#endif

namespace libmaus
{
	namespace aio
	{
		#if ! defined(_WIN32)
		typedef SynchronousOutputFile8ArrayTemplate<libmaus::aio::SynchronousOutputBuffer8Posix> SynchronousOutputFile8ArrayPosix;
		#endif
	}
}
#endif
