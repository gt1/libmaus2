/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_NNPALIGNRESULT_HPP)
#define LIBMAUS2_LCS_NNPALIGNRESULT_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>
#include <istream>
#include <libmaus2/exception/LibMausException.hpp>
#include <cassert>
#include <libmaus2/math/IntegerInterval.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NNPAlignResult
		{
			uint64_t abpos;
			uint64_t aepos;
			uint64_t bbpos;
			uint64_t bepos;
			uint64_t dif;

			NNPAlignResult(
				uint64_t rabpos = 0,
				uint64_t raepos = 0,
				uint64_t rbbpos = 0,
				uint64_t rbepos = 0,
				uint64_t rdif = 0
			) : abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos), dif(rdif)
			{

			}

			libmaus2::math::IntegerInterval<int64_t> getAInterval() const
			{
				return libmaus2::math::IntegerInterval<int64_t>(abpos,aepos-1);
			}

			libmaus2::math::IntegerInterval<int64_t> getBInterval() const
			{
				return libmaus2::math::IntegerInterval<int64_t>(bbpos,bepos-1);
			}

			static void expect(std::istream & in, std::string const & s)
			{
				uint64_t i = 0;
				while ( i < s.size() )
				{
					int const c = in.peek();

					if ( c == std::istream::traits_type::eof() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "NNPAlignResult::expect(): got EOF while expecting " << s << std::endl;
						lme.finish();
						throw lme;
					}
					else if ( c != s[i] )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "NNPAlignResult::expect(): got wrong character while expecting " << s << std::endl;
						lme.finish();
						throw lme;
					}

					int const cd = in.get();
					assert ( cd == s[i] );

					i += 1;
				}
			}

			static std::string getUntil(std::istream & in, char const term)
			{
				int c;
				std::ostringstream ostr;

				while (
					(c = in.peek()) != std::istream::traits_type::eof() &&
					(c != term)
				)
				{
					ostr.put(c);
					int const cd = in.get();
					assert ( cd == c );
				}

				if ( c == std::istream::traits_type::eof() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "NNPAlignResult::getUntil(): got EOF while expecting " << term << std::endl;
					lme.finish();
					throw lme;
				}

				return ostr.str();
			}

			static uint64_t getUnsignedIntegerUntil(std::istream & in, char const term)
			{
				std::string const s = getUntil(in,term);
				std::istringstream istr(s);
				uint64_t u;
				istr >> u;

				if ( (! istr) || istr.peek() != std::istream::traits_type::eof() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "NNPAlignResult::getUnsignedIntegerUntil(): unable to parse " << s << std::endl;
					lme.finish();
					throw lme;
				}

				return u;
			}

			static uint64_t getDoubleUntil(std::istream & in, char const term)
			{
				std::string const s = getUntil(in,term);
				std::istringstream istr(s);
				double d;
				istr >> d;

				if ( (! istr) || istr.peek() != std::istream::traits_type::eof() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "NNPAlignResult::getUnsignedIntegerUntil(): unable to parse " << s << std::endl;
					lme.finish();
					throw lme;
				}

				return d;
			}

			static NNPAlignResult parse(std::istream & in)
			{
				expect(in,"NNPAlignResult(");
				NNPAlignResult res;

				res.abpos = getUnsignedIntegerUntil(in,','); expect(in,",");
				res.aepos = getUnsignedIntegerUntil(in,','); expect(in,",");
				res.bbpos = getUnsignedIntegerUntil(in,','); expect(in,",");
				res.bepos = getUnsignedIntegerUntil(in,','); expect(in,",");
				res.dif = getUnsignedIntegerUntil(in,','); expect(in,",");
				getDoubleUntil(in,')'); expect(in,")");

				return res;
			}

			double getErrorRate() const
			{
				return (static_cast<double>(dif) / static_cast<double>(aepos-abpos));
			}

			void shiftA(int64_t const s)
			{
				abpos = static_cast<uint64_t>(static_cast<int64_t>(abpos) + s);
				aepos = static_cast<uint64_t>(static_cast<int64_t>(aepos) + s);
			}

			void shiftB(int64_t const s)
			{
				bbpos = static_cast<uint64_t>(static_cast<int64_t>(bbpos) + s);
				bepos = static_cast<uint64_t>(static_cast<int64_t>(bepos) + s);
			}

			void shift(int64_t const s)
			{
				shiftA(s);
				shiftB(s);
			}

			void shift(int64_t const sa, int64_t const sb)
			{
				shiftA(sa);
				shiftB(sb);
			}

			int64_t getScore() const
			{
				return static_cast<int64_t>(aepos-abpos)-static_cast<int64_t>(dif);
			}
		};

		std::ostream & operator<<(std::ostream & out, NNPAlignResult const & O);
	}
}
#endif
