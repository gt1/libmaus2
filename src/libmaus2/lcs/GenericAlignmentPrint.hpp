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

#if ! defined(GENERICALIGNMENTPRINT_HPP)
#define GENERICALIGNMENTPRINT_HPP

#include <libmaus2/lcs/BaseConstants.hpp>
#include <iostream>
#include <sstream>

namespace libmaus2
{
	namespace lcs
	{
		struct GenericAlignmentPrint : public BaseConstants
		{
			virtual ~GenericAlignmentPrint() {}

			static std::string stepToString(step_type const s)
			{
				switch ( s )
				{
					case STEP_MATCH: return "+";
					case STEP_MISMATCH: return "-";
					case STEP_INS: return "I";
					case STEP_DEL: return "D";
					case STEP_RESET: return "R";
					default: return "?";
				}
			}

			template<typename alignment_iterator>
			static std::ostream & printTrace(
				std::ostream & out,
				alignment_iterator const rta,
				alignment_iterator const rte,
				uint64_t const offset = 0
			)
			{
				out << std::string(offset,' ');
				for ( alignment_iterator tc = rta; tc != rte; ++tc )
					out << stepToString(*tc);
				return out;
			}

			template<typename string_iterator, typename alignment_iterator>
			static std::ostream & printAlignment(
				std::ostream & out,
				string_iterator ita,
				string_iterator itb,
				alignment_iterator const rta,
				alignment_iterator const rte
			)
			{
				printTrace(out,rta,rte);
				out << std::endl;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_INS:
							out << (*ita++);
							break;
						case STEP_DEL:
							out << " ";
							// ita++;
							break;
						case STEP_RESET:
							break;
					}
				}
				out << std::endl;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_DEL:
							out << (*itb++);
							break;
						case STEP_INS:
							out << " ";
							// ita++;
							break;
						case STEP_RESET:
							break;
					}
				}
				out << std::endl;

				return out;
			}

			template<typename alignment_iterator>
			static std::ostream & printAlignmentLines(
				std::ostream & out, std::string const & a, std::string const & b,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte
			)
			{
				std::ostringstream astr;

				std::string::const_iterator ita = a.begin();

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_INS:
							if ( ita == a.end() )
								std::cerr << "accessing a beyond end." << std::endl;
							astr << (*ita++);
							break;
						case STEP_DEL:
							astr << " ";
							// ita++;
							break;
						case STEP_RESET:
							break;
					}
				}
				astr << std::string(ita,a.end());

				std::ostringstream bstr;
				// out << std::string(SPR.aclip,' ') << std::endl;

				std::string::const_iterator itb = b.begin();

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_DEL:
							if ( itb == b.end() )
								std::cerr << "accessing b beyond end." << std::endl;
							bstr << (*itb++);
							break;
						case STEP_INS:
							bstr << " ";
							// ita++;
							break;
						case STEP_RESET:
							break;
					}
				}
				bstr << std::string(itb,b.end());

				std::ostringstream cstr;
				printTrace(cstr,rta,rte);

				std::string const aa = astr.str();
				std::string const ba = bstr.str();
				std::string const ca = cstr.str();
				uint64_t const linewidth = rlinewidth-2;
				uint64_t const numlines = (std::max(aa.size(),ba.size()) + linewidth-1) / linewidth;

				for ( uint64_t i = 0; i < numlines; ++i )
				{
					uint64_t pl = i*linewidth;

					out << "A ";
					if ( pl < aa.size() )
					{
						uint64_t const alen = std::min(linewidth,aa.size()-pl);
						out << aa.substr(pl,alen);
					}
					out << std::endl;

					out << "B ";
					if ( pl < ba.size() )
					{
						uint64_t const blen = std::min(linewidth,ba.size()-pl);
						out << ba.substr(pl,blen);
					}
					out << std::endl;

					out << "  ";
					if ( pl < ca.size() )
					{
						uint64_t const clen = std::min(linewidth,ca.size()-pl);
						out << ca.substr(pl,clen);
					}
					out << std::endl;
				}

				return out;
			}
		};
	}
}
#endif
