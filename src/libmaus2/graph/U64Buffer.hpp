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
#if ! defined(U64BUFFER_HPP)
#define U64BUFFER_HPP

#include <libmaus2/graph/TripleEdgeOutput.hpp>
#include <libmaus2/graph/TripleEdgeOutputMerge.hpp>
#include <libmaus2/graph/TripleEdgeInput.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <sstream>
#include <deque>

namespace libmaus2
{
	namespace graph
	{
		struct U64Buffer
		{
			typedef U64Buffer this_type;
			typedef ::libmaus2::util::unique_ptr < this_type >::type unique_ptr_type;

			std::string const kmerfilename;
			unsigned int fileidx;

			uint64_t const buffersize;
			uint64_t const bufferbytes;
			uint64_t const buffersperfile;
			uint64_t curfilebuffers;

			typedef ::libmaus2::graph::TripleEdgeOutput output_type;
			typedef ::libmaus2::util::unique_ptr < output_type > :: type ouput_ptr_type;
			output_ptr_type W;

			std::string getFileName(uint64_t const idx) const
			{
				std::ostringstream ostr;
				ostr << kmerfilename << "_" << idx;
				return ostr.str();
			}

			std::string getNextFileName()
			{
				return getFileName(fileidx++);
			}

			void openNextFile()
			{
				if ( W.get() )
					W->flush();
				W = output_ptr_type(new output_type( getNextFileName(), buffersize ));
				curfilebuffers = 0;
			}

			U64Buffer(std::string const & rkmerfilename, uint64_t const rbuffersize, uint64_t const maxmem)
			: kmerfilename(rkmerfilename), fileidx(0), 
				buffersize(rbuffersize),
				bufferbytes(buffersize*sizeof(::libmaus2::graph::TripleEdge)),
				buffersperfile( (maxmem+(bufferbytes-1))/bufferbytes )
			{
				openNextFile();
			}
			~U64Buffer()
			{
				close();
			}
			void flush()
			{
				if ( W.get() )
					W->flush();
			}
			void close()
			{
				flush();
				W.reset();
			}
			void write(::libmaus2::graph::TripleEdge const & T)
			{
				if ( W->write ( T ) )
				{
					if ( curfilebuffers++ >= buffersperfile )
						openNextFile();
				}
			}

			uint64_t getFileLength(uint64_t const idx) const
			{
				std::ifstream istr(getFileName(idx).c_str());
				istr.seekg(0,std::ios::end);
				uint64_t const l = istr.tellg();
				istr.close();
				return l;
			}

			void sortFile(uint64_t const idx) const
			{
				uint64_t const len = getFileLength(idx);
				assert ( len % sizeof(::libmaus2::graph::TripleEdge) == 0 );
				uint64_t const numtriples = len / sizeof(::libmaus2::graph::TripleEdge);
				std::string const filename = getFileName(idx);
				std::ifstream istr(filename.c_str(), std::ios::binary);
				::libmaus2::autoarray::AutoArray< ::libmaus2::graph::TripleEdge> T(numtriples);
				istr.read ( reinterpret_cast<char *>(T.get()), len);
				assert ( istr );
				assert ( istr.gcount() == static_cast<int64_t>(len) );
				istr.close(); 
				
				std::sort ( T.get(), T.get() + numtriples );

				std::ofstream ostr(filename.c_str(), std::ios::binary);
				ostr.write( reinterpret_cast<char const *>(T.get()), len );
				assert ( ostr );
				ostr.flush();
				assert ( ostr );
				ostr.close();
			}

			void sortFiles() const
			{
				for ( uint64_t i = 0; i < fileidx; ++i )
					sortFile(i);
			}

			std::string mergeFiles()
			{
				std::deque < uint64_t > Q;
				for ( uint64_t i = 0; i < fileidx; ++i )
					Q.push_back(i);

				while ( Q.size() > 1 )
				{
					std::deque<uint64_t> NQ;

					for ( uint64_t i = 0; i+1 < Q.size(); i += 2 )
					{
						NQ.push_back ( mergeFiles(Q[i+0],Q[i+1]) );

						remove ( getFileName ( Q[i+0] ) . c_str() );
						remove ( getFileName ( Q[i+1] ) . c_str() );
					}
					if ( Q.size() % 2 )
						NQ.push_back( Q.back() );

					Q = NQ;
				}

				assert ( Q.size() == 1 );

				return getFileName( Q.front() );
			}

			static void mergeFiles(
				std::string const & filenamea,
				std::string const & filenameb,
				std::string const & outputfilename
				)
			{
				::libmaus2::graph::TripleEdgeInput inputa(filenamea,32*1024);
				::libmaus2::graph::TripleEdgeInput inputb(filenameb,32*1024);
				::libmaus2::graph::TripleEdgeOutputMerge output(outputfilename, 32*1024);

				::libmaus2::graph::TripleEdge triplea;
				::libmaus2::graph::TripleEdge tripleb;
				bool oka = inputa.getNextTriple(triplea);
				bool okb = inputb.getNextTriple(tripleb);

				while ( oka && okb )
				{
					if ( triplea < tripleb )
					{
						output.write(triplea);
						oka = inputa.getNextTriple(triplea);
					}
					else
					{
						output.write(tripleb);
						okb = inputb.getNextTriple(tripleb);
					}
				}

				while ( oka )
				{
					output.write(triplea);
					oka = inputa.getNextTriple(triplea);
				}

				while ( okb )
				{
					output.write(tripleb);
					okb = inputb.getNextTriple(tripleb);
				}
			}

			uint64_t mergeFiles(uint64_t const idxa, uint64_t const idxb)
			{
				std::string const filenamea = getFileName(idxa);
				std::string const filenameb = getFileName(idxb);
				uint64_t const nextid = fileidx++;
				std::string const outputfilename = getFileName(nextid);
				mergeFiles(filenamea,filenameb,outputfilename);
				return nextid;
			}
		};
	}
}
#endif
