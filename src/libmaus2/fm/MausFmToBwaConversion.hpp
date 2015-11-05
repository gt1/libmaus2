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
#if ! defined(LIBMAUS2_FM_MAUSFMTOBWACONVERSION_HPP)
#define LIBMAUS2_FM_MAUSFMTOBWACONVERSION_HPP

#include <string>
#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace fm
	{
		struct MausFmToBwaConversion
		{
			private:
			/*
			 * load cumulative symbol frequences for 5 symbol alphabet (4 regular symbols + 1 terminator symbol)
			 */
			static ::libmaus2::autoarray::AutoArray<uint64_t> loadL2T(std::string const & infn);

			/**
			 * load cumulative symbol frequences for 5 symbol alphabet (1 terminator symbol + 4 regular symbols)
			 * and remove terminator symbol
			 **/
			static ::libmaus2::autoarray::AutoArray<uint64_t> loadL2(std::string const & infn);

			/**
			 * load ISA[0]
			 **/
			static uint64_t loadPrimary(std::string const & inisa);

			static void rewriteBwt(std::string const & infn, std::ostream & out);
			static void rewriteSa(std::string const & infn, std::ostream & out);

			public:
			static void rewrite(std::string const & inbwt, std::string const & outbwt, std::string const & outsa);
		};
	}
}
#endif
