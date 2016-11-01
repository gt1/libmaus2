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

					SplitResultElement() {}
					SplitResultElement(
						uint64_t const rlow,
						uint64_t const rhigh,
						uint64_t const rsize
					) : low(rlow), high(rhigh), size(rsize) {}
				};

				struct SplitResult : public std::vector<SplitResultElement>
				{

				};

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

									SR.push_back(SplitResultElement(l,h,s));

									s = rlen;
									l = h;
								}
							}
						}
					}

					if ( init )
					{
						uint64_t const h = Ptrim->rank1(n-1);
						SR.push_back(SplitResultElement(l,h,s));
					}

					return SR;
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

				std::string operator[](size_t const i) const
				{
					libmaus2::autoarray::AutoArray<char> A;
					size_t const rlen = decodeRead(i,A);
					return std::string(A.begin(),A.begin()+rlen);
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

					Track::unique_ptr_type track(new Track(trackname,PDanno,Adata));

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
			};

			std::ostream & operator<<(std::ostream & out, DatabaseFile const & D);
		}
	}
}
#endif
