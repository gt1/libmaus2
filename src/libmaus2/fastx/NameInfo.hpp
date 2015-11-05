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
#if ! defined(LIBMAUS2_FASTX_NAMEINFO_HPP)
#define LIBMAUS2_FASTX_NAMEINFO_HPP

#include <libmaus2/fastx/NameInfoBase.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct NameInfo
		{
			uint8_t const * name;
			size_t namelength;
			bool ispair;
			bool isfirst;
			uint64_t gl;
			uint64_t gr;
			NameInfoBase::fastq_name_scheme_type const namescheme;

			NameInfo() : name(0), namelength(0), ispair(false), isfirst(false), gl(0), gr(0), namescheme(NameInfoBase::fastq_name_scheme_generic) {}
			NameInfo(uint8_t const * rname, size_t rnamelength,
				libmaus2::fastx::SpaceTable const & ST,
				NameInfoBase::fastq_name_scheme_type const rnamescheme
			)
			: name(rname), namelength(rnamelength), ispair(false), isfirst(true), gl(0), gr(namelength), namescheme(rnamescheme)
			{
				switch ( namescheme )
				{
					case NameInfoBase::fastq_name_scheme_generic:
					{
						uint64_t l = 0;
						// skip space at start
						while ( l != namelength && ST.spacetable[name[l]] )
							++l;
						if ( l != namelength )
						{
							// skip non space symbols
							uint64_t r = l;
							while ( (r != namelength) && (!ST.spacetable[name[r]]) )
								++r;

							// check whether this part of the name ends on /1 or /2
							if ( r-l >= 2 && name[r-2] == '/' && (name[r-1] == '1' || name[r-1] == '2') )
							{
								if ( name[r-1] == '2' )
									isfirst = false;

								gl = l;
								gr = r;
								ispair = true;
							}
							else
							{
								gl = l;
								gr = r;
								ispair = false;
							}

							l = r;
						}

						break;
					}
					case NameInfoBase::fastq_name_scheme_casava18_single:
					case NameInfoBase::fastq_name_scheme_casava18_paired_end:
					{
						uint64_t l = 0;

						while ( l != namelength && ST.spacetable[name[l]] )
							++l;

						uint64_t const l0 = l;

						while ( l != namelength && ! ST.spacetable[name[l]] )
							++l;

						uint64_t const l0l = l-l0;

						while ( l != namelength && ST.spacetable[name[l]] )
							++l;

						uint64_t const l1 = l;

						while ( l != namelength && ! ST.spacetable[name[l]] )
							++l;

						uint64_t const l1l = l-l1;

						// count number of colons
						uint64_t l0c = 0;
						for ( uint64_t i = l0; i != l0+l0l; ++i )
							l0c += (name[i] == ':');
						uint64_t l1c = 0;
						for ( uint64_t i = l1; i != l1+l1l; ++i )
							l1c += (name[i] == ':');

						if ( l0c != 6 || l1c != 3 )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "malformed read name " << name << " (wrong number of colon separated fields) for name scheme " << namescheme << std::endl;
							se.finish();
							throw se;
						}

						gl = l0;
						gr = l0+l0l;

						if ( namescheme == NameInfoBase::fastq_name_scheme_casava18_single )
						{
							ispair = false;
						}
						else
						{
							ispair = true;

							uint64_t fragid = 0;
							unsigned int fragidlen = 0;
							uint64_t p = l1;
							while ( isdigit(name[p]) )
							{
								fragid *= 10;
								fragid += name[p++]-'0';
								fragidlen++;
							}

							if ( (! fragidlen) || (fragid<1) || (fragid>2) || name[p] != ':' )
							{
								::libmaus2::exception::LibMausException se;
								se.getStream() << "malformed read name " << name << " (malformed fragment id) for name scheme" << namescheme << std::endl;
								se.finish();
								throw se;
							}

							isfirst = (fragid==1);
						}

						break;
					}
					case NameInfoBase::fastq_name_scheme_pairedfiles:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "pairedfiles name scheme is not supported in NameInfo" << std::endl;
						se.finish();
						throw se;
						break;
					}
				}
			}

			std::pair<uint8_t const *, uint8_t const *> getName() const
			{
				switch ( namescheme )
				{
					case NameInfoBase::fastq_name_scheme_generic:
						if ( ispair )
							return std::pair<uint8_t const *, uint8_t const *>(name+gl,name+gr-2);
						else
							return std::pair<uint8_t const *, uint8_t const *>(name+gl,name+gr);
						break;
					case NameInfoBase::fastq_name_scheme_casava18_single:
					case NameInfoBase::fastq_name_scheme_casava18_paired_end:
						return std::pair<uint8_t const *, uint8_t const *>(name+gl,name+gr);
						break;
					default:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "NameInfo::getName(): invalid name scheme" << std::endl;
						se.finish();
						throw se;
					}
				}
			}
		};
	}
}
#endif
