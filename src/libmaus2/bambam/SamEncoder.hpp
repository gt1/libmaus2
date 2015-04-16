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
#if ! defined(LIBMAUS_BAMBAM_SAMENCODER_HPP)
#define LIBMAUS_BAMBAM_SAMENCODER_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/aio/PosixFdOutputStream.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct SamEncoder : public BamBlockWriterBase
		{
			typedef SamEncoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::aio::PosixFdOutputStream::unique_ptr_type PFOS;
			std::ostream & ostr;
			libmaus2::bambam::BamHeader const & header;
			::libmaus2::bambam::BamFormatAuxiliary auxdata;

			SamEncoder(std::string const & filename, libmaus2::bambam::BamHeader const & rheader)
			: PFOS(new libmaus2::aio::PosixFdOutputStream(filename)), ostr(*PFOS), header(rheader), auxdata()
			{
				ostr << header.text;
			}

			SamEncoder(std::ostream & rostr, libmaus2::bambam::BamHeader const & rheader)
			: PFOS(), ostr(rostr), header(rheader), auxdata()
			{
				ostr << header.text;			
			}
			~SamEncoder() {}

			/**
			 * write a BAM data block
			 **/
			virtual void writeBamBlock(uint8_t const * E, uint64_t const blocksize)
			{
				libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(ostr,E,blocksize,header,auxdata);
				ostr.put('\n');			
			}
			
			/**
			 * write alignment
			 **/
			virtual void writeAlignment(libmaus2::bambam::BamAlignment const & A)
			{
				writeBamBlock(A.D.begin(),A.blocksize);
			}
		};
	}
}
#endif
