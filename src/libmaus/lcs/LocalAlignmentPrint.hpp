/*
    libmaus
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
#if ! defined(LIBMAUS_LCS_LOCALALIGNMENTPRINT_HPP)
#define LIBMAUS_LCS_LOCALALIGNMENTPRINT_HPP

#include <libmaus/lcs/LocalBaseConstants.hpp>
#include <libmaus/lcs/LocalEditDistanceResult.hpp>
#include <iostream>
#include <sstream>

namespace libmaus
{
	namespace lcs
	{
		struct LocalAlignmentPrint : public LocalBaseConstants
		{
			virtual ~LocalAlignmentPrint() {}
		
			static std::string stepToString(step_type const s)
			{
				switch ( s )
				{
					case STEP_MATCH: return "+";
					case STEP_MISMATCH: return "-";
					case STEP_DEL: return "D";
					case STEP_INS: return "I";
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
						case STEP_DEL:
							out << (*ita++);
							break;
						case STEP_INS:
							out << " ";
							// ita++;
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
						case STEP_INS:
							out << (*itb++);
							break;
						case STEP_DEL:
							out << " ";
							// ita++;
							break;
					}
				}
				out << std::endl;
				
				return out;
			}
		
			template<
				typename a_iterator,
				typename b_iterator,
				typename alignment_iterator
			>
			static std::ostream & printAlignmentLines(
				std::ostream & out,
				a_iterator ita,
				a_iterator aend,
				b_iterator itb,
				b_iterator bend,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte
			)
			{
				std::ostringstream astr;
								
				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_DEL:
							if ( ita == aend )
								std::cerr << "accessing a beyond end." << std::endl;
							astr << (*ita++);
							break;
						case STEP_INS:
							astr << " ";
							// ita++;
							break;
					}
				}
				astr << std::string(ita,aend);
				
				std::ostringstream bstr;
				// out << std::string(SPR.aclip,' ') << std::endl;

				for ( alignment_iterator ta = rta; ta != rte; ++ta )
				{
					switch ( *ta )
					{
						case STEP_MATCH:
						case STEP_MISMATCH:
						case STEP_INS:
							if ( itb == bend )
								std::cerr << "accessing b beyond end." << std::endl;
							bstr << (*itb++);
							break;
						case STEP_DEL:
							bstr << " ";
							// ita++;
							break;
					}
				}
				bstr << std::string(itb,bend);
				
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

			template<
				typename a_iterator,
				typename b_iterator,
				typename alignment_iterator
			>
			static std::ostream & printAlignmentLines(
				std::ostream & out,
				a_iterator ita,
				a_iterator aend,
				b_iterator itb,
				b_iterator bend,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte,
				libmaus::lcs::LocalEditDistanceResult const & LED
			)
			{
				return printAlignmentLines(out,ita+LED.a_clip_left,aend-LED.a_clip_right,itb+LED.b_clip_left,bend-LED.b_clip_right,rlinewidth,rta,rte);
			}

			template<
				typename a_type,
				typename b_type,
				typename alignment_iterator
			>
			static std::ostream & printAlignmentLines(
				std::ostream & out,
				a_type const & a,
				b_type const & b,
				uint64_t const rlinewidth,
				alignment_iterator const rta,
				alignment_iterator const rte,
				libmaus::lcs::LocalEditDistanceResult const & LED
			)
			{
				return printAlignmentLines(out,a.begin()+LED.a_clip_left,a.end()-LED.a_clip_right,b.begin()+LED.b_clip_left,b.end()-LED.b_clip_right,rlinewidth,rta,rte);
			}
		};
	}
}
#endif
