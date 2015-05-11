/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_DB_DATABASEFILE_HPP)
#define LIBMAUS2_DAZZLER_DB_DATABASEFILE_HPP

#include <libmaus2/dazzler/db/FastaInfo.hpp>
#include <libmaus2/dazzler/db/IndexBase.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/dazzler/db/Read.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct DatabaseFile
			{
				bool isdam;
				std::string root;
				std::string path;
				int part;
				std::string dbpath;
				uint64_t nfiles;
				std::vector<libmaus2::dazzler::db::FastaInfo> fileinfo;
				
				uint64_t numblocks;
				uint64_t blocksize;
				int64_t cutoff;
				uint64_t all;
				
				typedef std::pair<uint64_t,uint64_t> upair;
				std::vector<upair> blocks;
				
				std::string idxpath;
				libmaus2::dazzler::db::IndexBase indexbase;
				uint64_t indexoffset;

				std::string bpspath;

				static std::string getPath(std::string const & s)
				{
					size_t p = s.find_last_of('/');
					
					if ( p == std::string::npos )
						return ".";
					else
						return s.substr(0,p);
				}

				static bool endsOn(std::string const & s, std::string const & suffix)
				{
					return s.size() >= suffix.size() && s.substr(s.size()-suffix.size()) == suffix;
				}

				static std::string getRoot(std::string s, std::string suffix = std::string())
				{
					size_t p = s.find_last_of('/');

					if ( p != std::string::npos )
						s = s.substr(p+1);
						
					if ( suffix.size() )
					{
						if ( endsOn(s,suffix) )
							s = s.substr(0,s.size()-suffix.size());
					}
					else if ( (p = s.find_first_of('.')) != std::string::npos )
						s = s.substr(0,p);
						
					return s;
				}

				static bool isNumber(std::string const & s)
				{
					if ( ! s.size() )
						return false;
						
					bool aldig = true;
					for ( uint64_t i = 0; aldig && i < s.size(); ++i )
						if ( ! isdigit(s[i]) )
							aldig = false;
					
					return aldig && (s.size() == 1 || s[0] != '0');
				}

				static std::vector<std::string> tokeniseByWhitespace(std::string const & s)
				{
					uint64_t i = 0;
					std::vector<std::string> V;
					while ( i < s.size() )
					{
						while ( i < s.size() && isspace(s[i]) )
							++i;
						
						uint64_t low = i;
						while ( i < s.size() && !isspace(s[i]) )
							++i;
							
						if ( i != low )
							V.push_back(s.substr(low,i-low));
					}
					
					return V;
				}

				static bool nextNonEmptyLine(std::istream & in, std::string & line)
				{
					while ( in )
					{
						std::getline(in,line);
						if ( line.size() )
							return true;
					}
					
					return false;
				}

				size_t decodeRead(
					std::istream & bpsstream, 
					libmaus2::autoarray::AutoArray<char> & A,
					int32_t const rlen
				) const
				{
					if ( static_cast<int32_t>(A.size()) < rlen )
						A = libmaus2::autoarray::AutoArray<char>(rlen,false);
					return decodeRead(bpsstream,A.begin(),rlen);
				}
				
				size_t decodeRead(
					std::istream & bpsstream,
					char * const C,
					int32_t const rlen
				) const
				{
					bpsstream.read(C + (rlen - (rlen+3)/4),(rlen+3)/4);
					if ( bpsstream.gcount() != (rlen+3)/4 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::dazzler::db::Read::decode(): input failure" << std::endl;
						lme.finish();
						throw lme;
					}

					unsigned char * p = reinterpret_cast<unsigned char *>(C + ( rlen - ((rlen+3)>>2) ));
					char * o = C;
					for ( int32_t i = 0; i < (rlen>>2); ++i )
					{
						unsigned char v = *(p++);
						
						*(o++) = libmaus2::fastx::remapChar((v >> 6)&3);
						*(o++) = libmaus2::fastx::remapChar((v >> 4)&3);
						*(o++) = libmaus2::fastx::remapChar((v >> 2)&3);
						*(o++) = libmaus2::fastx::remapChar((v >> 0)&3);
					}
					if ( rlen & 3 )
					{
						unsigned char v = *(p++);
						size_t rest = rlen - ((rlen>>2)<<2);

						if ( rest )
						{
							*(o++) = libmaus2::fastx::remapChar((v >> 6)&3);
							rest--;
						}
						if ( rest )
						{
							*(o++) = libmaus2::fastx::remapChar((v >> 4)&3);
							rest--;
						}
						if ( rest )
						{
							*(o++) = libmaus2::fastx::remapChar((v >> 2)&3);
							rest--;
						}
					}
					
					return rlen;
				}

				
				DatabaseFile()
				{
				
				}

				DatabaseFile(std::string const & s)
				{
					isdam = endsOn(s,".dam");
					root = isdam ? getRoot(s,".dam") : getRoot(s,".db");
					path = getPath(s);
					part = 0;
					if ( 
						root.find_last_of('.') != std::string::npos &&
						isNumber(root.substr(root.find_last_of('.')+1))
					)
					{
						std::string const spart = root.substr(root.find_last_of('.')+1);
						std::istringstream istr(spart);
						istr >> part;
						root = root.substr(0,root.find_last_of('.'));
					}
					
					if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(path + "/" + root + ".db") )
						dbpath = path + "/" + root + ".db";
					else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(path + "/" + root + ".dam") )
						dbpath = path + "/" + root + ".dam";
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Cannot find database file" << std::endl;
						lme.finish();
						throw lme;
					}
					
					libmaus2::aio::InputStream::unique_ptr_type Pdbin(libmaus2::aio::InputStreamFactoryContainer::constructUnique(dbpath));
					
					std::string nfilesline;
					std::vector<std::string> nfilestokenvector;
					if ( (! nextNonEmptyLine(*Pdbin,nfilesline)) || (nfilestokenvector=tokeniseByWhitespace(nfilesline)).size() != 3 || (nfilestokenvector[0] != "files") || (nfilestokenvector[1] != "=") || (!isNumber(nfilestokenvector[2])))
					{			
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Unexpected line " << nfilesline << std::endl;
						lme.finish();
						throw lme;
					}
					nfiles = atol(nfilestokenvector[2].c_str());
					
					for ( uint64_t z = 0; z < nfiles; ++z )
					{
						std::string fileline;
						if ( ! nextNonEmptyLine(*Pdbin,fileline) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "Unexpected eof reading file line" << std::endl;
							lme.finish();
							throw lme;
						}
						
						std::vector<std::string> const tokens = tokeniseByWhitespace(fileline);
						
						if ( tokens.size() != 3 || (! isNumber(tokens[0])) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "Malformed file line " << fileline << std::endl;
							lme.finish();
							throw lme;			
						}
						
						uint64_t const fnumreads = atol(tokens[0].c_str());
						std::string const fastaprolog = tokens[1];
						std::string const fastafn = tokens[2];
						
						fileinfo.push_back(libmaus2::dazzler::db::FastaInfo(fnumreads,fastaprolog,fastafn));			
					}
					
					std::string numblocksline;
					if ( nextNonEmptyLine(*Pdbin,numblocksline) )
					{
						std::vector<std::string> numblockstokens;
						if ( (numblockstokens=tokeniseByWhitespace(numblocksline)).size() != 3 || numblockstokens[0] != "blocks" || numblockstokens[1] != "=" || !isNumber(numblockstokens[2]) )
						{			
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "Malformed num blocks line " << numblocksline << std::endl;
							lme.finish();
							throw lme;			
						}
						numblocks = atol(numblockstokens[2].c_str());
						
						std::string sizeline;
						std::vector<std::string> sizelinetokens;
						if ( !nextNonEmptyLine(*Pdbin,sizeline) || (sizelinetokens=tokeniseByWhitespace(sizeline)).size() != 9 ||
							sizelinetokens[0] != "size" ||
							sizelinetokens[1] != "=" ||
							!isNumber(sizelinetokens[2]) ||
							sizelinetokens[3] != "cutoff" ||
							sizelinetokens[4] != "=" ||
							!isNumber(sizelinetokens[5]) ||
							sizelinetokens[6] != "all" ||
							sizelinetokens[7] != "=" ||
							!isNumber(sizelinetokens[8]) )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "Malformed block size line " << sizeline << std::endl;
							lme.finish();
							throw lme;
						}

						blocksize = atol(sizelinetokens[2].c_str());
						cutoff = atol(sizelinetokens[5].c_str());
						all = atol(sizelinetokens[8].c_str());
						
						for ( uint64_t z = 0; z < numblocks+1; ++z )
						{
							std::string blockline;
							std::vector<std::string> blocklinetokens;
							
							if ( 
								!nextNonEmptyLine(*Pdbin,blockline)
								||
								(blocklinetokens=tokeniseByWhitespace(blockline)).size() != 2
								||
								!isNumber(blocklinetokens[0])
								||
								!isNumber(blocklinetokens[1])
							)
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Malformed block line " << blockline << std::endl;
								lme.finish();
								throw lme;
							}
							
							uint64_t const unfiltered = atol(blocklinetokens[0].c_str());
							uint64_t const filtered   = atol(blocklinetokens[1].c_str());
							
							blocks.push_back(upair(unfiltered,filtered));
						}
					}
					
					idxpath = path + "/." + root + ".idx";
							
					if ( ! libmaus2::aio::InputStreamFactoryContainer::tryOpen(idxpath) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Cannot find index file " << idxpath << std::endl;
						lme.finish();
						throw lme;			
					}
					
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					libmaus2::dazzler::db::IndexBase ldb(*Pidxfile);
					
					indexbase = ldb;
					indexoffset = Pidxfile->tellg();

					bpspath = path + "/." + root + ".bps";
					if ( ! libmaus2::aio::InputStreamFactoryContainer::tryOpen(bpspath) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Cannot find index file " << bpspath << std::endl;
						lme.finish();
						throw lme;			
					}
	
					#if 0
					libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
					libmaus2::autoarray::AutoArray<char> B;
					for ( int64_t i = 0; i < indexbase.nreads; ++i )
					{
						Read R = getRead(i);
						Pbpsfile->seekg(R.boff,std::ios::beg);
						size_t const l = R.decode(*Pbpsfile,B,R.rlen);
						std::cerr << R << "\t" << std::string(B.begin(),B.begin()+l) << std::endl;
					}
					#endif
					
					#if 0
					libmaus2::autoarray::AutoArray<char> A;
					std::vector<uint64_t> off;
					decodeReads(0, indexbase.nreads, A, off);
					
					for ( uint64_t i = 0; i < indexbase.nreads; ++i )
					{
						std::cerr << std::string(A.begin()+off[i],A.begin()+off[i+1]) << std::endl;
					}
					std::cerr << off[indexbase.nreads] << std::endl;
					#endif
				}
				
				Read getRead(size_t const i) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));

					Pidxfile->seekg(indexoffset + i * Read::serialisedSize);
					
					Read R(*Pidxfile);
					
					return R;
				}
				
				void getReadInterval(size_t const low, size_t const high, std::vector<Read> & V) const
				{
					V.resize(high-low);
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					Pidxfile->seekg(indexoffset + low * Read::serialisedSize);
					for ( size_t i = low; i < high; ++i )
						V[i-low].deserialise(*Pidxfile);
				}
				
				void decodeReads(size_t const low, size_t const high, libmaus2::autoarray::AutoArray<char> & A, std::vector<uint64_t> & off) const
				{
					off.resize((high-low)+1);
					off[0] = 0;
					
					if ( high - low )
					{
						libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
						Pidxfile->seekg(indexoffset + low * Read::serialisedSize);
						
						Read R0(*Pidxfile);
						Pidxfile->seekg(indexoffset + low * Read::serialisedSize);

						libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
						Pbpsfile->seekg(R0.boff,std::ios::beg);
						
						for ( size_t i = 0; i < high-low; ++i )
						{
							Read R(*Pidxfile);
							off[i+1] = off[i] + R.rlen;
						}

						A = libmaus2::autoarray::AutoArray<char>(off[high],false);
						for ( size_t i = 0; i < high-low; ++i )
							decodeRead(*Pbpsfile,A.begin()+off[i],off[i+1]-off[i]);
					}
				}
				
				size_t decodeRead(size_t const i, libmaus2::autoarray::AutoArray<char> & A) const
				{
					Read const R = getRead(i);
					libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
					Pbpsfile->seekg(R.boff,std::ios::beg);
					return decodeRead(*Pbpsfile,A,R.rlen);
				}
				
				std::string operator[](size_t const i) const
				{
					libmaus2::autoarray::AutoArray<char> A;
					size_t const rlen = decodeRead(i,A);
					return std::string(A.begin(),A.begin()+rlen);
				}
				
				size_t size() const
				{
					return indexbase.nreads;
				}
			};

			std::ostream & operator<<(std::ostream & out, DatabaseFile const & D);
		}
	}
}
#endif
