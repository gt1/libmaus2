/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_SAMDECODERWRAPPER_HPP)
#define LIBMAUS_BAMBAM_SAMDECODERWRAPPER_HPP

#include <libmaus/bambam/SamDecoder.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * class wrapping a SamDecoder object
		 **/		
		struct SamDecoderWrapper : public libmaus::bambam::BamAlignmentDecoderWrapper
		{
			//! wrapped object
			SamDecoder samdec;

			/**
			 * constructor
			 *
			 * @param filename input filename
			 * @param rputrank put rank (line number) on alignments
			 **/
			SamDecoderWrapper(std::string const & filename, bool const rputrank = false) : samdec(filename,rputrank)
			{
			}
			
			/**
			 * constructor
			 *
			 * @param istr inputstream
			 * @param rputrank put rank (line number) on alignments
			 **/
			SamDecoderWrapper(std::istream & istr, bool const rputrank = false) : samdec(istr,rputrank)
			{
			}

			/**
			 * constructor
			 *
			 * @param istr inputstream
			 * @param rputrank put rank (line number) on alignments
			 **/
			SamDecoderWrapper(libmaus::aio::InputStream::unique_ptr_type & istr, bool const rputrank = false) : samdec(istr,rputrank)
			{
			}
			
			libmaus::bambam::BamAlignmentDecoder & getDecoder()
			{
				return samdec;
			}
		};
	}
}
#endif

