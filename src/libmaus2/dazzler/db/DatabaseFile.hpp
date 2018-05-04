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
#include <libmaus2/dazzler/db/Track.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/dazzler/db/Read.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/ArrayFileSet.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <libmaus2/util/FileEnumerator.hpp>
#include <libmaus2/util/PathTools.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct DatabaseFile
			{
				typedef DatabaseFile this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				bool isdam;
				std::string root;
				std::string path;
				int part;
				std::string dbpath;
				uint64_t nfiles;
				std::vector<libmaus2::dazzler::db::FastaInfo> fileinfo;
				std::vector<uint64_t> fileoffsets;

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

				libmaus2::rank::ImpCacheLineRank::unique_ptr_type Ptrim;

				struct PairFirstComparator
				{
					bool operator()(upair const & A, upair const & B) const
					{
						return A.first < B.first;
					}
				};

				struct PairSecondComparator
				{
					bool operator()(upair const & A, upair const & B) const
					{
						return A.second < B.second;
					}
				};

				uint64_t getBlockForIdUntrimmed(uint64_t const id) const
				{
					if ( ! blocks.size() || id >= blocks.back().first )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getBlockForIdUntrimmed(): id " << id << " is out of range" << std::endl;
						lme.finish();
						throw lme;
					}

					std::vector<upair>::const_iterator it = std::lower_bound(blocks.begin(),blocks.end(),upair(id,0),PairFirstComparator());

					uint64_t idx;

					if ( it != blocks.end() && it->first == id )
						idx = it-blocks.begin();
					else
						idx = (it-blocks.begin())-1;

					assert ( id >= blocks[idx].first );
					assert ( id < blocks[idx+1].first );

					// block ids are 1 based
					return idx + 1;
				}

				uint64_t getBlockForIdTrimmed(uint64_t const id) const
				{
					if ( ! blocks.size() || id >= blocks.back().second )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getBlockForIdTrimmed(): id " << id << " is out of range" << std::endl;
						lme.finish();
						throw lme;
					}

					std::vector<upair>::const_iterator it = std::lower_bound(blocks.begin(),blocks.end(),upair(0,id),PairSecondComparator());

					uint64_t idx;

					if ( it != blocks.end() && it->second == id )
						idx = it-blocks.begin();
					else
						idx = (it-blocks.begin())-1;

					assert ( id >= blocks[idx].second );
					assert ( id < blocks[idx+1].second );

					// block ids are 1 based
					return idx + 1;
				}

				uint64_t getUntrimmedBlockSize(uint64_t const blockid) const
				{
					if ( ! blocks.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getUntrimmedBlockSize(): blocks vector is empty" << std::endl;
						lme.finish();
						throw lme;
					}

					// no block id given, return whole database
					if ( ! blockid )
						return blocks.back().first;

					if ( ! ( blockid < blocks.size() ) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getUntrimmedBlockSize(): invalid block id " << blockid << std::endl;
						lme.finish();
						throw lme;
					}

					return blocks[blockid].first - blocks[blockid-1].first;
				}

				uint64_t getTrimmedBlockSize(uint64_t const blockid) const
				{
					if ( ! blocks.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getTrimmedBlockSize(): blocks vector is empty" << std::endl;
						lme.finish();
						throw lme;
					}

					// no block id given, return whole database
					if ( ! blockid )
						return blocks.back().second;

					if ( ! ( blockid < blocks.size() ) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getTrimmedBlockSize(): invalid block id " << blockid << std::endl;
						lme.finish();
						throw lme;
					}

					return blocks[blockid].second - blocks[blockid-1].second;
				}

				std::pair<uint64_t,uint64_t> getUntrimmedBlockInterval(uint64_t const blockid) const
				{
					if ( ! blocks.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getUntrimmedBlockInterval(): blocks vector is empty" << std::endl;
						lme.finish();
						throw lme;
					}

					// no block id given, return whole database
					if ( ! blockid )
						return std::pair<uint64_t,uint64_t>(blocks.front().first,blocks.back().first);

					if ( ! ( blockid < blocks.size() ) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getUntrimmedBlockInterval(): invalid block id " << blockid << std::endl;
						lme.finish();
						throw lme;
					}

					return std::pair<uint64_t,uint64_t>(blocks[blockid-1].first,blocks[blockid].first);
				}

				std::pair<uint64_t,uint64_t> getTrimmedBlockInterval(uint64_t const blockid) const
				{
					if ( ! blocks.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getTrimmedBlockInterval(): blocks vector is empty" << std::endl;
						lme.finish();
						throw lme;
					}

					// no block id given, return whole database
					if ( ! blockid )
						return std::pair<uint64_t,uint64_t>(blocks.front().second,blocks.back().second);

					if ( ! ( blockid < blocks.size() ) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getTrimmedBlockInterval(): invalid block id " << blockid << std::endl;
						lme.finish();
						throw lme;
					}

					return std::pair<uint64_t,uint64_t>(blocks[blockid-1].second,blocks[blockid].second);
				}




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
					std::istream & idxstream,
					std::istream & bpsstream,
					uint64_t const id,
					libmaus2::autoarray::AutoArray<char> & A
				) const
				{
					uint64_t const mappedindex = Ptrim->select1(id);
					idxstream.seekg(indexoffset + mappedindex * Read::serialisedSize);
					Read R(idxstream);
					bpsstream.seekg(R.boff,std::ios::beg);
					decodeRead(bpsstream,A,R.rlen);
					return R.rlen;
				}

				size_t decodeReadAndRC(
					std::istream & idxstream,
					std::istream & bpsstream,
					uint64_t const id,
					libmaus2::autoarray::AutoArray<char> & A
				) const
				{
					uint64_t const mappedindex = Ptrim->select1(id);
					idxstream.seekg(indexoffset + mappedindex * Read::serialisedSize);
					Read R(idxstream);
					bpsstream.seekg(R.boff,std::ios::beg);
					decodeReadAndRC(bpsstream,A,R.rlen);
					return R.rlen;
				}

				static size_t decodeRead(
					std::istream & bpsstream,
					libmaus2::autoarray::AutoArray<char> & A,
					int32_t const rlen
				)
				{
					if ( static_cast<int32_t>(A.size()) < rlen )
						A = libmaus2::autoarray::AutoArray<char>(rlen,false);
					return decodeRead(bpsstream,A.begin(),rlen);
				}

				static size_t decodeReadAndRC(
					std::istream & bpsstream,
					libmaus2::autoarray::AutoArray<char> & A,
					int32_t const rlen
				)
				{
					if ( static_cast<int32_t>(A.size()) < 2*rlen )
						A = libmaus2::autoarray::AutoArray<char>(2*rlen,false);
					return decodeReadAndRC(bpsstream,A.begin(),rlen);
				}

				static size_t decodeRead(
					std::istream & bpsstream,
					char * const C,
					int32_t const rlen
				)
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

				static size_t decodeReadNoDecode(
					std::istream & bpsstream,
					char * const C,
					int32_t const rlen
				)
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

						*(o++) = ((v >> 6)&3);
						*(o++) = ((v >> 4)&3);
						*(o++) = ((v >> 2)&3);
						*(o++) = ((v >> 0)&3);
					}
					if ( rlen & 3 )
					{
						unsigned char v = *(p++);
						size_t rest = rlen - ((rlen>>2)<<2);

						if ( rest )
						{
							*(o++) = ((v >> 6)&3);
							rest--;
						}
						if ( rest )
						{
							*(o++) = ((v >> 4)&3);
							rest--;
						}
						if ( rest )
						{
							*(o++) = ((v >> 2)&3);
							rest--;
						}
					}

					return rlen;
				}

				static size_t decodeReadRC(
					std::istream & bpsstream,
					char * const C,
					int32_t const rlen
				)
				{
					int64_t const inbytes = (rlen+3)/4;
					bpsstream.read(C,inbytes);
					if ( bpsstream.gcount() != inbytes )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::dazzler::db::Read::decode(): input failure" << std::endl;
						lme.finish();
						throw lme;
					}
					std::reverse(C,C+inbytes);

					unsigned char * p = reinterpret_cast<unsigned char *>(C+inbytes);
					char * o = C + rlen;
					for ( int32_t i = 0; i < (rlen>>2); ++i )
					{
						unsigned char v = (*(--p)) ^ 0xFF;

						*(--o) = libmaus2::fastx::remapChar((v >> 6)&3);
						*(--o) = libmaus2::fastx::remapChar((v >> 4)&3);
						*(--o) = libmaus2::fastx::remapChar((v >> 2)&3);
						*(--o) = libmaus2::fastx::remapChar((v >> 0)&3);
					}
					if ( rlen & 3 )
					{
						unsigned char v = (*(--p)) ^ 0xFF;
						size_t rest = rlen - ((rlen>>2)<<2);

						if ( rest )
						{
							*(--o) = libmaus2::fastx::remapChar((v >> 6)&3);
							rest--;
						}
						if ( rest )
						{
							*(--o) = libmaus2::fastx::remapChar((v >> 4)&3);
							rest--;
						}
						if ( rest )
						{
							*(--o) = libmaus2::fastx::remapChar((v >> 2)&3);
							rest--;
						}
					}

					return rlen;
				}

				static size_t decodeReadRCNoDecode(
					std::istream & bpsstream,
					char * const C,
					int32_t const rlen
				)
				{
					int64_t const inbytes = (rlen+3)/4;
					bpsstream.read(C,inbytes);
					if ( bpsstream.gcount() != inbytes )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::dazzler::db::Read::decode(): input failure" << std::endl;
						lme.finish();
						throw lme;
					}
					std::reverse(C,C+inbytes);

					unsigned char * p = reinterpret_cast<unsigned char *>(C+inbytes);
					char * o = C + rlen;
					for ( int32_t i = 0; i < (rlen>>2); ++i )
					{
						unsigned char v = (*(--p)) ^ 0xFF;

						*(--o) = ((v >> 6)&3);
						*(--o) = ((v >> 4)&3);
						*(--o) = ((v >> 2)&3);
						*(--o) = ((v >> 0)&3);
					}
					if ( rlen & 3 )
					{
						unsigned char v = (*(--p)) ^ 0xFF;
						size_t rest = rlen - ((rlen>>2)<<2);

						if ( rest )
						{
							*(--o) = ((v >> 6)&3);
							rest--;
						}
						if ( rest )
						{
							*(--o) = ((v >> 4)&3);
							rest--;
						}
						if ( rest )
						{
							*(--o) = ((v >> 2)&3);
							rest--;
						}
					}

					return rlen;
				}

				static size_t decodeReadAndRC(
					std::istream & bpsstream,
					char * const C,
					int32_t const rlen
				)
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
					char * r = C + 2*rlen;
					for ( int32_t i = 0; i < (rlen>>2); ++i )
					{
						unsigned char v = *(p++);

						uint8_t const c3 = (v >> 6)&3;
						*(o++) = libmaus2::fastx::remapChar(c3);
						*(--r) = libmaus2::fastx::remapChar(c3^3);
						uint8_t const c2 = (v >> 4)&3;
						*(o++) = libmaus2::fastx::remapChar(c2);
						*(--r) = libmaus2::fastx::remapChar(c2^3);
						uint8_t const c1 = (v >> 2)&3;
						*(o++) = libmaus2::fastx::remapChar(c1);
						*(--r) = libmaus2::fastx::remapChar(c1^3);
						uint8_t const c0 = (v >> 0)&3;
						*(o++) = libmaus2::fastx::remapChar(c0);
						*(--r) = libmaus2::fastx::remapChar(c0^3);
					}
					if ( rlen & 3 )
					{
						unsigned char v = *(p++);
						size_t rest = rlen - ((rlen>>2)<<2);

						if ( rest )
						{
							uint8_t const c3 = (v >> 6)&3;
							*(o++) = libmaus2::fastx::remapChar(c3);
							*(--r) = libmaus2::fastx::remapChar(c3^3);

							rest--;
						}
						if ( rest )
						{
							uint8_t const c2 = (v >> 4)&3;
							*(o++) = libmaus2::fastx::remapChar(c2);
							*(--r) = libmaus2::fastx::remapChar(c2^3);
							rest--;
						}
						if ( rest )
						{
							uint8_t const c1 = (v >> 2)&3;
							*(o++) = libmaus2::fastx::remapChar(c1);
							*(--r) = libmaus2::fastx::remapChar(c1^3);
							rest--;
						}
					}

					return rlen;
				}

				static size_t decodeReadSegment(
					std::istream & bpsstream,
					char * C,
					int32_t offset,
					int32_t rlen
				)
				{
					int32_t const rrlen = rlen;

					if ( (offset >> 2) )
					{
						bpsstream.seekg((offset >> 2), std::ios::cur);
						offset -= ((offset >> 2) << 2);
					}

					assert ( offset < 4 );

					if ( offset & 3 )
					{
						int const c = bpsstream.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "libmaus2::dazzler::db::Read::decodeReadSegment(): input failure" << std::endl;
							lme.finish();
							throw lme;
						}

						char t[4] = {
							libmaus2::fastx::remapChar((c >> 6)&3),
							libmaus2::fastx::remapChar((c >> 4)&3),
							libmaus2::fastx::remapChar((c >> 2)&3),
							libmaus2::fastx::remapChar((c >> 0)&3)
						};

						uint64_t const align = 4-(offset&3);
						uint64_t const tocopy = std::min(rlen,static_cast<int32_t>(align));

						std::copy(
							&t[0] + (offset & 3),
							&t[0] + (offset & 3) + tocopy,
							C
						);

						C += tocopy;
						offset -= tocopy;
						rlen -= tocopy;
					}

					if ( rlen )
					{
						assert ( offset == 0 );

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
					}

					return rrlen;
				}

				DatabaseFile()
				{

				}

				DatabaseFile(std::string const & s) : cutoff(-1)
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
						lme.getStream() << "Cannot find database file " << s << std::endl;
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

					// compute untrimmed vector (each bit set)
					uint64_t const n = indexbase.ureads;
					libmaus2::rank::ImpCacheLineRank::unique_ptr_type Ttrim(new libmaus2::rank::ImpCacheLineRank(n));
					Ptrim = UNIQUE_PTR_MOVE(Ttrim);

					libmaus2::rank::ImpCacheLineRank::WriteContext context = Ptrim->getWriteContext();
					for ( uint64_t i = 0; i < n; ++i )
						context.writeBit(true);
					context.flush();

					fileoffsets.resize(fileinfo.size());
					for ( uint64_t i = 0; i < fileinfo.size(); ++i )
						fileoffsets[i] = fileinfo[i].fnumreads;
					// turn prefix sums into read counts
					for ( uint64_t i = 1; i < fileoffsets.size(); ++i )
						fileoffsets[fileoffsets.size()-i] -= fileoffsets[fileoffsets.size()-i-1];

					uint64_t sum = 0;
					for ( uint64_t i = 0; i < fileoffsets.size(); ++i )
					{
						uint64_t const t = fileoffsets[i];
						fileoffsets[i] = sum;
						sum += t;
					}
					fileoffsets.push_back(sum);

					if ( part )
					{
						indexbase.nreads = blocks[part].first - blocks[part-1].first;
						indexbase.ufirst = blocks[part-1].first;
						indexbase.tfirst = blocks[part-1].second;
					}
					else
					{
						if ( blocks.size() )
							indexbase.nreads = blocks.back().first - blocks.front().first;
						else
							indexbase.nreads = 0;

						indexbase.ufirst = 0;
						indexbase.tfirst = 0;
					}

					{
						libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
						std::istream & idxfile = *Pidxfile;
						idxfile.seekg(indexoffset + /* mappedindex */ indexbase.ufirst * Read::serialisedSize);
						Read R;
						indexbase.totlen = 0;
						indexbase.maxlen = 0;
						for ( int64_t i = 0; i < indexbase.nreads; ++i )
						{
							R.deserialise(idxfile);
							indexbase.totlen += R.rlen;
							indexbase.maxlen = std::max(indexbase.maxlen,R.rlen);
						}
					}
				}

				struct DBFileSet
				{
					typedef DBFileSet this_type;
					typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
					typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

					std::string fn;
					std::string idxfn;
					std::string bpsfn;
					bool deleteondeconstruct;

					DBFileSet() : deleteondeconstruct(false) {}
					DBFileSet(
						std::string const & rfn,
						std::string const & ridxfn,
						std::string const & rbpsfn,
						bool const rdeleteondeconstruct
					) : fn(rfn), idxfn(ridxfn), bpsfn(rbpsfn), deleteondeconstruct(rdeleteondeconstruct) {}

					~DBFileSet()
					{
						if ( deleteondeconstruct )
						{
							libmaus2::aio::FileRemoval::removeFile(fn);
							libmaus2::aio::FileRemoval::removeFile(idxfn);
							libmaus2::aio::FileRemoval::removeFile(bpsfn);
						}
					}
				};

				struct DBArrayFileSet
				{
					typedef DBArrayFileSet this_type;
					typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
					typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

					libmaus2::autoarray::AutoArray<char> Adb;
					libmaus2::autoarray::AutoArray<char> Aidx;
					libmaus2::autoarray::AutoArray<char> Abps;

					typedef
					std::map <
						std::string,
						std::pair <
							libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
							libmaus2::autoarray::AutoArray<char>::shared_ptr_type
						>
					> Mtrack_type;
					Mtrack_type Mtrack;

					libmaus2::aio::ArrayFileSet<char const *>::unique_ptr_type PAFS;

					DBArrayFileSet(
						libmaus2::autoarray::AutoArray<char> & rAdb,
						libmaus2::autoarray::AutoArray<char> & rAidx,
						libmaus2::autoarray::AutoArray<char> & rAbps,
						Mtrack_type rMtrack
					) : Adb(rAdb), Aidx(rAidx), Abps(rAbps), Mtrack(rMtrack)
					{
						std::vector< std::pair<char const *,char const *> > Vdata;
						Vdata.push_back(std::pair<char const *,char const *>(Adb.begin(),Adb.end()));
						Vdata.push_back(std::pair<char const *,char const *>(Aidx.begin(),Aidx.end()));
						Vdata.push_back(std::pair<char const *,char const *>(Abps.begin(),Abps.end()));
						std::vector< std::string > Vfn;
						Vfn.push_back("readsdir/reads.db");
						Vfn.push_back("readsdir/.reads.idx");
						Vfn.push_back("readsdir/.reads.bps");

						for ( Mtrack_type::const_iterator ita = Mtrack.begin(); ita != Mtrack.end(); ++ita )
						{
							Vdata.push_back(
								std::pair<char const *,char const *>(
									ita->second.first->begin(),
									ita->second.first->end()
								)
							);
							Vfn.push_back(std::string("readsdir/.reads.") + ita->first + ".anno");
							Vdata.push_back(
								std::pair<char const *,char const *>(
									ita->second.second->begin(),
									ita->second.second->end()
								)
							);
							Vfn.push_back(std::string("readsdir/.reads.") + ita->first + ".data");
						}

						libmaus2::aio::ArrayFileSet<char const *>::unique_ptr_type tptr(
							new libmaus2::aio::ArrayFileSet<char const *>(Vdata,Vfn)
						);

						PAFS = UNIQUE_PTR_MOVE(tptr);
					}

					~DBArrayFileSet()
					{
					}

					std::string getDBURL() const
					{
						return PAFS->getURL(0);
					}
				};

				static DBArrayFileSet::unique_ptr_type copyToArrays(
					std::string const & s,
					std::vector<std::string> const * tracklist = 0
				)
				{
					if ( ! libmaus2::util::GetFileSize::fileExists(s) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::copyToArrays: file " << s << " does not exist or is not accessible" << std::endl;
						lme.finish();
						throw lme;
					}

					bool const isdb = endsOn(s,".db");
					bool const isdam = endsOn(s,".dam");
					bool const issup = isdb || isdam;

					if ( ! issup )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::copyToArrays: file " << s << " is not supported (file name does not end in .db or .dam)" << std::endl;
						lme.finish();
						throw lme;
					}

					std::string const path = getPath(s);
					std::string const root = isdam ? getRoot(s,".dam") : getRoot(s,".db");

					std::string dbpath;

					if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(path + "/" + root + ".db") )
						dbpath = path + "/" + root + ".db";
					else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(path + "/" + root + ".dam") )
						dbpath = path + "/" + root + ".dam";
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::copyToPrefix: cannot construct db file name" << std::endl;
						lme.finish();
						throw lme;
					}

					std::string const idxpath = path + "/." + root + ".idx";
					std::string const bpspath = path + "/." + root + ".bps";

					libmaus2::autoarray::AutoArray<char> Adb = libmaus2::autoarray::AutoArray<char>::readFile(dbpath);
					libmaus2::autoarray::AutoArray<char> Aidx = libmaus2::autoarray::AutoArray<char>::readFile(idxpath);
					libmaus2::autoarray::AutoArray<char> Abps = libmaus2::autoarray::AutoArray<char>::readFile(bpspath);

					std::map <
						std::string,
						std::pair <
							libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
							libmaus2::autoarray::AutoArray<char>::shared_ptr_type
						>
					> Mtrack;

					if ( tracklist )
					{
						for ( uint64_t i = 0; i < tracklist->size(); ++i )
						{
							std::string const & trackname = tracklist->at(i);

							std::string const annosrc = path + "/." + root + "." + trackname + ".anno";
							std::string const datasrc = path + "/." + root + "." + trackname + ".data";

							libmaus2::autoarray::AutoArray<char>::shared_ptr_type Aanno(new libmaus2::autoarray::AutoArray<char>);
							*Aanno = libmaus2::autoarray::AutoArray<char>::readFile(annosrc);
							libmaus2::autoarray::AutoArray<char>::shared_ptr_type Adata(new libmaus2::autoarray::AutoArray<char>);
							*Adata = libmaus2::autoarray::AutoArray<char>::readFile(datasrc);

							Mtrack [ trackname ] = std::pair <
								libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
								libmaus2::autoarray::AutoArray<char>::shared_ptr_type
							>(Aanno,Adata);
						}
					}

					DBArrayFileSet::unique_ptr_type PAFS(new DBArrayFileSet(Adb,Aidx,Abps,Mtrack));

					#if 0
					if ( tracklist )
					{
						for ( uint64_t i = 0; i < tracklist->size(); ++i )
						{
							std::string const & trackname = tracklist->at(i);
							std::string const annosrc = path + "/." + root + "." + trackname + ".anno";
							std::string const datasrc = path + "/." + root + "." + trackname + ".data";
						}
					}
					#endif

					return UNIQUE_PTR_MOVE(PAFS);
				}
				static DBFileSet::unique_ptr_type copyToPrefix(
					std::string const & s, std::string const & dstprefix, bool const registertmp = true,
					std::vector<std::string> const * tracklist = 0
				)
				{
					if ( ! libmaus2::util::GetFileSize::fileExists(s) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::copyToPrefix: file " << s << " does not exist or is not accessible" << std::endl;
						lme.finish();
						throw lme;
					}

					bool const isdb = endsOn(s,".db");
					bool const isdam = endsOn(s,".dam");
					bool const issup = isdb || isdam;

					if ( ! issup )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::copyToPrefix: file " << s << " is not supported (file name does not end in .db or .dam)" << std::endl;
						lme.finish();
						throw lme;
					}

					std::string const path = getPath(s);
					std::string const root = isdam ? getRoot(s,".dam") : getRoot(s,".db");

					std::string dbpath;

					if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(path + "/" + root + ".db") )
						dbpath = path + "/" + root + ".db";
					else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(path + "/" + root + ".dam") )
						dbpath = path + "/" + root + ".dam";
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::copyToPrefix: cannot construct db file name" << std::endl;
						lme.finish();
						throw lme;
					}

					std::string const idxpath = path + "/." + root + ".idx";
					std::string const bpspath = path + "/." + root + ".bps";

					std::string const dstfn = dstprefix + "/" + root + (isdam ? ".dam" : ".db");
					std::string const dstidx = dstprefix + "/." + root + ".idx";
					std::string const dstbps = dstprefix + "/." + root + ".bps";

					libmaus2::util::GetFileSize::copy(s,dstfn);
					libmaus2::util::GetFileSize::copy(idxpath,dstidx);
					libmaus2::util::GetFileSize::copy(bpspath,dstbps);

					if ( tracklist )
					{
						for ( uint64_t i = 0; i < tracklist->size(); ++i )
						{
							std::string const & trackname = tracklist->at(i);

							std::string const annosrc = path + "/." + root + "." + trackname + ".anno";
							std::string const annodst = dstprefix + "/." + root + "." + trackname + ".anno";
							std::string const datasrc = path + "/." + root + "." + trackname + ".data";
							std::string const datadst = dstprefix + "/." + root + "." + trackname + ".data";

							if ( registertmp )
								libmaus2::util::TempFileRemovalContainer::addTempFile(annodst);

							libmaus2::util::GetFileSize::copy(annosrc,annodst);

							if ( registertmp )
								libmaus2::util::TempFileRemovalContainer::addTempFile(datadst);

							libmaus2::util::GetFileSize::copy(datasrc,datadst);
						}
					}

					if ( registertmp )
					{
						libmaus2::util::TempFileRemovalContainer::addTempFile(dstfn);
						libmaus2::util::TempFileRemovalContainer::addTempFile(dstidx);
						libmaus2::util::TempFileRemovalContainer::addTempFile(dstbps);
					}

					DBFileSet::unique_ptr_type tptr(new DBFileSet(dstfn,dstidx,dstbps,registertmp));

					return UNIQUE_PTR_MOVE(tptr);
				}


				uint64_t readIdToFileId(uint64_t const readid) const
				{
					if ( readid >= fileoffsets.back() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::readIdToFileId: file id is out of range" << std::endl;
						lme.finish();
						throw lme;
					}

					std::vector<uint64_t>::const_iterator ita = std::lower_bound(fileoffsets.begin(),fileoffsets.end(),readid);

					assert ( ita != fileoffsets.end() );
					if ( readid != *ita )
					{
						assert ( ita != fileoffsets.begin() );
						--ita;
					}
					assert ( readid >= *ita );
					assert ( readid < *(ita+1) );

					return (ita-fileoffsets.begin());
				}

				std::string getReadName(uint64_t const i, Read const & R) const
				{
					std::ostringstream ostr;
					uint64_t const fileid = readIdToFileId(i);
					ostr << fileinfo[fileid].fastaprolog << '/' << R.origin << '/' << R.fpulse << '_' << R.fpulse + R.rlen;
					return ostr.str();
				}

				std::string getReadName(uint64_t const i, std::vector<Read> const & meta) const
				{
					std::ostringstream ostr;
					uint64_t const fileid = readIdToFileId(i);
					Read const R = meta.at(i);
					ostr << fileinfo[fileid].fastaprolog << '/' << R.origin << '/' << R.fpulse << '_' << R.fpulse + R.rlen;
					return ostr.str();
				}

				std::string getReadName(uint64_t const i) const
				{
					std::ostringstream ostr;
					uint64_t const fileid = readIdToFileId(i);
					Read const R = getRead(i);
					ostr << fileinfo[fileid].fastaprolog << '/' << R.origin << '/' << R.fpulse << '_' << R.fpulse + R.rlen;
					return ostr.str();
				}

				Read getRead(size_t const i) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));

					if ( i >= size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getRead: read index " << i << " out of range (not in [" << 0 << "," << size() << "))" << std::endl;
						lme.finish();
						throw lme;
					}

					uint64_t const mappedindex = Ptrim->select1(i);

					Pidxfile->seekg(indexoffset + mappedindex * Read::serialisedSize);

					Read R(*Pidxfile);

					return R;
				}

				struct SplitResultElement
				{
					uint64_t low;
					uint64_t high;
					uint64_t size;
					uint64_t ulow;
					uint64_t uhigh;

					SplitResultElement() {}
					SplitResultElement(
						uint64_t const rlow,
						uint64_t const rhigh,
						uint64_t const rsize,
						uint64_t const rulow,
						uint64_t const ruhigh
					) : low(rlow), high(rhigh), size(rsize), ulow(rulow), uhigh(ruhigh) {}
				};

				struct SplitResult : public std::vector<SplitResultElement>
				{

				};

				void setUntrimmedSplit(SplitResult & SR) const
				{
					for ( uint64_t i = 0; i < SR.size(); ++i )
					{
						if ( SR[i].low < size() )
							SR[i].ulow = trimmedToUntrimmed(SR[i].low);
						else
							SR[i].ulow = indexbase.ureads;

						if ( SR[i].high < size() )
							SR[i].uhigh = trimmedToUntrimmed(SR[i].high);
						else
							SR[i].uhigh = indexbase.ureads;
					}
					if ( SR.size() )
					{
						SR.front().ulow = 0;
						SR.back().uhigh = indexbase.ureads;
					}
				}

				SplitResult splitDb(uint64_t const maxmem) const
				{
					SplitResult SR;

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					Pidxfile->seekg(indexoffset);

					uint64_t const n = Ptrim->n;
					bool init = false;
					uint64_t l = 0;
					uint64_t s = 0;

					for ( uint64_t i = 0; i < n; ++i )
					{
						Read R(*Pidxfile);

						if ( (*Ptrim)[i] )
						{
							uint64_t const rlen = R.rlen;

							if ( ! init )
							{
								assert ( Ptrim->rank1(i) );
								l = Ptrim->rank1(i)-1;
								s = rlen;
								init = true;
							}
							else
							{
								if ( s + rlen <= maxmem )
								{
									s += rlen;
								}
								else
								{
									uint64_t const h = Ptrim->rank1(i)-1;

									SR.push_back(SplitResultElement(l,h,s,0,0));

									s = rlen;
									l = h;
								}
							}
						}
					}

					if ( init )
					{
						uint64_t const h = Ptrim->rank1(n-1);
						SR.push_back(SplitResultElement(l,h,s,0,0));
					}

					setUntrimmedSplit(SR);

					return SR;
				}

				SplitResult splitDbRL(uint64_t const maxmem, std::vector<uint64_t> const & RL) const
				{
					SplitResult SR;

					if ( RL.size() )
					{
						uint64_t low = 0;

						while ( low < RL.size() )
						{
							uint64_t s = RL[low];
							uint64_t high = low+1;

							while ( high < RL.size() && s+RL[high] <= maxmem )
								s += RL[high++];

							SR.push_back(SplitResultElement(low,high,s,0,0));

							low = high;
						}
					}

					setUntrimmedSplit(SR);

					return SR;
				}

				struct SplitDbRLTwoResult
				{
					SplitResult SR;
					SplitResult TSR;
					std::vector<uint64_t> V;

					SplitDbRLTwoResult() {}
				};

				SplitDbRLTwoResult splitDbRLTwo(uint64_t const maxmem, uint64_t const superblocksize, std::vector<uint64_t> const & RL, double const fuzz = 0.01) const
				{
					SplitResult SR;
					SplitResult TSR;
					std::vector<uint64_t> V;

					if ( RL.size() )
					{
						uint64_t low = 0;

						while ( low < RL.size() )
						{
							uint64_t const tlow = low;

							uint64_t t = 0;
							uint64_t ti = 0;
							while ( low < RL.size() && t+RL[low] <= superblocksize )
							{
								uint64_t s = RL[low];
								t += RL[low];
								uint64_t high = low+1;

								while ( high < RL.size() && s+RL[high] <= maxmem && t+RL[high] <= superblocksize )
								{
									t += RL[high];
									s += RL[high];
									high += 1;
								}

								SR.push_back(SplitResultElement(low,high,s,0,0));
								ti += 1;

								low = high;
							}

							uint64_t const thigh = low;

							while (
								ti > 1 &&
								(SR[SR.size()-2].size + SR[SR.size()-1].size) <= (maxmem * (1.0+fuzz))
							)
							{
								SR[SR.size()-2].size += SR[SR.size()-1].size;
								SR[SR.size()-2].high = SR[SR.size()-1].high;
								SR.pop_back();
								ti -= 1;
							}

							TSR.push_back(SplitResultElement(tlow,thigh,t,0,0));
							V.push_back(ti);
						}
					}

					setUntrimmedSplit(SR);
					setUntrimmedSplit(TSR);

					SplitDbRLTwoResult R;
					R.SR = SR;
					R.TSR = TSR;
					R.V = V;

					return R;
				}

				template<typename iterator>
				SplitResult splitDbRLPrefix(
					uint64_t const maxmem,
					uint64_t const pref,
					iterator rl_ita,
					iterator rl_ite
				) const
				{
					SplitResult SR;

					iterator const rl_its = rl_ita;

					if ( rl_ita != rl_ite )
					{
						iterator const rl_low = rl_ita;

						uint64_t s = *(rl_ita++);

						while ( (rl_ita != rl_ite) && (s + *rl_ita <= maxmem) && (rl_ita - rl_low < static_cast< ::ptrdiff_t>(pref)) )
							s += *(rl_ita++);

						iterator const rl_high = rl_ita;

						SR.push_back(SplitResultElement(rl_low-rl_its,rl_high-rl_its,s,0,0));
					}

					while ( rl_ita != rl_ite )
					{
						iterator const rl_low = rl_ita;

						uint64_t s = *(rl_ita++);

						while ( (rl_ita != rl_ite) && (s + *rl_ita <= maxmem) )
							s += *(rl_ita++);

						iterator const rl_high = rl_ita;

						SR.push_back(SplitResultElement(rl_low-rl_its,rl_high-rl_its,s,0,0));
					}

					setUntrimmedSplit(SR);

					return SR;
				}

				template<typename iterator>
				std::pair<SplitResult,uint64_t> splitDbRLPrefixStrict(
					uint64_t const maxmem,
					uint64_t pref,
					iterator rl_ita,
					iterator rl_ite
				) const
				{
					SplitResult SR;

					iterator const rl_its = rl_ita;

					uint64_t numprefblocks = 0;

					while ( pref && (rl_ita != rl_ite) )
					{
						iterator const rl_low = rl_ita;

						uint64_t s = *(rl_ita++);
						pref -= 1;

						while ( pref && (rl_ita != rl_ite) && (s + *rl_ita <= maxmem) )
						{
							s += *(rl_ita++);
							pref -= 1;
						}

						iterator const rl_high = rl_ita;

						SR.push_back(SplitResultElement(rl_low-rl_its,rl_high-rl_its,s,0,0));
						numprefblocks += 1;
					}

					while ( rl_ita != rl_ite )
					{
						iterator const rl_low = rl_ita;

						uint64_t s = *(rl_ita++);

						while ( (rl_ita != rl_ite) && (s + *rl_ita <= maxmem) )
							s += *(rl_ita++);

						iterator const rl_high = rl_ita;

						SR.push_back(SplitResultElement(rl_low-rl_its,rl_high-rl_its,s,0,0));
					}

					setUntrimmedSplit(SR);

					return std::pair<SplitResult,uint64_t>(SR,numprefblocks);
				}

				uint64_t trimmedToUntrimmed(size_t const i) const
				{
					if ( i >= size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::trimmedToUntrimmed: read index " << i << " out of range (not in [" << 0 << "," << size() << "))" << std::endl;
						lme.finish();
						throw lme;
					}

					uint64_t const mappedindex = Ptrim->select1(i);

					return mappedindex;
				}

				uint64_t untrimmedToTrimmed(size_t const i) const
				{
					if ( ! isInTrimmed(i) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::untrimmedToTrimmed: read index " << i << " is not in trimmed database" << std::endl;
						lme.finish();
						throw lme;
					}

					return Ptrim->rank1(i)-1;
				}

				bool isInTrimmed(size_t const i) const
				{
					if ( ! blocks.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::isInTrimmed(): blocks vector is empty" << std::endl;
						lme.finish();
						throw lme;
					}

					if ( i >= blocks.back().first )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::isInTrimmed(): index " << i << " is out of range (excluded limit is " << blocks.back().first << ")" << std::endl;
						lme.finish();
						throw lme;
					}

					return (*Ptrim)[i];
				}

				void getReadInterval(size_t const low, size_t const high, std::vector<Read> & V) const
				{
					V.resize(0);
					V.resize(high-low);

					if ( high-low && high > size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getReadInterval: read index " << high-1 << " out of range (not in [" << 0 << "," << size() << "))" << std::endl;
						lme.finish();
						throw lme;
					}

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					for ( size_t i = low; i < high; ++i )
					{
						uint64_t const mappedindex = Ptrim->select1(i);

						if (
							static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
						)
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						V[i-low].deserialise(*Pidxfile);
					}
				}

				uint64_t getReadNameInterval(
					size_t const low, size_t const high,
					libmaus2::autoarray::AutoArray<char> & V,
					libmaus2::autoarray::AutoArray<char const *> & P
					) const
				{
					if ( high < low )
						return 0;

					if ( high-low && high > size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getReadInterval: read index " << high-1 << " out of range (not in [" << 0 << "," << size() << "))" << std::endl;
						lme.finish();
						throw lme;
					}

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;
					char * p = V.begin();
					libmaus2::autoarray::AutoArray<uint64_t> U(high-low);
					P.resize(high-low);

					for ( size_t i = low; i < high; ++i )
					{
						uint64_t const mappedindex = Ptrim->select1(i);

						if (
							static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
						)
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read R(*Pidxfile);
						std::string const rn = getReadName(i,R);

						while ( (V.end() - p) < static_cast<ptrdiff_t>(rn.size()+1) )
						{
							uint64_t const poff = p - V.begin();
							uint64_t const oldsize = V.size();
							uint64_t const one = 1;
							uint64_t const newsize = std::max(one,oldsize<<1);
							V.resize(newsize);
							p = V.begin() + poff;
						}

						char const * s = rn.c_str();
						U[i-low] = p-V.begin();
						std::copy(s,s+rn.size()+1,p);
						p += rn.size()+1;
					}

					for ( uint64_t i = 0; i < U.size(); ++i )
						P[i] = V.begin() + U[i];

					return high-low;
				}

				void getReadVector(std::vector<uint64_t> const & I, std::vector<Read> & V) const
				{
					V.resize(0);

					if ( ! I.size() )
						return;

					V.resize(I.size());

					for ( uint64_t i = 1; i < I.size(); ++i )
						if ( I[i-1] >= I[i] )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "DatabaseFile::getReadVector: index vector is not strictly increasing" << std::endl;
							lme.finish();
							throw lme;
						}
					if ( I.back() >= size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getReadVector: index vector contains values out of range" << std::endl;
						lme.finish();
						throw lme;
					}

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					for ( uint64_t j = 0; j < I.size(); ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(I[j]);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						V[j].deserialise(*Pidxfile);
					}
				}

				void getReadDataVector(
					std::vector<uint64_t> const & I,
					libmaus2::autoarray::AutoArray<char> & B,
					libmaus2::autoarray::AutoArray<uint64_t> & O
				) const
				{
					B.resize(0);
					O.resize(1);
					O[0] = 0;

					if ( ! I.size() )
						return;

					O.resize(I.size()+1);

					for ( uint64_t i = 1; i < I.size(); ++i )
						if ( I[i-1] >= I[i] )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "DatabaseFile::getReadVector: index vector is not strictly increasing" << std::endl;
							lme.finish();
							throw lme;
						}
					if ( I.back() >= size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getReadVector: index vector contains values out of range" << std::endl;
						lme.finish();
						throw lme;
					}

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					for ( uint64_t j = 0; j < I.size(); ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(I[j]);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						O[j] = R.rlen;
					}

					O.prefixSums();
					assert ( O.size() );
					B.resize(O[O.size()-1]);

					libmaus2::aio::InputStream::unique_ptr_type Pbasestr(openBaseStream());

					for ( uint64_t j = 0; j < I.size(); ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(I[j]);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						assert ( static_cast<int64_t>(O[j+1]-O[j]) == R.rlen );
						if ( static_cast<int64_t>(Pbasestr->tellg()) != R.boff )
							Pbasestr->seekg(R.boff,std::ios::beg);

						decodeRead(*Pbasestr,B.begin() + O[j],R.rlen);
					}
				}

				uint64_t getReadDataVectorMemInterval(
					uint64_t const low,
					uint64_t const maxmem,
					libmaus2::autoarray::AutoArray<char> & B,
					libmaus2::autoarray::AutoArray<uint64_t> & O
				) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					uint64_t h = low;
					uint64_t m = 0;
					while ( h < size() && m < maxmem )
					{
						uint64_t const mappedindex = Ptrim->select1(h);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						m += R.rlen;
						h += 1;
					}

					O.resize(h-low+1);
					O[0] = 0;
					B.resize(m);

					libmaus2::aio::InputStream::unique_ptr_type Pbasestr(openBaseStream());
					for ( uint64_t j = low; j < h; ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(j);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						O[j-low+1] = O[j-low] + R.rlen;
						if ( static_cast<int64_t>(Pbasestr->tellg()) != R.boff )
							Pbasestr->seekg(R.boff,std::ios::beg);

						decodeRead(*Pbasestr,B.begin() + O[j-low],R.rlen);
					}

					return h;
				}

				std::pair<uint64_t,uint64_t> getReadDataVectorMemInterval(
					uint64_t const low,
					uint64_t const high,
					uint64_t const maxmem,
					libmaus2::autoarray::AutoArray<char> & B,
					libmaus2::autoarray::AutoArray<uint64_t> & O
				) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					uint64_t h = low;
					uint64_t m = 0;
					while ( h < size() && h < high && m < maxmem )
					{
						uint64_t const mappedindex = Ptrim->select1(h);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						m += R.rlen;
						h += 1;
					}

					O.resize(h-low+1);
					O[0] = 0;
					B.resize(m);

					libmaus2::aio::InputStream::unique_ptr_type Pbasestr(openBaseStream());
					for ( uint64_t j = low; j < h; ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(j);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						O[j-low+1] = O[j-low] + R.rlen;
						if ( static_cast<int64_t>(Pbasestr->tellg()) != R.boff )
							Pbasestr->seekg(R.boff,std::ios::beg);

						decodeRead(*Pbasestr,B.begin() + O[j-low],R.rlen);
					}

					return std::pair<uint64_t,uint64_t>(h,m);
				}

				uint64_t getReadLengthSumInterval(uint64_t const low, uint64_t const high) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;
					uint64_t s = 0;

					for ( uint64_t j = low; j < high; ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(j);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						s += R.rlen;
					}

					return s;
				}

				uint64_t getReadLengthSum() const
				{
					return getReadLengthSumInterval(0,size());
				}

				double getAverageReadLength() const
				{
					return static_cast<double>(getReadLengthSum()) / size();
				}

				template<typename it>
				uint64_t getReadLengthArray(uint64_t const low, uint64_t const high, it A) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					uint64_t maxlen = 0;
					for ( uint64_t j = low; j < high; ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(j);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						A[j-low] = R.rlen;
						maxlen = std::max(maxlen,static_cast<uint64_t>(R.rlen));
					}

					return maxlen;
				}

				template<typename it>
				uint64_t getReadLengthArrayParallel(
					uint64_t const low,
					uint64_t const high,
					it A,
					uint64_t const numthreads
				) const
				{
					uint64_t const size = high-low;
					uint64_t const packsize = (size + numthreads - 1)/numthreads;
					uint64_t maxlen = 0;
					libmaus2::parallel::PosixSpinLock lock;

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const l = std::min(low + t * packsize,high);
						uint64_t const h = std::min(l + packsize,high);
						uint64_t const lmaxlen = getReadLengthArray(l,h,A + (l-low));

						lock.lock();
						maxlen = std::max(maxlen,lmaxlen);
						lock.unlock();
					}

					return maxlen;
				}

				template<typename it>
				uint64_t getReadLengthArrayParallel(it A, uint64_t const numthreads) const
				{
					uint64_t const low = 0;
					uint64_t const high = size();
					uint64_t const size = high-low;
					uint64_t const packsize = (size + numthreads - 1)/numthreads;
					uint64_t maxlen = 0;
					libmaus2::parallel::PosixSpinLock lock;

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const l = std::min(low + t * packsize,high);
						uint64_t const h = std::min(l + packsize,high);
						uint64_t const lmaxlen = getReadLengthArray(l,h,A + (l-low));

						lock.lock();
						maxlen = std::max(maxlen,lmaxlen);
						lock.unlock();
					}

					return maxlen;
				}

				libmaus2::autoarray::AutoArray<uint64_t> getReadLengthArrayParallel(uint64_t const numthreads) const
				{
					libmaus2::autoarray::AutoArray<uint64_t> A(size(),false);
					getReadLengthArrayParallel(A.begin(),numthreads);
					return A;
				}

				struct ReadDataRange
				{
					typedef ReadDataRange this_type;
					typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
					typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

					libmaus2::autoarray::AutoArray<uint8_t> D;
					libmaus2::autoarray::AutoArray<uint64_t> L;
					uint64_t n;
					uint64_t maxlen;

					uint64_t byteSize() const
					{
						return D.byteSize() + L.byteSize() + sizeof(n) + sizeof(maxlen);
					}

					ReadDataRange() : n(0), maxlen(0) {}

					std::string operator[](uint64_t const i) const
					{
						std::pair<uint8_t const *, uint8_t const *> P = get(i);
						return std::string(P.first,P.second);
					}

					std::pair<uint8_t const *, uint8_t const *> get(uint64_t const i) const
					{
						assert ( i < size() );

						uint8_t const * p = D.begin() + L[i] + 2*i + 1;
						uint64_t const l = L[i+1]-L[i];

						return std::pair<uint8_t const *, uint8_t const *>(p,p + l);
					}

					uint64_t size() const
					{
						return n;
					}
				};

				template<bool rc>
				void decodeReadDataInterval(
					ReadDataRange & R,
					uint64_t const base,
					uint64_t const low,
					uint64_t const high,
					uint8_t const termval
				) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;
					libmaus2::aio::InputStream::unique_ptr_type Pbasestr(openBaseStream());
					std::istream & basestr = *Pbasestr;

					uint8_t * p = R.D.begin() + R.L[low] + 2*low;

					for ( uint64_t j = low; j < high; ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(base + j);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const RE(*Pidxfile);

						if ( static_cast<int64_t>(basestr.tellg()) != RE.boff )
							basestr.seekg(RE.boff,std::ios::beg);

						assert ( static_cast<int64_t>(RE.rlen) == static_cast<int64_t>(R.L[j+1] - R.L[j]) );

						*(p++) = termval;
						if ( rc )
							decodeReadRCNoDecode(basestr,reinterpret_cast<char * >(p),RE.rlen);
						else
							decodeReadNoDecode(basestr,reinterpret_cast<char * >(p),RE.rlen);
						p += RE.rlen;
						*(p++) = termval;
					}

					assert ( p == R.D.begin() + R.L[high] + 2*high );
				}

				template<typename iterator>
				void decodeReadLengthByArray(
					ReadDataRange & R,
					iterator ita,
					uint64_t const low,
					uint64_t const high
				) const
				{
					libmaus2::aio::InputStreamInstance idxfile(idxpath);

					for ( uint64_t j = low; j < high; ++j )
					{
						uint64_t const unmappedindex = ita[j];
						uint64_t const mappedindex = Ptrim->select1(unmappedindex);

						if (
							static_cast<int64_t>(idxfile.tellg())
							!=
							static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
						)
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const RE(idxfile);

						R.L[j] = RE.rlen;
					}
				}

				template<bool rc, typename iterator>
				void decodeReadDataByArray(
					ReadDataRange & R,
					iterator ita,
					uint64_t const low,
					uint64_t const high,
					uint8_t const termval
				) const
				{
					libmaus2::aio::InputStreamInstance idxfile(idxpath);
					libmaus2::aio::InputStreamInstance basestr(bpspath);

					uint8_t * p = R.D.begin() + R.L[low] + 2*low;

					for ( uint64_t j = low; j < high; ++j )
					{
						uint64_t const unmappedindex = ita[j];
						uint64_t const mappedindex = Ptrim->select1(unmappedindex);

						if (
							static_cast<int64_t>(idxfile.tellg())
							!=
							static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
						)
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const RE(idxfile);

						if ( static_cast<int64_t>(basestr.tellg()) != RE.boff )
							basestr.seekg(RE.boff,std::ios::beg);

						assert ( static_cast<int64_t>(RE.rlen) == static_cast<int64_t>(R.L[j+1] - R.L[j]) );

						*(p++) = termval;
						if ( rc )
							decodeReadRCNoDecode(basestr,reinterpret_cast<char * >(p),RE.rlen);
						else
							decodeReadNoDecode(basestr,reinterpret_cast<char * >(p),RE.rlen);
						p += RE.rlen;
						*(p++) = termval;
					}

					assert ( p == R.D.begin() + R.L[high] + 2*high );
				}

				template<bool rc, typename iterator>
				void decodeReadDataByArrayDecode(
					ReadDataRange & R,
					iterator ita,
					uint64_t const low,
					uint64_t const high,
					uint8_t const termval
				) const
				{
					libmaus2::aio::InputStreamInstance idxfile(idxpath);
					libmaus2::aio::InputStreamInstance basestr(bpspath);

					uint8_t * p = R.D.begin() + R.L[low] + 2*low;

					for ( uint64_t j = low; j < high; ++j )
					{
						uint64_t const unmappedindex = ita[j];
						uint64_t const mappedindex = Ptrim->select1(unmappedindex);

						if (
							static_cast<int64_t>(idxfile.tellg())
							!=
							static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
						)
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const RE(idxfile);

						if ( static_cast<int64_t>(basestr.tellg()) != RE.boff )
							basestr.seekg(RE.boff,std::ios::beg);

						assert ( static_cast<int64_t>(RE.rlen) == static_cast<int64_t>(R.L[j+1] - R.L[j]) );

						*(p++) = termval;
						if ( rc )
							decodeReadRC(basestr,reinterpret_cast<char * >(p),RE.rlen);
						else
							decodeRead(basestr,reinterpret_cast<char * >(p),RE.rlen);
						p += RE.rlen;
						*(p++) = termval;
					}

					assert ( p == R.D.begin() + R.L[high] + 2*high );
				}

				template<typename iterator>
				ReadDataRange::unique_ptr_type decodeReadDataByArrayParallel(
					iterator ita,
					uint64_t const n,
					uint64_t const numthreads,
					bool const rc,
					uint8_t const termval
				) const
				{
					ReadDataRange::unique_ptr_type tptr(new ReadDataRange);
					tptr->n = n;
					tptr->L.resize(tptr->n + 1);

					uint64_t const readsperthread = (n + numthreads - 1)/numthreads;

					uint64_t volatile gmaxlen = 0;
					libmaus2::parallel::PosixSpinLock gmaxlenlock;
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const low  = std::min(t * readsperthread  , n);
						uint64_t const high = std::min(low + readsperthread, n);

						decodeReadLengthByArray(
							*tptr,
							ita,
							low,
							high
						);

						uint64_t lmaxlen = 0;
						for ( uint64_t i = low; i < high; ++i )
							lmaxlen = std::max(lmaxlen,tptr->L[i]);

						gmaxlenlock.lock();
						gmaxlen = std::max(static_cast<uint64_t>(gmaxlen),static_cast<uint64_t>(lmaxlen));
						gmaxlenlock.unlock();
					}

					tptr->maxlen = gmaxlen;
					libmaus2::util::PrefixSums::parallelPrefixSums(tptr->L.begin(),tptr->L.end(),numthreads);
					tptr->D.resize(tptr->L[tptr->n]+2*tptr->n);

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const low  = std::min(t * readsperthread  , n);
						uint64_t const high = std::min(low + readsperthread, n);

						if ( rc )
							decodeReadDataByArray<true>(*tptr,ita,low,high,termval);
						else
							decodeReadDataByArray<false>(*tptr,ita,low,high,termval);
					}

					return UNIQUE_PTR_MOVE(tptr);
				}

				template<typename iterator>
				ReadDataRange::unique_ptr_type decodeReadDataByArrayParallelDecode(
					iterator ita,
					uint64_t const n,
					uint64_t const numthreads,
					bool const rc,
					uint8_t const termval
				) const
				{
					ReadDataRange::unique_ptr_type tptr(new ReadDataRange);
					tptr->n = n;
					tptr->L.resize(tptr->n + 1);

					uint64_t const readsperthread = (n + numthreads - 1)/numthreads;

					uint64_t volatile gmaxlen = 0;
					libmaus2::parallel::PosixSpinLock gmaxlenlock;
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const low  = std::min(t * readsperthread  , n);
						uint64_t const high = std::min(low + readsperthread, n);

						decodeReadLengthByArray(
							*tptr,
							ita,
							low,
							high
						);

						uint64_t lmaxlen = 0;
						for ( uint64_t i = low; i < high; ++i )
							lmaxlen = std::max(lmaxlen,tptr->L[i]);

						gmaxlenlock.lock();
						gmaxlen = std::max(static_cast<uint64_t>(gmaxlen),static_cast<uint64_t>(lmaxlen));
						gmaxlenlock.unlock();
					}

					tptr->maxlen = gmaxlen;
					libmaus2::util::PrefixSums::parallelPrefixSums(tptr->L.begin(),tptr->L.end(),numthreads);
					tptr->D.resize(tptr->L[tptr->n]+2*tptr->n);

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const low  = std::min(t * readsperthread  , n);
						uint64_t const high = std::min(low + readsperthread, n);

						if ( rc )
							decodeReadDataByArrayDecode<true>(*tptr,ita,low,high,termval);
						else
							decodeReadDataByArrayDecode<false>(*tptr,ita,low,high,termval);
					}

					return UNIQUE_PTR_MOVE(tptr);
				}

				ReadDataRange::unique_ptr_type decodeReadIntervalParallel(
					uint64_t const low,
					uint64_t const high,
					uint64_t const numthreads,
					bool const rc,
					uint8_t const termval
				) const
				{
					ReadDataRange::unique_ptr_type tptr(new ReadDataRange);
					tptr->n = high-low;
					tptr->L.resize(tptr->n + 1);
					tptr->maxlen = getReadLengthArrayParallel(low,high,tptr->L.begin(),numthreads);
					libmaus2::util::PrefixSums::parallelPrefixSums(tptr->L.begin(),tptr->L.end(),numthreads);
					tptr->D.resize(tptr->L[tptr->n]+2*tptr->n);

					uint64_t const size = high-low;
					uint64_t const packsize = (size+numthreads-1)/numthreads;
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						uint64_t const l = std::min(i*packsize,size);
						uint64_t const h = std::min(l+packsize,size);
						if ( rc )
							decodeReadDataInterval<true>(*tptr,low,l,h,termval);
						else
							decodeReadDataInterval<false>(*tptr,low,l,h,termval);
					}

					return UNIQUE_PTR_MOVE(tptr);
				}

				template<typename iterator>
				iterator getReadDataVectorMemInterval(
					iterator ita, iterator ite,
					uint64_t const maxmem,
					libmaus2::autoarray::AutoArray<char> & B,
					libmaus2::autoarray::AutoArray<uint64_t> & O
				) const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;

					uint64_t h = 0;
					uint64_t m = 0;
					iterator itc = ita;
					while ( itc != ite && m < maxmem )
					{
						uint64_t const mappedindex = Ptrim->select1(*itc);

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						m += R.rlen;
						h += 1;
						itc ++;
					}

					O.resize(h+1);
					O[0] = 0;
					B.resize(m);

					libmaus2::aio::InputStream::unique_ptr_type Pbasestr(openBaseStream());
					itc = ita;
					for ( uint64_t j = 0; j < h; ++j )
					{
						uint64_t const mappedindex = Ptrim->select1(*(itc++));

						if ( static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize) )
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

						Read const R(*Pidxfile);
						O[j+1] = O[j] + R.rlen;
						if ( static_cast<int64_t>(Pbasestr->tellg()) != R.boff )
							Pbasestr->seekg(R.boff,std::ios::beg);

						decodeRead(*Pbasestr,B.begin() + O[j],R.rlen);
					}

					return itc;
				}


				void getReadLengthInterval(size_t const low, size_t const high, std::vector<uint64_t> & V) const
				{
					V.resize(0);
					V.resize(high-low);

					if ( high-low && high > size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "DatabaseFile::getReadInterval: read index " << high-1 << " out of range (not in [" << 0 << "," << size() << "))" << std::endl;
						lme.finish();
						throw lme;
					}

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					std::istream & idxfile = *Pidxfile;
					libmaus2::dazzler::db::Read R;

					for ( size_t i = low; i < high; ++i )
					{
						uint64_t const mappedindex = Ptrim->select1(i);

						if (
							static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
						)
							idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);


						R.deserialise(*Pidxfile);
						V[i-low] = R.rlen;
					}
				}

				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				typedef ::libmaus2::bitio::SynchronousCompactArray read_length_array_type;
				#else
				typedef ::libmaus2::bitio::CompactArray read_length_array_type;
				#endif

				typedef read_length_array_type::unique_ptr_type read_length_array_ptr_type;

				read_length_array_ptr_type getReadLengthArray(uint64_t const numthreads) const
				{
					uint64_t const b = libmaus2::math::numbits(indexbase.maxlen);
					uint64_t const n = size();

					uint64_t const readsperthread = (n + numthreads - 1)/numthreads;

					read_length_array_ptr_type tptr(new read_length_array_type(n,b,0/*pad*/,0/*erase*/));

					#if !defined(LIBMAUS2_HAVE_SYNC_OPS)
					libmaus2::parallel::OMPLock lock;
					#endif

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numthreads ; ++t )
					{
						uint64_t const low = t * readsperthread;
						uint64_t const high = std::min(low+readsperthread,n);

						libmaus2::aio::InputStreamInstance idxfile(idxpath);
						libmaus2::dazzler::db::Read R;

						for ( uint64_t i = low; i < high; ++i )
						{
							uint64_t const mappedindex = Ptrim->select1(i);

							if (
								static_cast<int64_t>(idxfile.tellg()) != static_cast<int64_t>(indexoffset + mappedindex * Read::serialisedSize)
							)
								idxfile.seekg(indexoffset + mappedindex * Read::serialisedSize);

							R.deserialise(idxfile);

							#if defined(LIBMAUS2_HAVE_SYNC_OPS)
							tptr->set(i,R.rlen);
							#else
							libmaus2::parallel::ScopeLock slock(lock);
							tptr->set(i,R.rlen);
							#endif
						}

					}

					return UNIQUE_PTR_MOVE(tptr);
				}

				void getAllReads(std::vector<Read> & V) const
				{
					getReadInterval(0,size(),V);
				}

				void getAllReadLengths(std::vector<uint64_t> & V) const
				{
					getReadLengthInterval(0,size(),V);
				}

				void decodeReads(size_t const low, size_t const high, libmaus2::autoarray::AutoArray<char> & A, std::vector<uint64_t> & off) const
				{
					off.resize((high-low)+1);
					off[0] = 0;

					if ( high - low )
					{
						std::vector<Read> V;
						getReadInterval(low,high,V);

						for ( size_t i = 0; i < high-low; ++i )
							off[i+1] = off[i] + V[i].rlen;

						libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
						std::istream & bpsfile = *Pbpsfile;

						A = libmaus2::autoarray::AutoArray<char>(off.back(),false);
						for ( size_t i = 0; i < high-low; ++i )
						{
							if (
								static_cast<int64_t>(bpsfile.tellg()) != static_cast<int64_t>(V[i].boff)
							)
								bpsfile.seekg(V[i].boff,std::ios::beg);
							decodeRead(*Pbpsfile,A.begin()+off[i],off[i+1]-off[i]);
						}
					}
				}

				void decodeReadsMappedTerm(size_t const low, size_t const high, libmaus2::autoarray::AutoArray<char> & A, std::vector<uint64_t> & off) const
				{
					off.resize((high-low)+1);
					off[0] = 0;

					if ( high - low )
					{
						std::vector<Read> V;
						getReadInterval(low,high,V);

						for ( size_t i = 0; i < high-low; ++i )
						{
							uint64_t const rlen = V[i].rlen;
							uint64_t const rlenp = rlen + 2; // padding
							off[i+1] = off[i] + rlenp;
						}

						libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
						std::istream & bpsfile = *Pbpsfile;

						A = libmaus2::autoarray::AutoArray<char>(off.back(),false);
						for ( size_t i = 0; i < high-low; ++i )
						{
							if (
								static_cast<int64_t>(bpsfile.tellg()) != static_cast<int64_t>(V[i].boff)
							)
								bpsfile.seekg(V[i].boff,std::ios::beg);
							{
								// offset of padded read in data array
								uint64_t const p = off[i];
								// padded read length
								uint64_t const rlenp = off[i+1]-off[i];
								assert ( rlenp >= 2 );
								// actual read length
								uint64_t const rlen = rlenp-2;
								// decode read data to A,C,G,T char array
								decodeRead(*Pbpsfile,A.begin()+p+1,rlen);
								// put N in front of and behind read data
								A[p] = 'N';
								A[p+rlen+1] = 'N';
								// map A,C,G,T,N to 0,1,2,3,4
								char * c = A.begin() + p;
								for ( uint64_t j = 0; j < rlenp; ++j )
									c[j] = libmaus2::fastx::mapChar(c[j]);
							}
						}
					}
				}

				void decodeReadsAndReverseComplement(size_t const low, size_t const high, libmaus2::autoarray::AutoArray<char> & A, std::vector<uint64_t> & off) const
				{
					off.resize((high-low)+1);
					off[0] = 0;

					if ( high - low )
					{
						std::vector<Read> V;
						getReadInterval(low,high,V);

						for ( size_t i = 0; i < high-low; ++i )
							off[i+1] = off[i] + 2*V[i].rlen;

						libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
						std::istream & bpsfile = *Pbpsfile;

						A = libmaus2::autoarray::AutoArray<char>(off.back(),false);
						for ( size_t i = 0; i < high-low; ++i )
						{
							if (
								static_cast<int64_t>(bpsfile.tellg()) != static_cast<int64_t>(V[i].boff)
							)
								bpsfile.seekg(V[i].boff,std::ios::beg);

							assert ( (off[i+1]-off[i]) % 2 == 0 );
							uint64_t const rl = (off[i+1]-off[i])/2;

							decodeRead(*Pbpsfile,A.begin()+off[i],rl);
							std::copy(A.begin()+off[i],A.begin()+off[i]+rl,A.begin()+off[i]+rl);
							std::reverse(A.begin()+off[i]+rl,A.begin()+off[i]+2*rl);
							for ( uint64_t j = 0; j < rl; ++j )
								A[off[i]+rl+j] = libmaus2::fastx::invertUnmapped(A[off[i]+rl+j]);
						}
					}
				}

				void decodeReadsAndReverseComplementMappedTerm(size_t const low, size_t const high, libmaus2::autoarray::AutoArray<char> & A, std::vector<uint64_t> & off) const
				{
					off.resize((high-low)+1);
					off[0] = 0;

					if ( high - low )
					{
						std::vector<Read> V;
						getReadInterval(low,high,V);

						for ( size_t i = 0; i < high-low; ++i )
						{
							uint64_t const rlen = V[i].rlen;
							uint64_t const rlenp = rlen + 2; // padding
							off[i+1] = off[i] + 2*rlenp;
						}

						libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
						std::istream & bpsfile = *Pbpsfile;

						A = libmaus2::autoarray::AutoArray<char>(off.back(),false);
						for ( size_t i = 0; i < high-low; ++i )
						{
							if (
								static_cast<int64_t>(bpsfile.tellg()) != static_cast<int64_t>(V[i].boff)
							)
								bpsfile.seekg(V[i].boff,std::ios::beg);
							{
								// offset of padded read in data array
								uint64_t const p = off[i];
								// padded read length
								uint64_t const rlenp = (off[i+1]-off[i])/2;
								assert ( rlenp >= 2 );
								// offset of padded reverse complement
								uint64_t const pr = p + rlenp;
								// actual read length
								uint64_t const rlen = rlenp-2;
								// decode read data to A,C,G,T char array
								decodeRead(*Pbpsfile,A.begin()+p+1,rlen);
								// put N in front of and behind read data
								A[p] = 'N';
								A[p+rlen+1] = 'N';
								// copy
								std::copy(A.begin()+p,A.begin()+p+rlenp,A.begin()+pr);
								// reverse
								std::reverse(A.begin()+pr,A.begin()+pr+rlenp);
								// complement
								for ( uint64_t j = 0; j < rlenp; ++j )
									A[pr+j] = libmaus2::fastx::invertUnmapped(A[pr+j]);
								// map A,C,G,T,N to 0,1,2,3,4
								char * c = A.begin() + p;
								for ( uint64_t j = 0; j < 2*rlenp; ++j )
									c[j] = libmaus2::fastx::mapChar(c[j]);
							}
						}
					}
				}


				size_t decodeRead(size_t const i, libmaus2::autoarray::AutoArray<char> & A) const
				{
					Read const R = getRead(i);
					libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
					Pbpsfile->seekg(R.boff,std::ios::beg);
					return decodeRead(*Pbpsfile,A,R.rlen);
				}

				libmaus2::aio::InputStream::unique_ptr_type openBaseStream() const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pbpsfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(bpspath));
					return UNIQUE_PTR_MOVE(Pbpsfile);
				}

				libmaus2::aio::InputStream::unique_ptr_type openIndexStream() const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					return UNIQUE_PTR_MOVE(Pidxfile);
				}

				std::string operator[](size_t const i) const
				{
					libmaus2::autoarray::AutoArray<char> A;
					size_t const rlen = decodeRead(i,A);
					return std::string(A.begin(),A.begin()+rlen);
				}

				std::basic_string<uint8_t> getu(size_t const i, bool const inv = false) const
				{
					libmaus2::autoarray::AutoArray<char> A;
					size_t const rlen = decodeRead(i,A);

					if ( inv )
					{
						std::reverse(A.begin(),A.begin()+rlen);
						for ( size_t i = 0; i < rlen; ++i )
							A[i] = libmaus2::fastx::invertUnmapped(A[i]);
					}

					return std::basic_string<uint8_t>(A.begin(),A.begin()+rlen);
				}

				std::string decodeRead(size_t const i, bool const inv) const
				{
					libmaus2::autoarray::AutoArray<char> A;
					size_t const rlen = decodeRead(i,A);
					if ( inv )
					{
						std::reverse(A.begin(),A.begin()+rlen);
						for ( size_t i = 0; i < rlen; ++i )
							A[i] = libmaus2::fastx::invertUnmapped(A[i]);
					}
					return std::string(A.begin(),A.begin()+rlen);
				}

				void decodeAllReads(libmaus2::autoarray::AutoArray<char> & A, std::vector<uint64_t> & off) const
				{
					decodeReads(0, size(), A, off);
				}

				size_t size() const
				{
					return Ptrim->n ? Ptrim->rank1(Ptrim->n-1) : 0;
				}

				/**
				 * compute the trim vector
				 **/
				void computeTrimVector()
				{
					if ( all && cutoff < 0 )
						return;

					int64_t const n = indexbase.ureads;
					uint64_t numkept = 0;
					libmaus2::rank::ImpCacheLineRank::unique_ptr_type Ttrim(new libmaus2::rank::ImpCacheLineRank(n));
					Ptrim = UNIQUE_PTR_MOVE(Ttrim);

					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					Pidxfile->seekg(indexoffset);

					libmaus2::rank::ImpCacheLineRank::WriteContext context = Ptrim->getWriteContext();
					indexbase.totlen = 0;
					indexbase.maxlen = 0;
					for ( int64_t i = 0; i < n; ++i )
					{
						Read R(*Pidxfile);

						// do we keep the read after filtering?
						bool const keep =
							(all || (R.flags & Read::DB_BEST) != 0)
							&&
							((cutoff < 0) || (R.rlen >= cutoff))
						;

						if ( keep )
						{
							if ( i >= indexbase.ufirst && i < indexbase.ufirst+indexbase.nreads )
							{
								numkept++;
								indexbase.totlen += R.rlen;
								indexbase.maxlen = std::max(indexbase.maxlen,R.rlen);
							}
						}

						context.writeBit(keep);
					}

					context.flush();

					indexbase.trimmed = true;
					indexbase.nreads = numkept;
				}

				uint64_t computeReadLengthSum() const
				{
					libmaus2::aio::InputStream::unique_ptr_type Pidxfile(libmaus2::aio::InputStreamFactoryContainer::constructUnique(idxpath));
					Pidxfile->seekg(indexoffset);

					uint64_t s = 0;
					for ( uint64_t i = 0; i < Ptrim->n; ++i )
					{
						Read R(*Pidxfile);
						if ( (*Ptrim)[i] )
							s += R.rlen;
					}

					return s;
				}

				static std::string getBlockTrackFileName(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname, std::string const & type)
				{
					if ( part )
					{
						std::ostringstream ostr;
						ostr << path << "/" << "." << root << "." << part << "." << trackname << "." << type;
						return ostr.str();
					}
					else
					{
						std::ostringstream ostr;
						ostr << path << "/" << "." << root << "." << trackname << "." << type;
						return ostr.str();
					}
				}

				static std::string getBlockTrackAnnoFileName(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname)
				{
					return getBlockTrackFileName(path,root,part,trackname,"anno");
				}

				static std::string getBlockTrackDataFileName(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname)
				{
					return getBlockTrackFileName(path,root,part,trackname,"data");
				}

				std::string getBlockTrackAnnoFileName(std::string const & trackname, int64_t const part) const
				{
					return getBlockTrackAnnoFileName(path,root,part,trackname);
				}

				std::string getBlockTrackDataFileName(std::string const & trackname, int64_t const part) const
				{
					return getBlockTrackDataFileName(path,root,part,trackname);
				}

				static std::string getTrackFileName(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname, std::string const & type)
				{
					if ( part )
					{
						std::ostringstream ostr;
						ostr << path << "/" << "." << root << "." << part << "." << trackname << "." << type;
						if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(ostr.str()) )
							return ostr.str();
					}

					std::ostringstream ostr;
					ostr << path << "/" << "." << root << "." << trackname << "." << type;

					return ostr.str();
				}

				static std::string getTrackAnnoFileName(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname)
				{
					return getTrackFileName(path,root,part,trackname,"anno");
				}

				static std::string getTrackDataFileName(std::string const & path, std::string const & root, int64_t const part, std::string const & trackname)
				{
					return getTrackFileName(path,root,part,trackname,"data");
				}

				std::string getTrackAnnoFileName(std::string const & trackname) const
				{
					return getTrackAnnoFileName(path,root,part,trackname);
				}

				std::string getTrackDataFileName(std::string const & trackname) const
				{
					return getTrackDataFileName(path,root,part,trackname);
				}

				bool haveTrackAnno(std::string const & trackname, int64_t const rpart = -1) const
				{
					int64_t const part = (rpart >= 0) ? rpart : this->part;

					std::string annoname;

					// check whether annotation/track is specific to this block
					if (
						part &&
						libmaus2::aio::InputStreamFactoryContainer::tryOpen(
							this->path + "/" + "." + this->root + "." + libmaus2::util::NumberSerialisation::formatNumber(part,0) + "." + trackname + ".anno"
						)
					)
					{
						annoname = this->path + "/" + "." + this->root + "." + libmaus2::util::NumberSerialisation::formatNumber(part,0) + "." + trackname + ".anno";
					}
					// no, try whole database
					else
					{
						annoname = this->path + "/" + "." + this->root + "." + trackname + ".anno";
					}

					return libmaus2::aio::InputStreamFactoryContainer::tryOpen(annoname);
				}

				Track::unique_ptr_type readTrack(std::string const & trackname, int64_t const rpart = -1) const
				{
					int64_t const part = (rpart >= 0) ? rpart : this->part;

					bool ispart = false;
					std::string annoname;

					// check whether annotation/track is specific to this block
					if (
						part &&
						libmaus2::aio::InputStreamFactoryContainer::tryOpen(
							this->path + "/" + "." + this->root + "." + libmaus2::util::NumberSerialisation::formatNumber(part,0) + "." + trackname + ".anno"
						)
					)
					{
						ispart = true;
						annoname = this->path + "/" + "." + this->root + "." + libmaus2::util::NumberSerialisation::formatNumber(part,0) + "." + trackname + ".anno";
					}
					// no, try whole database
					else
					{
						annoname = this->path + "/" + "." + this->root + "." + trackname + ".anno";
					}

					if ( ! libmaus2::aio::InputStreamFactoryContainer::tryOpen(annoname) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Unable to open " << annoname << std::endl;
						lme.finish();
						throw lme;
					}

					std::string dataname;
					if ( ispart )
						dataname = this->path + "/" + "." + this->root + "." + libmaus2::util::NumberSerialisation::formatNumber(part,0) + "." + trackname + ".data";
					else
						dataname = this->path + "/" + "." + this->root + "." + trackname + ".data";

					libmaus2::aio::InputStream::unique_ptr_type Panno(libmaus2::aio::InputStreamFactoryContainer::constructUnique(annoname));
					std::istream & anno = *Panno;

					uint64_t offset = 0;
					int32_t tracklen = InputBase::getLittleEndianInteger4(anno,offset);
					int32_t size = InputBase::getLittleEndianInteger4(anno,offset);
					uint64_t const tsize = size;

					// mask track
					if ( size == 0 )
						size = 8;

					libmaus2::aio::InputStream::unique_ptr_type Pdata;
					if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(dataname) )
					{
						libmaus2::aio::InputStream::unique_ptr_type Tdata(libmaus2::aio::InputStreamFactoryContainer::constructUnique(dataname));
						Pdata = UNIQUE_PTR_MOVE(Tdata);
					}

					TrackAnnoInterface::unique_ptr_type PDanno;
					libmaus2::autoarray::AutoArray<unsigned char>::unique_ptr_type Adata;

					// number of reads in database loaded
					uint64_t const nreads = (ispart ? tracklen : this->indexbase.nreads);

					// check whether we have a consistent number of reads
					if ( static_cast<int64_t>(nreads) != tracklen )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Number of reads in track " << tracklen << " does not match number of reads in block/database " << nreads << std::endl;
						lme.finish();
						throw lme;
					}

					// if database is part but track is for complete database then seek
					if ( part && ! ispart )
					{
						if ( this->indexbase.trimmed )
							anno.seekg(size * this->indexbase.tfirst, std::ios::cur);
						else
							anno.seekg(size * this->indexbase.ufirst, std::ios::cur);
					}

					uint64_t const annoread = Pdata ? (nreads+1) : nreads;

					if ( size == 1 )
					{
						TrackAnno<uint8_t>::unique_ptr_type Tanno(new TrackAnno<uint8_t>(annoread));

						for ( uint64_t i = 0; i < annoread; ++i )
						{
							int const c = Panno->get();
							if ( c < 0 )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "TrackIO::readTrack: unexpected EOF" << std::endl;
								lme.finish();
								throw lme;
							}
							Tanno->A[i] = c;
						}

						TrackAnnoInterface::unique_ptr_type Tint(Tanno.release());
						PDanno = UNIQUE_PTR_MOVE(Tint);
					}
					if ( size == 2 )
					{
						TrackAnno<uint16_t>::unique_ptr_type Tanno(new TrackAnno<uint16_t>(annoread));

						uint64_t offset = 0;
						for ( uint64_t i = 0; i < annoread; ++i )
							Tanno->A[i] = InputBase::getLittleEndianInteger2(*Panno,offset);

						TrackAnnoInterface::unique_ptr_type Tint(Tanno.release());
						PDanno = UNIQUE_PTR_MOVE(Tint);
					}
					if ( size == 4 )
					{
						TrackAnno<uint32_t>::unique_ptr_type Tanno(new TrackAnno<uint32_t>(annoread));

						uint64_t offset = 0;
						for ( uint64_t i = 0; i < annoread; ++i )
							Tanno->A[i] = InputBase::getLittleEndianInteger4(*Panno,offset);

						TrackAnnoInterface::unique_ptr_type Tint(Tanno.release());
						PDanno = UNIQUE_PTR_MOVE(Tint);
					}
					else if ( size == 8 )
					{
						TrackAnno<uint64_t>::unique_ptr_type Tanno(new TrackAnno<uint64_t>(annoread));

						uint64_t offset = 0;
						for ( uint64_t i = 0; i < annoread; ++i )
							Tanno->A[i] = InputBase::getLittleEndianInteger8(*Panno,offset);

						TrackAnnoInterface::unique_ptr_type Tint(Tanno.release());
						PDanno = UNIQUE_PTR_MOVE(Tint);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "TrackIO::readTrack: unknown size " << size << std::endl;
						lme.finish();
						throw lme;
					}

					if ( Pdata )
					{
						if ( (*PDanno)[0] )
							Pdata->seekg((*PDanno)[0]);

						int64_t const toread = (*PDanno)[nreads] - (*PDanno)[0];

						libmaus2::autoarray::AutoArray<unsigned char>::unique_ptr_type Tdata(new libmaus2::autoarray::AutoArray<unsigned char>(toread,false));
						Adata = UNIQUE_PTR_MOVE(Tdata);

						Pdata->read ( reinterpret_cast<char *>(Adata->begin()), toread );

						if ( Pdata->gcount() != toread )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "TrackIO::readTrack: failed to read data" << size << std::endl;
							lme.finish();
							throw lme;
						}

						PDanno->shift((*PDanno)[0]);
					}
					// incomplete

					Track::unique_ptr_type track(new Track(trackname,PDanno,Adata,tsize));

					return UNIQUE_PTR_MOVE(track);
				}

				static std::ostream & serialiseSingleFileRawDatabase(std::ostream & out, uint64_t const numreads, std::string const prolog, std::string const fn)
				{
					out << "files = " << std::setw(9) << std::setfill(' ') << 1 << std::setw(0) << "\n";
					out << "  " << std::setw(9) << std::setfill(' ') << numreads << std::setw(0) << " " << prolog << " " << fn << "\n";
					out << "blocks = " << std::setw(9) << std::setfill(' ') << 1 << std::setw(0) << "\n";
					out << "size = " << std::setw(9) << std::setfill(' ') << 200 << std::setw(0)
						<< " cutoff = " << std::setw(9) << std::setfill(' ') << 0 << std::setw(0)
						<< " all = " << std::setw(1) << std::setfill(' ') << 0 << std::setw(0)
						<< "\n";
					out << " "
						<< std::setw(9) << std::setfill(' ') << 0 << std::setw(0) << " "
						<< std::setw(9) << std::setfill(' ') << 0 << std::setw(0) << "\n";
					out << " "
						<< std::setw(9) << std::setfill(' ') << numreads << std::setw(0) << " "
						<< std::setw(9) << std::setfill(' ') << numreads << std::setw(0) << "\n";

					return out;
				}

				std::ostream & serialise(std::ostream & out) const
				{
					out << "files = " << std::setw(9) << std::setfill(' ') << fileinfo.size() << std::setw(0) << "\n";
					for ( uint64_t i = 0; i < fileinfo.size(); ++i )
						out << "  " << std::setw(9) << std::setfill(' ') << fileinfo[i].fnumreads << std::setw(0) << " " << fileinfo[i].fastaprolog << " " << fileinfo[i].fastafn << "\n";
					out << "blocks = " << std::setw(9) << std::setfill(' ') << 1 << std::setw(0) << "\n";
					out << "size = " << std::setw(9) << std::setfill(' ') << blocksize << std::setw(0)
						<< " cutoff = " << std::setw(9) << std::setfill(' ') << cutoff << std::setw(0)
						<< " all = " << std::setw(1) << std::setfill(' ') << all << std::setw(0)
						<< "\n";
					for ( uint64_t i = 0; i < blocks.size(); ++i )
						out << " "
							<< std::setw(9) << std::setfill(' ') << blocks[i].first /* unfiltered */ << std::setw(0) << " "
							<< std::setw(9) << std::setfill(' ') << blocks[i].second /* filtered */ << std::setw(0) << "\n";

					return out;
				}

				std::ostream & serialise(std::ostream & out, SplitResult const & SR) const
				{
					out << "files = " << std::setw(9) << std::setfill(' ') << fileinfo.size() << std::setw(0) << "\n";
					for ( uint64_t i = 0; i < fileinfo.size(); ++i )
						out << "  " << std::setw(9) << std::setfill(' ') << fileinfo[i].fnumreads << std::setw(0) << " " << fileinfo[i].fastaprolog << " " << fileinfo[i].fastafn << "\n";
					out << "blocks = " << std::setw(9) << std::setfill(' ') << SR.size() << std::setw(0) << "\n";
					out << "size = " << std::setw(9) << std::setfill(' ') << blocksize << std::setw(0)
						<< " cutoff = " << std::setw(9) << std::setfill(' ') << cutoff << std::setw(0)
						<< " all = " << std::setw(1) << std::setfill(' ') << all << std::setw(0)
						<< "\n";
					if ( ! SR.size() )
					{
						out << " "
							<< std::setw(9) << std::setfill(' ') << 0 /* unfiltered */ << std::setw(0) << " "
							<< std::setw(9) << std::setfill(' ') << 0 /* filtered */ << std::setw(0) << "\n";
					}
					else
					{
						out << " "
							<< std::setw(9) << std::setfill(' ') << SR[0].ulow /* unfiltered */ << std::setw(0) << " "
							<< std::setw(9) << std::setfill(' ') << SR[0].low  /* filtered */ << std::setw(0) << "\n";

						for ( uint64_t i = 0; i < SR.size(); ++i )
							out << " "
								<< std::setw(9) << std::setfill(' ') << SR[i].uhigh /* unfiltered */ << std::setw(0) << " "
								<< std::setw(9) << std::setfill(' ') << SR[i].high /* filtered */ << std::setw(0) << "\n";
					}

					return out;
				}

				std::vector < std::string > enumerateTracks() const
				{
					libmaus2::util::FileEnumerator D(path);
					std::vector < std::string > V;

					std::string const prefix = "." + root + ".";
					std::string const suffix = ".anno";

					std::string fn;
					while ( D.getNextFile(fn,prefix,suffix) )
						V.push_back(fn);

					return V;
				}

				void writeSplitDatabase(
					std::string const & outfn,
					bool const force,
					libmaus2::dazzler::db::DatabaseFile::SplitResult const & SR,
					std::ostream * errstr = 0
				)
				{
					if ( ! force && libmaus2::util::GetFileSize::fileExists(outfn) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "[E] file " << outfn << " does already exist" << std::endl;
						lme.finish();
						throw lme;
					}

					if ( libmaus2::util::GetFileSize::fileExists(outfn) )
					{
						if ( errstr )
							*errstr << "[W] removing " << outfn << std::endl;
						libmaus2::aio::FileRemoval::removeFile(outfn);
					}

					libmaus2::aio::OutputStreamInstance::unique_ptr_type pOSI(new libmaus2::aio::OutputStreamInstance(outfn));

					this->serialise(*pOSI,SR);

					pOSI->flush();
					pOSI.reset();

					bool const isdb = libmaus2::dazzler::db::DatabaseFile::endsOn(outfn,".db");
					bool const isdam = libmaus2::dazzler::db::DatabaseFile::endsOn(outfn,".dam");
					bool const issup = isdb || isdam;

					if ( ! issup )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Output file has unknown suffix (not .db or .dam)" << std::endl;
						lme.finish();
						throw lme;
					}

					std::string const path = libmaus2::dazzler::db::DatabaseFile::getPath(outfn);
					std::string const root = isdam ? libmaus2::dazzler::db::DatabaseFile::getRoot(outfn,".dam") : libmaus2::dazzler::db::DatabaseFile::getRoot(outfn,".db");
					std::string const outidxpath = path + "/." + root + ".idx";
					std::string const outbpspath = path + "/." + root + ".bps";

					if ( force && libmaus2::util::GetFileSize::fileExists(outidxpath) )
					{
						if ( errstr )
							*errstr << "[W] removing " << outidxpath << std::endl;
						libmaus2::aio::FileRemoval::removeFile(outidxpath);
					}

					int r;
					r = symlink(libmaus2::util::PathTools::getAbsPath(this->idxpath).c_str(),outidxpath.c_str());

					if ( r < 0 )
					{
						int const error = errno;
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "[E] linking " << this->idxpath << " to " << outidxpath << ": " << strerror(error) << std::endl;
						lme.finish();
						throw lme;
					}

					if ( force && libmaus2::util::GetFileSize::fileExists(outbpspath) )
					{
						if ( errstr )
							*errstr << "[W] removing " << outbpspath << std::endl;
						libmaus2::aio::FileRemoval::removeFile(outbpspath);
					}

					r = symlink(libmaus2::util::PathTools::getAbsPath(this->bpspath).c_str(),outbpspath.c_str());

					if ( r < 0 )
					{
						int const error = errno;
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "[E] linking " << this->bpspath << " to " << outbpspath << ": " << strerror(error) << std::endl;
						lme.finish();
						throw lme;
					}

					std::vector < std::string > VT = this->enumerateTracks();
					for ( uint64_t i = 0; i < VT.size(); ++i )
					{
						std::string const trackname = VT[i];

						std::string const annosrc = this->path + "/." + this->root + "." + trackname + ".anno";
						std::string const datasrc = this->path + "/." + this->root + "." + trackname + ".data";
						std::string const annotgt = path + "/." + root + "." + trackname + ".anno";
						std::string const datatgt = path + "/." + root + "." + trackname + ".data";

						if ( force && libmaus2::util::GetFileSize::fileExists(annotgt) )
						{
							if ( errstr )
								*errstr << "[W] removing " << annotgt << std::endl;
							libmaus2::aio::FileRemoval::removeFile(annotgt);
						}
						if ( force && libmaus2::util::GetFileSize::fileExists(datatgt) )
						{
							if ( errstr )
								*errstr << "[W] removing " << datatgt << std::endl;
							libmaus2::aio::FileRemoval::removeFile(datatgt);
						}

						int r;
						r = symlink(libmaus2::util::PathTools::getAbsPath(annosrc).c_str(),annotgt.c_str());

						if ( r < 0 )
						{
							int const error = errno;
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] linking " << annosrc << " to " << annotgt << ": " << strerror(error) << std::endl;
							lme.finish();
							throw lme;
						}

						if ( libmaus2::util::GetFileSize::fileExists(datasrc) )
						{
							r = symlink(libmaus2::util::PathTools::getAbsPath(datasrc).c_str(),datatgt.c_str());

							if ( r < 0 )
							{
								int const error = errno;
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "[E] linking " << datasrc << " to " << datatgt << ": " << strerror(error) << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}
				}
			};

			std::ostream & operator<<(std::ostream & out, DatabaseFile const & D);
		}
	}
}
#endif
