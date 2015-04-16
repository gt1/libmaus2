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
#if ! defined(GZIPHEADER_HPP)
#define GZIPHEADER_HPP

#include <libmaus/lz/StreamWrapper.hpp>
#include <libmaus/types/types.hpp>
#include <libmaus/lz/Inflate.hpp>
#include <istream>
#include <ostream>
#include <vector>
#include <string>

namespace libmaus
{
	namespace lz
	{
		struct GzipExtraData
		{
			uint8_t I1;
			uint8_t I2;
			uint16_t L;
			std::string data; 
			
			GzipExtraData()
			{
			}
			GzipExtraData(
				uint8_t const rI1,
				uint8_t const rI2,
				uint16_t const rL,
				std::string const & rdata)
			: I1(rI1), I2(rI2), L(rL), data(rdata) 
			{}
		};
		
		struct GzipHeaderConstantsBase
		{
			static uint8_t const ID1 = 0x1F;
			static uint8_t const ID2 = 0x8B;
			static uint8_t const CM = 8; // compression method: deflate
		
			static uint8_t const FTEXT = (1u << 0);
			static uint8_t const FHCRC = (1u << 1);
			static uint8_t const FEXTRA = (1u << 2);
			static uint8_t const FNAME = (1u << 3);
			static uint8_t const FCOMMENT = (1u << 4);
			static uint8_t const FRES0 = (1u<<5);
			static uint8_t const FRES1 = (1u<<6);
			static uint8_t const FRES2 = (1u<<7);
			static uint8_t const FRES  = FRES0|FRES1|FRES2;

			static uint8_t getByte(::std::istream & in, std::ostream * out = 0)
			{
				int const c = in.get();
				
				if ( c < 0 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::getByte(): reached unexpected end of file (broken file?)" << std::endl;
					se.finish();
					throw se;
				}
				
				if ( out )
					out->put(c);
				
				return c;
			}
			
			static bool checkGzipMagic(::std::istream & in)
			{
				// stream already bad?
				if ( ! in )
					return false;
					
				int const c0 = in.get();
				if ( c0 < 0 )
				{
					in.clear();	
					return false;
				}
				if ( c0 != ID1 )
				{
					in.putback(c0);
					return false;
				}
				
				int const c1 = in.get();
				if ( c1 < 0 )
				{
					in.clear();
					in.putback(c0);
					return false;
				}
				if ( c1 != ID2 )
				{
					in.putback(c1);
					in.putback(c0);
					return false;
				}
				
				assert ( c0 == ID1 );
				assert ( c1 == ID2 );

				in.putback(c1);
				in.putback(c0);
				return true;
			}
			
			static uint64_t getByteAsWord(::std::istream & in, std::ostream * out = 0)
			{
				return getByte(in,out);
			}
			
			static uint64_t getLEInteger(std::istream & in, unsigned int length, std::ostream * out = 0)
			{
				uint64_t v = 0;
				
				for ( unsigned i = 0; i < length; ++i )
					v |= getByteAsWord(in,out) << (8*i);
					
				return v;
			}
			
			template<typename out_stream_type>
			static void putLEInteger(out_stream_type & out, uint64_t v, unsigned int length)
			{
				for ( unsigned i = 0; i < length; ++i )
					out.put( static_cast<uint8_t>((v >> (i*8)) & 0xFF) );				
			}
			
			static void writeBGZFHeader(
				std::ostream & out, 
				uint64_t const payloadsize
			)
			{
				out.put(ID1); // ID
				out.put(ID2); // ID
				out.put(8);   // CM
				out.put(4);   // FLG, extra data
				putLEInteger(out,0,4); // MTIME
				out.put(0); // XFL
				out.put(255); // undefined OS
				putLEInteger(out,6,2); // xlen
				out.put('B');
				out.put('C');
				putLEInteger(out,2,2); // length of field
				uint64_t const blocksize = 18/*header*/+8/*footer*/+payloadsize-1;
				putLEInteger(out,blocksize,2); 
				// std::cerr << "Blocksize " << blocksize << std::endl;
				// std::cerr << "payload size " << payloadsize << std::endl;
				// block size - 1 (including header an footer)
			}

			static uint64_t writeSimpleHeader(std::ostream & out)
			{
				out.put(ID1); // ID
				out.put(ID2); // ID
				out.put(8);   // CM
				out.put(0);   // FLG, none
				putLEInteger(out,0,4); // MTIME
				out.put(0); // XFL
				out.put(255); // undefined OS
				
				// number of bytes written
				return 10;
			}
		};
	
		struct GzipHeader : public GzipHeaderConstantsBase
		{
			uint8_t FLG; // flags
			uint32_t MTIME; // modification time
			uint8_t XFL; // extra flags, 2 for maximum compression, 4 for fastest
			uint8_t OS; // operating system, 0xFF for unspecified
			
			uint16_t XLEN; // length of extra field
			
			std::string extradata;
			std::string filename; // original file name
			std::string comment; // comment
			
			uint16_t CRC16; // CRC
			
			std::vector < GzipExtraData > extradataVector;
			uint16_t bcblocksize;
						
			void init(std::istream & in)
			{
				uint8_t const FID1 = getByte(in);
				uint8_t const FID2 = getByte(in);
				
				if ( (FID1 != ID1) || (FID2 != ID2) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::init(): file starts with non GZIP magic, expected " 
						<< std::hex << static_cast<int>(ID1)  << ":" << static_cast<int>(ID2) << std::dec
						<< " got "
						<< std::hex << static_cast<int>(FID1) << ":" << static_cast<int>(FID2) << std::dec
						<< std::endl;
					se.finish();
					throw se;				
				}
				
				uint8_t const FCM = getByte(in);

				if ( FCM != CM )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::init(): unknown compression method " << static_cast<int>(FCM) << std::endl;
					se.finish();
					throw se;				
				}			
				
				FLG = getByte(in);
				
				if ( FLG & FRES )
				{
					std::cerr << "WARNING: gzip header has unknown flags set." << std::endl;
				}
				
				MTIME = getLEInteger(in,4);

				XFL = getByte(in);
				
				if ( (XFL != 0 && XFL != 2 && XFL != 4) )
				{
					std::cerr << "WARNING: gzip header has unknown XFL flag value " << 
						static_cast<int>(XFL) << std::endl;
				}
				
				OS = getByte(in);
				
				if ( (FLG & FEXTRA) )
				{
					uint64_t const xtralen = getLEInteger(in,2);					
					std::vector<uint8_t> vextra(xtralen);
					for ( uint64_t i = 0; i < xtralen; ++i )
						vextra[i] = getByte(in);
					extradata = std::string(vextra.begin(),vextra.end());
					
					#if 0
					std::istringstream eistr(extradata);
					
					while ( eistr )
					{
						int ii1 = eistr.get();
						
						if ( ii1 >= 0 )
						{
							uint8_t const i1 = ii1;
							uint8_t const i2 = getByte(eistr);
							uint16_t const l = getLEInteger(eistr,2);
							std::vector<uint8_t> V;
							for ( uint64_t i = 0; i < l; ++i )
								V.push_back(getByte(eistr));
							std::string const s(V.begin(),V.end());
							extradataVector.push_back(GzipExtraData(i1,i2,l,s));
					
							#if 0
							std::cerr << "extra data " 
								<< std::hex << static_cast<int>(i1) << std::dec
								<< std::hex << static_cast<int>(i2) << std::dec
								<< " len " << l << std::endl;
							#endif
							
							if ( i1 == 'B' && i2 == 'C' )
							{
								std::istringstream bcistr(s);
								bcblocksize = getLEInteger(bcistr,2);
						
								#if 0		
								std::cerr << "extra data " 
									<< std::hex << static_cast<int>(i1) << std::dec
									<< std::hex << static_cast<int>(i2) << std::dec
									<< " len " << l 
									<< " block size " << blocksize
									<< std::endl;
								#endif
							}
						}
					}										
					#endif
				}
				
				if ( (FLG & FNAME) )
				{
					std::vector<uint8_t> vfn;
					uint8_t sym;
					while ( (sym=getByte(in)) != 0 )
						vfn.push_back(sym);
					filename = std::string(vfn.begin(),vfn.end());	
					
					// std::cerr << "Got file name " << filename << std::endl;
				}

				if ( (FLG & FCOMMENT) )
				{
					std::vector<uint8_t> vcomment;
					uint8_t sym;
					while ( (sym=getByte(in)) != 0 )
						vcomment.push_back(sym);
					comment = std::string(vcomment.begin(),vcomment.end());
					
					// std::cerr << "Got comment " << comment << std::endl;	
				}
				
				if ( (FLG & FHCRC) )
				{
					getLEInteger(in,2); // read crc for header
				}
			}

			GzipHeader(std::istream & in)
			{
				init(in);
			}
			
			GzipHeader(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::GzipHeader(): failed to open file " << filename << std::endl;
					se.finish();
					throw se;				
				}
				init(istr);
			}
		};

		struct GzipHeaderSimple : public GzipHeaderConstantsBase
		{
			uint8_t FLG; // flags
			uint32_t MTIME; // modification time
			uint8_t XFL; // extra flags, 2 for maximum compression, 4 for fastest
			uint8_t OS; // operating system, 0xFF for unspecified
			uint16_t XLEN; // length of extra field
			uint16_t CRC16; // CRC			
			uint16_t bcblocksize;
						
			void init(std::istream & in)
			{
				uint8_t const FID1 = getByte(in);
				uint8_t const FID2 = getByte(in);
				
				if ( (FID1 != ID1) || (FID2 != ID2) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::init(): file starts with non GZIP magic, expected " 
						<< std::hex << static_cast<int>(ID1)  << ":" << static_cast<int>(ID2) << std::dec
						<< " got "
						<< std::hex << static_cast<int>(FID1) << ":" << static_cast<int>(FID2) << std::dec
						<< std::endl;
					se.finish();
					throw se;				
				}
				
				uint8_t const FCM = getByte(in);

				if ( FCM != CM )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::init(): unknown compression method " << static_cast<int>(FCM) << std::endl;
					se.finish();
					throw se;				
				}			
				
				FLG = getByte(in);
				
				if ( FLG & FRES )
				{
					std::cerr << "WARNING: gzip header has unknown flags set." << std::endl;
				}
				
				MTIME = getLEInteger(in,4);

				XFL = getByte(in);
				
				if ( (XFL != 0 && XFL != 2 && XFL != 4) )
				{
					std::cerr << "WARNING: gzip header has unknown XFL flag value " << 
						static_cast<int>(XFL) << std::endl;
				}
				
				OS = getByte(in);
				
				if ( (FLG & FEXTRA) )
				{
					uint64_t const xtralen = getLEInteger(in,2);
					for ( uint64_t i = 0; i < xtralen; ++i )
						getByte(in);
				}
				
				if ( (FLG & FNAME) )
				{
					while ( getByte(in) != 0 )
					{
					}
				}

				if ( (FLG & FCOMMENT) )
				{
					while ( getByte(in) != 0 )
					{
					
					}
				}
				
				if ( (FLG & FHCRC) )
				{
					getLEInteger(in,2); // read crc for header
				}
			}

			static void ignoreHeader(std::istream & in)
			{
				uint8_t FLG; // flags
				// uint32_t MTIME; // modification time
				uint8_t XFL; // extra flags, 2 for maximum compression, 4 for fastest
				// uint8_t OS; // operating system, 0xFF for unspecified
				
				uint8_t const FID1 = getByte(in);
				uint8_t const FID2 = getByte(in);
				
				if ( (FID1 != ID1) || (FID2 != ID2) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::init(): file starts with non GZIP magic, expected " 
						<< std::hex << static_cast<int>(ID1)  << ":" << static_cast<int>(ID2) << std::dec
						<< " got "
						<< std::hex << static_cast<int>(FID1) << ":" << static_cast<int>(FID2) << std::dec
						<< std::endl;
					se.finish();
					throw se;				
				}
				
				uint8_t const FCM = getByte(in);

				if ( FCM != CM )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::init(): unknown compression method " << static_cast<int>(FCM) << std::endl;
					se.finish();
					throw se;				
				}			
				
				FLG = getByte(in);
				
				if ( FLG & FRES )
				{
					std::cerr << "WARNING: gzip header has unknown flags set." << std::endl;
				}
				
				/* MTIME = */ getLEInteger(in,4);

				XFL = getByte(in);
				
				if ( (XFL != 0 && XFL != 2 && XFL != 4) )
				{
					std::cerr << "WARNING: gzip header has unknown XFL flag value " << 
						static_cast<int>(XFL) << std::endl;
				}
				
				/* OS = */ getByte(in);
				
				if ( (FLG & FEXTRA) )
				{
					uint64_t const xtralen = getLEInteger(in,2);
					for ( uint64_t i = 0; i < xtralen; ++i )
						getByte(in);
				}
				
				if ( (FLG & FNAME) )
				{
					while ( getByte(in) != 0 )
					{
					}
				}

				if ( (FLG & FCOMMENT) )
				{
					while ( getByte(in) != 0 )
					{
					
					}
				}
				
				if ( (FLG & FHCRC) )
				{
					getLEInteger(in,2); // read crc for header
				}
			}

			GzipHeaderSimple(std::istream & in)
			{
				init(in);
			}
			
			GzipHeaderSimple(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "GzipHeader::GzipHeader(): failed to open file " << filename << std::endl;
					se.finish();
					throw se;				
				}
				init(istr);
			}
		};
	}
}
#endif
