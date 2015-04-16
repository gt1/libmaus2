/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_SAMENCODINGWORKPACKAGE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_SAMENCODINGWORKPACKAGE_HPP

#include <libmaus2/bambam/parallel/ScramCramEncoding.hpp>
#include <libmaus2/bambam/parallel/SamEncoderObject.hpp>
#include <libmaus2/bambam/BamFormatAuxiliary.hpp>
#include <libmaus2/bambam/BamAlignmentDecoderBase.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct SamEncodingWorkPackage
			{
				void *userdata;
				void *context;
				size_t  inblockid;
				char const **block;
				size_t const *blocksize;
				size_t const *blockelements;
				size_t numblocks;
				int final;
				cram_data_write_function_t writefunction;
				cram_compression_work_package_finished_t workfinishedfunction;

				SamEncodingWorkPackage() {}
				
				void dispatch()
				{
					SamEncoderObject * encoder = reinterpret_cast<SamEncoderObject *>(context);
					::libmaus2::bambam::BamFormatAuxiliary auxdata;
					
					for ( size_t b = 0; b < numblocks; ++b )
					{
						std::ostringstream ostr;
						
						char const * A = block[b];
						size_t const n = blocksize[b];
						char const * const Ae = A+n;
						
						while ( A != Ae )
						{
							uint32_t const len = libmaus2::bambam::DecoderBase::getLEInteger(
								reinterpret_cast<uint8_t const *>(A),sizeof(uint32_t));
							A += sizeof(uint32_t);

							libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(ostr,
								reinterpret_cast<uint8_t const *>(A),len,*(encoder->Pheader),auxdata);
							ostr.put('\n');
							
							A += len;
						}

						std::string const data = ostr.str();

						bool const blockfinal = (b+1)==numblocks;
						bool const filefinal = blockfinal && final;

						writefunction(
							userdata,
							inblockid,
							b,
							data.c_str(),
							data.size(),							
							filefinal ? cram_data_write_block_type_file_final : (blockfinal ? cram_data_write_block_type_block_final : cram_data_write_block_type_internal)
						);
					}
					
					if ( ! numblocks )
						writefunction(userdata,inblockid,0,NULL,0,
							final ? cram_data_write_block_type_file_final : cram_data_write_block_type_block_final
						);
										
					workfinishedfunction(userdata,inblockid,final);
				}
			};
		}
	}
}
#endif
