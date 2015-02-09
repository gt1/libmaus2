/*
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
*/
#if ! defined(LIBMAUS_BAMBAM_BAMCATHEADER_HPP)
#define LIBMAUS_BAMBAM_BAMCATHEADER_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/ProgramHeaderLineSet.hpp>
#include <libmaus/bambam/BamAlignmentDecoderFactory.hpp>
#include <libmaus/bambam/ChromosomeVectorMerge.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamCatHeader
		{
			typedef BamCatHeader this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus::bambam::BamAuxFilterVector rgfilter;
			libmaus::bambam::BamAuxFilterVector pgfilter;
			libmaus::bambam::ChromosomeVectorMerge::unique_ptr_type chromosomeMergeInfo;
			libmaus::bambam::ReadGroupVectorMerge::unique_ptr_type readGroupMergeInfo;
			libmaus::bambam::ProgramHeaderLinesMerge::unique_ptr_type programHeaderLinesMergeInfo;
			libmaus::autoarray::AutoArray<libmaus::bambam::BamHeader::unique_ptr_type> inputbamheaders;
			libmaus::bambam::BamHeader::unique_ptr_type bamheader;
			bool orderedCoordinates;
			bool orderedNames;
			
			struct IsCoordinateSorted
			{
				static bool issorted(BamCatHeader const & header)
				{
					return header.orderedCoordinates;
				}
				
				static bool istopological(BamCatHeader const & header)
				{
					return header.chromosomeMergeInfo->topological;
				}
				
				static char const * getSortOrder()
				{
					return "coordinate";
				}
			};

			struct IsQueryNameSorted
			{
				static bool issorted(BamCatHeader const & header)
				{
					return header.orderedNames;
				}

				static bool istopological(BamCatHeader const &)
				{
					return true;
				}

				static char const * getSortOrder()
				{
					return "queryname";
				}
			};
			
			void init()
			{
				rgfilter.set("RG");
				pgfilter.set("PG");

				std::vector < std::vector<libmaus::bambam::Chromosome> const * > V;
				std::vector < std::vector<libmaus::bambam::ReadGroup> const * > R;
				std::vector< std::string const * > H;
				for ( uint64_t i = 0; i < inputbamheaders.size(); ++i )
				{
					libmaus::bambam::BamHeader const & header = *inputbamheaders[i];
					
					V.push_back( & (header.getChromosomes()) );
					R.push_back( & (header.getReadGroups()) );
					H.push_back( & (header.text) );
					
					std::string const SO = libmaus::bambam::BamHeader::getSortOrderStatic(header.text);
					orderedCoordinates = orderedCoordinates && (SO == "coordinate");
					orderedNames = orderedNames && (SO == "queryname");
				}

				libmaus::bambam::ChromosomeVectorMerge::unique_ptr_type tchromosomeMergeInfo(new libmaus::bambam::ChromosomeVectorMerge(V));
				chromosomeMergeInfo = UNIQUE_PTR_MOVE(tchromosomeMergeInfo);

				libmaus::bambam::ReadGroupVectorMerge::unique_ptr_type treadGroupMergeInfo(new libmaus::bambam::ReadGroupVectorMerge(R));		
				readGroupMergeInfo = UNIQUE_PTR_MOVE(treadGroupMergeInfo);
				
				libmaus::bambam::ProgramHeaderLinesMerge::unique_ptr_type tprogramHeaderLinesMergeInfo(new libmaus::bambam::ProgramHeaderLinesMerge(H));
				programHeaderLinesMergeInfo = UNIQUE_PTR_MOVE(tprogramHeaderLinesMergeInfo);
				
				std::ostringstream headertextstr;
				if ( inputbamheaders.size() == 1 )
					headertextstr << "@HD\tVN:1.5\tSO:" << libmaus::bambam::BamHeader::getSortOrderStatic(inputbamheaders[0]->text) << std::endl;
				else
					headertextstr << "@HD\tVN:1.5\tSO:unknown" << std::endl;
				
				for ( uint64_t i = 0; i < chromosomeMergeInfo->chromosomes.size(); ++i )
					headertextstr << chromosomeMergeInfo->chromosomes[i].createLine() << "\n";
					
				for ( uint64_t i = 0; i < readGroupMergeInfo->readgroups.size(); ++i )
					headertextstr << readGroupMergeInfo->readgroups[i].createLine() << "\n";
					
				headertextstr << programHeaderLinesMergeInfo->PGtext;

				std::vector<std::string> otherlines;
				for ( uint64_t i = 0; i < inputbamheaders.size(); ++i )
				{
					std::vector<libmaus::bambam::HeaderLine> lines = libmaus::bambam::HeaderLine::extractLines(inputbamheaders[i]->text);
					
					for ( uint64_t j = 0; j < lines.size(); ++j )
					{
						libmaus::bambam::HeaderLine const & line = lines[j];
						
						if (
							line.type != "HD" &&
							line.type != "SQ" &&
							line.type != "RG" &&
							line.type != "PG"
						)
						{
							otherlines.push_back(line.line);
						}
					}
				}
				std::set<std::string> otherlinesseen;

				for ( uint64_t i = 0; i < otherlines.size(); ++i )
					if ( otherlinesseen.find(otherlines[i]) == otherlinesseen.end() )
					{
						headertextstr << otherlines[i] << std::endl;
						otherlinesseen.insert(otherlines[i]);
					}
					
				// std::cerr << std::string(80,'-') << std::endl;
				std::string const headertext = headertextstr.str();
				
				::libmaus::bambam::BamHeader::unique_ptr_type tbamheader(new ::libmaus::bambam::BamHeader(headertext));
				bamheader = UNIQUE_PTR_MOVE(tbamheader);
				
				// std::cerr << "topologically sorted: " << chromosomeMergeInfo->topological << std::endl;
				// std::cerr << bamheader->text;			
			}
			
			template<typename iterator>
			void init(iterator decs, uint64_t const decssize)
			{
				// allocate clone array
				inputbamheaders = libmaus::autoarray::AutoArray<libmaus::bambam::BamHeader::unique_ptr_type>(decssize);

				// clone headers
				for ( uint64_t i = 0; i < decssize; ++i )
				{
					libmaus::bambam::BamHeader::unique_ptr_type tinputbamheader(decs[i]->getHeader().uclone());
					inputbamheaders[i] = UNIQUE_PTR_MOVE(tinputbamheader);			
				}

				init();
			}

			BamCatHeader(libmaus::util::ArgInfo const & arginfo, std::vector<std::string> const & filenames)
			: orderedCoordinates(true), orderedNames(true)
			{
				// allocate header clone array
				inputbamheaders = libmaus::autoarray::AutoArray<libmaus::bambam::BamHeader::unique_ptr_type>(filenames.size());
				libmaus::util::ArgInfo arginfoCopy = arginfo;

				// open files one at a time and extract headers
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					arginfoCopy.replaceKey("I",filenames[i]);
					libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type Pdec(
						libmaus::bambam::BamAlignmentDecoderFactory::construct(arginfoCopy)
					);
					libmaus::bambam::BamAlignmentDecoder & dec = Pdec->getDecoder();
					libmaus::bambam::BamHeader::unique_ptr_type tinputbamheader(dec.getHeader().uclone());
					inputbamheaders[i] = UNIQUE_PTR_MOVE(tinputbamheader);					
				}
				
				// merge headers
				init();
			}

			BamCatHeader(std::vector<std::string> const & filenames)
			: orderedCoordinates(true), orderedNames(true)
			{
				// allocate header clone array
				inputbamheaders = libmaus::autoarray::AutoArray<libmaus::bambam::BamHeader::unique_ptr_type>(filenames.size());

				// open files one at a time and extract headers
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					libmaus::bambam::BamDecoder dec(filenames[i]);
					libmaus::bambam::BamHeader::unique_ptr_type tinputbamheader(dec.getHeader().uclone());
					inputbamheaders[i] = UNIQUE_PTR_MOVE(tinputbamheader);					
				}
				
				// merge headers
				init();
			}

			BamCatHeader(libmaus::autoarray::AutoArray < libmaus::bambam::BamAlignmentDecoder::unique_ptr_type > & decs)
			: orderedCoordinates(true), orderedNames(true)
			{
				init(decs.begin(),decs.size());
			}

			BamCatHeader(libmaus::autoarray::AutoArray < libmaus::bambam::BamAlignmentDecoder * > & decs)
			: orderedCoordinates(true), orderedNames(true)
			{
				init(decs.begin(),decs.size());
			}
			
			BamCatHeader(std::vector<libmaus::bambam::BamAlignmentDecoderInfo> const & V)
			: orderedCoordinates(true), orderedNames(true)
			{
				libmaus::autoarray::AutoArray< libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type > wraps(V.size());
				libmaus::autoarray::AutoArray< libmaus::bambam::BamAlignmentDecoder * > decs(V.size());
				
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr ( BamAlignmentDecoderFactory::construct(V[i]) );
					wraps[i] = UNIQUE_PTR_MOVE(tptr);
					decs[i] = &(wraps[i]->getDecoder());
				}
				
				init(decs.begin(),decs.size());
			}
						
			void updateAlignment(uint64_t const fileid, libmaus::bambam::BamAlignment & algn) const
			{
				// update ref ids (including unmapped reads to keep order)
				if ( algn.getRefID() >= 0 )
					algn.putRefId(chromosomeMergeInfo->chromosomesmap[fileid][algn.getRefID()]);
				if ( algn.getNextRefID() >= 0 )
					algn.putNextRefId(chromosomeMergeInfo->chromosomesmap[fileid][algn.getNextRefID()]);
				
				// replace read group if any
				char const * oldRG = algn.getReadGroup();
				int64_t const rgid = inputbamheaders[fileid]->getReadGroupId(oldRG);
				if ( rgid >= 0 )
				{
					std::string const & newID = bamheader->getReadGroupIdentifierAsString(readGroupMergeInfo->readgroupsmapping[fileid][rgid]);
					
					// replace if there is a change
					if ( strcmp(oldRG,newID.c_str()) )
					{
						algn.filterOutAux(rgfilter);
						algn.putAuxString("RG",newID);
					}
				}
				
				// map PG aux field if present
				char const * pg = algn.getAuxString("PG");
				if ( pg )
				{
					std::string const & newPG = programHeaderLinesMergeInfo->mapPG(fileid, pg);
					
					// replace if there is a change
					if ( strcmp(pg,newPG.c_str()) )
					{
						algn.filterOutAux(pgfilter);
						algn.putAuxString("PG",newPG);
					}
				}
			}
		};
	}
}
#endif
