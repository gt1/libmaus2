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
#if ! defined(LIBMAUS2_BAMBAM_CHECKSUMSINTERFACE_HPP)
#define LIBMAUS2_BAMBAM_CHECKSUMSINTERFACE_HPP

#include <libmaus2/bambam/BamAlignment.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ChecksumsInterface
		{
			typedef ChecksumsInterface this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			virtual ~ChecksumsInterface() {}
			virtual void update(libmaus2::bambam::BamAlignment const & algn) = 0;
			virtual void update(uint8_t const * D, uint32_t const blocksize) = 0;
			virtual void update(ChecksumsInterface const & IO) = 0;
			virtual void printVerbose(std::ostream & log, uint64_t const c,libmaus2::bambam::BamAlignment const & algn, double const etime) = 0;
			virtual void printChecksums(std::ostream & out) = 0;
			virtual void printChecksumsForBamHeader(std::ostream & out) = 0;
			virtual void get_b_seq_all(std::vector<uint8_t> & H) const = 0;
			virtual void get_name_b_seq_all(std::vector<uint8_t> & H) const = 0;
			virtual void get_b_seq_qual_all(std::vector<uint8_t> & H) const = 0;
			virtual void get_b_seq_tags_all(std::vector<uint8_t> & H) const = 0;
			virtual void get_b_seq_pass(std::vector<uint8_t> & H) const = 0;
			virtual void get_name_b_seq_pass(std::vector<uint8_t> & H) const = 0;
			virtual void get_b_seq_qual_pass(std::vector<uint8_t> & H) const = 0;
			virtual void get_b_seq_tags_pass(std::vector<uint8_t> & H) const = 0;
		};
	}
}
#endif
