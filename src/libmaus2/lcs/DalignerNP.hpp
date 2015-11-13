/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LCS_DALIGNERNP_HPP)
#define LIBMAUS2_LCS_DALIGNERNP_HPP

#include <libmaus2/lcs/AlignmentTraceContainer.hpp>
#include <libmaus2/lcs/Aligner.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct DalignerNP : public Aligner, public AlignmentTraceContainer
		{
			typedef DalignerNP this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			#if defined(LIBMAUS2_HAVE_DALIGNER)
			libmaus2::autoarray::AutoArray<char> A;
			libmaus2::autoarray::AutoArray<char> B;
			void * data;
			#endif

			public:
			DalignerNP();
			~DalignerNP();
			void align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b);
			void alignPreMapped(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b);
			AlignmentTraceContainer const & getTraceContainer() const;
		};
	}
}
#endif
