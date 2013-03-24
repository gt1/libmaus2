/**
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
**/
#if ! defined(LIBMAUS_BAMBAM_BAMHEADER_HPP)
#define LIBMAUS_BAMBAM_BAMHEADER_HPP

#include <libmaus/lz/GzipStream.hpp>
#include <libmaus/bambam/Chromosome.hpp>
#include <libmaus/bambam/EncoderBase.hpp>
#include <libmaus/bambam/DecoderBase.hpp>
#include <libmaus/bambam/HeaderLine.hpp>
#include <libmaus/util/stringFunctions.hpp>
#include <libmaus/util/unordered_map.hpp>
#include <libmaus/trie/TrieState.hpp>
#include <libmaus/lz/BgzfInflateStream.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ReadGroup
		{
			std::string ID;
			::libmaus::util::unordered_map<std::string,std::string>::type M;
			int64_t LBid;
			
			ReadGroup()
			: LBid(-1)
			{
			
			}
			
			ReadGroup & operator=(ReadGroup const & o)
			{
				if ( this != &o )
				{
					this->ID = o.ID;
					this->M = o.M;
					this->LBid = o.LBid;
				}
				return *this;
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, ReadGroup const & RG)
		{
			out << "ReadGroup(ID=" << RG.ID;
			
			for ( ::libmaus::util::unordered_map<std::string,std::string>::type::const_iterator ita = RG.M.begin();
				ita != RG.M.end(); ++ita )
				out << "," << ita->first << "=" << ita->second;
			
			out << ")";

			return out;
		}
	
		struct BamHeader : 
			public ::libmaus::bambam::EncoderBase, 
			public ::libmaus::bambam::DecoderBase
		{	
			std::string text;
			std::vector< ::libmaus::bambam::Chromosome > chromosomes;
			std::vector< ::libmaus::bambam::ReadGroup > RG;
			::libmaus::trie::LinearHashTrie<char,uint32_t>::shared_ptr_type RGTrie;
			std::vector<std::string> libs;
			uint64_t numlibs;
			
			std::string getRefIDName(int64_t const refid) const
			{
				if ( refid < 0 || refid >= static_cast<int64_t>(chromosomes.size()) )
					return "*";
				else
					return chromosomes[refid].name;
			}
			
			int64_t getReadGroupId(std::string const & ID) const
			{
				return RGTrie->searchCompleteNoFailure(ID);
			}
			
			::libmaus::bambam::ReadGroup const * getReadGroup(std::string const & ID) const
			{
				int64_t const id = getReadGroupId(ID);
				if ( id < 0 )
					return 0;
				else
					return &(RG[id]);
			}
			
			std::string getLibraryName(int64_t const libid) const
			{
				if ( libid >= static_cast<int64_t>(numlibs) )
					return "Unknown Library";
				else
					return libs[libid];			
			}
			
			std::string getLibraryName(std::string const & ID) const
			{
				return getLibraryName(getLibraryId(ID));
			}
			
			int64_t getLibraryId(std::string const & ID) const
			{
				int64_t const rgid = getReadGroupId(ID);
				if ( rgid < 0 )
					return numlibs;
				else
					return RG[rgid].LBid;
			}

			static ::libmaus::trie::LinearHashTrie<char,uint32_t>::shared_ptr_type computeRgTrie(std::vector< ::libmaus::bambam::ReadGroup > const & RG)
			{
				::libmaus::trie::Trie<char> trienofailure;
				std::vector<std::string> dict;
				for ( uint64_t i = 0; i < RG.size(); ++i )
					dict.push_back(RG[i].ID);
				trienofailure.insertContainer(dict);
				::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure = 
					UNIQUE_PTR_MOVE(trienofailure.toLinearHashTrie<uint32_t>());
				::libmaus::trie::LinearHashTrie<char,uint32_t>::shared_ptr_type LHTsnofailure(
					LHTnofailure.release()
					);

				return LHTsnofailure;
			}

			static bool startsWith(std::string const & s, std::string const & prefix)
			{
				return 
					s.size() >= prefix.size()
					&&
					s.substr(0,prefix.size()) == prefix;
			}

			static std::vector<ReadGroup> getReadGroups(std::string const & header)
			{
				std::vector<ReadGroup> RG;
				std::istringstream istr(header);
				
				while ( istr )
				{
					std::string line;
					::std::getline(istr,line);
					if ( istr && line.size() )
					{
						if ( 
							(startsWith(line,"@RG"))
						)
						{
							std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");
							ReadGroup RGI;
							for ( uint64_t i = 1; i < tokens.size(); ++i )
								if ( startsWith(tokens[i],"ID:") )
								{
									RGI.ID = tokens[i].substr(3);
								}
								else if ( tokens[i].size() < 3 || tokens[i][2] != ':' )
								{
									continue;	
								}
								else
								{
									std::string const tag = tokens[i].substr(0,2);
									std::string const val = tokens[i].substr(3);
									RGI.M[tag] = val;
								}
							
							if ( RGI.ID.size() )
								RG.push_back(RGI);
							
							// std::cerr << RGI << std::endl;	
						}
					}
				}
				
				return RG;
			}

			
			static std::string filterHeader(std::string const & header)
			{
				std::istringstream istr(header);
				std::ostringstream ostr;
				
				while ( istr )
				{
					std::string line;
					::std::getline(istr,line);
					if ( istr && line.size() )
					{
						if ( 
							!(startsWith(line,"@HD"))
							&&
							!(startsWith(line,"@SQ"))
						)
							ostr << line << std::endl;
					}
				}
				
				return ostr.str();
			}
			
			static std::string getVersionStatic(std::string const & header, std::string const defaultVersion = "1.4")
			{
				std::istringstream istr(header);
				std::string version = defaultVersion;
				
				while ( istr )
				{
					std::string line;
					::std::getline(istr,line);
					if ( istr && line.size() )
					{
						if ( 
							(startsWith(line,"@HD"))
						)
						{
							std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");
							for ( uint64_t i = 0; i < tokens.size(); ++i )
								if ( startsWith(tokens[i],"VN:") )
									version = tokens[i].substr(3);
						}
					}
				}
				
				return version;
			}

			std::string getVersion(std::string const defaultVersion = "1.4")
			{
				return getVersionStatic(text,defaultVersion);
			}
			
			std::string getSortOrder(std::string const defaultSortorder = "unknown")
			{
				return getSortOrderStatic(text,defaultSortorder);
			}
			
			static std::string getSortOrderStatic(std::string const & header, std::string const defaultSortOrder = "unknown")
			{
				std::istringstream istr(header);
				std::string sortorder = defaultSortOrder;
				
				while ( istr )
				{
					std::string line;
					::std::getline(istr,line);
					if ( istr && line.size() )
					{
						if ( 
							(startsWith(line,"@HD"))
						)
						{
							std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize<std::string>(line,"\t");
							for ( uint64_t i = 0; i < tokens.size(); ++i )
								if ( startsWith(tokens[i],"SO:") )
									sortorder = tokens[i].substr(3);
						}
					}
				}
				
				return sortorder;
			}
			
			static std::string rewriteHeader(
				std::string const & header, std::vector< ::libmaus::bambam::Chromosome > const & chromosomes,
				std::string const & rsortorder = std::string()
			)
			{
				std::vector<HeaderLine> const hlv = ::libmaus::bambam::HeaderLine::extractLines(header);
				HeaderLine const * hdline = 0;
				std::map<std::string,HeaderLine const *> sqmap;
				for ( uint64_t i = 0; i < hlv.size(); ++i )
					if ( hlv[i].type == "HD" )
						hdline = &(hlv[i]);
					else if ( hlv[i].type == "SQ" )
						sqmap[ hlv[i].getValue("SN") ] = &(hlv[i]);
				
				std::ostringstream ostr;
			
				if ( hdline )
				{
					if ( rsortorder.size() )
					{
						std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize(hdline->line,std::string("\t"));
						
						for ( uint64_t i = 1; i < tokens.size(); ++i )
							if ( tokens[i].size() >= 3 && tokens[i].substr(0,3) == "SO:" )
								tokens[i] = "SO:" + rsortorder;
								
						std::ostringstream hdlinestr;
						for ( uint64_t i = 0; i < tokens.size(); ++i )
						{
							hdlinestr << tokens[i];
							if ( i+1 < tokens.size() )
								hdlinestr << "\t";
						}
						
						//hdline->line = hdlinestr.str();
						
						ostr << hdlinestr.str() << std::endl;
					}
					else
					{
						ostr << hdline->line << std::endl;
					}
				}
				else
					ostr << "@HD"	
						<< "\tVN:" << getVersionStatic(header)
						<< "\tSO:" << (rsortorder.size() ? rsortorder : getSortOrderStatic(header))
						<< "\n";
					
				for ( uint64_t i = 0; i < chromosomes.size(); ++i )
					if ( sqmap.find(chromosomes[i].name) != sqmap.end() )
						ostr << sqmap.find(chromosomes[i].name)->second->line << std::endl;
					else
						ostr << "@SQ\tSN:" << chromosomes[i].name << "\tLN:" << chromosomes[i].len << "\n";
					
				ostr << filterHeader(header);
				
				return ostr.str();
			}
			
			char const * idToChromosome(int32_t const id) const
			{
				if ( id >= 0 && id < static_cast<int32_t>(chromosomes.size()) )
					return chromosomes[id].name.c_str();
				else
					return "*";
			}
			
			template<typename stream_type>
			static void encodeChromosomeVector(stream_type & ostr, std::vector< ::libmaus::bambam::Chromosome > const & V)
			{
				::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,V.size());
				
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					::libmaus::bambam::Chromosome const & chr = V[i];
					
					::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,chr.name.size()+1);
					ostr.write(chr.name.c_str(),chr.name.size());
					ostr.put(0);

					::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,chr.len);
				}
			}

			static std::string encodeChromosomeVector(std::vector< ::libmaus::bambam::Chromosome > const & V)
			{
				std::ostringstream ostr;
				encodeChromosomeVector(ostr,V);
				return ostr.str();
			}
			
			template<typename stream_type>
			void serialise(stream_type & ostr) const
			{
				// magic
				ostr.put('B');
				ostr.put('A');
				ostr.put('M');
				ostr.put('\1');

				// length of plain text
				::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,text.size()/*+1 */);
				// plain text
				ostr.write(text.c_str(),text.size());
				// ostr.put(0);
				
				encodeChromosomeVector(ostr,chromosomes);
			}

			template<typename stream_type>
			void init(stream_type & in)
			{
				uint8_t fmagic[4];
				
				for ( unsigned int i = 0; i < sizeof(fmagic)/sizeof(fmagic[0]); ++i )
					fmagic[i] = getByte(in);

				if ( 
					fmagic[0] != 'B' ||
					fmagic[1] != 'A' ||
					fmagic[2] != 'M' ||
					fmagic[3] != '\1' )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Wrong magic in BamHeader::init()" << std::endl;
					se.finish();
					throw se;					
				}
				
				uint64_t const l_text = getLEInteger(in,4);
				text.resize(l_text);
				
				for ( uint64_t i = 0; i < text.size(); ++i )
					text[i] = getByte(in);
					
				uint64_t textlen = 0;
				while ( textlen < text.size() && text[textlen] )
					textlen++;
				text.resize(textlen);
				
				uint64_t const n_ref = getLEInteger(in,4);
						
				for ( uint64_t i = 0; i < n_ref; ++i )
				{
					uint64_t l_name = getLEInteger(in,4);
					assert ( l_name );
					std::string name;
					name.resize(l_name-1);
					for ( uint64_t j = 0 ; j < name.size(); ++j )
						name[j] = getByte(in);
					int c = getByte(in); assert ( c == 0 );
					uint64_t l_ref = getLEInteger(in,4);	
					chromosomes.push_back( ::libmaus::bambam::Chromosome(name,l_ref) );
				}

				text = rewriteHeader(text,chromosomes);

				RG = getReadGroups(text);
				RGTrie = computeRgTrie(RG);
				
				// extract all library names
				std::set < std::string > slibs;
				for ( uint64_t i = 0; i < RG.size(); ++i )
					if ( RG[i].M.find("LB") != RG[i].M.end() )
						slibs.insert(RG[i].M.find("LB")->second);
				
				// assign library ids to read groups (if present)
				libs = std::vector<std::string>(slibs.begin(),slibs.end());
				numlibs = libs.size();
				for ( uint64_t i = 0; i < RG.size(); ++i )
					if ( RG[i].M.find("LB") != RG[i].M.end() )
						RG[i].LBid = std::lower_bound(libs.begin(),libs.end(),RG[i].M.find("LB")->second) - libs.begin();
					else
						RG[i].LBid = numlibs;
			}
			
			void changeSortOrder(std::string const newsortorder = std::string())
			{
				text = rewriteHeader(text,chromosomes,newsortorder);
			}
			
			void produceHeader()
			{
				changeSortOrder();
			}
			
			BamHeader()
			{
			
			}
			BamHeader(std::istream & in)
			{
				::libmaus::lz::GzipStream GS(in);
				init(GS);
			}

			BamHeader(::libmaus::lz::GzipStream & in)
			{
				init(in);
			}
			BamHeader(libmaus::lz::BgzfInflateStream & in)
			{
				init(in);
			}
			
			BamHeader(std::string const & text)
			{
				std::ostringstream ostr;
				
				ostr.put('B');
				ostr.put('A');
				ostr.put('M');
				ostr.put('\1');
				
				EncoderBase::putLE<std::ostringstream,uint32_t>(ostr,text.size());
				ostr << text;
				

				std::vector<HeaderLine> hlv = HeaderLine::extractLines(text);
				uint32_t nref = 0;
				for ( uint64_t i = 0; i < hlv.size(); ++i )
					if ( hlv[i].type == "SQ" )
						nref++;

				EncoderBase::putLE<std::ostringstream,uint32_t>(ostr,nref);
				
				for ( uint64_t i = 0; i < hlv.size(); ++i )
					if ( hlv[i].type == "SQ" )
					{
						std::string const name = hlv[i].getValue("SN");

						EncoderBase::putLE<std::ostringstream,uint32_t>(ostr,name.size()+1);
						ostr << name;
						ostr.put(0);

						std::string const ssn = hlv[i].getValue("LN");
						std::istringstream ssnistr(ssn);
						uint64_t sn;
						ssnistr >> sn;
						EncoderBase::putLE<std::ostringstream,uint32_t>(ostr,sn);
					}

				std::istringstream istr(ostr.str());
				init(istr);
			}
			
			uint64_t addChromosome(std::string const & name, uint64_t const len)
			{
				uint64_t const id = chromosomes.size();
				chromosomes.push_back(Chromosome(name,len));
				return id;
			}
		};
	}
}
#endif
