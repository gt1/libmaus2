/*
    libmaus2
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

#include <libmaus2/fastx/isFastQ.hpp>
#include <libmaus2/exception/LibMausException.hpp>

bool libmaus2::fastx::IsFastQ::isFastQ(std::istream & istr)
{
	int first = istr.get();
          
	if ( first >= 0 )
	{
		istr.unget();
	
		if ( first == '>' )
			return false;
		else if ( first == '@' )
			return true;
		else
		{
			::libmaus2::exception::LibMausException se;
			se.getStream() << "libmaus2::fastx::IsFastQ::isFastQ(std::istream &): Unable to determine type of pattern file.";
			se.finish();
			throw se;
		}
	}
	else
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::fastx::IsFastQ::isFastQ(std::istream &): Failed to read first character from pattern file..";
		se.finish();
		throw se;
	}	
}

bool libmaus2::fastx::IsFastQ::isFastQ(std::string const & filename)
{
	libmaus2::aio::InputStreamInstance istr(filename);
	return isFastQ(istr);
}

bool libmaus2::fastx::IsFastQ::isFastQ(std::vector<std::string> const & filenames)
{
	if ( ! filenames.size() )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::fastx::IsFastQ::isFastQ(): no filenames provided.";
		se.finish();
		throw se;
	}
	
	bool const isfq = isFastQ(filenames[0]);
	
	for ( uint64_t i = 1; i < filenames.size(); ++i )
	{
		bool const nisfq = isFastQ(filenames[i]);
		
		if ( nisfq != isfq )
		{
			::libmaus2::exception::LibMausException se;
			se.getStream() << "libmaus2::fastx::IsFastQ::isFastQ(): file type inconsistent (FastA and FastQ in same list)";
			se.finish();
			throw se;		
		}
	}
	
	return isfq;
}
