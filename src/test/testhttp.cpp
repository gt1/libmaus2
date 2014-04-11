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
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
