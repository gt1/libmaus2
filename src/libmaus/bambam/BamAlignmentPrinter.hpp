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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTPRINTER_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTPRINTER_HPP

#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus/bambam/BamHeader.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * class for printing BAM alignment blocks as SAM lines
		 **/
		struct BamAlignmentPrinter
		{
			//! BAM header for reference names
			libmaus::bambam::BamHeader const & header;
			//! temp mem block for formatting
			::libmaus::bambam::BamFormatAuxiliary aux;
			//! output stream
			std::ostream & out;
			
			/**
			 * constructor
			 *
			 * @param rheader BAM header
			 * @param rout output stream
			 **/
			BamAlignmentPrinter(libmaus::bambam::BamHeader const & rheader, std::ostream & rout)
			: header(rheader), out(rout) {}

			/**
			 * print alignment block D of length n as SAM line on output stream
			 *
			 * @param D alignment block
			 * @param n size of alignment block
			 **/
			void put(uint8_t const * D, uint64_t const n) 
			{
				out << libmaus::bambam::BamAlignmentDecoderBase::formatAlignment(D,n,header.chromosomes,aux) << "\n";
			}
		};
	}
}
#endif
