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

#if ! defined(COMPACTFASTDECODERBASE_HPP)
#define COMPACTFASTDECODERBASE_HPP

#include <libmaus/util/utf8.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>
#include <libmaus/fastx/CompactFastTerminator.hpp>

#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/PutObject.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/fastx/CompactFastEncoder.hpp>

namespace libmaus
{
	namespace fastx
	{			
		struct CompactFastDecoderBase : public ::libmaus::util::UTF8, public CompactFastTerminator
		{
			::libmaus::parallel::SynchronousCounter<uint64_t> nextid;
			
			CompactFastDecoderBase() : nextid(0) {}
			
			// get length of terminator
			static uint64_t getTermLength(uint32_t const v = getTerminator())
			{
				std::ostringstream ostr;
				encodeUTF8(v,ostr);
				return ostr.str().size();
			}

			template<typename in_type>
			inline int64_t skipPattern(
				in_type & istr, 
				uint64_t & patlencodelen, 
				uint64_t & flagcodelen, 
				uint64_t & datalen
			)
			{
				uint32_t const patlen = decodeUTF8(istr,patlencodelen);
							
				if ( patlen == getTerminator() )
				{
					return -1;
				}
				else
				{
					uint32_t const flags = decodeUTF8(istr,flagcodelen);
					uint32_t const skip = (flags&1)?((patlen+1)/2):(patlen+3)/4;
					
					for ( uint64_t i = 0; i < skip; ++i )
						istr.get();

					datalen += skip;

					return patlen;
				}
			}

			template<typename in_type>
			inline int64_t skipPattern(
				in_type & istr, 
				uint64_t & codelen)
			{
				// decode pattern length
				uint32_t const patlen = decodeUTF8(istr,codelen);
							
				if ( patlen == getTerminator() )
				{
					return -1;
				}
				else
				{
					// decode flags
					uint32_t const flags = decodeUTF8(istr,codelen);
					// compute length of encoded pattern
					uint32_t const skip = (flags&1)?((patlen+1)/2):(patlen+3)/4;
					
					// skip pattern data
					for ( uint64_t i = 0; i < skip; ++i )
						istr.get();

					codelen += skip;

					return patlen;
				}
			}

			template<typename in_type>
			inline int64_t skipPattern(in_type & istr)
			{
				uint32_t const patlen = decodeUTF8(istr);
							
				if ( patlen == getTerminator() )
				{
					return -1;
				}
				else
				{
					uint32_t const flags = decodeUTF8(istr);
					uint32_t const skip = (flags&1)?((patlen+1)/2):(patlen+3)/4;
					
					for ( uint64_t i = 0; i < skip; ++i )
						istr.get();

					return patlen;
				}
			}
			

			template<typename pattern_type, typename in_type>
			static bool decode(
				pattern_type & pattern, in_type & istr,
				::libmaus::parallel::SynchronousCounter<uint64_t> & nextid
			)
			{
				try
				{
					// decode pattern length
					uint32_t const patlen = decodeUTF8(istr);
					
					// check for terminator
					if ( patlen == getTerminator() )
					{
						return false;
					}
					else
					{
						// decode flags
						uint32_t const flags = decodeUTF8(istr);
						
						// resize pattern
						try
						{
							pattern.spattern.resize(patlen);
						}
						catch(std::exception const & ex)
						{
							std::cerr << "exception while resizing pattern to length " << patlen << " in CompactFastDecoderBase::decode(): " << ex.what() << std::endl;
							throw;
						}
						
						// pattern has indeterminate bases
						if ( flags & 1 )
						{
							uint64_t const full = (patlen >> 1);		
							uint64_t const brok = patlen&1;
							std::string::iterator ita = pattern.spattern.begin();
							
							for ( uint64_t i = 0; i < full; ++i )
							{
								int v = istr.get();
								if ( v < 0 )
								{
									::libmaus::exception::LibMausException se;
									se.getStream() << "EOF in getNextPatternUnlocked()";
									se.finish();
									throw se;
								}

								*(ita++) = v & 0xF; v >>= 4;
								*(ita++) = v & 0xF; v >>= 4;
							}
							
							if ( brok )
							{
								int v = istr.get();
								if ( v < 0 )
								{
									::libmaus::exception::LibMausException se;
									se.getStream() << "EOF in getNextPatternUnlocked()";
									se.finish();
									throw se;
								}

								*(ita++) = v & 0xF; v >>= 4;				
							}
						}
						// pattern has determinate bases only
						else
						{
							// full bytes
							uint64_t const full = (patlen >> 2);		
							// fractional rest
							uint64_t const brok = patlen&3;
							std::string::iterator ita = pattern.spattern.begin();
							
							// decode full bytes (four symbols each)
							for ( uint64_t i = 0; i < full; ++i )
							{
								int v = istr.get();
								if ( v < 0 )
								{
									::libmaus::exception::LibMausException se;
									se.getStream() << "EOF in getNextPatternUnlocked()";
									se.finish();
									throw se;
								}

								*(ita++) = v & 0x3; v >>= 2;
								*(ita++) = v & 0x3; v >>= 2;
								*(ita++) = v & 0x3; v >>= 2;
								*(ita++) = v & 0x3; v >>= 2;
							}
							
							// decode fractional
							if ( brok == 3 )
							{
								int v = istr.get();
								if ( v < 0 )
								{
									::libmaus::exception::LibMausException se;
									se.getStream() << "EOF in getNextPatternUnlocked()";
									se.finish();
									throw se;
								}

								*(ita++) = v & 0x3; v >>= 2;
								*(ita++) = v & 0x3; v >>= 2;
								*(ita++) = v & 0x3; v >>= 2;
							}
							else if ( brok == 2 )
							{
								int v = istr.get();
								if ( v < 0 )
								{
									::libmaus::exception::LibMausException se;
									se.getStream() << "EOF in getNextPatternUnlocked()";
									se.finish();
									throw se;
								}

								*(ita++) = v & 0x3; v >>= 2;
								*(ita++) = v & 0x3; v >>= 2;
							}
							else if ( brok == 1 )
							{
								int v = istr.get();
								if ( v < 0 )
								{
									::libmaus::exception::LibMausException se;
									se.getStream() << "EOF in getNextPatternUnlocked()";
									se.finish();
									throw se;
								}

								*(ita++) = v & 0x3; v >>= 2;
							}
						}
								
						pattern.patlen = patlen;	
						pattern.unmapSource();
						pattern.pattern = pattern.spattern.c_str();			
						pattern.patid = nextid++;
						
						return true;
					}
				}
				catch(std::exception const & ex)
				{
					std::cerr << "caught exception in CompactFastDecoderBase::decode(): " << ex.what() << std::endl;
					throw;
				}
			}

			template<typename pattern_type, typename in_type>
			bool decode(
				pattern_type & pattern, in_type & istr
			)
			{
				return decode(pattern,istr,nextid);
			}
		};
	}
}
#endif
