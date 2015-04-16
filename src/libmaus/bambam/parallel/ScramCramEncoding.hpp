/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_SCRAMCRAMENCODING_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_SCRAMCRAMENCODING_HPP

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/DynamicLoading.hpp>
#include <libmaus/bambam/parallel/CramInterface.h>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct ScramCramEncoding
			{
				typedef ScramCramEncoding this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				#if defined(LIBMAUS_HAVE_DL_FUNCS)
				libmaus::util::DynamicLibrary scramlib;	
				typedef void * (*alloc_func_t)(void *, char const *, size_t const, cram_data_write_function_t);
				libmaus::util::DynamicLibraryFunction<alloc_func_t>::unique_ptr_type Palloc_func;
				typedef void (*dealloc_func_t)(void *);
				libmaus::util::DynamicLibraryFunction<dealloc_func_t>::unique_ptr_type Pdealloc_func;				
				typedef int (*enque_func_t)(void *,void *,size_t const,char const **,size_t const *,size_t const *,size_t const,int const,cram_enque_compression_work_package_function_t,cram_data_write_function_t, cram_compression_work_package_finished_t);
				libmaus::util::DynamicLibraryFunction<enque_func_t>::unique_ptr_type Penque_func;
				typedef int (*dispatch_func_t)(void *);
				libmaus::util::DynamicLibraryFunction<dispatch_func_t>::unique_ptr_type Pdispatch_func;
				#endif
				
				ScramCramEncoding()
				#if defined(LIBMAUS_HAVE_DL_FUNCS)
				: scramlib("libmaus_scram_mod.so"), Palloc_func()
				#endif
				{
					#if ! defined(LIBMAUS_HAVE_DL_FUNCS)
					libmaus::exception::LibMausException lme;
					lme.getStream() << "ScramCramEncoding: no support for dl functions" << std::endl;
					lme.finish();
					throw lme;
					#endif
					
					#if ! defined(LIBMAUS_HAVE_IO_NEW_CRAM_INTERFACE)
					libmaus::exception::LibMausException lme;
					lme.getStream() << "ScramCramEncoding: no support for new CRAM encoding interface" << std::endl;
					lme.finish();
					throw lme;					
					#endif
					
					#if defined(LIBMAUS_HAVE_DL_FUNCS)
					libmaus::util::DynamicLibraryFunction<alloc_func_t>::unique_ptr_type Talloc_func(
						new libmaus::util::DynamicLibraryFunction<alloc_func_t>(scramlib,"scram_cram_allocate_encoder")
					);
					Palloc_func = UNIQUE_PTR_MOVE(Talloc_func);
					libmaus::util::DynamicLibraryFunction<dealloc_func_t>::unique_ptr_type Tdealloc_func(
						new libmaus::util::DynamicLibraryFunction<dealloc_func_t>(scramlib,"scram_cram_deallocate_encoder")
					);
					Pdealloc_func = UNIQUE_PTR_MOVE(Tdealloc_func);
					libmaus::util::DynamicLibraryFunction<enque_func_t>::unique_ptr_type Tenque_func(
						new libmaus::util::DynamicLibraryFunction<enque_func_t>(scramlib,"scram_cram_enque_compression_block")
					);
					Penque_func = UNIQUE_PTR_MOVE(Tenque_func);
					libmaus::util::DynamicLibraryFunction<dispatch_func_t>::unique_ptr_type Tdispatch_func(
						new libmaus::util::DynamicLibraryFunction<dispatch_func_t>(scramlib,"scram_cram_process_work_package")
					);
					Pdispatch_func = UNIQUE_PTR_MOVE(Tdispatch_func);
					#endif
				}

				void * cram_allocate_encoder(void *userdata, char const *header, size_t const headerlength, cram_data_write_function_t writefunc)
				{
					#if defined(LIBMAUS_HAVE_DL_FUNCS)
					return Palloc_func->func(userdata,header,headerlength,writefunc);
					#else					
					libmaus::exception::LibMausException lme;
					lme.getStream() << "ScramCramEncoding::cram_allocate_encoder: no support for new CRAM encoding interface" << std::endl;
					lme.finish();
					throw lme;					
					#endif
				}
				void cram_deallocate_encoder(void * context)
				{
					#if defined(LIBMAUS_HAVE_DL_FUNCS)
					Pdealloc_func->func(context);
					#else					
					libmaus::exception::LibMausException lme;
					lme.getStream() << "ScramCramEncoding::cram_deallocate_encoder: no support for new CRAM encoding interface" << std::endl;
					lme.finish();
					throw lme;					
					#endif
				}

				int cram_enque_compression_block(
					void *userdata,
					void *context,
					size_t const inblockid,
					char const **block,
					size_t const *blocksize,
					size_t const *blockelements,
					size_t const numblocks,
					int const final,
					cram_enque_compression_work_package_function_t workenqueuefunction,
					cram_data_write_function_t writefunction,
					cram_compression_work_package_finished_t workfinishedfunction
				)
				{				
					#if defined(LIBMAUS_HAVE_DL_FUNCS)
					return Penque_func->func(userdata,context,inblockid,block,blocksize,blockelements,numblocks,final,workenqueuefunction,writefunction,workfinishedfunction);
					#else					
					libmaus::exception::LibMausException lme;
					lme.getStream() << "ScramCramEncoding::cram_enque_compression_block: no support for new CRAM encoding interface" << std::endl;
					lme.finish();
					throw lme;					
					#endif
				}
				
				int cram_process_work_package(void *workpackage)
				{					
					#if defined(LIBMAUS_HAVE_DL_FUNCS)
					return Pdispatch_func->func(workpackage);
					#else					
					libmaus::exception::LibMausException lme;
					lme.getStream() << "ScramCramEncoding::cram_process_work_package: no support for new CRAM encoding interface" << std::endl;
					lme.finish();
					throw lme;					
					#endif
				}
			};
		}
	}
}
#endif
