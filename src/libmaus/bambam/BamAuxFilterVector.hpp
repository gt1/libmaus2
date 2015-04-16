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
#if ! defined(LIBMAUS_BAMBAM_BAMAUXFILTERVECTOR_HPP)
#define LIBMAUS_BAMBAM_BAMAUXFILTERVECTOR_HPP

#include <libmaus/bitio/BitVector.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/stringFunctions.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAuxFilterVector
		{
			typedef BamAuxFilterVector this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			libmaus::bitio::BitVector B;
			
			size_t byteSize() const
			{
				return B.byteSize();
			}

			static libmaus::bambam::BamAuxFilterVector::unique_ptr_type parseAuxFilterList(libmaus::util::ArgInfo const & arginfo)
			{
				libmaus::bambam::BamAuxFilterVector::unique_ptr_type pfilter;
				
				if ( arginfo.hasArg("auxfilter") )
				{
					libmaus::bambam::BamAuxFilterVector::unique_ptr_type tfilter(
						new libmaus::bambam::BamAuxFilterVector
					);

					std::string const filterlist = arginfo.getUnparsedValue("auxfilter","");
					std::deque<std::string> tokens = libmaus::util::stringFunctions::tokenize<std::string>(filterlist,std::string(","));
					
					for ( uint64_t i = 0; i < tokens.size(); ++i )
					{
						if ( tokens[i].size() != 2 )
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "Malformed tag name " << tokens[i] << std::endl;
							se.finish();
							throw se;
						}
						
						tfilter->set(tokens[i]);
					}
					
					pfilter = UNIQUE_PTR_MOVE(tfilter);
				}
				
				return UNIQUE_PTR_MOVE(pfilter);
			}
			
			BamAuxFilterVector() : B(256*256)  {}
			BamAuxFilterVector(char const * const * tags) : B(256*256) 
			{
				while ( *tags )
				{
					uint8_t const * c = reinterpret_cast<uint8_t const *>(*(tags++));
					assert ( c[0] );
					assert ( c[1] );
					assert ( ! c[2] );
					set(c[0],c[1]);
				}
			}
			BamAuxFilterVector(std::vector<std::string> const & tags) : B(256*256) 
			{
				for ( uint64_t i = 0; i < tags.size(); ++i )
					set(tags[i]);
			}
			template<typename iterator>
			BamAuxFilterVector(iterator ita, iterator ite) : B(256*256) 
			{
				for ( ; ita != ite ; ++ita )
					set(*ita);
			}
						
			void set(std::string const & s)
			{
				if ( s.size() == 2 )
				{
					uint8_t const * c = reinterpret_cast<uint8_t const *>(s.c_str());
					set(c[0],c[1]);
				}
				else
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BamAuxFilterVector called for string which is not of length 2: " << s << std::endl;
					se.finish();
					throw se;
				}
			}
			
			void set(uint8_t const ca, uint8_t const cb)
			{
				B.set( 
					(static_cast<uint64_t>(ca) << 8)
					|
					(static_cast<uint64_t>(cb) << 0),
					1
				);
			}

			void clear(uint8_t const ca, uint8_t const cb)
			{
				B.set( 
					(static_cast<uint64_t>(ca) << 8)
					|
					(static_cast<uint64_t>(cb) << 0),
					0
				);
			}
			
			bool operator()(uint8_t const ca, uint8_t const cb) const
			{
				return B[(static_cast<uint64_t>(ca) << 8) | (static_cast<uint64_t>(cb) << 0)];
			}
		};
	}
}
#endif
