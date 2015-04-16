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
#if ! defined(LIBMAUS2_UTIL_SUCCINCTBORDERARRAY_HPP)
#define LIBMAUS2_UTIL_SUCCINCTBORDERARRAY_HPP

#include <libmaus2/rank/ERank222BAppendDynamic.hpp>

namespace libmaus2
{
	namespace util
	{
		struct SuccinctBorderArray
		{
			private:
			struct SuccinctBorderArrayAccessor
			{
				libmaus2::rank::ERank222BAppendDynamic & B;
				
				SuccinctBorderArrayAccessor(libmaus2::rank::ERank222BAppendDynamic & rB) : B(rB)
				{
				}
				
				uint64_t operator[](uint64_t const i) const
				{
					return i-B.rank0(B.select1(i));
				}
				
				void init()
				{
					B.appendBit(1);
				}
				
				void push(uint64_t const z)
				{
					for ( uint64_t k = 0; k < z; ++k )
						B.appendBit(0);
					B.appendBit(1);		
				}
			};

			libmaus2::rank::ERank222BAppendDynamic B;
			SuccinctBorderArrayAccessor S;
			uint64_t const n;
			
			/**
			 * construct border array (see CHL: Algorithms on strings)
			 **/
			template<typename iterator>
			void init(iterator s)
			{
				int64_t i = 0;
				S.init();
				
				for ( uint64_t j = 1; j < n; ++j )
				{
					while ( i >= 0 && s[j] != s[i] )
						if ( !i )
							i = -1;
						else
							i = S[i-1];
					
					++i;
					
					S.push(static_cast<uint64_t>(-((i-static_cast<int64_t>(S[j-1]))-1)));
				}	
			}
			
			public:
			template<typename iterator>
			SuccinctBorderArray(iterator s, uint64_t const rn) : B(), S(B), n(rn)
			{
				init(s);
			}

			SuccinctBorderArray(char const * s) : B(), S(B), n(strlen(s))
			{
				init(s);
			}

			SuccinctBorderArray(std::string const & s) : B(), S(B), n(s.size())
			{
				init(s.begin());
			}
			
			std::vector<uint64_t> getFrontSquares() const
			{
				std::vector<uint64_t> V;
				for ( uint64_t j = 1; j < n; ++j )
					if ( 2*S[j] == j+1 )
						V.push_back(S[j]);
				return V;
			}
			
			uint64_t operator[](uint64_t const i) const
			{
				return S[i];
			}
			
			uint64_t size() const
			{
				return n;
			}

			/**
			 * simple sanity check, not a check for correctness!
			 **/
			bool checkString(std::string const & s) const
			{
				bool ok = ( s.size() == size() );
				
				for ( uint64_t i = 0; ok && i < size(); ++i )
					ok = ok && ( s.substr(0,S[i]) == s.substr(i+1-S[i],S[i]) );
					
				return ok;
			}
			
			/**
			 * call checkString function for given string
			 **/
			static bool check(std::string const & s)
			{
				libmaus2::util::SuccinctBorderArray SBA(s);
				return SBA.checkString(s);
			}

			/**
			 * run check for Fibonacci strings up to (including) length n
			 **/
			static bool checkFibonacci(uint64_t const n)
			{
				bool ok = true;
				
				std::map<uint64_t,std::string> M;
				M[0] = "b";
				M[1] = "a";
					
				for ( uint64_t i = 0; i <= n; ++i )
				{
					if ( i > 1 )
						M[i] = M[i-1]+M[i-2];

					ok = ok && check(M[i]);
				}
		
				return ok;
			}
		};
	}
}

std::ostream & operator<<(std::ostream & out, libmaus2::util::SuccinctBorderArray const & S);
#endif
