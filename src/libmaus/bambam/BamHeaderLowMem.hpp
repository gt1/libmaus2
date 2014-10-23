/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_BAMHEADERLOWMEM_HPP)
#define LIBMAUS_BAMBAM_BAMHEADERLOWMEM_HPP

#include <libmaus/bambam/DecoderBase.hpp>
#include <libmaus/bambam/EncoderBase.hpp>
#include <libmaus/bitio/BitVector.hpp>
#include <libmaus/hashing/ConstantStringHash.hpp>
#include <libmaus/rank/ImpCacheLineRank.hpp>
#include <libmaus/trie/TrieState.hpp>
#include <libmaus/util/CountPutObject.hpp>
#include <libmaus/util/LineAccessor.hpp>
#include <libmaus/util/DigitTable.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamHeaderLowMem;
		std::ostream & operator<<(::std::ostream & out, ::libmaus::bambam::BamHeaderLowMem const & BHLM);
			
		struct BamHeaderLowMem :
			public ::libmaus::bambam::EncoderBase, 
			public ::libmaus::bambam::DecoderBase
		{
			public:
			typedef BamHeaderLowMem this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			friend std::ostream & operator<<(::std::ostream & out, ::libmaus::bambam::BamHeaderLowMem const & BHLM);
			
			typedef uint32_t offset_type;
			
			libmaus::autoarray::AutoArray<char> Atext;
			char * text;
			libmaus::util::LineAccessor::unique_ptr_type PLA;

			libmaus::autoarray::AutoArray<char> SQtext;
			libmaus::autoarray::AutoArray<int32_t> LNvec;
			libmaus::autoarray::AutoArray<offset_type> SQoffsets;
			
			libmaus::bitio::IndexedBitVector::unique_ptr_type Psqbitvec;
			
			int64_t HDid;
			
			struct IdSortInfo
			{
				uint32_t idid;
				offset_type idtextoffset;
				uint32_t lineid;
				
				IdSortInfo() : idid(0), idtextoffset(0), lineid(0) {}
				IdSortInfo(
					uint32_t const ridid,
					offset_type const ridtextoffset,
					uint32_t const rlineid		
				) : idid(ridid), idtextoffset(ridtextoffset), lineid(rlineid) {}
			};
			
			libmaus::autoarray::AutoArray<char> PGidtext;
			libmaus::autoarray::AutoArray< IdSortInfo > PGidsort;

			libmaus::autoarray::AutoArray<char> RGidtext;
			libmaus::autoarray::AutoArray< IdSortInfo > RGidsort;
			
			char const * noparidstring;
			
			std::vector<uint32_t> rglines;
			std::vector<std::string> libs;
			std::vector<uint32_t> rgtolib;

			//! trie for read group names
			::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type RGTrie;
			//! hash for read names
			libmaus::hashing::ConstantStringHash::unique_ptr_type RGCSH;
			
			struct IdSortComparator
			{
				char const * text;
				
				IdSortComparator(char const * rtext)
				: text(rtext)
				{
				
				}
				
				bool operator()(IdSortInfo const & A, IdSortInfo const & B) const
				{
					return strcmp(text + A.idtextoffset, text + B.idtextoffset) < 0;
				}
			};
			
			struct ReadGroupHashProxy
			{
				BamHeaderLowMem const * owner;
				uint64_t id;
				
				ReadGroupHashProxy()
				{
				
				}
				
				ReadGroupHashProxy(BamHeaderLowMem const * const rowner, uint64_t const rid)
				: owner(rowner), id(rid) {}
				
				/**
				 * compute 32 bit hash value from iterator range
				 *
				 * @param ita start iterator (inclusive)
				 * @param ite end iterator (exclusive)
				 * @return hash value
				 **/
				template<typename iterator>
				static uint32_t hash(iterator ita, iterator ite)
				{
					return libmaus::hashing::EvaHash::hash(reinterpret_cast<uint8_t const *>(ita),ite-ita);
				}

				/**
				 * compute hash value from read group id
				 *
				 * @return hash value
				 **/
				uint32_t hash() const
				{
					std::pair<char const *, uint64_t> const P = owner->getReadGroupIdentifier(id);
					uint8_t const * ita = reinterpret_cast<uint8_t const *>(P.first);
					return hash(ita,ita+P.second);
				}
				
			};
			
			struct IdArrayAccessor
			{
				typedef IdArrayAccessor this_type;
			
				libmaus::autoarray::AutoArray<char> const & text;
				libmaus::autoarray::AutoArray< IdSortInfo > const & offsets;
				
				typedef libmaus::util::ConstIterator<this_type,char const *> const_iterator;
				
				IdArrayAccessor(
					libmaus::autoarray::AutoArray<char> const & rtext,
					libmaus::autoarray::AutoArray< IdSortInfo > const & roffsets
				
				)
				: text(rtext), offsets(roffsets)
				{
				
				}
				
				char const * get(uint64_t const i) const
				{
					return text.begin() + offsets[i].idtextoffset;
				}
				
				const_iterator begin() const
				{
					return const_iterator(this);
				}
				const_iterator end() const
				{
					return begin() + offsets.size();
				}
			};
			
			struct StringComparator
			{
				bool operator()(char const * A, char const * B)
				{
					return strcmp(A,B) < 0;
				}
			};
			
			IdSortInfo const * findPG(char const * c) const
			{
				IdArrayAccessor acc(PGidtext,PGidsort);
				IdArrayAccessor::const_iterator it = std::lower_bound(acc.begin(),acc.end(),c,StringComparator());
				
				if ( it == acc.end() )
					return 0;
				else
				{
					IdSortInfo const & ISI = PGidsort [ it-acc.begin() ];

					// check name
					if ( strcmp(c,PGidtext.begin() + ISI.idtextoffset) == 0 )
						return &ISI;
					else
						return 0;
				}
			}

			IdSortInfo const * findRG(char const * c) const
			{
				IdArrayAccessor acc(RGidtext,RGidsort);
				IdArrayAccessor::const_iterator it = std::lower_bound(acc.begin(),acc.end(),c,StringComparator());
				
				if ( it == acc.end() )
					return 0;
				else
					return &(RGidsort [ it-acc.begin() ]);
			}
			
			BamHeaderLowMem() : text(0), HDid(-1), noparidstring(0)
			{
			
			}

			/**
			 * compute trie for read group names
			 *
			 * @param RG read group vector
			 * @return trie for read group names
			 **/
			::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type computeRgTrie()
			{
				::libmaus::trie::Trie<char> trienofailure;
				std::vector<std::string> dict;
				for ( uint64_t i = 0; i < getNumReadGroups(); ++i )
					dict.push_back(getReadGroupIdentifierAsString(i));
				trienofailure.insertContainer(dict);
				::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure 
					(trienofailure.toLinearHashTrie<uint32_t>());

				return UNIQUE_PTR_MOVE(LHTnofailure);
			}
			
			void setupFromText()
			{
				#if 0
				for ( uint64_t i = 0; i < PLA->size(); ++i )
					std::cerr << "[" << i << "]=" << PLA->getLine(text,i) << "\n";
				#endif
				
				libmaus::util::DigitTable digtab;
				
				uint64_t sqtextlen = 0;
				uint64_t numsq = 0;

				uint64_t numrg = 0;
				uint64_t rgidtextlen = 0;
				
				uint64_t numpg = 0;
				uint64_t pgidtextlen = 0;
				
				for ( uint64_t i = 0; i < PLA->size(); ++i )
				{
					std::pair<uint64_t,uint64_t> P = PLA->lineInterval(i);
					if ( P.second != P.first && text[P.second-1] == '\r' )
						--P.second;
						
					bool const isSQ =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'S' &&
						text[P.first+2] == 'Q';
					bool const isHD =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'H' &&
						text[P.first+2] == 'D';
					bool const isPG =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'P' &&
						text[P.first+2] == 'G';
					bool const isRG =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'R' &&
						text[P.first+2] == 'G';
					
					if ( isHD )
					{
						if ( HDid != -1 )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: second HD line " << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;																			
						}
						
						HDid = i;
					}
					else if ( isSQ )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'S' ||
								text[j+1] != 'N' ||
								text[j+2] != ':'
							)
						)
							++j;
							
						if ( j+2 >= P.second )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @SQ line " << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}
						
						uint64_t snstart = j+3;
						uint64_t snend = snstart;
						while ( snend < P.second && text[snend] != '\t' )
							++snend;

						j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'L' ||
								text[j+1] != 'N' ||
								text[j+2] != ':'
							)
						)
							++j;

						if ( j+2 >= P.second )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @SQ line " << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}

						uint64_t lnstart = j+3;
						uint64_t lnend = lnstart;
						while ( lnend < P.second && text[lnend] != '\t' )
							++lnend;

						if ( ! (lnend-lnstart) )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @SQ line " << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}
						
						uint64_t ln = 0;
						for ( uint64_t k = lnstart; k < lnend; ++k )
						{
							ln *= 10;
							
							if ( ! digtab[static_cast<unsigned char>(text[k])] )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "BamHeaderLowMem: defect @SQ line " << PLA->getLine(text,i) << std::endl;
								se.finish();
								throw se;
							}
							
							ln += text[k]-'0';						
						}

						#if 0
						std::cerr << "[" << i << "]=" << PLA->getLine(text,i) 
							<< "\t" << std::string(text+snstart,text+snend)
							<< "\t" << std::string(text+lnstart,text+lnend)
							<< "\t" << ln
							<< "\n";
						#endif
						
						numsq++;
						sqtextlen += (snend-snstart);
					}
					else if ( isPG )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'I' ||
								text[j+1] != 'D' ||
								text[j+2] != ':'
							)
						)
							++j;
					
						if ( j+2 >= P.second )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @PG line with no ID field:\n" << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}
						
						uint64_t idstart = j+3;
						uint64_t idend = idstart;
						
						while ( idend != P.second && text[idend] != '\t' )
							++idend;

						if ( idend == idstart )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @PG line with empty ID field:\n" << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}

						// std::cerr << "[" << i << "]=" << std::string(text+idstart,text+idend) << "\t" << PLA->getLine(text,i) << "\n";

						numpg += 1;
						pgidtextlen += idend-idstart;
					}
					else if ( isRG )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'I' ||
								text[j+1] != 'D' ||
								text[j+2] != ':'
							)
						)
							++j;
					
						if ( j+2 >= P.second )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @RG line with no ID field:\n" << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}
						
						uint64_t idstart = j+3;
						uint64_t idend = idstart;
						
						while ( idend != P.second && text[idend] != '\t' )
							++idend;

						if ( idend == idstart )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @RG line with empty ID field:\n" << PLA->getLine(text,i) << std::endl;
							se.finish();
							throw se;													
						}

						// std::cerr << "[" << i << "]=" << std::string(text+idstart,text+idend) << "\t" << PLA->getLine(text,i) << "\n";

						numrg += 1;
						rgidtextlen += idend-idstart;
					}
				}
				
				SQtext = libmaus::autoarray::AutoArray<char>(sqtextlen + numsq,false);
				char * SQtextc = SQtext.begin();
				LNvec = libmaus::autoarray::AutoArray<int32_t>(numsq,false);
				SQoffsets = libmaus::autoarray::AutoArray<offset_type>(numsq+1,false);
				libmaus::bitio::IndexedBitVector::unique_ptr_type Tsqbitvec(new libmaus::bitio::IndexedBitVector(PLA->size()));
				Psqbitvec = UNIQUE_PTR_MOVE(Tsqbitvec);
				
				rglines.resize(numrg);
				rgtolib.resize(numrg);
				RGidtext = libmaus::autoarray::AutoArray<char>(rgidtextlen + numrg,false);
				RGidsort = libmaus::autoarray::AutoArray< IdSortInfo >(numrg,false);
				char * RGtextc = RGidtext.begin();
				numrg = 0;
				
				PGidtext = libmaus::autoarray::AutoArray<char>(pgidtextlen + numpg,false);
				PGidsort = libmaus::autoarray::AutoArray< IdSortInfo >(numpg,false);
				char * PGtextc = PGidtext.begin();
				numpg = 0;

				numsq = 0;
				for ( uint64_t i = 0; i < PLA->size(); ++i )
				{
					std::pair<uint64_t,uint64_t> P = PLA->lineInterval(i);
					if ( P.second != P.first && text[P.second-1] == '\r' )
						--P.second;
						
					bool const isSQ = 
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'S' &&
						text[P.first+2] == 'Q';
					bool const isPG =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'P' &&
						text[P.first+2] == 'G';
					bool const isRG =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'R' &&
						text[P.first+2] == 'G';
						
					Psqbitvec->set(i,isSQ);
					
					if ( isSQ )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'S' ||
								text[j+1] != 'N' ||
								text[j+2] != ':'
							)
						)
							++j;
							
						uint64_t snstart = j+3;
						uint64_t snend = snstart;
						while ( snend < P.second && text[snend] != '\t' )
							++snend;

						j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'L' ||
								text[j+1] != 'N' ||
								text[j+2] != ':'
							)
						)
							++j;

						uint64_t lnstart = j+3;
						uint64_t lnend = lnstart;
						while ( lnend < P.second && text[lnend] != '\t' )
							++lnend;

						uint64_t ln = 0;
						for ( uint64_t k = lnstart; k < lnend; ++k )
						{
							ln *= 10;
							ln += text[k]-'0';						
						}
						
						SQoffsets[numsq] = SQtextc - SQtext.begin();
						
						std::copy(
							text + snstart,
							text + snend,
							SQtextc
						);
						SQtextc += snend-snstart;
						*(SQtextc++) = 0;
						
						LNvec[numsq++] = ln;
						
						#if 0
						std::cerr << "[" << i << "]=" << PLA->getLine(text,i) 
							<< "\t" << std::string(text+snstart,text+snend)
							<< "\t" << std::string(text+lnstart,text+lnend)
							<< "\t" << ln
							<< "\n";
						#endif
					}
					else if ( isPG )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'I' ||
								text[j+1] != 'D' ||
								text[j+2] != ':'
							)
						)
							++j;
					
						uint64_t idstart = j+3;
						uint64_t idend = idstart;
						
						while ( idend != P.second && text[idend] != '\t' )
							++idend;
							
						PGidsort[numpg] = IdSortInfo(numpg,PGtextc-PGidtext.begin(),i);
						std::copy ( text + idstart, text+idend, PGtextc);
						PGtextc += (idend-idstart);
						*(PGtextc++) = 0;
						numpg++;
					}
					else if ( isRG )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								text[j] != 'I' ||
								text[j+1] != 'D' ||
								text[j+2] != ':'
							)
						)
							++j;
					
						uint64_t idstart = j+3;
						uint64_t idend = idstart;
						
						while ( idend != P.second && text[idend] != '\t' )
							++idend;

						rglines[numrg] = i;
						RGidsort[numrg] = IdSortInfo(numrg,RGtextc-RGidtext.begin(),i);
						std::copy ( text + idstart, text+idend, RGtextc);
						RGtextc += (idend-idstart);
						*(RGtextc++) = 0;
						numrg++;
					}
				}
				
				std::sort(
					PGidsort.begin(),
					PGidsort.end(),
					IdSortComparator(PGidtext.begin())
				);
				std::sort(
					RGidsort.begin(),
					RGidsort.end(),
					IdSortComparator(RGidtext.begin())
				);
			
				std::vector<bool> PGparvec(PGidsort.size(),false);	
				for ( uint64_t i = 0; i < PGidsort.size(); ++i )
				{
					#if 0
					std::cerr << "PG\t" 
						<< PGidsort[i].idid << "\t" 
						<< PGidtext.begin() + PGidsort[i].idtextoffset << "\t"
						<< PGidsort[i].lineid
						<< std::endl;
					#endif
					
					assert (
						( findPG(PGidtext.begin() + PGidsort[i].idtextoffset) != 0 )
						&& 
						( findPG(PGidtext.begin() + PGidsort[i].idtextoffset)->idid == PGidsort[i].idid )
					);
					
					uint64_t const lineid = PGidsort[i].lineid;
					std::pair<uint64_t,uint64_t> P = PLA->lineInterval(lineid);
					if ( P.second != P.first && text[P.second-1] == '\r' )
						--P.second;

					uint64_t j = P.first;
					while ( 
						j+2 < P.second &&
						(
							text[j]   != 'P' ||
							text[j+1] != 'P' ||
							text[j+2] != ':'
						)
					)
						++j;
						
					if ( j+2 >= P.second )
					{
						// no parent
					}
					else
					{
						// extract parent id
						uint64_t parstart = j+3;
						uint64_t parend = parstart;
						while ( parend != P.second && text[parend] != '\t' )
							++parend;
						if ( parend==parstart )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @PG line with empty PP field:\n" << PLA->getLine(text,lineid) << std::endl;
							se.finish();
							throw se;						
						}

						// find it
						IdSortInfo const * isi = findPG(std::string(text + parstart,text+parend).c_str());

						if ( ! isi )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "BamHeaderLowMem: defect @PG line with non-existant parent " << std::string(text+parstart,text+parend) << ":\n" << PLA->getLine(text,lineid) << std::endl;
							se.finish();
							throw se;													
						}
						
						assert ( isi->idid < PGparvec.size() );
						// mark parent
						PGparvec[isi->idid] = true;
					}
				}
				
				// if we have any PG lines
				if ( PGparvec.size() )
				{
					// look for PG lines which are not parent of another
					std::vector<uint64_t> noparids;
					for ( uint64_t i = 0; i < PGparvec.size(); ++i )
						if ( ! PGparvec[i] )
							noparids.push_back(i);
					
					uint64_t noparid;
					if ( noparids.size() == 0 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "BamHeaderLowMem: there is no PG line which is not a parent of another (PG chain is circular)" << "\n";
						se.finish();
						throw se;													
					}
					else if ( noparids.size() == 1 )
					{
						noparid = noparids[0];
					}
					else
					{
						std::cerr << "[W] PG lines do not form a linear chain" << std::endl;
						std::sort(noparids.begin(),noparids.end());
						noparid = noparids[noparids.size()-1];
					}
					
					for ( uint64_t i = 0; i < PGidsort.size(); ++i )
						if ( PGidsort[i].idid == noparid )
							noparidstring = PGidtext.begin() + PGidsort[i].idtextoffset;
					
					assert ( noparidstring != 0 );
				}
				// std::cerr << "noparidstring=" << noparidstring << std::endl;
				// std::cerr << "PG id text " << PGidtext.begin() << std::endl;

				for ( uint64_t i = 0; i < RGidsort.size(); ++i )
				{
					#if 0
					std::cerr << "RG\t" 
						<< RGidsort[i].idid << "\t" 
						<< RGidtext.begin() + RGidsort[i].idtextoffset << "\t"
						<< RGidsort[i].lineid
						<< std::endl;
					#endif
					
					assert (
						( findRG(RGidtext.begin() + RGidsort[i].idtextoffset) != 0 )
						&& 
						( findRG(RGidtext.begin() + RGidsort[i].idtextoffset)->idid == RGidsort[i].idid )
					);					
				}
				
				SQoffsets[numsq] = SQtextc - SQtext.begin();
				Psqbitvec->setupIndex();
				
				// get list of libraries
				libs = getLibrarySet();

				// set up read group id to library id mapping
				for ( uint64_t i = 0; i < getNumReadGroups(); ++i )
				{
					std::pair<char const *, uint64_t> const P = getLibraryIdentifier(i);
					rgtolib[i] = P.first ? (std::lower_bound(libs.begin(),libs.end(),std::string(P.first,P.first+P.second)) - libs.begin()) : libs.size();
				}
				
				::libmaus::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type TRGTrie ( computeRgTrie() );
				RGTrie = UNIQUE_PTR_MOVE(TRGTrie);
				
				std::vector<ReadGroupHashProxy> RGproxies(getNumReadGroups());
				for ( uint64_t i = 0; i < getNumReadGroups(); ++i )
					RGproxies[i] = ReadGroupHashProxy(this,i);
				libmaus::hashing::ConstantStringHash::unique_ptr_type TRGCSH (
					libmaus::hashing::ConstantStringHash::construct(RGproxies.begin(),RGproxies.end())
				);
				RGCSH = UNIQUE_PTR_MOVE(TRGCSH);

				if ( !RGCSH )
				{
					std::set<std::string> RGids;
					for ( uint64_t i = 0; i < getNumReadGroups(); ++i )
						RGids.insert(getReadGroupIdentifierAsString(i));
					if ( RGids.size() != getNumReadGroups() )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "Read group identifiers are not unique." << std::endl;
						se.finish();
						throw se;
					}
				}
			}

			void readTextFromBAM(std::istream & in)
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
					se.getStream() << "Wrong magic in BamHeaderLowMem constructor" << std::endl;
					se.finish();
					throw se;					
				}

				uint64_t l_text = getLEInteger(in,4);
				Atext = libmaus::autoarray::AutoArray<char>(l_text,false);
				text = Atext.begin();
				
				in.read(text,l_text);
				if ( static_cast<int64_t>(in.gcount()) != static_cast<int64_t>(l_text) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to read header text in BamHeaderLowMem constructor" << std::endl;
					se.finish();
					throw se;									
				}
				
				// remove null bytes at end of header (if any)
				while ( l_text && !text[l_text-1] )
					--l_text;
				
				libmaus::util::LineAccessor::unique_ptr_type TLA(new libmaus::util::LineAccessor(text,text+l_text));
				PLA = UNIQUE_PTR_MOVE(TLA);
			}

			template<typename iterator>
			void constructFromTextInternal(iterator ita, iterator ite)
			{			
				Atext = libmaus::autoarray::AutoArray<char>(ite-ita,false);
				text = Atext.begin();
				std::copy(ita,ite,text);

				libmaus::util::LineAccessor::unique_ptr_type TLA(new libmaus::util::LineAccessor(text,text+(ite-ita)));
				PLA = UNIQUE_PTR_MOVE(TLA);

				setupFromText();
			}

			void constructFromBAMInternal(std::istream & in)
			{			
				readTextFromBAM(in);
				setupFromText();
				
				uint64_t const n_ref = getLEInteger(in,4);
				libmaus::autoarray::AutoArray<char> name;
				
				if ( n_ref != getNumRef() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamHeaderLowMem: text and binary header information is not consistent:\n";
					se.getStream() << "Number of sequences in text is " << getNumRef() << "\n";
					se.getStream() << "Number of sequences in binary is " << n_ref << "\n";
					se.finish();
					throw se;					
				}
						
				for ( uint64_t i = 0; i < n_ref; ++i )
				{
					uint64_t l_name = getLEInteger(in,4);
					assert ( l_name );
					if ( l_name > name.size() )
						name = libmaus::autoarray::AutoArray<char>(l_name,false);
					for ( uint64_t j = 0 ; j < l_name; ++j )
						name[j] = getByte(in);
					assert ( name[l_name-1] == 0 );
					uint64_t l_ref = getLEInteger(in,4);	

					if ( strcmp(name.begin(),SQtext.begin()+SQoffsets[i]) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "BamHeaderLowMem: text and binary header information is not consistent:\n";
						se.getStream() << "Sequence name in text: " << SQtext.begin()+SQoffsets[i] << "\n";
						se.getStream() << "Sequence name in binary: " << name.begin() << "\n";
						se.finish();
						throw se;					
					}
					if ( static_cast<int64_t>(l_ref) != static_cast<int64_t>(LNvec[i]) )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "BamHeaderLowMem: text and binary header information is not consistent for sequence " << name.begin() << ":\n";
						se.getStream() << "Sequence length in text: " << LNvec[i] << "\n";
						se.getStream() << "Sequence length in binary: " << l_ref << "\n";
						se.finish();
						throw se;										
					}
				}
			}

			/**
			 * encode binary reference sequence information
			 *
			 * @param ostr binary BAM header construction stream
			 * @param V chromosomes (ref seqs)
			 **/
			template<typename stream_type>
			void encodeFilteredChromosomeVector(stream_type & ostr, ::libmaus::bitio::IndexedBitVector const & IBV) const
			{
				// number of sequences to be kept
				uint64_t const keep = IBV.size() ? IBV.rank1(IBV.size()-1) : 0;
				
				// std::cerr << "IBV.size()=" << IBV.size() << " SQoffsets.size()=" << SQoffsets.size() << std::endl;
				
				assert ( (IBV.size()+1) == SQoffsets.size() );
			
				::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,keep);

				for ( uint64_t i = 0; i < SQoffsets.size(); ++i )
					if ( IBV.get(i) )
					{
						char const * name = getRefIDName(i);
						uint64_t const namesize = strlen(name);
						::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,namesize+1);
						ostr.write(name,namesize);
						ostr.put(0);
						::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,getRefIDLength(i));	
					}
			}
			
			std::string getUniquePGId(std::string pgID) const
			{
				std::string prefix = pgID;
				uint64_t add = 0;
				
				while ( findPG(pgID.c_str()) )
				{
					std::ostringstream ostr;
					ostr << prefix << "_" << std::setw(4) << std::setfill('0') << (add++);
					pgID = ostr.str();
				}
				
				return pgID;
			}
			
			template<typename stream_type>
			void writeTextSubset(
				stream_type & ostr, ::libmaus::bitio::IndexedBitVector const & IBV,
				std::string const & pgID,
				std::string const & pgPN,
				std::string const & pgCL,
				std::string const & pgVN			
			) const
			{
				if (  HDid == -1 )
				{
					char const * hdtext = "@HD\tVN:1.5\tSO:unknown\n";
					ostr.write(hdtext,strlen(hdtext));
				}
				
				uint64_t seqid = 0;
				
				for ( uint64_t i = 0; i < PLA->size(); ++i )
				{				
					std::pair<uint64_t,uint64_t> P = PLA->lineInterval(i);
					if ( P.second != P.first && text[P.second-1] == '\r' )
						--P.second;
						
					bool const isSQ = 
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'S' &&
						text[P.first+2] == 'Q';
					bool const isPG =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'P' &&
						text[P.first+2] == 'G';
					bool const isRG =
						P.second-P.first >= 3 && 
						text[P.first+0] == '@' &&
						text[P.first+1] == 'R' &&
						text[P.first+2] == 'G';
				
					if ( isSQ )
					{
						if ( IBV.get(seqid) )
						{
							ostr.write(
								text+P.first,
								P.second-P.first
							);
							ostr.put('\n');				
						}
						seqid++;
						
						if ( seqid == IBV.size() && !noparidstring )
						{
							std::ostringstream pgostr;
							pgostr << "@PG" << "\tID:" << getUniquePGId(pgID);
							if ( pgPN.size() )
								pgostr << "\tPN:" << pgPN;
							if ( pgCL.size() )
								pgostr << "\tCL:" << pgCL;
							if ( pgVN.size() )
								pgostr << "\tVN:" << pgVN;
							pgostr << "\n";

							std::string const pgline = pgostr.str();							
							
							ostr.write(pgline.c_str(),pgline.size());
						}
					}
					else if ( isPG )
					{
						uint64_t j = P.first;
						while ( 
							j+2 < P.second &&
							(
								(text[j] != 'I') ||
								(text[j+1] != 'D') ||
								(text[j+2] != ':')
							)
						)
							++j;
					
						assert ( j+2 < P.second );
					
						uint64_t idstart = j+3;						
						uint64_t idend = idstart;
						
						while ( idend < P.second && text[idend] != '\t' )
							++idend;
							
						assert ( idstart != idend );
						
						IdSortInfo const * idinfo = findPG(std::string(text+idstart,text+idend).c_str());
												
						assert ( idinfo );
						
						ostr.write(
							text+P.first,
							P.second-P.first
						);
						ostr.put('\n');
						
						if ( 
							noparidstring && 
							(strcmp(PGidtext.begin() + idinfo->idtextoffset,noparidstring) == 0) 
						)
						{
							std::ostringstream pgostr;
							pgostr << "@PG" << "\tID:" << getUniquePGId(pgID);
							if ( pgPN.size() )
								pgostr << "\tPN:" << pgPN;
							if ( pgCL.size() )
								pgostr << "\tCL:" << pgCL;
							// if ( PP.size() )
							pgostr << "\tPP:" << noparidstring;
							if ( pgVN.size() )
								pgostr << "\tVN:" << pgVN;
							pgostr << "\n";

							std::string const pgline = pgostr.str();							
							
							ostr.write(pgline.c_str(),pgline.size());
						}
					}
					else if ( isRG )
					{					
						ostr.write(
							text+P.first,
							P.second-P.first
						);
						ostr.put('\n');
					}
					else
					{
						ostr.write(
							text+P.first,
							P.second-P.first
						);
						ostr.put('\n');
					}
				}				
			}

			public:
			static unique_ptr_type constructFromBAM(std::istream & in)
			{
				unique_ptr_type ptr(new this_type);
				ptr->constructFromBAMInternal(in);
				return UNIQUE_PTR_MOVE(ptr);
			}

			template<typename iterator>
			static unique_ptr_type constructFromText(iterator ita, iterator ite)
			{
				unique_ptr_type ptr(new this_type);
				ptr->constructFromTextInternal(ita,ite);
				return UNIQUE_PTR_MOVE(ptr);
			}

			/**
			 * get name for reference id
			 *
			 * @param refid reference id
			 * @return name for reference id or "*" if invalid ref id
			 **/
			char const * getRefIDName(int64_t const refid) const
			{
				if ( refid < 0 || refid >= static_cast<int64_t>(getNumRef()) )
					return "*";
				else
					return SQtext.begin() + SQoffsets[refid];
			}
			
			/**
			 * get reference id length
			 **/
			int64_t getRefIDLength(int64_t const refid) const
			{
				if ( refid < 0 || refid >= static_cast<int64_t>(getNumRef()) )
					return -1;
				else
					return LNvec[refid];
			}
			
			/**
			 * get number of reference sequences
			 **/
			uint64_t getNumRef() const
			{
				return LNvec.size();
			}
			
			/**
			 * get reference id for name
			 **/
			int64_t getRefIdForName(char const * c) const
			{
				for ( uint64_t i = 0; i < getNumRef(); ++i )
					if ( strcmp(c,SQtext.begin() + SQoffsets[i]) == 0 )
						return i;
				return -1;
			}
			
			/**
			 * get number of read groups
			 **/
			uint64_t getNumReadGroups() const
			{
				return rglines.size();
			}
			
			std::string getReadGroupIdentifierAsString(int64_t const i) const
			{
				std::pair<char const *, uint64_t> const P = getReadGroupIdentifier(i);
				
				if ( ! P.first )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Invalid read group id " << i << "\n";
					se.finish();
					throw se;
				}
				
				return std::string(P.first,P.first+P.second);
			}
			
			/**
			 * get read group identifier string for read group with numerical id
			 **/
			std::pair<char const *, uint64_t> getReadGroupIdentifier(int64_t const i) const
			{
				if ( i < 0 || i >= static_cast<int64_t>(getNumReadGroups()) )
				{
					char const * null = 0;
					return std::pair<char const *, uint64_t>(null,0);
				}
				
				uint64_t const lineid = rglines.at(i);
				
				std::pair<uint64_t,uint64_t> P = PLA->lineInterval(lineid);
				if ( P.second != P.first && text[P.second-1] == '\r' )
					--P.second;
						
				bool const isRG =
					P.second-P.first >= 3 && 
					text[P.first+0] == '@' &&
					text[P.first+1] == 'R' &&
					text[P.first+2] == 'G';
				
				assert ( isRG );

				uint64_t j = P.first;
				while ( 
					j+2 < P.second &&
					(
						text[j] != 'I' ||
						text[j+1] != 'D' ||
						text[j+2] != ':'
					)
				)
					++j;
			
				assert ( j+2 < P.second );
				
				uint64_t idstart = j+3;
				uint64_t idend = idstart;
				
				while ( idend != P.second && text[idend] != '\t' )
					++idend;

				assert ( idstart != idend );
				
				return std::pair<char const *, uint64_t>(text+idstart,idend-idstart);
			}
			
			std::vector<std::string> getLibrarySet() const
			{
				std::set<std::string> S;
				
				for ( uint64_t i = 0; i < getNumReadGroups(); ++i )
				{
					std::pair<char const *, uint64_t> const P = getLibraryIdentifier(i);
					
					if ( P.first )
						S.insert(
							std::string(P.first,P.first+P.second)
						);
				}
				
				return std::vector<std::string>(S.begin(),S.end());
			}

			/**
			 * get library identifier string for read group with numerical id
			 **/
			std::pair<char const *, uint64_t> getLibraryIdentifier(int64_t const i) const
			{
				if ( i < 0 || i >= static_cast<int64_t>(getNumReadGroups()) )
				{
					char const * null = 0;
					return std::pair<char const *, uint64_t>(null,0);
				}
				
				uint64_t const lineid = rglines.at(i);
				
				std::pair<uint64_t,uint64_t> P = PLA->lineInterval(lineid);
				if ( P.second != P.first && text[P.second-1] == '\r' )
					--P.second;
						
				bool const isRG =
					P.second-P.first >= 3 && 
					text[P.first+0] == '@' &&
					text[P.first+1] == 'R' &&
					text[P.first+2] == 'G';
				
				assert ( isRG );

				uint64_t j = P.first;
				while ( 
					j+2 < P.second &&
					(
						text[j]   != 'L' ||
						text[j+1] != 'B' ||
						text[j+2] != ':'
					)
				)
					++j;
			
				if ( j+2 >= P.second )
				{
					char const * null = 0;
					return std::pair<char const *, uint64_t>(null,0);
				}
				
				uint64_t idstart = j+3;
				uint64_t idend = idstart;
				
				while ( idend != P.second && text[idend] != '\t' )
					++idend;

				if ( idend == idstart )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Invalid empty library identifier in read group " 
						<< getReadGroupIdentifierAsString(i) 
						<< "\n";
					se.finish();
					throw se;			
				}
				
				return std::pair<char const *, uint64_t>(text+idstart,idend-idstart);
			}

			char const * getNoParentPGId() const
			{
				return noparidstring;
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
						return (*RGCSH)[ ReadGroupHashProxy::hash(ID,ID+idlen) ];
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
			 * get library name for library id
			 *
			 * @param libid library id
			 * @return name of library for libid of "Unknown Library" if libid is invalid
			 **/
			std::string getLibraryName(int64_t const libid) const
			{
				if ( 
					(libid < 0)
					||
					(libid >= static_cast<int64_t>(libs.size()))
				)
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
					return libs.size();
				else
					return rgtolib[rgid];
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
					return libs.size();
				else
					return rgtolib[rgid];
			}


			/**
			 * serialise header to BAM
			 *
			 * @param ostr output stream
			 **/
			template<typename stream_type>
			void serialiseSequenceSubset(
				stream_type & ostr, ::libmaus::bitio::IndexedBitVector const & IBV,
				std::string const & pgID,
				std::string const & pgPN,
				std::string const & pgCL,
				std::string const & pgVN
			) const
			{
				// magic
				ostr.put('B');
				ostr.put('A');
				ostr.put('M');
				ostr.put('\1');

				// compute length of header text
				libmaus::util::CountPutObject CPO;
				writeTextSubset(CPO,IBV,pgID,pgPN,pgCL,pgVN);
				
				// write length of text
				::libmaus::bambam::EncoderBase::putLE<stream_type,int32_t>(ostr,CPO.c);
				// write text
				writeTextSubset(ostr,IBV,pgID,pgPN,pgCL,pgVN);
				
				// write binary 
				encodeFilteredChromosomeVector(ostr,IBV);
			}

		};
		
		std::ostream & operator<<(::std::ostream & out, ::libmaus::bambam::BamHeaderLowMem const & BHLM)
		{
			if ( BHLM.HDid != -1 )
				out << BHLM.PLA->getLine(BHLM.text,BHLM.HDid) << "\n";
			for ( uint64_t i = 0; i < BHLM.getNumRef(); ++i )
				out << "@SQ\tSN:" << BHLM.getRefIDName(i) << "\tLN:" << BHLM.getRefIDLength(i) << "\n";
				
			std::vector<uint64_t> pgidlines;
			for ( uint64_t i = 0; i < BHLM.PGidsort.size(); ++i )
				pgidlines.push_back(BHLM.PGidsort[i].lineid);
			std::sort(pgidlines.begin(),pgidlines.end());
			for ( uint64_t i = 0; i < pgidlines.size(); ++i )
				out << BHLM.PLA->getLine(BHLM.text,pgidlines[i]) << "\n";

			std::vector<uint64_t> rgidlines;
			for ( uint64_t i = 0; i < BHLM.RGidsort.size(); ++i )
				rgidlines.push_back(BHLM.RGidsort[i].lineid);
			std::sort(rgidlines.begin(),rgidlines.end());
			for ( uint64_t i = 0; i < rgidlines.size(); ++i )
				out << BHLM.PLA->getLine(BHLM.text,rgidlines[i]) << "\n";
			return out;
		}
		
	}
}
#endif
