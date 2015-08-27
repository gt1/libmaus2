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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_SORTINGOVERLAPOUTPUTBUFFER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_SORTINGOVERLAPOUTPUTBUFFER_HPP

#include <libmaus2/dazzler/align/SortingOverlapOutputBufferMerger.hpp>
#include <libmaus2/dazzler/align/SortingOverlapBlockInput.hpp>
#include <libmaus2/dazzler/align/OverlapHeapComparator.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <queue>
			
		
namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct SortingOverlapOutputBuffer
			{
				std::string const filename;
				bool const small;
				libmaus2::aio::OutputStreamInstance::unique_ptr_type Pout;
				libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::Overlap> B;
				uint64_t f;
				std::streampos p;
				std::vector< std::pair<uint64_t,uint64_t> > blocks;
				
				SortingOverlapOutputBuffer(std::string const & rfilename, bool const rsmall, uint64_t const rn)
				: filename(rfilename), small(rsmall), Pout(new libmaus2::aio::OutputStreamInstance(filename)), B(rn), f(0), p(0)
				{
				
				}
				
				void flush()
				{
					std::streampos const pos = Pout->tellp();
					assert ( pos >= 0 );
					assert ( pos == p );
					
					if ( f )
					{
						std::sort(B.begin(),B.begin()+f);
						
						for ( uint64_t i = 1; i < f; ++i )
						{
							bool const ok = !(B[i] < B[i-1]);
							assert ( ok );
						}
					
						for ( uint64_t i = 0; i < f; ++i )
							p += B[i].serialiseWithPath(*Pout, small);
							
						blocks.push_back(std::pair<uint64_t,uint64_t>(pos,f));
						f = 0;
					}
				}
				
				void put(libmaus2::dazzler::align::Overlap const & OVL)
				{
					assert ( f < B.size() );
					B[f++] = OVL;
					if ( f == B.size() )
						flush();
				}
				
				SortingOverlapOutputBufferMerger::unique_ptr_type getMerger()
				{
					flush();
					Pout.reset();
					SortingOverlapOutputBufferMerger::unique_ptr_type tptr(new SortingOverlapOutputBufferMerger(filename,small,blocks));
					return UNIQUE_PTR_MOVE(tptr);
				}
				
				void mergeToFile(std::ostream & out, int64_t const tspace)
				{
					SortingOverlapOutputBufferMerger::unique_ptr_type merger(getMerger());
					libmaus2::dazzler::align::AlignmentFile::serialiseHeader(out,0/*novl*/,tspace);
					libmaus2::dazzler::align::Overlap NOVL;
					uint64_t novl = 0;
					
					libmaus2::dazzler::align::Overlap OVLprev;
					bool haveprev = false;
					
					while ( merger->getNext(NOVL) )
					{
						if ( haveprev )
						{
							bool const ok = !(NOVL < OVLprev);
							assert ( ok );
						}
					
						NOVL.serialiseWithPath(out,small);
						novl += 1;
						
						haveprev = true;
						OVLprev = NOVL;
					}
					
					out.clear();
					out.seekp(0,std::ios::beg);
					uint64_t offset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,novl,offset);                                        
				}

				void mergeToFile(std::string const & filename, int64_t const tspace)
				{
					libmaus2::aio::OutputStreamInstance OSI(filename);
					mergeToFile(OSI,tspace);
				}
				
				static void sortFile(std::string const & infilename, std::string const & outfilename, uint64_t const n = 64*1024, std::string tmpfilename = std::string())
				{
					if ( ! tmpfilename.size() )
						tmpfilename = outfilename + ".S";
					
					libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilename);
					libmaus2::aio::InputStreamInstance::unique_ptr_type PISI(new libmaus2::aio::InputStreamInstance(infilename));
					libmaus2::dazzler::align::AlignmentFile algnfile(*PISI);
					libmaus2::dazzler::align::Overlap OVL;

					SortingOverlapOutputBuffer SOOB(tmpfilename,algnfile.small,n);
										
					while ( algnfile.getNextOverlap(*PISI,OVL) )
						SOOB.put(OVL);

					SOOB.mergeToFile(outfilename,algnfile.tspace);
					
					libmaus2::aio::FileRemoval::removeFile(tmpfilename);
				}
				
				static void mergeFiles(std::vector<std::string> const & infilenames, std::string const & outfilename)
				{
					libmaus2::autoarray::AutoArray<libmaus2::aio::InputStreamInstance::unique_ptr_type> AISI(infilenames.size());
					libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::AlignmentFile::unique_ptr_type> AF(infilenames.size());
					std::priority_queue<
						std::pair<uint64_t,libmaus2::dazzler::align::Overlap>,
						std::vector< std::pair<uint64_t,libmaus2::dazzler::align::Overlap> >,
						OverlapHeapComparator
					> Q;
					libmaus2::dazzler::align::Overlap OVL;
					for ( uint64_t i = 0; i < AISI.size(); ++i )
					{
						libmaus2::aio::InputStreamInstance::unique_ptr_type tptr(new libmaus2::aio::InputStreamInstance(infilenames[i]));
						AISI[i] = UNIQUE_PTR_MOVE(tptr);
						libmaus2::dazzler::align::AlignmentFile::unique_ptr_type aptr(new libmaus2::dazzler::align::AlignmentFile(*(AISI[i])));
						AF[i] = UNIQUE_PTR_MOVE(aptr);
					
						if ( AF[i]->getNextOverlap(*(AISI[i]),OVL) )
							Q.push(std::pair<uint64_t,libmaus2::dazzler::align::Overlap>(i,OVL));
					}
					
					int64_t tspace = -1;
					uint64_t novl = 0;
					for ( uint64_t i = 0; i < AF.size(); ++i )
					{
						if ( tspace < 0 )
							tspace = AF[i]->tspace;
						else
							assert ( tspace == AF[i]->tspace );
							
						novl += AF[i]->novl;
					}
							
					libmaus2::aio::OutputStreamInstance OSI(outfilename);

					libmaus2::dazzler::align::AlignmentFile::serialiseHeader(OSI,novl,(tspace < 0) ? 0 : tspace);
					
					bool haveprev = false;
					libmaus2::dazzler::align::Overlap OVLprev;
					
					while ( Q.size() )
					{
						std::pair<uint64_t,libmaus2::dazzler::align::Overlap> const P = Q.top();
						Q.pop();
						
						if ( haveprev )
						{
							bool const ok = !(P.second < OVLprev);
							assert ( ok );
						}
						
						P.second.serialiseWithPath(OSI, AF[P.first]->small);

						if ( AF[P.first]->getNextOverlap(*(AISI[P.first]),OVL) )
							Q.push(std::pair<uint64_t,libmaus2::dazzler::align::Overlap>(P.first,OVL));
							
						haveprev = true;
						OVLprev = P.second;
					}
				}
			};
		}
	}
}
#endif
