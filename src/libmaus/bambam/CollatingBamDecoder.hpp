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
#if ! defined(LIBMAUS_BAMBAM_COLLATINGBAMDECODER_HPP)
#define LIBMAUS_BAMBAM_COLLATINGBAMDECODER_HPP
				
#include <libmaus/bambam/MergeQueueElement.hpp>
#include <libmaus/bambam/BamAlignmentComparator.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/lz/SnappyCompress.hpp>
#include <libmaus/bambam/CollatingBamDecoderAlignmentInputCallback.hpp>
#include <queue>

#define LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY

namespace libmaus
{
	namespace bambam
	{	
		struct CollatingBamDecoder
		{
			typedef CollatingBamDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef ::libmaus::fastx::FASTQEntry pattern_type;
			typedef ::libmaus::bambam::BamAlignment alignment_type;
			typedef alignment_type::shared_ptr_type alignment_ptr_type;
			
			private:
			enum decoder_state { reading, merging, done };
		
			BamDecoder bamdecoder;
			std::string const tempfilename;
			::libmaus::aio::CheckedOutputStream::unique_ptr_type tempfileout;
			::libmaus::util::unique_ptr<std::ifstream>::type tempfilein;
			::libmaus::lz::SnappyInputStreamArrayFile::unique_ptr_type temparrayin;
			decoder_state state;
						
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,alignment_ptr_type> > hash;
			::std::deque < alignment_ptr_type > outputlist;
			::std::deque < alignment_ptr_type > writeoutlist;
			std::vector < std::pair<uint64_t,uint64_t> > writeoutindex;
			std::vector < uint64_t > writeoutcnt;
			std::vector < uint64_t > readbackindex;
			std::vector < uint64_t > readbackcnt;
			std::priority_queue < MergeQueueElement > mergequeue;
			std::deque < alignment_ptr_type > pairbuffer;
			
			static unsigned int const defaulthashbits;
			static unsigned int const defaulthashsize;
			static unsigned int const defaulthashmask;
			static unsigned int const defaultwriteoutlistmax;
			
			unsigned int const hashbits;
			unsigned int const hashsize;
			unsigned int const hashmask;
			unsigned int const writeoutlistmax;
			
			CollatingBamDecoderAlignmentInputCallback * inputcallback;
			std::map<int64_t,uint64_t> writeOutHist;
			
			::libmaus::aio::CheckedOutputStream * getTempFile()
			{
				if ( ! tempfileout.get() )
				{
					::libmaus::aio::CheckedOutputStream::unique_ptr_type rtmpfile(new ::libmaus::aio::CheckedOutputStream(tempfilename));
					tempfileout = UNIQUE_PTR_MOVE(rtmpfile);
				}
				return tempfileout.get();
			}
			
			bool closeTempFile()
			{
				if ( tempfileout.get() )
				{
					tempfileout->flush();
					tempfileout->close();
					tempfileout.reset();
					return true;
				}
				else
				{
					return false;
				}
			}
			
			void init()
			{
				hash = ::libmaus::autoarray::AutoArray < std::pair<uint64_t, alignment_ptr_type> >(hashsize);
				state = reading;
			}
			
			void flushWriteOutList()
			{
				std::sort(writeoutlist.begin(),writeoutlist.end(),BamAlignmentComparator());

				uint64_t j = 0;							
				for ( uint64_t i = 0; i < writeoutlist.size(); )
					if ( i+1 >= writeoutlist.size() )
						writeoutlist[j++] = writeoutlist[i++];
					else if ( ! alignment_type::isPair(*writeoutlist[i],*writeoutlist[i+1]) )
						writeoutlist[j++] = writeoutlist[i++];
					else
					{
						assert ( alignment_type::isPair(*writeoutlist[i],*writeoutlist[i+1]) );
						#if 0
						std::cerr << "Found pair "
							<< writeoutlist[i]->getName() << "::"
							<< writeoutlist[i+1]->getName() << std::endl;
						#endif
						outputlist.push_back(writeoutlist[i++]);
						outputlist.push_back(writeoutlist[i++]);
					}
					
				// std::cerr << "Reducing size from " << writeoutlist.size() << " to " << j << std::endl;
				
				writeoutlist.resize(j);
				
				// if there is anything left, then write it out to file/disk
				if ( writeoutlist.size() )
				{
					::libmaus::aio::CheckedOutputStream & tmpfile = *getTempFile();

					uint64_t const prepos = tmpfile.tellp();
					
					#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY)
					::libmaus::lz::SnappyOutputStream< ::libmaus::aio::CheckedOutputStream > SOS(tmpfile);
					for ( uint64_t i = 0; i < writeoutlist.size(); ++i )
						writeoutlist[i]->serialise(SOS);
					SOS.flush();
					#else
					for ( uint64_t i = 0; i < writeoutlist.size(); ++i )
						writeoutlist[i]->serialise(tmpfile);
					#endif
					
					uint64_t const postpos = tmpfile.tellp();
					writeoutindex.push_back(std::pair<uint64_t,uint64_t>(prepos,postpos));
					writeoutcnt.push_back(writeoutlist.size());
					readbackcnt.push_back(0);
					
					#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY) && defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY_DEBUG)
					tmpfile.flush();
					
					::libmaus::lz::SnappyOffsetFileInputStream SOFIS(tempfilename,prepos);
					for ( uint64_t i = 0; i < writeoutlist.size(); ++i )
					{
						::libmaus::bambam::BamAlignment::shared_ptr_type ptr = 
							::libmaus::bambam::BamAlignment::load(SOFIS);
							
						// std::cerr << "Expecting " << writeoutlist[i]->getName() << std::endl;
						// std::cerr << "Got       " << ptr->getName() << std::endl;	
							
						assert ( std::string(ptr->getName()) == std::string(writeoutlist[i]->getName()) );
						assert ( ptr->blocksize == writeoutlist[i]->blocksize );
						assert ( 
							std::string(ptr->D.get(),ptr->D.get()+ptr->blocksize)
							==
							std::string(writeoutlist[i]->D.get(),writeoutlist[i]->D.get()+writeoutlist[i]->blocksize)
						);
					}

					std::cerr << "Snappy block " << readbackcnt.size() << " written, size of tmp file is now " << postpos << std::endl;
					#endif
					
					// std::cerr << "[" << readbackcnt.size()-1 << "]: " << "index [" << prepos << "," << postpos << ")" << std::endl;
					for ( uint64_t z = 0; z < writeoutlist.size(); ++z )
					{
						int64_t const chrid = writeoutlist[z]->getRefID();
						writeOutHist[chrid]++;
					}

					writeoutlist.resize(0);
				}			
			}
			
			std::ostream & printWriteOutHist(std::ostream & out, std::string const & prefix) const
			{
				if ( writeOutHist.size() )
				{
					out << prefix << " " << "Overflow histogram:" << std::endl;
					
					for ( std::map<int64_t,uint64_t>::const_iterator ita = writeOutHist.begin();
						ita != writeOutHist.end(); ++ita )
					{
						std::string const name = bamdecoder.getHeader().getRefIDName(ita->first);
						uint64_t const cnt = ita->second;
						
						out << prefix << "\t" << name << "\t" << cnt << std::endl;
					}
				}
				
				return out;
			}

			void pushWriteOut(alignment_ptr_type oldalgn)
			{
				// std::cerr << "Pushing out." << std::endl;
			
				writeoutlist.push_back(oldalgn);
				
				if ( writeoutlist.size() == writeoutlistmax )
					flushWriteOutList();
			}
			
			void fillOutputList()
			{
				assert ( outputlist.size() == 0 );
				
				while ( (state == reading) && (! outputlist.size()) )
				{
					// read alignment
					bool rok = bamdecoder.readAlignment(true /* delay putting rank */);
					if ( ! rok )
					{
						// flush hash table
						for ( uint64_t i = 0; i < hash.size(); ++i )
							if ( hash[i].second )
								pushWriteOut(hash[i].second);
					
						// release memory for hash			
						hash.release();

						// write remaining entries to disk
						flushWriteOutList();
								
						if ( closeTempFile() )
						{
							// std::cerr << "switching to merging." << std::endl;
							state = merging;
						}
						else
						{
							// std::cerr << "switching to done." << std::endl;
							state = done;
						}
					}
					else
					{	
						if ( inputcallback )
							(*inputcallback)(bamdecoder.getAlignment());
							
						// put rank
						bamdecoder.putRank();
							
						// get copy of the alignment as shared ptr
						alignment_ptr_type algn = bamdecoder.salignment();

						if ( algn->isSecondary() )
							continue;							
						
						uint64_t const hv = algn->hash();
						
						if ( ! hash[hv & hashmask].second.get() )
						{
							hash[hv & hashmask] = std::pair<uint64_t,alignment_ptr_type>(hv,algn);
						}
						else if ( 
							(hash[hv & hashmask].first != hv) ||
							(! alignment_type::isPair(*(hash[hv & hashmask].second),*algn))
						)
						{
							alignment_ptr_type oldalgn = hash[hv&hashmask].second;
							assert ( oldalgn.get() );
					
							pushWriteOut(oldalgn);		
							
							// replace old alignment by new one
							hash[hv & hashmask] = std::pair<uint64_t,alignment_ptr_type>(hv,algn);						
						}
						else
						{
							assert ( hash[hv & hashmask].second.get() );
							assert ( hash[hv & hashmask].first == hv );
							assert ( alignment_type::isPair(*(hash[hv & hashmask].second),*algn) );
							
							alignment_ptr_type oldalgn = hash[hv&hashmask].second;
							assert ( oldalgn.get() );
							
							hash[hv&hashmask].second.reset();
							
							if ( oldalgn->isRead1() )
							{
								outputlist.push_back(oldalgn);
								outputlist.push_back(algn);
							}
							else
							{
								outputlist.push_back(algn);
								outputlist.push_back(oldalgn);							
							}
						}
					}
				}
				
				while ( (state == merging) && (! outputlist.size()) )
				{
					if ( 
						#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY)
						(! temparrayin)
						#else
						(! tempfilein) 
						#endif
						&& 
						writeoutindex.size() 
					)
					{
						// std::cerr << "Setting up merging." << std::endl;
					
						#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY)
						// construct interval vector
						std::vector<uint64_t> writeoutints;
						for ( uint64_t i = 0; i < writeoutindex.size(); ++i )
							writeoutints.push_back(writeoutindex[i].first);
						writeoutints.push_back(writeoutindex.back().second);

						temparrayin = UNIQUE_PTR_MOVE(::libmaus::lz::SnappyInputStreamArrayFile::construct(tempfilename,writeoutints.begin(),writeoutints.end()));
						#else
						::libmaus::util::unique_ptr<std::ifstream>::type rtmpfile(new std::ifstream(tempfilename.c_str(),std::ios::binary));
						tempfilein = UNIQUE_PTR_MOVE(rtmpfile);
						#endif
						
						for ( uint64_t i = 0; i < writeoutindex.size(); ++i )
							readbackindex.push_back(writeoutindex[i].first);

						for ( uint64_t i = 0; i < readbackindex.size(); ++i )
							if ( readbackcnt[i] != writeoutcnt[i] )
							{
								#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY)
								alignment_ptr_type ptr = alignment_type::load((*temparrayin)[i]);
								mergequeue.push(MergeQueueElement(ptr,i));
								readbackcnt[i]++;
								#else
								tempfilein->clear();
								tempfilein->seekg(readbackindex[i],std::ios::beg);
								
								alignment_ptr_type ptr = alignment_type::load(*tempfilein);
								mergequeue.push(MergeQueueElement(ptr,i));
								
								readbackindex[i] = tempfilein->tellg();
								readbackcnt[i]++;
								#endif
							}
					}
				
					if ( mergequeue.size() )
					{
						MergeQueueElement MQE = mergequeue.top();
						mergequeue.pop();
						
						outputlist.push_back(MQE.algn);
						
						uint64_t const i = MQE.index;
						
						if ( readbackcnt[i] != writeoutcnt[i] )
						{
							#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY)
							alignment_ptr_type ptr = alignment_type::load((*temparrayin)[i]);
							mergequeue.push(MergeQueueElement(ptr,i));
							readbackcnt[i]++;							
							#else
							tempfilein->clear();
							tempfilein->seekg(readbackindex[i],std::ios::beg);
							
							alignment_ptr_type ptr = alignment_type::load(*tempfilein);
							mergequeue.push(MergeQueueElement(ptr,i));
							
							readbackindex[i] = tempfilein->tellg();
							readbackcnt[i]++;
							#endif
						}
					}
					else
					{
						#if defined(LIBMAUS_BAMBAM_COLLATION_USE_SNAPPY)
						if ( temparrayin )
						{
							temparrayin.reset();
							remove ( tempfilename.c_str() );
						}
						#else
						if ( tempfilein )
						{
							tempfilein.reset();
							remove ( tempfilename.c_str() );
						}
						#endif
						state = done;
					}
				}
			}

			public:	
			CollatingBamDecoder(
				std::string const & filename, 
				std::string const & rtempfilename,
				bool const rputrank = false,
				unsigned int const rhashbits = defaulthashbits,
				unsigned int const rwriteoutlistmax = defaultwriteoutlistmax
			)
			: bamdecoder(filename,rputrank), tempfilename(rtempfilename),
			  hashbits(rhashbits), hashsize(1u << hashbits), hashmask(hashsize-1), 
			  writeoutlistmax(rwriteoutlistmax), inputcallback(0)
			{ init(); }
			CollatingBamDecoder(
				std::istream & in, 
				std::string const & rtempfilename,
				bool const rputrank = false,
				unsigned int const rhashbits = defaulthashbits,
				unsigned int const rwriteoutlistmax = defaultwriteoutlistmax
			)
			: bamdecoder(in,rputrank), tempfilename(rtempfilename),
			  hashbits(rhashbits), hashsize(1u << hashbits), hashmask(hashsize-1), 
			  writeoutlistmax(rwriteoutlistmax), inputcallback(0)
			{ init(); }
			
			alignment_ptr_type get()
			{
				if ( ! outputlist.size() )
					fillOutputList();
			
				if ( outputlist.size() )
				{
					alignment_ptr_type algn = outputlist.front();
					outputlist.pop_front();
					return algn;
				}
				else
				{
					return alignment_ptr_type();
				}
			}
			
			std::pair<alignment_ptr_type,alignment_ptr_type> tryPair()
			{
				alignment_ptr_type algn_a, algn_b;
				algn_a = get();
				algn_b = get();
				
				// no more reads
				if ( ! algn_a )
				{
					return std::pair<alignment_ptr_type,alignment_ptr_type>();
				}
				// only one read left
				else if ( ! algn_b )
				{
					if ( algn_a->isRead1() )
						return std::pair<alignment_ptr_type,alignment_ptr_type>(algn_a,algn_b);
					else
						return std::pair<alignment_ptr_type,alignment_ptr_type>(algn_b,algn_a);
				}
				else
				{
					assert ( algn_a );
					assert ( algn_b );

					// we have a pair
					if ( alignment_type::isPair(*algn_a,*algn_b) )
					{
						if ( algn_a->isRead1() )
							return std::pair<alignment_ptr_type,alignment_ptr_type>(algn_a,algn_b);
						else
							return std::pair<alignment_ptr_type,alignment_ptr_type>(algn_b,algn_a);
					}
					else
					{
						// put back second read, it does not match the first one
						outputlist.push_front(algn_b);
						// reset second read
						algn_b.reset();

						if ( algn_a->isRead1() )
							return std::pair<alignment_ptr_type,alignment_ptr_type>(algn_a,algn_b);
						else
							return std::pair<alignment_ptr_type,alignment_ptr_type>(algn_b,algn_a);
					}
				}
			}
			
			bool tryPair(std::pair<alignment_ptr_type,alignment_ptr_type> & P)
			{
				P = tryPair();
				return P.first || P.second;
			}

			alignment_ptr_type getPair()
			{
				while ( ! pairbuffer.size() )
				{
					alignment_ptr_type algn_a, algn_b;
					algn_a = get();
					algn_b = get();

					// no more pairs?
					if ( ! algn_b )
						break;

					if ( alignment_type::isPair(*algn_a,*algn_b) )
					{
						pairbuffer.push_back(algn_a);
						pairbuffer.push_back(algn_b);
					}
					else
					{
						// put back second read, it does not match the first one
						outputlist.push_front(algn_b);
					}
				}
				
				if ( pairbuffer.size() )
				{
					alignment_ptr_type algn = pairbuffer.front();
					pairbuffer.pop_front();
					return algn;
				}
				else
				{
					return alignment_ptr_type();
				}
			}

			void setInputCallback(CollatingBamDecoderAlignmentInputCallback * rinputcallback)
			{
				inputcallback = rinputcallback;
			}

			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamdecoder.getHeader();
			}
		};
		
		struct CollatingBamDecoderNoOrphans : public CollatingBamDecoder
		{
			typedef CollatingBamDecoderNoOrphans this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef ::libmaus::fastx::FASTQEntry pattern_type;
			
			uint64_t id;
		
			CollatingBamDecoderNoOrphans(
				std::string const & filename, 
				std::string const & rtempfilename
			)
			: CollatingBamDecoder(filename,rtempfilename), id(0) {}
			CollatingBamDecoderNoOrphans(
				std::istream & in, 
				std::string const & rtempfilename
			)
			: CollatingBamDecoder(in,rtempfilename), id(0) {}
			
			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				CollatingBamDecoder::alignment_ptr_type algn = CollatingBamDecoder::getPair();
				
				if ( algn )
				{
					algn->toPattern(pattern,id++);
					return true;	
				}
				else
				{
					return false;
				}
			}
		};
	}	
}
#endif
