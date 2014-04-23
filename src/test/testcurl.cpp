
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
#include <libmaus/LibMausConfig.hpp>
#include <cstdlib>

#if !defined(LIBMAUS_HAVE_LIBCURL)
#include <iostream>

int main()
{
	std::cerr << "[D] libcurl support is not present." << std::endl;
	return EXIT_FAILURE;
}
#else
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/fastx/StreamFastAReader.hpp>

void countFastA()
{
	libmaus::fastx::StreamFastAReaderWrapper S(std::cin);
	libmaus::fastx::StreamFastAReaderWrapper::pattern_type pat;
	uint64_t const bs = 64*1024;
	uint64_t ncnt = 0;
	uint64_t rcnt = 0;
	while ( S.getNextPatternUnlocked(pat) )
	{
		std::string const & s = pat.spattern;
		
		uint64_t low = 0;
		while ( low != s.size() )
		{
			uint64_t const high = std::min(low+bs,static_cast<uint64_t>(s.size()));
			bool none = false;
			
			for ( uint64_t i = low; i != high; ++i )
				switch ( s[i] )
				{
					case 'A':
					case 'C':
					case 'G':
					case 'T':
						break;
					default:
						none = true;
				}
				
			if ( none )
				++ncnt;
			else
				++rcnt;
				
			low = high;
		}
		
		std::cerr << "[V] " << pat.sid << std::endl;
	}

	std::cerr << "ncnt=" << ncnt << std::endl;
	std::cerr << "rcnt=" << rcnt << std::endl;
}

#include <libmaus/network/CurlInit.hpp>
#include <libmaus/network/Socket.hpp>

int main()
{
	try
	{	
		std::string const host = "ngs.sanger.ac.uk";
		libmaus::network::ClientSocket CS(80,host.c_str());
		std::ostringstream reqastr;
		reqastr << "HEAD /production/ensembl/regulation/hg19/trackDb.txt HTTP/1.1\n";
		// reqastr << "HEAD / HTTP/1.1\n";
		reqastr << "Host: " << host << "\n\n";
		std::string const reqa = reqastr.str();

		// CS.setNonBlocking();

		CS.write(reqa.c_str(),reqa.size());
		char last4[4] = {0,0,0,0};
		bool done = false;

		std::ostringstream headstr;
		
		while ( !done )
		{
			char c[16];
			ssize_t const r = CS.readPart(&c[0],sizeof(c));
			
			for ( ssize_t i = 0; (!done) && i < r; ++i )
			{
				headstr.put(c[i]);
				
				last4[0] = last4[1];
				last4[1] = last4[2];
				last4[2] = last4[3];
				last4[3] = c[i];
			
				if ( last4[0] == '\r' && last4[1] == '\n' && last4[2] == '\r' && last4[3] == '\n' )
					done = true;
			}			
		}
		
		std::istringstream linestr(headstr.str());
		std::vector<std::string> lines;
		while ( linestr )
		{
			std::string line;
			std::getline(linestr,line);
			
			while ( line.size() && isspace(line[line.size()-1]) )
			{
				line = line.substr(0,line.size()-1);
			}
			
			if ( line.size() )
			{
				lines.push_back(line);
				std::cerr << "[" << lines.size()-1 << "]=" << lines.back() << std::endl;
			}
		}
		
		std::cerr << headstr.str();
	
		#if 0
		libmaus::network::CurlInit cinit;

		CURL * handle = curl_easy_init( );
	
		if ( ! handle )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "curl_easy_init failed" << std::endl;
			lme.finish();
			throw lme;
		}
		
		curl_easy_setopt(handle, CURLOPT_URL, "http://www.sanger.ac.uk/");
		// curl_easy_setopt(handle, CURLOPT_URL, "http://www.google.co.uk");
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);

		CURLcode const perfres = curl_easy_perform(handle);
		
		if(perfres != CURLE_OK)
		{
			curl_easy_cleanup(handle);
			
			libmaus::exception::LibMausException lme;
			lme.getStream() << "curl_easy_perform failed: " << curl_easy_strerror(perfres) << std::endl;
			lme.finish();
			throw lme;			
		}
		
		long httpcode;
		CURLcode getinfook = curl_easy_getinfo(handle,CURLINFO_RESPONSE_CODE,&httpcode);

		if(getinfook != CURLE_OK)
		{
			curl_easy_cleanup(handle);
			
			libmaus::exception::LibMausException lme;
			lme.getStream() << "curl_easy_getinfo failed: " << curl_easy_strerror(getinfook) << std::endl;
			lme.finish();
			throw lme;			
		}
		
		std::cerr << "response code " << httpcode << std::endl;
		
		char const * cp = 0;
		getinfook = curl_easy_getinfo(handle,CURLINFO_EFFECTIVE_URL,&cp);

		if(getinfook != CURLE_OK)
		{
			curl_easy_cleanup(handle);
			
			libmaus::exception::LibMausException lme;
			lme.getStream() << "curl_easy_getinfo failed: " << curl_easy_strerror(getinfook) << std::endl;
			lme.finish();
			throw lme;			
		}

		std::cerr << "effective url " << cp << std::endl;

		double length = 0;
		getinfook = curl_easy_getinfo(handle,CURLINFO_CONTENT_LENGTH_DOWNLOAD,&length);

		if(getinfook != CURLE_OK)
		{
			curl_easy_cleanup(handle);
			
			libmaus::exception::LibMausException lme;
			lme.getStream() << "curl_easy_getinfo failed: " << curl_easy_strerror(getinfook) << std::endl;
			lme.finish();
			throw lme;			
		}

		std::cerr << "length " << length << std::endl;		
		
		curl_easy_cleanup(handle);

		return EXIT_SUCCESS;
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
#endif
