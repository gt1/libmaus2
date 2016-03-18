/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/aio/ArrayInputStream.hpp>

int main()
{
	try
	{
		std::string const s = "hello world";
		libmaus2::aio::ArrayInputStream<std::string::const_iterator> AIS(s.begin(),s.end());

		for ( uint64_t i = 0; i < s.size(); ++i )
			assert ( AIS.peek() != std::istream::traits_type::eof() && AIS.get() == s[i] );

		for ( uint64_t i = 0; i < s.size(); ++i )
		{
			AIS.clear();
			AIS.seekg(i);
			for ( uint64_t j = i; j < s.size(); ++j )
				assert ( AIS.peek() != std::istream::traits_type::eof() && AIS.get() == s[j] );
		}
	}
	catch(std::exception const & ex)
	{

	}
}
