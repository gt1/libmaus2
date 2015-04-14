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
#if ! defined(LIBMAUS_FASTX_FASTASTREAMSET_HPP)
#define LIBMAUS_FASTX_FASTASTREAMSET_HPP

#include <libmaus/fastx/FastAStream.hpp>
#include <libmaus/fastx/SpaceTable.hpp>
#include <libmaus/util/ToUpperTable.hpp>
#include <libmaus/util/md5.hpp>

#include <libmaus/fastx/RefPathTokenVectorSequence.hpp>
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct FastAStreamSet
		{
			::libmaus::fastx::FastALineParser parser;
			
			FastAStreamSet(std::istream & in) : parser(in) {}
			
			bool getNextStream(std::pair<std::string,FastAStream::shared_ptr_type> & P)
			{
				::libmaus::fastx::FastALineParserLineInfo line;
				
				if ( ! parser.getNextLine(line) )
					return false;
				
				if ( line.linetype != ::libmaus::fastx::FastALineParserLineInfo::libmaus_fastx_fasta_id_line )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "FastAStreamSet::getNextStream(): unexpected line type" << std::endl;
					se.finish();
					throw se;
				}
				
				std::string const id(line.line,line.line+line.linelen);		

				libmaus::fastx::FastAStream::shared_ptr_type ptr(
					new libmaus::fastx::FastAStream(parser,64*1024,0));
					
				P.first = id;
				P.second = ptr;
				
				return true;
			}
						
			std::map<std::string,std::string> computeMD5(bool writedata = true, bool verify = true)
			{
				std::pair<std::string,FastAStream::shared_ptr_type> P;
				std::map<std::string,std::string> M;
				libmaus::autoarray::AutoArray<char> B(64*1024,false);
				unsigned char * u = reinterpret_cast<unsigned char *>(B.begin());
				libmaus::fastx::SpaceTable const S;
				libmaus::util::ToUpperTable const T;
				uint8_t digest[libmaus::util::MD5::digestlength];
				
				char const * datadir = writedata ? getenv("REF_CACHE") : NULL;
				if ( (!datadir) || (!*datadir) )
					datadir = NULL;
				// do not write data if no location is given
				if ( (!datadir) )
					writedata = false;
					
				char const * refpath = getenv("REF_PATH");
				if ( (!refpath) || (!*refpath) )
					refpath = NULL;

				RefPathTokenVector refcacheexp(writedata ? std::string(datadir) : std::string());
				RefPathTokenVectorSequence refpathexp(refpath ? std::string(refpath) : std::string());				

				while ( getNextStream(P) )
				{
					std::string id = P.first;
					std::istream & str = *(P.second);
					libmaus::util::MD5 md5;
					md5.init();

					std::ostringstream data;
			
					// shorten id by cutting off everystring from first white space		
					uint64_t z = 0;
					while ( z < id.size() && S.nospacetable[static_cast<unsigned char>(id[z])] )
						++z;
					id = id.substr(0,z);
					
					while ( str )
					{
						str.read(B.begin(),B.size());
						size_t const n = str.gcount();
						
						size_t o = 0;
						for ( size_t i = 0; i < n; ++i )
							if ( S.nospacetable[ u[i] ] )
								u[o++] = T.touppertable[u[i]];
								
						md5.update(reinterpret_cast<uint8_t const *>(u),o);
						if ( writedata )
							data.write(B.begin(),o);
					}
					
					md5.digest(&digest[0]);
					std::string const sdigest = md5.digestToString(&digest[0]);
					
					#if 0
					std::cerr << id << "\t" << sdigest << "\t" << refcacheexp.expand(sdigest);
					std::vector<std::string> E = refpathexp.expand(sdigest);
					for ( uint64_t z = 0; z < E.size(); ++z )
						std::cerr << "\t" << E[z];
					std::cerr << std::endl;
					#endif
					
					M[id] = sdigest;
					
					if ( writedata )
					{
						std::vector<std::string> E = refpathexp.expand(sdigest);
						E.push_back(refcacheexp.expand(sdigest));
						
						bool found = false;
						std::string foundfn;
				
						// check if the data is in the cache		
						for ( size_t z = 0; (!found) && z < E.size(); ++z )
						{
							std::string e = E[z];

							if ( e.find("URL=") != std::string::npos && e.find("URL=") == 0 )
								e = e.substr(strlen("URL="));

							if ( libmaus::aio::InputStreamFactoryContainer::tryOpen(e) )
							{
								found = true;
								foundfn = e;
							}
						}
						
						// data not found in cache
						if ( !found )
						{
							libmaus::aio::PosixFdOutputStream PFOS(E.back());
							std::string const sdata = data.str();
							PFOS.write(sdata.c_str(),sdata.size());
							PFOS.flush();
							if ( ! PFOS )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "libmaus::fastx::FastAStreamSet: computeMD5 failed to write sequence data to file" << E.back() << std::endl;
								lme.finish();
								throw lme;
							}
						}
						else if ( verify )
						{
							libmaus::aio::InputStream::unique_ptr_type Pin(libmaus::aio::InputStreamFactoryContainer::constructUnique(foundfn));
							std::istream & PFIS = *Pin;

							libmaus::util::MD5 checkmd5;
							checkmd5.init();

							while ( PFIS )
							{
								PFIS.read(B.begin(),B.size());
								checkmd5.update(reinterpret_cast<uint8_t const *>(B.begin()),PFIS.gcount());
							}

							checkmd5.digest(&digest[0]);
							std::string const scheckdigest = checkmd5.digestToString(&digest[0]);
							
							if ( scheckdigest != sdigest )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "libmaus::fastx::FastAStreamSet: checksum for file " << foundfn << " is wrong" << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}
				}
				
				return M;
			}
		};
	}
}
#endif
