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
#include <libmaus/network/OpenSSLSocket.hpp>
#include <libmaus/network/HttpBody.hpp>
#include <libmaus/network/HttpHeader.hpp>
#include <libmaus/util/ArgInfo.hpp>

int main(int argc, char * argv[])
{
	try
	{	
		libmaus::util::ArgInfo const arginfo(argc,argv);
				
		std::string const url = arginfo.getRestArg<std::string>(0);
		libmaus::network::HttpHeader preheader("HEAD","",url);
		int64_t const length = preheader.getContentLength();
		int64_t const packetsize = 
			(
				arginfo.hasArg("packetsize")
				&& 
				arginfo.getUnparsedValue("packetsize","").size() 
				&&
				isdigit(arginfo.getUnparsedValue("packetsize","")[0])
			)
			?
			arginfo.getValueUnsignedNumeric<uint64_t>("packetsize",2*1024)
			:
			arginfo.getValue<int64_t>("packetsize",2*1024);
		
		// if length is known and server supports range then read document in blocks of size 2048
		if ( length >= 0 && preheader.hasRanges() && packetsize > 0 )
		{
			uint64_t const packetsize = 2048;
			uint64_t const numpackets = (length + packetsize - 1)/packetsize;
			libmaus::autoarray::AutoArray<char> A(256,false);

			std::cerr << preheader.statusline << std::endl;
			
			for ( uint64_t p = 0; p < numpackets; ++p )
			{
				uint64_t const low = p * packetsize;
				uint64_t const high = std::min(low+packetsize,static_cast<uint64_t>(length));
				
				std::ostringstream addreqstr;
				addreqstr << "Range: bytes=" << low << "-" << (high-1) << "\r\n";
				std::string const addreq = addreqstr.str();

				libmaus::network::HttpHeader header("GET",addreq,url);
				libmaus::network::HttpBody body(header.getStream(),header.isChunked(),header.getContentLength());

				uint64_t n = 0;
				while ( (n = body.read(A.begin(),A.size())) != 0 )
					std::cout.write(A.begin(),n);				
			}
		}
		// otherwise read document in one go
		else
		{
			libmaus::network::HttpHeader header("GET","",url);
			std::cerr << header.statusline << std::endl;
			libmaus::network::HttpBody body(header.getStream(),header.isChunked(),header.getContentLength());
		
			libmaus::autoarray::AutoArray<char> A(256,false);
			uint64_t n = 0;
			while ( (n = body.read(A.begin(),A.size())) != 0 )
				std::cout.write(A.begin(),n);

		}

		std::cout.flush();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
