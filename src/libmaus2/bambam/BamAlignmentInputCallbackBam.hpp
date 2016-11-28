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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTINPUTCALLBACKBAM_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTINPUTCALLBACKBAM_HPP

#include <libmaus2/bambam/CircularHashCollatingBamDecoder.hpp>
#include <libmaus2/bambam/BamWriter.hpp>
#include <libmaus2/bambam/BamAlignmentInputPositionCallbackNull.hpp>

namespace libmaus2
{
	namespace bambam
	{
		template<typename _update_base_type = libmaus2::bambam::BamAlignmentInputPositionCallbackNull>
		struct BamAlignmentInputCallbackBam :
			public ::libmaus2::bambam::CollatingBamDecoderAlignmentInputCallback,
			public _update_base_type
		{
			typedef _update_base_type update_base_type;
			typedef BamAlignmentInputCallbackBam<update_base_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t als;
			::libmaus2::bambam::BamWriter::unique_ptr_type BWR;

			BamAlignmentInputCallbackBam(
				std::string const & filename,
				::libmaus2::bambam::BamHeader const & bamheader,
				int const rewritebamlevel
			)
			: update_base_type(bamheader), als(0), BWR(new ::libmaus2::bambam::BamWriter(filename,bamheader,rewritebamlevel))
			{

			}

			~BamAlignmentInputCallbackBam()
			{
			}

			void operator()(::libmaus2::bambam::BamAlignment const & A)
			{
				als++;
				update_base_type::updatePosition(A);
				A.serialise(BWR->getStream());
			}
		};

		template<typename _update_base_type = libmaus2::bambam::BamAlignmentInputPositionCallbackNull>
		struct BamAlignmentInputCallbackBamRef :
			public ::libmaus2::bambam::CollatingBamDecoderAlignmentInputCallback
		{
			typedef _update_base_type update_base_type;
			typedef typename update_base_type::unique_ptr_type update_base_ptr_type;
			typedef BamAlignmentInputCallbackBamRef<update_base_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t als;
			::libmaus2::bambam::BamWriter::unique_ptr_type BWR;
			update_base_ptr_type update_base;

			BamAlignmentInputCallbackBamRef(
				std::string const & filename,
				::libmaus2::bambam::BamHeader const & bamheader,
				int const rewritebamlevel,
				update_base_ptr_type & rupdate_base
			)
			: als(0), BWR(new ::libmaus2::bambam::BamWriter(filename,bamheader,rewritebamlevel)), update_base(UNIQUE_PTR_MOVE(rupdate_base))
			{

			}

			~BamAlignmentInputCallbackBamRef()
			{
			}

			void operator()(::libmaus2::bambam::BamAlignment const & A)
			{
				als++;
				update_base->updatePosition(A);
				A.serialise(BWR->getStream());
			}

			update_base_type * getUpdateBase()
			{
				return update_base.get();
			}
		};
	}
}
#endif
