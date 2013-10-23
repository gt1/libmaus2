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

			BamCatHeader(std::vector<std::string> const & filenames)
			: orderedCoordinates(true), orderedNames(true)
			{
				rgfilter.set("RG");
				pgfilter.set("PG");
				libmaus::autoarray::AutoArray < libmaus::bambam::BamDecoder::unique_ptr_type > decs(filenames.size());
				inputbamheaders = libmaus::autoarray::AutoArray<libmaus::bambam::BamHeader::unique_ptr_type>(filenames.size());
				std::vector < std::vector<libmaus::bambam::Chromosome> const * > V;
				std::vector < std::vector<libmaus::bambam::ReadGroup> const * > R;
				std::vector< std::string const * > H;
				for ( uint64_t i = 0; i < decs.size(); ++i )
				{
					std::string const fn = filenames[i];
					libmaus::bambam::BamDecoder::unique_ptr_type tdec(new libmaus::bambam::BamDecoder(fn));
					decs[i] = UNIQUE_PTR_MOVE(tdec);
					
					V.push_back( & (decs[i]->getHeader().chromosomes) );
					R.push_back( & (decs[i]->getHeader().RG) );
					H.push_back( & (decs[i]->getHeader().text) );
					
					libmaus::bambam::BamHeader::unique_ptr_type tinputbamheader(decs[i]->getHeader().uclone());
					inputbamheaders[i] = UNIQUE_PTR_MOVE(tinputbamheader);			

					std::string const SO = libmaus::bambam::BamHeader::getSortOrderStatic(decs[i]->getHeader().text);	
					
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
				if ( decs.size() == 1 )
					headertextstr << "@HD\tVN:1.5\tSO:" << libmaus::bambam::BamHeader::getSortOrderStatic(decs[0]->getHeader().text) << std::endl;
				else
					headertextstr << "@HD\tVN:1.5\tSO:unknown" << std::endl;
				
				for ( uint64_t i = 0; i < chromosomeMergeInfo->chromosomes.size(); ++i )
					headertextstr << chromosomeMergeInfo->chromosomes[i].createLine() << "\n";
					
				for ( uint64_t i = 0; i < readGroupMergeInfo->readgroups.size(); ++i )
					headertextstr << readGroupMergeInfo->readgroups[i].createLine() << "\n";
					
				headertextstr << programHeaderLinesMergeInfo->PGtext;

				std::vector<std::string> otherlines;
				for ( uint64_t i = 0; i < decs.size(); ++i )
				{
					std::vector<libmaus::bambam::HeaderLine> lines = libmaus::bambam::HeaderLine::extractLines(decs[i]->getHeader().text);
					
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
					std::string const & newID = bamheader->RG[readGroupMergeInfo->readgroupsmapping[fileid][rgid]].ID;
					
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
