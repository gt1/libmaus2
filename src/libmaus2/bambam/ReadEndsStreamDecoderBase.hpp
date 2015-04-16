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
#if ! defined(LIBMAUS2_BAMBAM_READENDSSTREAMDECODERBASE_HPP)
#define LIBMAUS2_BAMBAM_READENDSSTREAMDECODERBASE_HPP

#include <libmaus2/bambam/ReadEnds.hpp>
#include <libmaus2/lz/SnappyInputStream.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ReadEndsStreamDecoderBase
		{
			typedef ReadEndsStreamDecoderBase this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			std::vector< std::istream * > Vin;
			std::vector< std::istream * >::iterator Vinnext;
			libmaus2::lz::SnappyInputStream::unique_ptr_type pSIS;
			
			bool setupNextStream()
			{
				if ( Vinnext != Vin.end() )
				{
					libmaus2::lz::SnappyInputStream::unique_ptr_type tSIS(new libmaus2::lz::SnappyInputStream(**(Vinnext++)));
					pSIS = UNIQUE_PTR_MOVE(tSIS);
					return true;					
				}
				else
				{
					return false;
				}
			}
			
			ReadEndsStreamDecoderBase(std::istream & rin) 
			: Vin(1,&rin), Vinnext(Vin.begin())
			{
				setupNextStream();
			}

			ReadEndsStreamDecoderBase(std::vector<std::istream *> const & rVin) 
			: Vin(rVin), Vinnext(Vin.begin())
			{
				setupNextStream();
			}

			bool get(libmaus2::bambam::ReadEnds & RE)
			{
				while ( true )
				{
					if ( expect_false(pSIS->peek() < 0) )
					{
						bool const ok = setupNextStream();
						if ( !ok )
							return false;				
					}
					else
					{
						RE.reset();
						RE.get(*pSIS);
						return true;		
					}
				}
			}
		};
	}
}
#endif
