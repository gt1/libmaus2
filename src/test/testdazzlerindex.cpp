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
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/dazzler/align/OverlapIndexer.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::util::ArgInfo const arginfo(argc,argv);

		std::string const lasfn = arginfo.getUnparsedRestArg(0);
		// int64_t const tspace = libmaus2::dazzler::align::AlignmentFile::getTSpace(lasfn);

		int64_t first_a_read = -1;
		{
			libmaus2::aio::InputStreamInstance ISI(lasfn);
			libmaus2::dazzler::align::AlignmentFile AF(ISI);
			libmaus2::dazzler::align::Overlap OVL;
			if ( AF.getNextOverlap(ISI,OVL) )
				first_a_read = OVL.aread;
		}

		if ( first_a_read != -1 )
		{
			std::string const indexname = libmaus2::dazzler::align::OverlapIndexer::getDalignerIndexName(lasfn);
			libmaus2::aio::InputStreamInstance indexISI(indexname);

			uint64_t off = 0;
			#if 0
			uint64_t const omax =
			#endif
				libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(indexISI,off);
			#if 0
			uint64_t const ttot =
			#endif
				libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(indexISI,off);
			#if 0
			uint64_t const smax =
			#endif
				libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(indexISI,off);
			#if 0
			uint64_t const tmax =
			#endif
				libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(indexISI,off);
			std::map<uint64_t,uint64_t> imap;
			uint64_t z = first_a_read;
			while ( indexISI.peek() != std::istream::traits_type::eof() )
				imap [ z++ ] = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(indexISI,off);

			libmaus2::aio::InputStreamInstance ISI(lasfn);
			libmaus2::dazzler::align::AlignmentFile AF(ISI);
			libmaus2::dazzler::align::Overlap OVL;

			bool ok = true;

			if ( imap.find(first_a_read) == imap.end() )
			{
				std::cerr << "[V] index is missing first read" << std::endl;
				ok = false;
			}

			uint64_t fpos = ISI.tellg();
			std::map<uint64_t,uint64_t> fmap;
			while ( AF.getNextOverlap(ISI,OVL) )
			{
				if ( fmap.find(OVL.aread) == fmap.end() )
				{
					fmap[OVL.aread] = fpos;

					if ( imap.find(OVL.aread) != imap.end() )
					{
						if (
							fpos != imap.find(OVL.aread)->second
						)
						{
							ok = false;
							std::cerr << "got " << fpos << " expected " << imap.find(OVL.aread)->second << std::endl;
						}
					}
					else
					{
						std::cerr << "[V] " << OVL.aread << " is missing from index" << std::endl;
						ok = false;
					}


					if ( OVL.aread % (256) == 0 )
						std::cerr << "[V] " << OVL.aread << std::endl;
				}
				fpos = ISI.tellg();
			}

			uint64_t const lastread = fmap.rbegin()->first;

			if ( imap.find(lastread+1) == imap.end() )
			{
				std::cerr << "[V] final index value missing" << std::endl;
				ok = false;
			}
			else if ( imap.find(lastread+1)->second != fpos )
			{
				std::cerr << "[V] final index value wrong" << std::endl;
				ok = false;
			}

			uint64_t const numreads = (lastread+1-first_a_read);
			uint64_t const expctdptrs = numreads+1;

			if ( imap.size() != expctdptrs )
			{
				std::cerr << "[V] number of pointers in index is wrong" << std::endl;
				ok = false;
			}

			for ( uint64_t i = first_a_read; i <= lastread; ++i )
				// check for empty piles
				if ( fmap.find(i) == fmap.end() )
				{
					if ( imap.find(i) != imap.end() && imap.find(i+1) != imap.end() )
					{
						int64_t const dif = imap.find(i+1)->second - imap.find(i)->second;

						if ( dif != 0 )
						{
							std::cerr << "[V] wrong value pair for pile " << i << std::endl;
							ok = false;
						}
					}
					else
					{
						std::cerr << "[V] missing imap value for empty pile " << i << std::endl;
						ok = false;
					}
				}

			std::cout << (ok ? "ok" : "failed") << std::endl;
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
