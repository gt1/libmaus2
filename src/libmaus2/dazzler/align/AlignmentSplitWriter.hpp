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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTSPLITWRITER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTSPLITWRITER_HPP

#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentSplitWriter
			{
				int64_t const tspace;
				bool const small;
				std::string const prefix;
				int64_t const idsperfile;

				uint64_t fid;
				int64_t previd;
				int64_t numids;
				int64_t novl;

				libmaus2::aio::OutputStreamInstance::unique_ptr_type Pstream;

				std::vector<std::string> outputfilenames;

				bool const tmp;

				AlignmentSplitWriter(
					int64_t const rtspace,
					std::string const rprefix,
					int64_t const ridsperfile,
					bool const rtmp
				)
				: tspace(rtspace), small(libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)), prefix(rprefix), idsperfile(ridsperfile),
				  fid(1), previd(-1), numids(idsperfile), novl(0), tmp(rtmp) {}

				~AlignmentSplitWriter()
				{
					flush();
				}

				void flush()
				{
					if ( Pstream )
					{
						Pstream->seekp(0,std::ios::beg);
						uint64_t offset = 0;
						libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*Pstream,novl,offset);

						Pstream->flush();
						Pstream.reset();
					}
				}

				void openNextFile()
				{
					flush();

					std::ostringstream fnostr;
					fnostr << prefix << "." << (fid++) << ".las";
					std::string const fn = fnostr.str();

					outputfilenames.push_back(fn);

					if ( tmp )
						libmaus2::util::TempFileRemovalContainer::addTempFile(fn);

					// std::cerr << "[V] opening new output file " << fn << std::endl;

					libmaus2::aio::OutputStreamInstance::unique_ptr_type Tstream(new libmaus2::aio::OutputStreamInstance(fn));
					Pstream = UNIQUE_PTR_MOVE(Tstream);

					uint64_t offset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(*Pstream,0 /* novl */,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger4(*Pstream,tspace,offset);

					novl = 0;
				}

				void push(libmaus2::dazzler::align::Overlap const & OVL)
				{
					if ( OVL.aread != previd )
					{
						numids += 1;

						if ( numids > idsperfile )
						{
							openNextFile();
							numids = 1;
						}

					}

					OVL.serialiseWithPath(*Pstream,small);
					novl += 1;
					previd = OVL.aread;
				}

				static std::vector<std::string> splitFile(std::string const & aligns, std::string const & prefix, uint64_t const n, bool const tmp)
				{
					libmaus2::aio::InputStreamInstance algnfile(aligns);
					libmaus2::dazzler::align::AlignmentFile algn(algnfile);
					libmaus2::dazzler::align::Overlap OVL;

					AlignmentSplitWriter AW(algn.tspace,prefix,n,tmp);

					// get next overlap
					while ( algn.getNextOverlap(algnfile,OVL) )
						AW.push(OVL);

					return AW.outputfilenames;
				}
			};
		}
	}
}
#endif
