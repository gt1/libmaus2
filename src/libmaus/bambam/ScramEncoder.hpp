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
#if ! defined(LIBMAUS_BAMBAM_SCRAMENCODER_HPP)
#define LIBMAUS_BAMBAM_SCRAMENCODER_HPP

#include <libmaus/LibMausConfig.hpp>
#include <iostream>
#include <cstdlib>

#include <libmaus/bambam/BamHeader.hpp>
#include <libmaus/bambam/Scram.h>
#include <libmaus/util/DynamicLoading.hpp>
#include <libmaus/bambam/BamBlockWriterBase.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * scram encoder class; alignment encoder based on io_lib
		 **/
		struct ScramEncoder : public libmaus::bambam::BamBlockWriterBase
		{
			//! this type
			typedef ScramEncoder this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			//! scram module
			::libmaus::util::DynamicLibrary scram_mod;
			//! encoder allocate function handle
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramEncoder_New_Type> e_new;
			//! encoder deallocation function handle
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramEncoder_Delete_Type> e_delete;
			//! encoder encoding function handle
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramEncoder_Encode_Type> e_encode;
			//! encoder wrapper
			libmaus_bambam_ScramEncoder * encoder;

			public:
			/**
			 * constructor
			 *
			 * @param filename input filename, - for stdin
			 * @param mode file mode r (SAM), rb (BAM) or rc (CRAM)
			 * @param reference reference file name (empty string for none)
			 * @param rputrank put rank (line number) on alignments
			 **/
			ScramEncoder(
				libmaus::bambam::BamHeader const & header,
				std::string const & filename, 
				std::string const & mode, 
				std::string const & reference,
				bool const verbose = false
			)
			: 
				scram_mod("libmaus_scram_mod.so"),
				e_new(scram_mod,"libmaus_bambam_ScramEncoder_New"),
				e_delete(scram_mod,"libmaus_bambam_ScramEncoder_Delete"),
				e_encode(scram_mod,"libmaus_bambam_ScramEncoder_Encode")
			{
				encoder = e_new.func(
					header.text.c_str(),
					filename.c_str(),
					mode.c_str(),
					reference.size() ? reference.c_str() : 0,
					verbose
				);
			
				if ( ! encoder )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "scram_open failed." << std::endl;
					se.finish();
					throw se;
				}
			}
			
			virtual ~ScramEncoder()
			{
				e_delete.func(encoder);
			}

			/**
			 * write a BAM data block
			 **/
			void writeBamBlock(uint8_t const * data, uint64_t const blocksize)
			{
				int const r = e_encode.func(encoder,data,blocksize);
				
				if ( r < 0 )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "scram_put_seq failed." << std::endl;
					se.finish();
					throw se;
				}
			}

			void encode(libmaus::bambam::BamAlignment const & A)
			{
				writeBamBlock(A.D.begin(),A.blocksize);
			}
		};
	}
}
#endif

