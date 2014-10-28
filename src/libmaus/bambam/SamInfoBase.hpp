/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_SAMINFOBASEB_HPP)
#define LIBMAUS_BAMBAM_SAMINFOBASE_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/DigitTable.hpp>
#include <libmaus/util/AlphaDigitTable.hpp>
#include <libmaus/util/AlphaTable.hpp>
#include <libmaus/bambam/SamPrintableTable.hpp>
#include <libmaus/bambam/SamZPrintableTable.hpp>
#include <libmaus/math/DecimalNumberParser.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct SamInfoBase
		{
			static char const qnameValid[256];
			static char const rnameFirstValid[256];
			static char const rnameOtherValid[256];
			static char const seqValid[256];
			static char const qualValid[256];
			static libmaus::util::DigitTable const DT;
			static libmaus::util::AlphaDigitTable const ADT;
			static libmaus::util::AlphaTable const AT;
			static libmaus::bambam::SamPrintableTable const SPT;
			static libmaus::bambam::SamZPrintableTable const SZPT;
			static libmaus::math::DecimalNumberParser const DNP;
			
			typedef char const * c_ptr_type;
			typedef c_ptr_type c_ptr_type_pair[2];
			
			enum sam_info_base_field_status 
			{
				sam_info_base_field_undefined,
				sam_info_base_field_defined
			};

			static void parseStringField(
				c_ptr_type_pair field,
				sam_info_base_field_status & defined
			)
			{
				if ( field[1]-field[0] == 1 && field[0][0] == '*' )
					defined = sam_info_base_field_undefined;
				else
					defined = sam_info_base_field_defined;
			}

			#if 0
			static void parseStringField(
				c_ptr_type_pair field,
				libmaus::autoarray::AutoArray<char> & str,
				sam_info_base_field_status & defined
			)
			{
				size_t const fieldlen = field[1]-field[0];
				
				/* undefined by default */
				defined = sam_info_base_field_undefined;
				
				/* extend space if necessary */
				if ( !((fieldlen+1) < str.size()) )
					str = libmaus::autoarray::AutoArray<char>(fieldlen+1,false);
				
				if ( fieldlen == 1 && field[0][0] == '*' )
				{				
					str[0] = '*';
					str[1] = 0;
				}
				else
				{
					memcpy(str.begin(),field[0],fieldlen);
					str[fieldlen] = 0;
					defined = sam_info_base_field_defined;
				}				
			}
			#endif
			
			static int32_t parseNumberField(c_ptr_type_pair field, char const * fieldname)
			{
				char const * p = field[0];
				bool neg = false;
				uint32_t const fieldlen = field[1]-field[0];
				
				if ( fieldlen > 0 && p[0] == '-' )				
				{
					neg = true;
					p += 1;
				}
				
				if ( p == field[1] )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "libmaus::bambam::SamInfoBase: unable to parse " << std::string(field[0],field[1]) << " as number for " << fieldname << "\n";
					lme.finish();
					throw lme;				
				}

				int32_t num = 0;

				while ( p != field[1] )
					if ( DT[static_cast<uint8_t>(*p)] )
					{
						num *= 10;
						num += (*p-'0');
						++p;
					}
					else
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "libmaus::bambam::SamInfoBase: unable to parse " << std::string(field[0],field[1]) << " as number\n";
						lme.finish();
						throw lme;
					}
										
				return neg ? -num : num;
			}
		};
	}
}
#endif
