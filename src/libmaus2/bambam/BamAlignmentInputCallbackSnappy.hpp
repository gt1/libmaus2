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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTINPUTCALLBACKSNAPPY_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTINPUTCALLBACKSNAPPY_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/lz/SnappyFileOutputStream.hpp>
#include <libmaus2/bambam/BamAlignmentInputPositionCallbackNull.hpp>
#include <libmaus2/bambam/CircularHashCollatingBamDecoder.hpp>

namespace libmaus2
{
	namespace bambam
	{
		template<typename _update_base_type = libmaus2::bambam::BamAlignmentInputPositionCallbackNull>
		struct BamAlignmentInputCallbackSnappy :
			public ::libmaus2::bambam::CollatingBamDecoderAlignmentInputCallback,
			public _update_base_type
		{
			typedef _update_base_type update_base_type;
			typedef BamAlignmentInputCallbackSnappy<update_base_type> this_type;

			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t als;
			::libmaus2::lz::SnappyFileOutputStream::unique_ptr_type SFOS;

			BamAlignmentInputCallbackSnappy(std::string const & filename, libmaus2::bambam::BamHeader const & bamheader)
			: update_base_type(bamheader), als(0), SFOS(new ::libmaus2::lz::SnappyFileOutputStream(filename))
			{

			}

			~BamAlignmentInputCallbackSnappy()
			{
				flush();
			}

			void operator()(::libmaus2::bambam::BamAlignment const & A)
			{
				als++;
				update_base_type::updatePosition(A);
				A.serialise(*SFOS);
			}

			void flush()
			{
				SFOS->flush();
			}
		};

		template<typename _update_base_type = libmaus2::bambam::BamAlignmentInputPositionCallbackNull>
		struct BamAlignmentInputCallbackSnappyRef :
			public ::libmaus2::bambam::CollatingBamDecoderAlignmentInputCallback
		{
			typedef _update_base_type update_base_type;
			typedef typename update_base_type::unique_ptr_type update_base_ptr_type;
			typedef BamAlignmentInputCallbackSnappyRef<update_base_type> this_type;

			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t als;
			::libmaus2::lz::SnappyFileOutputStream::unique_ptr_type SFOS;
			update_base_ptr_type update_base;

			BamAlignmentInputCallbackSnappyRef(std::string const & filename, libmaus2::bambam::BamHeader const & /* bamheader */, update_base_ptr_type & rupdate_base)
			: als(0), SFOS(new ::libmaus2::lz::SnappyFileOutputStream(filename)), update_base(UNIQUE_PTR_MOVE(rupdate_base))
			{

			}

			~BamAlignmentInputCallbackSnappyRef()
			{
				flush();
			}

			void operator()(::libmaus2::bambam::BamAlignment const & A)
			{
				als++;
				update_base->updatePosition(A);
				A.serialise(*SFOS);
			}

			void flush()
			{
				SFOS->flush();
			}

			update_base_type * getUpdateBase()
			{
				return update_base.get();
			}
		};
	}
}
#endif
