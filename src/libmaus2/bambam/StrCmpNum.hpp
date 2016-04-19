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
#if ! defined(LIBMAUS2_BAMBAM_STRCMPNUM_HPP)
#define LIBMAUS2_BAMBAM_STRCMPNUM_HPP

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct StrCmpNum
		{
			//! digit table (digit_table[i] == true iff isdigit(i)==true)
			static uint8_t const digit_table[256];

			/**
			 * compare strings a and b as described in class description
			 *
			 * @param a first string
			 * @param ae pointer beyond end of first string
			 * @param b second string
			 * @param ae pointer beyond end of second string
			 * @return -1 if a<b, 0 if a==b, 1 if a>b (names at a and b, not pointers)
			 **/
			static int strcmpnum(
				uint8_t const * a,
				uint8_t const * ae,
				uint8_t const * b,
				uint8_t const * be
			)
			{
				uint8_t const * sa = a;
				uint8_t const * sb = b;

				while ( (a!=ae) && (b!=be) )
				{
					uint8_t const ca = *a;
					uint8_t const cb = *b;

					if ( digit_table[ca] && digit_table[cb] )
					{
						uint64_t na = ca - '0';
						uint64_t nb = cb - '0';

						while ( (*(++a)) && digit_table[*a] )
						{
							na *= 10;
							na += *a - '0';
						}
						while ( (*(++b)) && digit_table[*b] )
						{
							nb *= 10;
							nb += *b - '0';
						}

						if ( na != nb )
						{
							if ( na < nb )
								return -1;
							else
								return 1;
						}
					}
					else if ( ca != cb )
					{
						if ( ca < cb )
							return -1;
						else
							return 1;
					}
					else
					{
						++a;
						++b;
					}
				}

				if ( a == ae && b == be )
				{
					while ( sa != ae && sb != be )
					{
						uint8_t const ca = *(sa++);
						uint8_t const cb = *(sb++);

						if ( ca != cb )
						{
							if ( ca < cb )
								return -1;
							else
								return 1;
						}
					}

					if ( sa == ae && sb == be )
						return 0;
					else if ( sa == ae )
						return -1;
					else
						return 1;
				}
				else if ( a == ae )
					return -1;
				else
					return 1;
			}

			/**
			 * compare strings a and b as described in class description
			 *
			 * @param a first string
			 * @param b second string
			 * @return -1 if a<b, 0 if a==b, 1 if a>b (names at a and b, not pointers)
			 **/
			static int strcmpnum(
				char const * a,
				char const * ae,
				char const * b,
				char const * be
			)
			{
				return strcmpnum(
					reinterpret_cast<uint8_t const *>(a),
					reinterpret_cast<uint8_t const *>(ae),
					reinterpret_cast<uint8_t const *>(b),
					reinterpret_cast<uint8_t const *>(be)
				);
			}

			/**
			 * compare strings a and b as described in class description
			 *
			 * @param a first string
			 * @param b second string
			 * @return -1 if a<b, 0 if a==b, 1 if a>b (names at a and b, not pointers)
			 **/
			static int strcmpnum(uint8_t const * a, uint8_t const * b)
			{
				uint8_t const * sa = a;
				uint8_t const * sb = b;

				while ( *a && *b )
				{
					uint8_t const ca = *a;
					uint8_t const cb = *b;

					if ( digit_table[ca] && digit_table[cb] )
					{
						uint64_t na = ca - '0';
						uint64_t nb = cb - '0';

						while ( (*(++a)) && digit_table[*a] )
						{
							na *= 10;
							na += *a - '0';
						}
						while ( (*(++b)) && digit_table[*b] )
						{
							nb *= 10;
							nb += *b - '0';
						}

						if ( na != nb )
						{
							if ( na < nb )
								return -1;
							else
								return 1;
						}
					}
					else if ( ca != cb )
					{
						if ( ca < cb )
							return -1;
						else
							return 1;
					}
					else
					{
						++a;
						++b;
					}
				}

				if ( (!*a) && (!*b) )
				{
					while ( *sa && *sb )
					{
						uint8_t const ca = *(sa++);
						uint8_t const cb = *(sb++);

						if ( ca != cb )
						{
							if ( ca < cb )
								return -1;
							else
								return 1;
						}
					}

					if ( !(*sa) && !(*sb) )
						return 0;
					else if ( !*sa )
						return -1;
					else
						return 1;
				}
				else if ( !*a )
					return -1;
				else
					return 1;
			}

			/**
			 * compare strings a and b as described in class description
			 *
			 * @param a first string
			 * @param b second string
			 * @return -1 if a<b, 0 if a==b, 1 if a>b (names at a and b, not pointers)
			 **/
			static int strcmpnum(char const * a, char const * b)
			{
				return strcmpnum(
					reinterpret_cast<uint8_t const *>(a),
					reinterpret_cast<uint8_t const *>(b)
				);
			}
		};
	}
}
#endif
