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


#if ! defined(FILEFRAGMENT_HPP)
#define FILEFRAGMENT_HPP

#include <set>
#include <fstream>
#include <libmaus/util/StringSerialisation.hpp>

namespace libmaus
{
        namespace aio
        {
		struct FileFragment
		{
		        typedef FileFragment this_type;
		        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		        typedef ::libmaus::util::unique_ptr< std::vector<this_type> > vector_ptr_type;
		
			std::string filename;
			uint64_t offset;
			uint64_t len;
			
			static std::vector<std::string> getFileNames(std::vector<FileFragment> const & V)
			{
			        std::set < std::string > S;
			        for ( uint64_t i = 0; i < V.size(); ++i )
			                S.insert(V[i].filename);
                                return std::vector<std::string>(S.begin(),S.end());
			}
			
			static void removeFiles(std::vector<FileFragment> const & V)
			{
			        std::vector<std::string> S = getFileNames(V);
			        for ( uint64_t i = 0; i < S.size(); ++i )
			                remove ( S[i].c_str() );
			}
			
			static uint64_t getTotalLength(std::vector<FileFragment> const & V)
			{
			        uint64_t len = 0;
			        for ( uint64_t i = 0; i < V.size(); ++i )
			                len += V[i].len;
                                return len;
			}

			std::string toString() const
			{
			        std::ostringstream ostr;
			        ostr << "FileFragment(" << filename << "," << offset << "," << len << ")";
			        return ostr.str();
			}
			
			static std::vector<FileFragment> loadVector(std::string const & filename)
			{
			        std::ifstream istr(filename.c_str());
			        assert ( istr.is_open() );
			        std::vector<FileFragment> V = deserialiseVector(istr);
			        assert ( istr );
			        istr.close();
			        return V;
			}

			void serialise(std::ostream & out) const
			{
				::libmaus::util::StringSerialisation::serialiseString(out,filename);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,offset);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,len);
			}
			
			static void serialiseVector(std::ostream & out, std::vector<FileFragment> const & V)
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,V.size());			
				for ( uint64_t i = 0; i < V.size(); ++i )
				        V[i].serialise(out);
			}
			
			static std::string serialiseVector(std::vector<FileFragment> const & V)
			{
			        std::ostringstream ostr;
			        serialiseVector(ostr,V);
			        return ostr.str();
			}
			
			static std::vector<FileFragment> deserialiseVector(std::istream & in)
			{
			        uint64_t const n = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
			        std::vector<FileFragment> V;
			        
			        for ( uint64_t i = 0; i < n; ++i )
			                V.push_back( FileFragment(in) );
			                
                                return V;
			}
			
			FileFragment()
			: filename("/dev/null"), offset(0), len(0) {}
			
			FileFragment(std::string const & rfilename, uint64_t const roffset, uint64_t const rlen)
			: filename(rfilename), offset(roffset), len(rlen) {}

			FileFragment(std::istream & in)
			:
			        filename(::libmaus::util::StringSerialisation::deserialiseString(in)),
			        offset(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
			        len(::libmaus::util::NumberSerialisation::deserialiseNumber(in))
			{}
		};        
        }
}
#endif
