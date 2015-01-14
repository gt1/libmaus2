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
#include <libmaus/lz/BgzfInflateParallelStream.hpp>
#include <libmaus/lz/BgzfInflateDeflateParallelInputStream.hpp>
#include <libmaus/hashing/ConstantStringHash.hpp>
#include <libmaus/bambam/ReadGroup.hpp>

namespace libmaus
{
	namespace bambam
	{
		enum bam_header_parse_state
		{
			bam_header_read_magic,
			bam_header_read_l_text,
			bam_header_read_text,
			bam_header_read_n_ref,
			bam_header_reaf_ref_l_name,
			bam_header_read_ref_name,
			bam_header_read_ref_l_ref,
			bam_header_read_done,
			bam_header_read_failed
		};
			
		std::ostream & operator<<(std::ostream & out, bam_header_parse_state const & state);

		/**
		 * BAM file header class
		 **/
		struct BamHeader : 
			public ::libmaus::bambam::EncoderBase, 
			public ::libmaus::bambam::DecoderBase
		{
			public:
			typedef BamHeader this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			//! header text	
			std::string text;
			
			private:
			//! chromosome (reference sequence meta data) vector
			std::vector< ::libmaus::bambam::Chromosome > chromosomes;
			//! read groups vector
			std::vector< ::libmaus::bambam::ReadGroup > RG;
			//! trie for read group names
			::libmaus::trie::LinearHashTrie<char,uint32_t>::shared_ptr_type RGTrie;
			//! hash for read group names
			libmaus::hashing::ConstantStringHash::shared_ptr_type RGCSH;
			//! library names
			std::vector<std::string> libs;
			//! number of libaries
			uint64_t numlibs;
			
			public:
			/**
			 * clone this object and return clone in a unique pointer
			 *
			 * @return clone of this object
			 **/
			unique_ptr_type uclone() const
			{
				unique_ptr_type O(new this_type);
				O->text = this->text;
				O->chromosomes = this->chromosomes;
				O->RG = this->RG;
				if ( this->RGTrie.get() )
					O->RGTrie = this->RGTrie->sclone();
				if ( this->RGCSH.get() )
					O->RGCSH = this->RGCSH->sclone();
				O->libs = this->libs;
				O->numlibs = this->numlibs;
				return UNIQUE_PTR_MOVE(O);
			}

			/**
			 * clone this object and return clone in a shared pointer
			 *
			 * @return clone of this object
			 **/
			shared_ptr_type sclone() const
			{
				shared_ptr_type O(new this_type);
				O->text = this->text;
				O->chromosomes = this->chromosomes;
				O->RG = this->RG;
				if ( this->RGTrie.get() )
					O->RGTrie = this->RGTrie->sclone();
				if ( this->RGCSH.get() )
					O->RGCSH = this->RGCSH->sclone();
				O->libs = this->libs;
				O->numlibs = this->numlibs;
				return O;
			}
			
			/**
			 * get name for reference id
			 *
			 * @param refid reference id
			 * @return name for reference id or "*" if invalid ref id
			 **/
			std::string getRefIDName(int64_t const refid) const
			{
				if ( refid < 0 || refid >= static_cast<int64_t>(chromosomes.size()) )
					return "*";
				else
				{
					std::pair<char const *,char const *> const P = chromosomes[refid].getName();
					return std::string(P.first,P.second);
				}
			}
			
			/**
			 * get reference id length
			 **/
			int64_t getRefIDLength(int64_t const refid) const
			{
				if ( refid < 0 || refid >= static_cast<int64_t>(chromosomes.size()) )
					return -1;
				else
					return chromosomes[refid].getLength();
			}
			
			/**
			 * get number of reference sequences
			 **/
			uint64_t getNumRef() const
			{
				return chromosomes.size();
			}
			
			/**
			 * get vector of read groups
			 *
			 * @return read group vector
			 **/
			std::vector<ReadGroup> const & getReadGroups() const
			{
				return RG;
			}

			/**
			 * get vector of chromosomes
			 *
			 * @return chromosome vector
			 **/
			std::vector<Chromosome> const & getChromosomes() const
			{
				return chromosomes;
			}

			/**
			 * get string identifier fur numeric read group i
			 *
			 * @param i numeric identifier for read group
			 * @return string identifier for read group i in the header
			 **/
			std::string const & getReadGroupIdentifierAsString(int64_t const i) const
			{
				if ( i < 0 || i >= static_cast<int64_t>(getNumReadGroups()) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BamHeader::getReadGroupIdentifierAsString(): invalid numeric id " << i << std::endl;
					se.finish();
					throw se;					
				}
				return RG[i].ID;
			}

			/**
			 * get number of read groups
			 *
			 * @return number of read groups
			 **/
			uint64_t getNumReadGroups() const
			{
				return RG.size();
			}
			
			/**
			 * get read group numerical id for read group name
			 *
			 * @param ID read group name
			 * @return read group numerical id
			 **/
			int64_t getReadGroupId(char const * ID) const
			{
				if ( ID )
				{
					unsigned int const idlen = strlen(ID);
					
					if ( RGCSH )
					{
						return (*RGCSH)[ ReadGroup::hash(ID,ID+idlen) ];
					}
					else
					{
						return RGTrie->searchCompleteNoFailure(ID,ID+idlen);
					}
				}
				else
					return -1;
			}
			
			/**
			 * get read group object for read group name
			 *
			 * @param ID read group name
			 * @return read group object for ID
			 **/
			::libmaus::bambam::ReadGroup const * getReadGroup(char const * ID) const
			{
				int64_t const id = ID ? getReadGroupId(ID) : -1;
				
				if ( id < 0 )
					return 0;
				else
					return &(RG[id]);
			}
			
			/**
			 * get library name for library id
			 *
			 * @param libid library id
			 * @return name of library for libid of "Unknown Library" if libid is invalid
			 **/
			std::string getLibraryName(int64_t const libid) const
			{
				if ( libid >= static_cast<int64_t>(numlibs) )
					return "Unknown Library";
				else
					return libs[libid];			
			}
			
			/**
			 * get library name for read group id
			 *
			 * @param ID read group id
			 * @return library name for ID
			 **/
			std::string getLibraryName(char const * ID) const
			{
				return getLibraryName(getLibraryId(ID));
			}
			
			/**
			 * get library id for read group id
			 *
			 * @param ID read group id string
			 * @return library id for ID
			 **/
			int64_t getLibraryId(char const * ID) const
			{
				int64_t const rgid = getReadGroupId(ID);
				if ( rgid < 0 )
					return numlibs;
				else
					return RG[rgid].LBid;
			}

			/**
			 * get library id for numerical read group id
			 *
			 * @param rgid numerical read group id
			 * @return library id
			 **/ 
			int64_t getLibraryId(int64_t const rgid) const
			{
				if ( rgid < 0 )
					return numlibs;
				else
					return RG[rgid].LBid;
			}

			/**
			 * compute trie for read group names
			 *
			 * @param RG read group vector
			 * @return trie for read group names
			 **/
			static ::libmaus::trie::LinearHashTrie<char,uint32_t>::shared_ptr_type computeRgTrie(std::vector< ::libmaus::bambam::ReadGroup > const & RG)
			{
				::libmaus::trie::Trie<char> trienofailure;
				std::vector<std::string> dict;
				for ( uint64_t i = 0; i < RG.size(); ++i )
					dict.push_back(RG[i].ID);
				trienofailure.insertContainer(dict);
				::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure 
					(trienofailure.toLinearHashTrie<uint32_t>());
				::libmaus::trie::LinearHashTrie<char,uint32_t>::shared_ptr_type LHTsnofailure(
					LHTnofailure.release()
					);

				return LHTsnofailure;
			}

			/**
			 * @param s string
			 * @param prefix other string
			 * @return true if s starts with prefix
			 **/
			static bool startsWith(std::string const & s, std::string const & prefix)
			{
				return 
					s.size() >= prefix.size()
					&&
					s.substr(0,prefix.size()) == prefix;
			}

			/**
			 * extract read group vector from header text
			 *
			 * @param header text header
			 * @return read groups vector
			 **/
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

			/**
			 * filter header by removing HD and SQ lines (keep rest)
			 *
			 * @param header text header
			 * @return filtered header
			 **/
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
			
			/**
			 * get version from header; if no HD line is present or it contains no version number, then
			 * return defaultVersion
			 *
			 * @param header text header
			 * @param defaultVersion is "1.4"
			 * @return BAM file version
			 **/
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

			/**
			 * get version BAM file version from header text
			 *
			 * @param defaultVersion is returned if no version is present in the text
			 * @return BAM file version
			 **/
			std::string getVersion(std::string const defaultVersion = "1.4")
			{
				return getVersionStatic(text,defaultVersion);
			}

			/**
			 * get sort order from header; if sort order is not present in text then return defaultSortOrder
			 *
			 * @param header text header
			 * @param defaultSortOrder order to be assume if no order is given in header
			 * @return BAM sort order as recorded in header or given default
			 **/
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
			
			/**
			 * @param defaultSortorder default order to be returned if no order is recorded in the text
			 * @return BAM sort order
			 **/
			std::string getSortOrder(std::string const defaultSortorder = "unknown")
			{
				return getSortOrderStatic(text,defaultSortorder);
			}

			/**
			 * rewrite BAM header text
			 *
			 * @param header input header
			 * @param chromosomes reference sequence information to be inserted in rewritten header
			 * @param rsortorder sort order override if not the empty string
			 * @return rewritten header text
			 **/
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
				{
					std::pair<char const *,char const *> chrP = chromosomes[i].getName();
					std::string const chrname(chrP.first,chrP.second);
				
					if ( sqmap.find(chrname) != sqmap.end() )
						ostr << sqmap.find(chrname)->second->line << std::endl;
					else
						ostr << "@SQ\tSN:" << chrname << "\tLN:" << chromosomes[i].getLength() << "\n";
				}
					
				ostr << filterHeader(header);
				
				return ostr.str();
			}
			
			/**
			 * encode binary reference sequence information
			 *
			 * @param ostr binary BAM header construction stream
			 * @param V chromosomes (ref seqs)
			 **/
			template<typename stream_type>
			static void encodeChromosomeVector(stream_type & ostr, std::vector< ::libmaus::bambam::Chromosome > const & V)
			{
				::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,V.size());
				
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					::libmaus::bambam::Chromosome const & chr = V[i];
					std::pair<char const *, char const *> P = chr.getName();
					
					::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,(P.second-P.first)+1);
					ostr.write(P.first,P.second-P.first);
					ostr.put(0);

					::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,chr.getLength());
				}
			}

			/**
			 * encode chromosome (ref seq info) vector to binary
			 *
			 * @param V reference sequence info vector
			 * @return BAM binary encoding of V
			 **/
			static std::string encodeChromosomeVector(std::vector< ::libmaus::bambam::Chromosome > const & V)
			{
				std::ostringstream ostr;
				encodeChromosomeVector(ostr,V);
				return ostr.str();
			}
			
			/**
			 * serialise header to BAM
			 *
			 * @param ostr output stream
			 **/
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
			
			struct BamHeaderParserState
			{
				// state
				bam_header_parse_state state;
			
				// number of magic bytes read
				uint64_t b_magic_read;
				
				// number of text length bytes read
				uint64_t b_l_text_read;
				// length of text
				uint64_t l_text;
				libmaus::autoarray::AutoArray<char> text;
				
				// number of text bytes read
				uint64_t b_text_read;
				
				// number of nref bytes read
				uint64_t b_n_ref;
				// number of reference sequences
				uint64_t n_ref;
				// number of references read
				uint64_t b_ref;
				
				// number of name length bytes read
				uint64_t b_l_name_read;
				// length of name
				uint64_t l_name;
				
				// number of name bytes read
				uint64_t b_name_read;
				// name
				libmaus::autoarray::AutoArray<char> name;
				
				// number of reference sequence length bytes read
				uint64_t b_l_ref_read;
				// length of ref seq
				uint64_t l_ref;

				//! chromosome (reference sequence meta data) vector
				std::vector< ::libmaus::bambam::Chromosome > chromosomes;
				
				BamHeaderParserState()
				: state(bam_header_read_magic),
				  b_magic_read(0),
				  b_l_text_read(0),
				  l_text(0),
				  text(),
				  b_text_read(0),
				  b_n_ref(0),
				  n_ref(0),
				  b_ref(0),
				  b_l_name_read(0),
				  l_name(0),
				  b_name_read(0),
				  name(0),
				  b_l_ref_read(0),
				  l_ref(0),
				  chromosomes()
				{
				
				}
			};

			static void parseHeader(std::istream & in)
			{
				libmaus::lz::BgzfInflateStream bgzfin(in);
				BamHeaderParserState state;
				
				while ( ! parseHeader(bgzfin,state,1).first )
				{
				
				}
			}
			
			BamHeader(BamHeaderParserState & state)
			{
				init(state);
			}
			
			void init(BamHeaderParserState & state)
			{
				text = std::string(state.text.begin(),state.text.begin()+state.l_text);
				chromosomes.swap(state.chromosomes);
				initSetup();
			}

			template<typename stream_type>
			static std::pair<bool,uint64_t> parseHeader(stream_type & in, BamHeaderParserState & state, 
				uint64_t const n = std::numeric_limits<uint64_t>::max()
			)
			{
				uint64_t r = 0;
				
				while ( r != n && state.state != bam_header_read_done && state.state != bam_header_read_failed )
				{
					switch ( state.state )
					{
						case bam_header_read_magic:
						{
							uint8_t const sym = getByte(in);
							r += 1;
							
							if ( 
								(state.b_magic_read == 0 && sym == 'B')
								||
								(state.b_magic_read == 1 && sym == 'A')
								||
								(state.b_magic_read == 2 && sym == 'M')
								||
								(state.b_magic_read == 3 && sym == '\1')
							)
							{
								state.b_magic_read++;
							}
							else
							{
								libmaus::exception::LibMausException se;
								se.getStream() << "Wrong magic number for BAM file." << std::endl;
								se.finish();
								throw se;
							}
							
							// switch to next state if we got the whole magic
							if ( state.b_magic_read == 4 )
								state.state = bam_header_read_l_text;
							
							break;
						}
						case bam_header_read_l_text:
						{
							uint8_t const sym = getByte(in);
							r += 1;
							
							state.l_text |= static_cast<uint64_t>(sym) << (8*state.b_l_text_read);
							
							state.b_l_text_read++;
							
							if ( state.b_l_text_read == 4 )
							{
								if ( state.l_text )
								{
									state.text.resize(state.l_text);
									state.state = bam_header_read_text;
								}
								else
									state.state = bam_header_read_n_ref;
							}
							break;
						}
						case bam_header_read_text:
						{
							uint8_t const sym = getByte(in);
							r += 1;
							
							state.text[state.b_text_read++] = static_cast<char>(static_cast<unsigned char>(sym));
							
							if ( state.b_text_read == state.l_text )
							{
								// removing padding null bytes
								while ( state.l_text && (!state.text[state.l_text-1]) )
									--state.l_text;
									
								#if 0
								std::cerr << 
									std::string(
										state.text.begin(),
										state.text.begin()+state.l_text);
								#endif
							
								state.state = bam_header_read_n_ref;
							}
							
							break;
						}
						case bam_header_read_n_ref:
						{
							uint8_t const sym = getByte(in);
							r += 1;
							
							state.n_ref |= static_cast<uint64_t>(sym) << (8*state.b_n_ref);
							state.b_n_ref += 1;
							
							if ( state.b_n_ref == 4 )
							{
								state.chromosomes.resize(state.n_ref);
								
								if ( state.n_ref )
									state.state = bam_header_reaf_ref_l_name;
								else
									state.state = bam_header_read_done;
							}
							
							break;
						}
						case bam_header_reaf_ref_l_name:
						{
							uint8_t const sym = getByte(in);
							r += 1;

							state.l_name |= static_cast<uint64_t>(sym) << (8*state.b_l_name_read);
							state.b_l_name_read += 1;
							
							if ( state.b_l_name_read == 4 )
							{
								state.b_name_read = 0;
								if ( state.l_name > state.name.size() )
									state.name.resize(state.l_name);
								
								if ( state.l_name )
									state.state = bam_header_read_ref_name;
								else
									state.state = bam_header_read_ref_l_ref;
							}
							break;
						}
						case bam_header_read_ref_name:
						{
							uint8_t const sym = getByte(in);
							r += 1;
							
							state.name[state.b_name_read] = static_cast<char>(static_cast<unsigned char>(sym));
							state.b_name_read += 1;
							
							if ( state.b_name_read == state.l_name )
								state.state = bam_header_read_ref_l_ref;
							
							break;
						}
						case bam_header_read_ref_l_ref:
						{
							uint8_t const sym = getByte(in);
							r += 1;
							
							state.l_ref |= static_cast<uint64_t>(sym) << (8*state.b_l_ref_read);
							state.b_l_ref_read += 1;
							
							if ( state.b_l_ref_read == 4 )
							{
								// remove padding null bytes from name
								while ( state.l_name && (!state.name[state.l_name-1]) )
									-- state.l_name;							

								state.chromosomes[state.b_ref] = ::libmaus::bambam::Chromosome(
									std::string(state.name.begin(),state.name.begin()+state.l_name),
									state.l_ref
								);

								#if 0
								std::cerr << 
									std::string(state.name.begin(),state.name.begin()+state.l_name)
									<< "\t"
									<< state.l_ref << std::endl;
								#endif

								state.b_ref += 1;
								
								if ( state.b_ref == state.n_ref )
								{
									state.state = bam_header_read_done;
								}
								else
								{
									state.b_l_name_read = 0;
									state.l_name = 0;
									state.b_name_read = 0;
									state.b_l_ref_read = 0;
									state.l_ref = 0;
									state.state = bam_header_reaf_ref_l_name;
								}
							}				
							break;
						}
						case bam_header_read_done:
						{
							break;
						}
						case bam_header_read_failed:
						{
							break;
						}
						default:
							state.state = bam_header_read_failed;
							break;
					}
				} 
				
				if ( state.state == bam_header_read_failed )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BamHeader::parseHeader failed." << std::endl;
					se.finish();
					throw se;
				}
				
				return 
					std::pair<bool,uint64_t>(state.state == bam_header_read_done,r);
			}
			
			struct HeaderLineSQNameComparator
			{
				bool operator()(HeaderLine const & A, HeaderLine const & B)
				{
					return A.getValue("SN") < B.getValue("SN");
				}
			};
			
			void initSetup()
			{
				text = rewriteHeader(text,chromosomes);

				RG = getReadGroups(text);
				RGTrie = computeRgTrie(RG);
				RGCSH = libmaus::hashing::ConstantStringHash::constructShared(RG.begin(),RG.end());
				
				if ( !RGCSH )
				{
					std::set<std::string> RGids;
					for ( uint64_t i = 0; i < RG.size(); ++i )
						RGids.insert(RG[i].ID);
					if ( RGids.size() != RG.size() )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "BamHeader::init(): Read group identifiers are not unique." << std::endl;
						se.finish();
						throw se;
					}
				}
				
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
						
				std::vector<HeaderLine> headerlines = libmaus::bambam::HeaderLine::extractLinesByType(text,"SQ");
				std::sort(headerlines.begin(),headerlines.end(),HeaderLineSQNameComparator());
				
				// fill information from text into refseq info
				for ( uint64_t i = 0; i < chromosomes.size(); ++i )
				{
					typedef std::vector<HeaderLine>::const_iterator it;
					HeaderLine ref;
					
					ref.type = "SQ";
					ref.M["SN"] = chromosomes[i].getNameString();
					
					// look for chromosome/refseq in parsed text
					std::pair<it,it> const p = std::equal_range(
						headerlines.begin(),headerlines.end(),
						ref,
						HeaderLineSQNameComparator()
					);

					// if line is in text
					if ( p.first != p.second )
					{
						// get line
						HeaderLine const & line = *(p.first);
						// build string from rest of arguments
						std::ostringstream restkvostr;
						// 
						uint64_t restkvostrcont = 0;

						// iterate over key:value pairs
						for ( std::map<std::string,std::string>::const_iterator ita = line.M.begin();
							ita != line.M.end(); ++ita )
						{
							std::pair<std::string,std::string> const pp = *ita;
							
							// sequence name should fit (or the equal_range call above is broken)
							if ( pp.first == "SN" )
							{
								assert ( pp.second == chromosomes[i].getNameString() );
							}
							// check that sequence length is consistent between text and binary
							else if ( pp.first == "LN" )
							{
								std::istringstream istr(pp.second);
								uint64_t len;
								istr >> len;
								if ( chromosomes[i].getLength() != len )
								{
									libmaus::exception::LibMausException se;
									se.getStream() << "BAM header is not consistent (binary and text do not match) for " << line.line << std::endl;
									se.finish();
									throw se;
								}
							}
							else
							{
								if ( restkvostrcont++ )
									restkvostr.put('\t');
									
								restkvostr << pp.first << ":" << pp.second;
									
								// chromosomes[i].addKeyValuePair(pp.first,pp.second);
							}
						}
						
						if ( restkvostrcont )
							chromosomes[i].setRestKVString(restkvostr.str());
					}
				}
			}

			/**
			 * init BAM header from uncompressed stream
			 *
			 * @param in input stream
			 **/
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

				initSetup();
			}
			
			/**
			 * change sort order to newsortorder; if newsortorder is the empty string then
			 * just rewrite header keeping the previous sort order description
			 *
			 * @param newsortorder
			 **/
			void changeSortOrder(std::string const newsortorder = std::string())
			{
				text = rewriteHeader(text,chromosomes,newsortorder);
			}
			
			/**
			 * produce header text
			 **/
			void produceHeader()
			{
				changeSortOrder();
			}
			
			/**
			 * get size of BAM header given the name of the BAM file
			 *
			 * @param fn BAM file name
			 * @return length of BAM header
			 **/
			static uint64_t getHeaderSize(std::string const & fn)
			{
				libmaus::aio::CheckedInputStream CIS(fn);
				::libmaus::lz::GzipStream GS(CIS);
				BamHeader header(GS);
				return GS.tellg();
			}
			
			/**
			 * constructor for empty header
			 **/
			BamHeader()
			{
			
			}
			
			/**
			 * constructor from compressed stream in
			 *
			 * @param in compressed stream
			 **/
			BamHeader(std::istream & in)
			{
				::libmaus::lz::GzipStream GS(in);
				init(GS);
			}

			/**
			 * constructor from compressed general GZ type stream
			 *
			 * @param in gzip intput stream
			 **/
			BamHeader(::libmaus::lz::GzipStream & in)
			{
				init(in);
			}
			/**
			 * constructor from serial bgzf decompressor
			 *
			 * @param in serial bgzf decompressor
			 **/
			BamHeader(libmaus::lz::BgzfInflateStream & in)
			{
				init(in);
			}
			/**
			 * constructor from parallel bgzf decompressor
			 *
			 * @param in parallel bgzf decompressor
			 **/
			BamHeader(libmaus::lz::BgzfInflateParallelStream & in)
			{
				init(in);
			}
			/**
			 * constructor from parallel bgzf decompressor
			 *
			 * @param in parallel bgzf decompressor
			 **/
			BamHeader(libmaus::lz::BgzfInflateDeflateParallelInputStream & in)
			{
				init(in);
			}
			
			void replaceReadGroupNames(std::map<std::string,std::string> const & M)
			{
				std::istringstream istr(text);
				std::ostringstream ostr;
				
				while ( istr )
				{
					std::string line;
					std::getline(istr,line);
					
					if ( line.size() >= 3 && line[0] == '@' && line[1] == 'R' && line[2] == 'G' )
					{
						HeaderLine HL(line);
						assert ( HL.type == "RG" );
						
						if ( HL.hasKey("ID") )
						{
							std::string const oldID = HL.getValue("ID");
							std::map<std::string,std::string>::const_iterator it = M.find(oldID);
							if ( it != M.end() )
							{
								HL.M["ID"] = it->second;
								HL.constructLine();
								line = HL.line;
							}
						}
					}
					
					if ( line.size() )
						ostr << line << '\n';
				}
				
				*this = BamHeader(ostr.str());
			}
			
			/**
			 * constructor from header text
			 *
			 * @param text header text
			 **/
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
			
			/**
			 * add a reference sequence to the header
			 *
			 * @param name ref seq name
			 * @param len ref seq length
			 * @return id of the newly inserted ref seq in this header
			 **/
			uint64_t addChromosome(std::string const & name, uint64_t const len)
			{
				uint64_t const id = chromosomes.size();
				chromosomes.push_back(Chromosome(name,len));
				return id;
			}
			
			/**
			 * get id for reference name
			 *
			 * @param name reference name
			 * @return id for name
			 **/
			uint64_t getIdForRefName(std::string const & name) const
			{
				for ( uint64_t i = 0; i < chromosomes.size(); ++i )
				{
					std::pair<char const *, char const *> const P = chromosomes[i].getName();
					char const * qa = P.first;
					char const * qe = P.second;
					char const * ca = name.c_str();
					char const * ce = ca + name.size();
					
					// same length
					if ( qe-qa == ce-ca )
					{
						// check for equal sequence of letters
						for ( ; qa != qe ; ++qa, ++ca )
							if ( *qa != *ca )
								break;
								
						if ( qa == qe )
						{
							assert ( name == chromosomes[i].getNameString() );
							return i;
						}
					}					
				}
						
				libmaus::exception::LibMausException se;
				se.getStream() << "Reference name " << name << " does not exist in file." << std::endl;
				se.finish();
				throw se;
			}
		};
	}
}
#endif
