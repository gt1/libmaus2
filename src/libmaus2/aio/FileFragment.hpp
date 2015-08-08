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


#if ! defined(FILEFRAGMENT_HPP)
#define FILEFRAGMENT_HPP

#include <set>
#include <fstream>
#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/util/IntervalTree.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

namespace libmaus2
{
        namespace aio
        {
        	/**
        	 * class storing a file fragment descriptor (filename, offset, length)
        	 **/
		struct FileFragment
		{
			//! this type
		        typedef FileFragment this_type;
		        //! unique pointer type
		        typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		        //! pointer to a vector of this type
		        typedef ::libmaus2::util::unique_ptr< std::vector<this_type> > vector_ptr_type;
		
		        //! name of file
			std::string filename;
			//! offset in file
			uint64_t offset;
			//! length of fragment
			uint64_t len;
			
			/**
			 * extract interval vector from vector of FileFragment objects
			 *
			 * @param V fragment vector
			 * @return interval vector
			 **/
			static libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > toIntervalVector(std::vector<FileFragment> const & V)
			{
				libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > H(V.size());
				
				if ( V.size() )
				{
					H[0] = std::pair<uint64_t,uint64_t>(0,V[0].len);
					
					for ( uint64_t i = 1; i < V.size(); ++i )
						H[i] = std::pair<uint64_t,uint64_t>(H[i-1].second,H[i-1].second + V[i].len);
				}
					
				return H;
			}
			
			/**
			 * extract interval tree from vector of FileFragment objects
			 *
			 * @param V fragment vector
			 * @return interval tree
			 **/
			static libmaus2::util::IntervalTree::unique_ptr_type toIntervalTree(std::vector<FileFragment> const & V)
			{
				if ( ! V.size() )
				{
					libmaus2::util::IntervalTree::unique_ptr_type ptr;
					return UNIQUE_PTR_MOVE(ptr);
				}
				
				libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > H = toIntervalVector(V);
				libmaus2::util::IntervalTree::unique_ptr_type PIT(new libmaus2::util::IntervalTree(H,0,H.size()));
				
				return UNIQUE_PTR_MOVE(PIT);
			}
			
			/**
			 * extract list of file names from a fragment vector
			 *
			 * @param V fragment vector
			 * @return list of file names in V
			 **/
			static std::vector<std::string> getFileNames(std::vector<FileFragment> const & V)
			{
			        std::set < std::string > S;
			        for ( uint64_t i = 0; i < V.size(); ++i )
			                S.insert(V[i].filename);
                                return std::vector<std::string>(S.begin(),S.end());
			}
			
			/**
			 * remove/delete the files in V
			 *
			 * @param V vector of fragments
			 **/
			static void removeFiles(std::vector<FileFragment> const & V)
			{
			        std::vector<std::string> S = getFileNames(V);
			        for ( uint64_t i = 0; i < S.size(); ++i )
			                libmaus2::aio::FileRemoval::removeFile ( S[i] );
			}
			
			/**
			 * get total length of fragments in vector V
			 *
			 * @param V vector of file fragments
			 * @return sum over the length fields of the elements of V
			 **/
			static uint64_t getTotalLength(std::vector<FileFragment> const & V)
			{
			        uint64_t len = 0;
			        for ( uint64_t i = 0; i < V.size(); ++i )
			                len += V[i].len;
                                return len;
			}

			/**
			 * @return string representation of this object
			 **/
			std::string toString() const
			{
			        std::ostringstream ostr;
			        ostr << "FileFragment(" << filename << "," << offset << "," << len << ")";
			        return ostr.str();
			}
			
			/**
			 * load a serialised vector of FileFragments from file named filename
			 *
			 * @param filename name of file containing a serialised file fragment vector
			 * @return deserialised vector
			 **/
			static std::vector<FileFragment> loadVector(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance CIS(filename);
			        std::vector<FileFragment> V = deserialiseVector(CIS);
			        return V;
			}

			/**
			 * serialise file fragment to output stream out
			 *
			 * @param out output stream
			 **/
			void serialise(std::ostream & out) const
			{
				::libmaus2::util::StringSerialisation::serialiseString(out,filename);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,offset);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,len);
			}
			
			/**
			 * serialise vector of file fragments to output stream out
			 *
			 * @param out output stream
			 * @param V vector of file fragments
			 **/
			static void serialiseVector(std::ostream & out, std::vector<FileFragment> const & V)
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,V.size());			
				for ( uint64_t i = 0; i < V.size(); ++i )
				        V[i].serialise(out);
			}
			
			/**
			 * serialise vector of file fragments to a string
			 *
			 * @param V vector of file fragments
			 * @return string containing the serialised file fragment vector
			 **/
			static std::string serialiseVector(std::vector<FileFragment> const & V)
			{
			        std::ostringstream ostr;
			        serialiseVector(ostr,V);
			        return ostr.str();
			}
			
			/**
			 * deserialise a vector of file fragments
			 *
			 * @param in input stream
			 * @return deserialised vector of file fragments
			 **/
			static std::vector<FileFragment> deserialiseVector(std::istream & in)
			{
			        uint64_t const n = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			        std::vector<FileFragment> V;
			        
			        for ( uint64_t i = 0; i < n; ++i )
			                V.push_back( FileFragment(in) );
			                
                                return V;
			}
			
			/**
			 * construct empty file fragment
			 **/
			FileFragment() : filename("/dev/null"), offset(0), len(0) {}
			/**
			 * constructor from parameter list
			 *
			 * @param rfilename file name
			 * @param roffset fragment offset
			 * @param rlen fragment length
			 **/
			FileFragment(std::string const & rfilename, uint64_t const roffset, uint64_t const rlen)
			: filename(rfilename), offset(roffset), len(rlen) {}

			/**
			 * constructor deserialising object from input stream
			 *
			 * @param in input stream
			 **/
			FileFragment(std::istream & in)
			:
			        filename(::libmaus2::util::StringSerialisation::deserialiseString(in)),
			        offset(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			        len(::libmaus2::util::NumberSerialisation::deserialiseNumber(in))
			{}
		};        
        }
}
#endif
