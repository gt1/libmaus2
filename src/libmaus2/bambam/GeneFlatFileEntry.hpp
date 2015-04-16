/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_GENEFLATFILEENTRY_HPP)
#define LIBMAUS2_BAMBAM_GENEFLATFILEENTRY_HPP

#include <cassert>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace bambam
	{
		
		struct GeneFlatFileEntry
		{
			std::pair<char const *, char const *> geneName;
			std::pair<char const *, char const *> name;
			std::pair<char const *, char const *> chrom;
			char strand;
			uint64_t txStart;
			uint64_t txEnd;
			uint64_t cdsStart;
			uint64_t cdsEnd;
			std::vector < std::pair<uint64_t,uint64_t> > exons;
			
			template<typename iterator>
			static uint64_t parseNumberUnsigned(iterator ita, iterator ite)
			{				
				if ( ! (ite-ita) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "GeneFlatFileEntry: cannot parse " << std::string(ita,ite) << " as number (empty field)\n";
					lme.finish();
					throw lme;				
				}

				uint64_t v = 0;
				
				iterator its = ita;
				while ( ita != ite )
				{
					unsigned char const c = *(ita++);
					
					if ( ! isdigit(c) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "GeneFlatFileEntry: cannot parse " << std::string(its,ite) << " as number\n";
						lme.finish();
						throw lme;		
					}
					
					v *= 10;
					v += c-'0';
				}
				
				return v;
			}
			
			static int64_t parseNumber(std::string const & s)
			{
				uint64_t i = 0;
				
				bool const neg = s.size() && s[0] == '-';
				
				if ( neg )
					++i;
					
				if ( i == s.size() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "GeneFlatFileEntry: cannot parse " << s << " as number\n";
					lme.finish();
					throw lme;					
				}
				
				int64_t v = 0;
				
				while ( i < s.size() )
				{
					if ( ! isdigit(static_cast<unsigned char>(s[i])) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "GeneFlatFileEntry: cannot parse " << s << " as number\n";
						lme.finish();
						throw lme;		
					}

					v *= 10;
					v += s[i++] - '0';
				}
				
				return neg ? -v : v;
			}
			
			static std::string checkStringNotEmpty(std::string const & s)
			{
				if ( s.size() )
				{
					return s;
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "GeneFlatFileEntry: empty field" << std::endl;
					lme.finish();
					throw lme;						
				}
			}

			static char checkChar(std::string const & s)
			{
				if ( s.size() == 1 )
				{
					return s[0];
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "GeneFlatFileEntry: field " << s << " is not a single character" << std::endl;
					lme.finish();
					throw lme;						
				}
			}
			
			static char checkStrand(std::string const & s)
			{
				char const c = checkChar(s);
				
				switch ( c )
				{
					case '+': case '-': return c;
					default:
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "GeneFlatFileEntry: string " << s << " is not valid for the strand column" << std::endl;
						lme.finish();
						throw lme;							
					}
				}
			}
						
			GeneFlatFileEntry() 
			: geneName(), name(), chrom(), strand('?'), txStart(-1), txEnd(-1), cdsStart(-1), cdsEnd(-1), exons() {}
			
			void reset(char const * s, char const * se)
			{
				char const * const sa = s;
				unsigned int col = 0;
				
				while ( s != se )
				{
					char const * sc = s;
					while ( sc != se && *sc != '\t' )
						++sc;
					
					switch ( col++ )
					{
						case 0:
							geneName = std::pair<char const *, char const *>(s,sc);
							break;
						case 1:
							name = std::pair<char const *, char const *>(s,sc);
							break;
						case 2:
							chrom = std::pair<char const *, char const *>(s,sc);
							break;
						case 3:
						{
							if ( sc-s != 1 || (*s != '+' && *s != '-') )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "GeneFlatFileEntry: column is not a valid strand: " << std::string(s,sc) << "\n";
								lme.finish();
								throw lme;	
							}
							
							strand = *s;
							break;
						}
						case 4:
							txStart = parseNumberUnsigned(s,sc);
							break;
						case 5:
							txEnd = parseNumberUnsigned(s,sc);
							break;
						case 6:
							cdsStart = parseNumberUnsigned(s,sc);
							break;
						case 7:
							cdsEnd = parseNumberUnsigned(s,sc);
							break;
						case 8:
							exons.resize(parseNumberUnsigned(s,sc));
							break;
						case 9:
						{
							char const * nc = s;
							char const * ne = sc;
							uint64_t ncols = 0;
							
							while ( ncols < exons.size() && nc != ne )
							{
								char const * nt = nc;
								while ( nt != ne && *nt != ',' )
									++nt;
								exons[ncols++].first = parseNumberUnsigned(nc,nt);
								
								if ( nt == ne )
									nc = nt;
								else
								{
									assert ( *nt == ',' );
									nc = nt+1;
								}
							}
							
							if ( ncols != exons.size() )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "GeneFlatFileEntry: column does not contains sufficient number of fields: " << std::string(s,sc) << "\n";
								lme.finish();
								throw lme;								
							}
							break;
						}
						case 10:
						{
							char const * nc = s;
							char const * ne = sc;
							uint64_t ncols = 0;
							
							while ( ncols < exons.size() && nc != ne )
							{
								char const * nt = nc;
								while ( nt != ne && *nt != ',' )
									++nt;
								exons[ncols++].second = parseNumberUnsigned(nc,nt);
								
								if ( nt == ne )
									nc = nt;
								else
								{
									assert ( *nt == ',' );
									nc = nt+1;
								}
							}
							
							if ( ncols != exons.size() )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "GeneFlatFileEntry: column does not contains sufficient number of fields: " << std::string(s,sc) << "\n";
								lme.finish();
								throw lme;								
							}
							break;
						}
					}	
					
					if ( sc != se )
					{
						assert ( *sc == '\t' );
						++sc;
					}
						
					s = sc;
				}
				
				if ( col < 11 )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "GeneFlatFileEntry: line does not have sufficient columns: "	<< std::string(sa,se) << "\n";
					lme.finish();
					throw lme;
				}
			}
			
			GeneFlatFileEntry(char const * s, char const * se)
			{
				reset(s,se);
			}
		};

		std::ostream & operator<<(std::ostream & out, GeneFlatFileEntry const & O);
	}
}
#endif
