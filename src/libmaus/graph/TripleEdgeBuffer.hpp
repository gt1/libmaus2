/**
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
**/

#if ! defined(TRIPLEEDGEBUFFER_HPP)
#define TRIPLEEDGEBUFFER_HPP

#include <libmaus/graph/TripleEdgeOperations.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <libmaus/util/TempFileNameGenerator.hpp>
#include <vector>
#include <string>
#include <sstream>

namespace libmaus
{
	namespace graph
	{
		/**
		 * buffer for one part, containing levels of files for this part
		 **/
		struct TripleEdgeBuffer
		{
			std::vector < std::vector < std::string > > filenames;
			::libmaus::parallel::OMPLock filenamelock;
			// std::string const filenameprefix;
			::libmaus::util::TempFileNameGenerator & tmpgen;
			uint64_t nextid;

			TripleEdgeBuffer(::libmaus::util::TempFileNameGenerator & rtmpgen)
			: tmpgen(rtmpgen), nextid(0)
			{

			}
			~TripleEdgeBuffer()
			{
				// flush();
			}
			
			std::string flush()
			{
				return finalMerge();
			}
			
			std::string getNextFileName()
			{
				return tmpgen.getFileName();
			}

			std::string finalMerge()
			{
				std::vector < std::string > rest;
				
				for ( unsigned int i = 0; i < filenames.size(); ++i )
					for ( unsigned int j = 0; j < filenames[i].size(); ++j )
						rest.push_back(filenames[i][j]);
				filenames.resize(0);

				std::string const outputfilename = getNextFileName();
				TripleEdgeOperations::multiwayMergeFiles ( rest, outputfilename );
				
				for ( uint64_t i = 0; i < rest.size(); ++i )
					remove ( rest[i].c_str() );
				
				return outputfilename;
			}

			void pushLevel()
			{
				filenames.push_back ( std::vector < std::string > () );
			}

			void registerFileName(std::string const & filename)
			{
				// std::cerr << "Sorting and registering " << filename << std::endl;
				TripleEdgeOperations::sortFile(filename);
				registerFileName(filename,0);
			}

			void registerFileName(std::string const & filename, uint64_t const level)
			{
				// std::cerr << "Registering " << filename << " at level " << level << std::endl;

				bool needmerge = false;
				std::string filenamea, filenameb;
				std::string outputfilename;

				filenamelock.lock();
				
				if ( ! (level < filenames.size()) )
					pushLevel();

				filenames[level].push_back(filename);

				if ( filenames[level].size() > 1 )
				{
					assert ( filenames[level].size() == 2 );
					needmerge = true;

					outputfilename = getNextFileName();
					filenamea = filenames[level][0];
					filenameb = filenames[level][1];
					filenames[level].pop_back();
					filenames[level].pop_back();
					assert ( filenames[level].size() == 0 );
				}

				filenamelock.unlock();

				if ( needmerge )
				{
					TripleEdgeOperations::mergeFiles(filenamea,filenameb,outputfilename);
					remove ( filenamea.c_str() );
					remove ( filenameb.c_str() );
					/*
					std::cerr << "Registering " << outputfilename << " from merging " <<
						filenamea << " and " << filenameb << std::endl;
					*/
					registerFileName(outputfilename,level+1);
				}
			}
		};
	}
}
#endif
