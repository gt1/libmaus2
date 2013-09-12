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
		/**
		 * collating bam decoder class, now deprecated; please use the circular hash based collating bam decoder
		 **/
		struct CollatingBamDecoder
		{
			//! this type
			typedef CollatingBamDecoder this_type;
			//! unique pointer
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			//! FastQ entry type
			typedef ::libmaus::fastx::FASTQEntry pattern_type;
			//! alignment type
			typedef ::libmaus::bambam::BamAlignment alignment_type;
			//! alignment pointer type
			typedef alignment_type::shared_ptr_type alignment_ptr_type;
			
			private:
			//! collator state values
			enum decoder_state { reading, merging, done };
		
			//! bam decoder
			BamDecoder bamdecoder;
			//! temporary file name
			std::string const tempfilename;
			//! temporary output stream
			::libmaus::aio::CheckedOutputStream::unique_ptr_type tempfileout;
			//! temporary input stream pointer
			::libmaus::util::unique_ptr<std::ifstream>::type tempfilein;
			//! snappy input array for reading back name sorted blocks
			::libmaus::lz::SnappyInputStreamArrayFile::unique_ptr_type temparrayin;
			//! collator state
			decoder_state state;
			
			//! hash: hash value -> alignment	
			::libmaus::autoarray::AutoArray < std::pair<uint64_t,alignment_ptr_type> > hash;
			//! output list for passing alignment back to the caller
			::std::deque < alignment_ptr_type > outputlist;
			//! write out list for writing alignments out to disk
			::std::deque < alignment_ptr_type > writeoutlist;
			//! block information for alignments written out to disk
			std::vector < std::pair<uint64_t,uint64_t> > writeoutindex;
			//! number of alignments written to each external memory block
			std::vector < uint64_t > writeoutcnt;
			std::vector < uint64_t > readbackindex;
			std::vector < uint64_t > readbackcnt;
			std::priority_queue < MergeQueueElement > mergequeue;
			std::deque < alignment_ptr_type > pairbuffer;
			
			//! default number of hash bits (log_2 of size of hash table)
			static unsigned int const defaulthashbits;
			//! 2^defaulthashbits
			static unsigned int const defaulthashsize;
			//! defaulthashsize-1
			static unsigned int const defaulthashmask;
			//! default maximal size of write out list
			static unsigned int const defaultwriteoutlistmax;
			
			//! log_2 of size of hash table
			unsigned int const hashbits;
			//! 2^hashsize
			unsigned int const hashsize;
			// hashsize-1
			unsigned int const hashmask;
			//! maximal size of write out list
			unsigned int const writeoutlistmax;
			
			//! callback called on alignment input (for observing alignments in file order)
			CollatingBamDecoderAlignmentInputCallback * inputcallback;
			//! histogram of alignments written back to disk for sorting (ref id is key)
			std::map<int64_t,uint64_t> writeOutHist;
			
			/**
			 * @return pointer to temporary file stream
			 **/
			::libmaus::aio::CheckedOutputStream * getTempFile()
			{
				if ( ! tempfileout.get() )
				{
					::libmaus::aio::CheckedOutputStream::unique_ptr_type rtmpfile(new ::libmaus::aio::CheckedOutputStream(tempfilename));
					tempfileout = UNIQUE_PTR_MOVE(rtmpfile);
				}
				return tempfileout.get();
			}
			
			/**
			 * close and flush temporary file
			 *
			 * @return true if temporary file is not empty, false if no data was written to temp file
			 **/
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
			
			/**
			 * initialize hash table and collator state
			 **/
			void init()
			{
				hash = ::libmaus::autoarray::AutoArray < std::pair<uint64_t, alignment_ptr_type> >(hashsize);
				state = reading;
			}
			
			/**
			 * sort write out list and flush entries to disk
			 **/
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
			
			/**
			 * print the write out histogram to out
			 *
			 * @param out output stream
			 * @param prefix each output line is prefixed with this string
			 * @return output stream
			 **/
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

			/**
			 * add alignment to the write out list
			 *
			 * @param oldalgn alignment to be put in write out list
			 **/
			void pushWriteOut(alignment_ptr_type oldalgn)
			{
				// std::cerr << "Pushing out." << std::endl;
			
				writeoutlist.push_back(oldalgn);
				
				if ( writeoutlist.size() == writeoutlistmax )
					flushWriteOutList();
			}
			
			/**
			 * try to fill the output list with at least one alignment, a pair if possible
			 **/
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

						::libmaus::lz::SnappyInputStreamArrayFile::unique_ptr_type ttemparrayin(
							::libmaus::lz::SnappyInputStreamArrayFile::construct(tempfilename,writeoutints.begin(),writeoutints.end())
						);
						temparrayin = UNIQUE_PTR_MOVE(ttemparrayin);
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
			/**
			 * constructor by file name
			 *
			 * @param filename name of input file
			 * @param rtempfilename name of temporary file
			 * @param rputrank put rank (line number in input file) on each alignment
			 * @param rhashbits log_2 of hash table size used for collation
			 * @param rwriteoutlistmax write out list in number of alignments for writing alignment out to disk
			 **/
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
			
			/**
			 * constructor by input stream
			 *
			 * @param in input stream
			 * @param rtempfilename name of temporary file
			 * @param rputrank put rank (line number in input file) on each alignment
			 * @param rhashbits log_2 of hash table size used for collation
			 * @param rwriteoutlistmax write out list in number of alignments for writing alignment out to disk
			 **/
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
			
			/**
			 * get next alignment from output list
			 *
			 * @return next alignment
			 **/
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
			
			/**
			 * try to get a pair; if no more alignments are avaible, then both alignment pointers
			 * in the return pair are null pointers; if only an orphan was available, then one of the 
			 * two pointers returned is a null pointer; if a pair was available, then read 1 is
			 * passed as the first pointer and read 2 as the second
			 *
			 * @return pointer pair
			 **/
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
			
			/**
			 * try to get a pair (see argument free tryPair method)
			 *
			 * @param P reference to pair to be filled
			 * @return true P contains at least one non null pointer on return
			 **/
			bool tryPair(std::pair<alignment_ptr_type,alignment_ptr_type> & P)
			{
				P = tryPair();
				return P.first || P.second;
			}

			/**
			 * get next pair alignment end; this call only returns alignments which are parts
			 * of pairs; single and orphan alignments are dropped implicitely
			 *
			 * @return next pair alignment end
			 **/
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

			/**
			 * set input callback; this function is called whenever an alignment is read from the BAM input file
			 *
			 * @param rinputcallback input call back
			 **/
			void setInputCallback(CollatingBamDecoderAlignmentInputCallback * rinputcallback)
			{
				inputcallback = rinputcallback;
			}

			/**
			 * @return BAM file header
			 **/
			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamdecoder.getHeader();
			}
		};
		
		/**
		 * FastQ type input class from BAM files; it yields pairs only; single and orphan reads are dropped
		 **/
		struct CollatingBamDecoderNoOrphans : public CollatingBamDecoder
		{
			//! this type
			typedef CollatingBamDecoderNoOrphans this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			//! pattern type
			typedef ::libmaus::fastx::FASTQEntry pattern_type;
			
			//! next pattern id
			uint64_t id;
		
			/**
			 * constructor by file name
			 *
			 * @param filename input file name
			 * @param rtempfilename temporary file name
			 **/
			CollatingBamDecoderNoOrphans(std::string const & filename, std::string const & rtempfilename) : CollatingBamDecoder(filename,rtempfilename), id(0) {}
			/**
			 * constructor by input stream
			 *
			 * @param in input stream
			 * @param rtempfilename temporary file name
			 **/
			CollatingBamDecoderNoOrphans(std::istream & in, std::string const & rtempfilename) : CollatingBamDecoder(in,rtempfilename), id(0) {}
			
			/**
			 * fill next FastQ entry
			 *
			 * @param pattern reference to pattern to be filled
			 * @return true if next pattern was available, false if no more patterns were available
			 **/
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
