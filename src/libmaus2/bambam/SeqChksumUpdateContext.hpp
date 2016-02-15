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
#if ! defined(LIBMAUS2_BAMBAM_BAMSEQCHKSUMUPDATECONTEXT_HPP)
#define LIBMAUS2_BAMBAM_BAMSEQCHKSUMUPDATECONTEXT_HPP

#include <libmaus2/math/UnsignedInteger.hpp>
#include <libmaus2/digest/Digests.hpp>

namespace libmaus2
{
	namespace bambam
	{
		template<typename _digest_type>
		struct SeqChksumUpdateContext
		{
			typedef _digest_type digest_type;

			digest_type ctx_name_flags_seq;
			uint8_t ctx_name_flags_seq_digest[digest_type::digestlength];
			libmaus2::math::UnsignedInteger<digest_type::digestlength/4> name_flags_seq_digest;

			digest_type ctx_flags_seq;
			uint8_t ctx_flags_seq_digest[digest_type::digestlength];
			libmaus2::math::UnsignedInteger<digest_type::digestlength/4> flags_seq_digest;

			digest_type ctx_flags_seq_qual;
			uint8_t ctx_flags_seq_qual_digest[digest_type::digestlength];
			libmaus2::math::UnsignedInteger<digest_type::digestlength/4> flags_seq_qual_digest;

			digest_type ctx_flags_seq_tags;
			uint8_t ctx_flags_seq_tags_digest[digest_type::digestlength];
			libmaus2::math::UnsignedInteger<digest_type::digestlength/4> flags_seq_tags_digest;

			bool pass;
			bool valid;
		};

		typedef SeqChksumUpdateContext<libmaus2::digest::Null> NullSeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::CRC32> CRC32SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::util::MD5> MD5SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA1> SHA1SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA2_224> SHA2_224_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA2_256> SHA2_256_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA2_256_sse4> SHA2_256_sse4_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA2_384> SHA2_384_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA2_512> SHA2_512_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA2_512_sse4> SHA2_512_sse4_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::MurmurHash3_x64_128> MurmurHash3_x64_128_SeqChksumUpdateContext;
		typedef SeqChksumUpdateContext<libmaus2::digest::SHA3_256> SHA3_256_SeqChksumUpdateContext;
	}
}
#endif
