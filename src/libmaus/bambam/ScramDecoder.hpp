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
#if ! defined(LIBMAUS_BAMBAM_SCRAMDECODER_HPP)
#define LIBMAUS_BAMBAM_SCRAMDECODER_HPP

#include <libmaus/LibMausConfig.hpp>
#include <iostream>
#include <cstdlib>

#if defined(LIBMAUS_HAVE_IO_LIB)
#include <libmaus/bambam/Scram.h>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/util/DynamicLoading.hpp>
#include <libmaus/bambam/AlignmentValidity.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ScramDecoder
		{
			typedef ::libmaus::fastx::FASTQEntry pattern_type;

			::libmaus::util::DynamicLibrary scram_mod;
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_New_Type> d_new;
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_Delete_Type> d_delete;
			::libmaus::util::DynamicLibraryFunction<libmaus_bambam_ScramDecoder_Decode_Type> d_decode;

			libmaus_bambam_ScramDecoder * dec;

			::libmaus::bambam::BamHeader bamheader;
			::libmaus::bambam::BamAlignment alignment;
			::libmaus::bambam::BamFormatAuxiliary auxiliary;
					
			uint64_t patid;
			uint64_t rank;
			bool putrank;
			bool validate;
			
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

			ScramDecoder(std::string const & filename, std::string const & mode, std::string const & reference, bool const rputrank = false)
			: 
				scram_mod("libmaus_scram_mod.so"),
				d_new(scram_mod,"libmaus_bambam_ScramDecoder_New"),
				d_delete(scram_mod,"libmaus_bambam_ScramDecoder_Delete"),
				d_decode(scram_mod,"libmaus_bambam_ScramDecoder_Decode"),
				dec(allocateDecoder(filename,mode,reference)),
				bamheader(std::string(dec->header,dec->header+dec->headerlen)),
				patid(0), rank(0), putrank(rputrank), validate(true)
			{
			}
			
			~ScramDecoder()
			{
				d_delete.func(dec);
			}

			void disableValidation()
			{
				validate = false;
			}
			
			::libmaus::bambam::BamAlignment::unique_ptr_type ualignment() const
			{
				return UNIQUE_PTR_MOVE(alignment.uclone());
			}
			
			::libmaus::bambam::BamAlignment::shared_ptr_type salignment() const
			{
				return alignment.sclone();
			}

			std::string formatAlignment()
			{
				return alignment.formatAlignment(bamheader,auxiliary);
			}
			
			std::string formatFastq()
			{
				return alignment.formatFastq(auxiliary);
			}
			
			void putRank()
			{
				uint64_t const lrank = rank++;
				if ( putrank )
				{
					alignment.putRank("ZR",lrank /*,bamheader */);
				}			
			}
			
			bool readAlignment(bool const delayPutRank = false)
			{
				if ( d_decode.func(dec) < 0 )
					return false;

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
			
			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				if ( !readAlignment() )
					return false;
				
				alignment.toPattern(pattern,patid++);
				
				return true;
			}
		};
	}
}
#endif
#endif

