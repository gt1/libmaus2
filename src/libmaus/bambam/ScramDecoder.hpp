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
#if ! defined(LIBMAUS_BAMBAM_SCRAMDECODER_HPP)
#define LIBMAUS_BAMBAM_SCRAMDECODER_HPP

#include <libmaus/LibMausConfig.hpp>
#include <iostream>
#include <cstdlib>

#include <libmaus/bambam/Scram.h>
#include <libmaus/bambam/BamAlignmentDecoder.hpp>
#include <libmaus/util/DynamicLoading.hpp>
#include <libmaus/bambam/AlignmentValidity.hpp>

namespace libmaus
{
	namespace bambam
	{
		#if defined(LIBMAUS_HAVE_DL_FUNCS)
		/**
		 * scram decoder class; alignment decoder based on io_lib
		 **/
		struct ScramDecoder : public libmaus::bambam::BamAlignmentDecoder
		{
			//! this type
			typedef ScramDecoder this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			//! FastQ pattern type
			typedef ::libmaus::fastx::FASTQEntry pattern_type;

			private:
			//! scram module
			::libmaus::util::DynamicLibrary scram_mod;
			//! decoder allocate function handle
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Type> d_new;
			//! decoder allocate function handle via callback
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Type>::unique_ptr_type d_new_input_callback;
			//! decoder allocate function handle with range
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Range_Type> d_new_range;
			//! decoder allocate function handle via callback and range
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Range_Type>::unique_ptr_type d_new_input_callback_range;
			//! decoder deallocation function handle
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_Delete_Type> d_delete;
			//! decoder decoding function handle
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_Decode_Type> d_decode;

			//! scram decoder object
			libmaus_bambam_ScramDecoder * dec;

			//! bam header object
			::libmaus::bambam::BamHeader bamheader;

			/**
			 * allocate scram decoder object; throws exception on failure
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @return decoder object
			 **/					
			libmaus_bambam_ScramDecoder * allocateDecoder(std::string const & filename, std::string const & mode, std::string const & reference)
			{
				libmaus_bambam_ScramDecoder * dec = d_new.func(filename.c_str(),mode.c_str(),reference.size() ? reference.c_str() : 0);
				
				if ( ! dec )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ScramDecoder: failed to open file " << filename << " in mode " << mode << std::endl;
					se.finish();
					throw se;
				}
			
				return dec;
			}

			/**
			 * allocate scram decoder object with range; throws exception on failure
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @return decoder object
			 **/					
			libmaus_bambam_ScramDecoder * allocateDecoder(
				std::string const & filename, std::string const & mode, std::string const & reference,
				std::string const & ref,
				int64_t const start,
				int64_t const end
			)
			{
				if ( mode != "rc" )
				{				
					::libmaus::exception::LibMausException se;
					se.getStream() << "ScramDecoder: failed to open file " << filename << " in mode " << mode 
						<< " with range (" << ref << "," << start << "," << end << "), "
						<< "ranges are only supported for CRAM input" << std::endl;
					se.finish();
					throw se;
				}
			
				libmaus_bambam_ScramDecoder * dec = d_new_range.func(
					filename.c_str(),mode.c_str(),reference.size() ? reference.c_str() : 0,
					ref.c_str(),
					start,end
				);
				
				if ( ! dec )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ScramDecoder: failed to open file " << filename << " in mode " << mode 
						<< " with range (" << ref << "," << start << "," << end << ")"
						<< std::endl;
					se.finish();
					throw se;
				}
			
				return dec;
			}

			/**
			 * allocate cram decoder object via callbacks
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @return decoder object
			 **/
			libmaus_bambam_ScramDecoder * allocateDecoder(
				std::string const & filename,
				scram_cram_io_allocate_read_input_t   callback_allocate_function,
				scram_cram_io_deallocate_read_input_t callback_deallocate_function,
				size_t const bufsize,
				std::string const & rreferencefilename
			)
			{
				libmaus_bambam_ScramDecoder * dec = d_new_input_callback->func(
					filename.c_str(),
					callback_allocate_function,
					callback_deallocate_function,
					bufsize,
					rreferencefilename.size() ? rreferencefilename.c_str() : 0
				);
				
				if ( ! dec )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ScramDecoder: failed to open file " << filename << " in CRAM read mode via callback." << std::endl;
					se.finish();
					throw se;
				}
				
				return dec;
			}

			/**
			 * allocate cram decoder object via callbacks and range
			 *
			 * @param filename input filename, - for stdin
			 * @param reference reference file name (empty string for none)
			 * @return decoder object
			 **/
			libmaus_bambam_ScramDecoder * allocateDecoder(
				std::string const & filename,
				scram_cram_io_allocate_read_input_t   callback_allocate_function,
				scram_cram_io_deallocate_read_input_t callback_deallocate_function,
				size_t const bufsize,
				std::string const & rreferencefilename,
				std::string const & ref,
				int64_t const start,
				int64_t const end
			)
			{
				libmaus_bambam_ScramDecoder * dec = d_new_input_callback_range->func(
					filename.c_str(),
					callback_allocate_function,
					callback_deallocate_function,
					bufsize,
					rreferencefilename.size() ? rreferencefilename.c_str() : 0,
					ref.size() ? ref.c_str() : 0,
					start,end
				);
				
				if ( ! dec )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ScramDecoder: failed to open file " << filename << " in CRAM read mode via callback with range." << std::endl;
					se.finish();
					throw se;
				}
				
				return dec;
			}

			bool readAlignmentInternal(bool const delayPutRank = false)
			{
				int const r = d_decode.func(dec);
				
				// std::cerr << "got code " << r << std::endl;
				
				if ( r == -2 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ScramDecoder::readAlignment(): failed to read alignment without reaching EOF" << std::endl;
					se.finish();
					throw se;
				}
				else if ( r == -1 )
				{
					return false;
				}

				if ( dec->blocksize > alignment.D.size() )
					alignment.D = ::libmaus::bambam::BamAlignment::D_array_type(dec->blocksize,false);
						
				memcpy(alignment.D.begin(),dec->buffer,dec->blocksize);
				alignment.blocksize = dec->blocksize;

				if ( validate )
				{
					libmaus_bambam_alignment_validity const validity = alignment.valid(bamheader);
					if ( validity != ::libmaus::bambam::libmaus_bambam_alignment_validity_ok )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Invalid alignment: " << validity << std::endl;
						se.finish();
						throw se;					
					}
				}

				if ( ! delayPutRank )
					putRank();
			
				return true;
			}

			public:
			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoder(std::string const & filename, std::string const & mode, std::string const & reference, bool const rputrank = false)
			: 
				libmaus::bambam::BamAlignmentDecoder(rputrank),
				scram_mod("libmaus_scram_mod.so"),
				d_new(scram_mod,"libmaus_bambam_ScramDecoder_New"),
				d_new_input_callback(),
				d_new_range(scram_mod,"libmaus_bambam_ScramDecoder_New_Range"),
				d_delete(scram_mod,"libmaus_bambam_ScramDecoder_Delete"),
				d_decode(scram_mod,"libmaus_bambam_ScramDecoder_Decode"),
				dec(allocateDecoder(filename,mode,reference)),
				bamheader(std::string(dec->header,dec->header+dec->headerlen))
			{
			}

			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @param ref name of reference sequence for range
			 * @param start range start
			 * @param end range end
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoder(std::string const & filename, std::string const & mode, std::string const & reference,
				std::string const & ref,
				int64_t const start,
				int64_t const end,
				bool const rputrank = false)
			: 
				libmaus::bambam::BamAlignmentDecoder(rputrank),
				scram_mod("libmaus_scram_mod.so"),
				d_new(scram_mod,"libmaus_bambam_ScramDecoder_New"),
				d_new_input_callback(),
				d_new_range(scram_mod,"libmaus_bambam_ScramDecoder_New_Range"),
				d_delete(scram_mod,"libmaus_bambam_ScramDecoder_Delete"),
				d_decode(scram_mod,"libmaus_bambam_ScramDecoder_Decode"),
				dec(allocateDecoder(filename,mode,reference,ref,start,end)),
				bamheader(std::string(dec->header,dec->header+dec->headerlen))
			{
			}

			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param callback_allocate_function stream allocation callback
			 * @param callback_deallocate_function stream deallocation callback
			 * @param bufsize input buffer size
			 * @param reference reference file name (empty string for none)
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoder(
				std::string const & filename,
			        scram_cram_io_allocate_read_input_t   callback_allocate_function,
			        scram_cram_io_deallocate_read_input_t callback_deallocate_function,
			        size_t const bufsize,
			 	std::string const & reference,			                                        
				bool const rputrank = false)
			: 
				libmaus::bambam::BamAlignmentDecoder(rputrank),
				scram_mod("libmaus_scram_mod.so"),
				d_new(scram_mod,"libmaus_bambam_ScramDecoder_New"),
				d_new_input_callback(
					new ::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Type>(scram_mod,"libmaus_bambam_ScramDecoder_New_Cram_Input_Callback")
				),
				d_new_range(scram_mod,"libmaus_bambam_ScramDecoder_New_Range"),
				d_new_input_callback_range(
					new ::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Range_Type>(scram_mod,"libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Range")
				),
				d_delete(scram_mod,"libmaus_bambam_ScramDecoder_Delete"),
				d_decode(scram_mod,"libmaus_bambam_ScramDecoder_Decode"),
				dec(allocateDecoder(filename,callback_allocate_function,callback_deallocate_function,bufsize,reference)),
				bamheader(std::string(dec->header,dec->header+dec->headerlen))
			{
			}
			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param callback_allocate_function stream allocation callback
			 * @param callback_deallocate_function stream deallocation callback
			 * @param bufsize input buffer size
			 * @param reference reference file name (empty string for none)
			 * @param ref name of reference sequence for range
			 * @param start range start
			 * @param end range end
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoder(
				std::string const & filename,
			        scram_cram_io_allocate_read_input_t   callback_allocate_function,
			        scram_cram_io_deallocate_read_input_t callback_deallocate_function,
			        size_t const bufsize,
			 	std::string const & reference,			                                        
				std::string const & ref,
				int64_t const start,
				int64_t const end,
				bool const rputrank = false)
			: 
				libmaus::bambam::BamAlignmentDecoder(rputrank),
				scram_mod("libmaus_scram_mod.so"),
				d_new(scram_mod,"libmaus_bambam_ScramDecoder_New"),
				d_new_input_callback(
					new ::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Type>(scram_mod,"libmaus_bambam_ScramDecoder_New_Cram_Input_Callback")
				),
				d_new_range(scram_mod,"libmaus_bambam_ScramDecoder_New_Range"),
				d_new_input_callback_range(
					new ::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Range_Type>(scram_mod,"libmaus_bambam_ScramDecoder_New_Cram_Input_Callback_Range")
				),
				d_delete(scram_mod,"libmaus_bambam_ScramDecoder_Delete"),
				d_decode(scram_mod,"libmaus_bambam_ScramDecoder_Decode"),
				dec(allocateDecoder(filename,callback_allocate_function,callback_deallocate_function,bufsize,reference,ref,start,end)),
				bamheader(std::string(dec->header,dec->header+dec->headerlen))
			{
			}
			
			/**
			 * destructor
			 **/
			~ScramDecoder()
			{
				d_delete.func(dec);
			}

			/**
			 * @return BAM header
			 **/
			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamheader;
			}
		};

		/**
		 * class wrapping a ScramDecoder object
		 **/		
		struct ScramDecoderWrapper : public libmaus::bambam::BamAlignmentDecoderWrapper
		{
			//! wrapped object
			ScramDecoder scramdec;

			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoderWrapper(
				std::string const & filename, 
				std::string const & mode, 
				std::string const & reference, 
				bool const rputrank = false
			)
			: scramdec(filename,mode,reference,rputrank)
			{
			
			}
			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @param ref name of reference sequence for range
			 * @param start range start
			 * @param end range end
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoderWrapper(
				std::string const & filename, 
				std::string const & mode, 
				std::string const & reference, 
				std::string const & ref,
				int64_t const start,
				int64_t const end,
				bool const rputrank = false
			)
			: scramdec(filename,mode,reference,ref,start,end,rputrank)
			{
			
			}
			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param callback_allocate_function stream allocation callback
			 * @param callback_deallocate_function stream deallocation callback
			 * @param reference reference file name (empty string for none)
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoderWrapper(
				std::string const & filename,
			        scram_cram_io_allocate_read_input_t   callback_allocate_function,
			        scram_cram_io_deallocate_read_input_t callback_deallocate_function,
			        size_t const bufsize,
			 	std::string const & reference,			                                        
				bool const rputrank = false)
			: scramdec(filename,callback_allocate_function,callback_deallocate_function,bufsize,reference,rputrank)
			{
			
			}
			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param callback_allocate_function stream allocation callback
			 * @param callback_deallocate_function stream deallocation callback
			 * @param bufsize input buffer size
			 * @param reference reference file name (empty string for none)
			 * @param ref name of reference sequence for range
			 * @param start range start
			 * @param end range end
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramDecoderWrapper(
				std::string const & filename,
			        scram_cram_io_allocate_read_input_t   callback_allocate_function,
			        scram_cram_io_deallocate_read_input_t callback_deallocate_function,
			        size_t const bufsize,
			 	std::string const & reference,			                                        
				std::string const & ref,
				int64_t const start,
				int64_t const end,
				bool const rputrank = false)
			: scramdec(filename,callback_allocate_function,callback_deallocate_function,bufsize,reference,ref,start,end,rputrank)
			{
			
			}
			
			libmaus::bambam::BamAlignmentDecoder & getDecoder()
			{
				return scramdec;
			}
		};
		#endif
	}
}
#endif

