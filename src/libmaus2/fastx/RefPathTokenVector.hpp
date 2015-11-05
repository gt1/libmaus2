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
#if ! defined(LIBMAUS2_FASTX_REFPATHTOKENVECTOR_HPP)
#define LIBMAUS2_FASTX_REFPATHTOKENVECTOR_HPP

#include <libmaus2/fastx/RefPathToken.hpp>
#include <vector>
#include <limits>
#include <sstream>

namespace libmaus2
{
	namespace fastx
	{
		struct RefPathTokenVector
		{
			std::vector<RefPathToken> V;

			RefPathTokenVector(std::string const & s = std::string())
			{
				uint64_t i = 0;

				while ( i < s.size() )
				{
					if ( s[i] == '%' )
					{
						std::ostringstream ostr;
						ostr.put(s[i++]);

						// get numbers after %
						std::ostringstream numostr;
						while ( i < s.size() && isdigit(static_cast<unsigned char>(s[i])) )
							numostr.put(s[i++]);

						// is next character an s?
						if ( i < s.size() && s[i] == 's' )
						{
							// step over 's'
							i += 1;

							// number before s
							if ( numostr.str().size() )
							{
								std::string nums = numostr.str();
								size_t v = 0;
								for ( uint64_t j = 0; j < nums.size(); ++j )
								{
									v *= 10;
									v += nums[j]-'0';
								}
								V.push_back(RefPathToken(v));
							}
							// no number before s
							else
							{
								V.push_back(RefPathToken(std::numeric_limits<size_t>::max()));
							}
						}
						else
						{
							// copy numbers
							ostr << numostr.str();

							// copy next symbol
							if ( i < s.size() )
								ostr.put(s[i++]);

							if ( V.size() && V.back().token == RefPathToken::path_token_literal )
								V.back().s += ostr.str();
							else
								V.push_back(RefPathToken(ostr.str()));
						}
					}
					else
					{
						std::ostringstream ostr;
						while ( i < s.size() && s[i] != '%' )
							ostr.put(s[i++]);

						V.push_back(RefPathToken(ostr.str()));
					}
				}
			}

			std::string expand(std::string const & s) const
			{
				std::ostringstream ostr;

				uint64_t j = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					if ( V[i].token == RefPathToken::path_token_literal )
					{
						ostr << V[i].s;
					}
					else
					{
						for ( uint64_t k = 0; k < V[i].n && j < s.size(); ++k )
							ostr.put(s[j++]);
					}
				}

				// append rest of s
				if ( j != s.size() )
				{
					// append slash if string is non empty but not ending on '/'
					if ( (ostr.str().size()) && (*(ostr.str().rbegin())) != '/' )
						ostr.put('/');
					ostr << s.substr(j);
				}

				return ostr.str();
			}
		};

		std::ostream & operator<<(std::ostream & out, libmaus2::fastx::RefPathTokenVector const & O);
	}
}
#endif
