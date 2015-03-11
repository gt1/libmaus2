/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGEDISPATCHER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_GENERICINPUTCONTROLREADWORKPACKAGEDISPATCHER_HPP

#include <libmaus/bambam/parallel/GenericInputControlReadWorkPackageReturnInterface.hpp>
#include <libmaus/bambam/parallel/GenericInputControlReadAddPendingInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockAllocator.hpp>
#include <libmaus/bambam/parallel/DecompressedBlockTypeInfo.hpp>
#include <libmaus/fastx/CharBuffer.hpp>
#include <libmaus/bambam/BamAlignmentEncoderBase.hpp>
#include <libmaus/bambam/BamAlignment.hpp>

enum fastq_name_scheme_type { 
	fastq_name_scheme_generic,
	fastq_name_scheme_casava18_single,
	fastq_name_scheme_casava18_paired_end,
	fastq_name_scheme_pairedfiles
};

std::ostream & operator<<(std::ostream & out, fastq_name_scheme_type const namescheme)
{
	switch ( namescheme )
	{
		case fastq_name_scheme_generic:
			out << "generic";
			break;
		case fastq_name_scheme_casava18_single:
			out << "c18s";
			break;
		case fastq_name_scheme_casava18_paired_end:
			out << "c18pe";
			break;
		case fastq_name_scheme_pairedfiles:
			out << "pairedfiles";
			break;
		default:
			out << "unknown";
			break;
	}
	return out;
}

static fastq_name_scheme_type parseNameScheme(std::string const & schemename)
{
	if ( schemename == "generic" )
		return fastq_name_scheme_generic;
	else if ( schemename == "c18s" )
		return fastq_name_scheme_casava18_single;
	else if ( schemename == "c18pe" )
		return fastq_name_scheme_casava18_paired_end;
	else if ( schemename == "pairedfiles" )
		return fastq_name_scheme_pairedfiles;
	else
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "unknown read name scheme " << schemename << std::endl;
		lme.finish();
		throw lme;	
	}
}

#include <libmaus/fastx/SpaceTable.hpp>

struct NameInfo
{
	uint8_t const * name;
	size_t namelength;
	bool ispair;
	bool isfirst;
	uint64_t gl;
	uint64_t gr;
	fastq_name_scheme_type const namescheme;
		
	NameInfo() : name(0), namelength(0), ispair(false), isfirst(false), gl(0), gr(0), namescheme(fastq_name_scheme_generic) {}
	NameInfo(uint8_t const * rname, size_t rnamelength,
		libmaus::fastx::SpaceTable const & ST,
		fastq_name_scheme_type const rnamescheme
	)
	: name(rname), namelength(rnamelength), ispair(false), isfirst(true), gl(0), gr(namelength), namescheme(rnamescheme)
	{
		switch ( namescheme )
		{
			case fastq_name_scheme_generic:
			{
				uint64_t l = 0;
				// skip space at start
				while ( l != namelength && ST.spacetable[name[l]] )
					++l;
				if ( l != namelength )
				{
					// skip non space symbols
					uint64_t r = l;
					while ( (r != namelength) && (!ST.spacetable[name[r]]) )
						++r;
					
					// check whether this part of the name ends on /1 or /2
					if ( r-l >= 2 && name[r-2] == '/' && (name[r-1] == '1' || name[r-1] == '2') )
					{
						if ( name[r-1] == '2' )
							isfirst = false;
							
						gl = l;
						gr = r;
						ispair = true;
					}
					else
					{
						gl = l;
						gr = r;
						ispair = false;
					}
					
					l = r;
				}	
				
				break;
			}
			case fastq_name_scheme_casava18_single:
			case fastq_name_scheme_casava18_paired_end:
			{
				uint64_t l = 0;
				
				while ( l != namelength && ST.spacetable[name[l]] )
					++l;
					
				uint64_t const l0 = l;
				
				while ( l != namelength && ! ST.spacetable[name[l]] )
					++l;
				
				uint64_t const l0l = l-l0;	

				while ( l != namelength && ST.spacetable[name[l]] )
					++l;
					
				uint64_t const l1 = l;

				while ( l != namelength && ! ST.spacetable[name[l]] )
					++l;
					
				uint64_t const l1l = l-l1;
			
				// count number of colons	
				uint64_t l0c = 0;
				for ( uint64_t i = l0; i != l0+l0l; ++i )
					l0c += (name[i] == ':');
				uint64_t l1c = 0;
				for ( uint64_t i = l1; i != l1+l1l; ++i )
					l1c += (name[i] == ':');
			
				if ( l0c != 6 || l1c != 3 )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "malformed read name " << name << " (wrong number of colon separated fields) for name scheme " << namescheme << std::endl;
					se.finish();
					throw se;
				}
				
				gl = l0;
				gr = l0+l0l;
				
				if ( namescheme == fastq_name_scheme_casava18_single )
				{
					ispair = false;
				}
				else
				{
					ispair = true;
					
					uint64_t fragid = 0;
					unsigned int fragidlen = 0;
					uint64_t p = l1;
					while ( isdigit(name[p]) )
					{
						fragid *= 10;
						fragid += name[p++]-'0';
						fragidlen++;
					}
					
					if ( (! fragidlen) || (fragid<1) || (fragid>2) || name[p] != ':' )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "malformed read name " << name << " (malformed fragment id) for name scheme" << namescheme << std::endl;
						se.finish();
						throw se;	
					}
					
					isfirst = (fragid==1);
				}
				
				break;
			}
			case fastq_name_scheme_pairedfiles:
			{
				::libmaus::exception::LibMausException se;
				se.getStream() << "pairedfiles name scheme is not supported in NameInfo" << std::endl;
				se.finish();
				throw se;	
				break;
			}
		}
	}
	
	std::pair<uint8_t const *, uint8_t const *> getName() const
	{
		switch ( namescheme )
		{
			case fastq_name_scheme_generic:
				if ( ispair )
					return std::pair<uint8_t const *, uint8_t const *>(name+gl,name+gr-2);
				else
					return std::pair<uint8_t const *, uint8_t const *>(name+gl,name+gr);
				break;
			case fastq_name_scheme_casava18_single:
			case fastq_name_scheme_casava18_paired_end:
				return std::pair<uint8_t const *, uint8_t const *>(name+gl,name+gr);
				break;
			default:
			{
				::libmaus::exception::LibMausException se;
				se.getStream() << "NameInfo::getName(): invalid name scheme" << std::endl;
				se.finish();
				throw se;			
			}
		}
	}
};


namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct FastQInputDescBase
			{
				// meta data type for block reading
				typedef libmaus::bambam::parallel::GenericInputBlockSubBlockInfo meta_type;
				// block type
				typedef libmaus::bambam::parallel::GenericInputBlock<meta_type> input_block_type;
				typedef libmaus::parallel::LockedFreeList<
					input_block_type,
					libmaus::bambam::parallel::GenericInputBlockAllocator<meta_type>,
					libmaus::bambam::parallel::GenericInputBlockTypeInfo<meta_type>
				> free_list_type;
			};

			struct FastqToBamControlInputBlockHeapComparator
			{
				bool operator()(FastQInputDescBase::input_block_type::shared_ptr_type A, FastQInputDescBase::input_block_type::shared_ptr_type B) const
				{
					if ( A->meta.streamid != B->meta.streamid )
						return A->meta.streamid > B->meta.streamid;
					else // if ( A->blockid != B->blockid )
						return A->meta.blockid > B->meta.blockid;
				}
			};

			struct FastqToBamControlSubReadPending
			{
				typedef FastqToBamControlSubReadPending this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				FastQInputDescBase::input_block_type::shared_ptr_type block;
				uint64_t volatile subid;
				uint64_t volatile absid;
				DecompressedBlock::shared_ptr_type deblock;
					
				FastqToBamControlSubReadPending() : block(), subid(0), absid(0), deblock() {}
				FastqToBamControlSubReadPending(
					FastQInputDescBase::input_block_type::shared_ptr_type rblock,
					uint64_t rsubid
				) : block(rblock), subid(rsubid), absid(0), deblock() {}
			};
			
			struct FastqToBamControlSubReadPendingHeapComparator
			{
				bool operator()(FastqToBamControlSubReadPending const & A, FastqToBamControlSubReadPending const & B) const
				{
					if ( A.block->meta.streamid != B.block->meta.streamid )
						return A.block->meta.streamid > B.block->meta.streamid;
					else if ( A.block->meta.blockid != B.block->meta.blockid )
						return A.block->meta.blockid > B.block->meta.blockid;
					else
						return A.subid > B.subid;
				}
			};
			
			struct DecompressedBlockReorderHeapComparator
			{
				bool operator()(DecompressedBlock::shared_ptr_type const & A, DecompressedBlock::shared_ptr_type const & B) const
				{
					return A->blockid > B->blockid;
				}
			};

			struct FastQInputDesc : public FastQInputDescBase
			{
				std::istream & in;
				libmaus::parallel::PosixSpinLock inlock;
				free_list_type blockFreeList;
				uint64_t streamid;
				libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> stallArray;
				uint64_t volatile stallArraySize;
				uint64_t volatile blockid;
				bool volatile eof;
				uint64_t volatile blocksproduced;
				libmaus::parallel::PosixSpinLock blocksproducedlock;
				uint64_t volatile blockspassed;
				libmaus::parallel::PosixSpinLock blockspassedlock;

				std::priority_queue<
					FastQInputDescBase::input_block_type::shared_ptr_type,
					std::vector<FastQInputDescBase::input_block_type::shared_ptr_type>,
					FastqToBamControlInputBlockHeapComparator
				> readpendingqueue;
				libmaus::parallel::PosixSpinLock readpendingqueuelock;
				uint64_t volatile readpendingqueuenext;

				std::priority_queue<
					FastqToBamControlSubReadPending,
					std::vector<FastqToBamControlSubReadPending>,
					FastqToBamControlSubReadPendingHeapComparator
				> readpendingsubqueue;
				libmaus::parallel::PosixSpinLock readpendingsubqueuelock;
				uint64_t volatile readpendingsubqueuenextid;
				
				libmaus::parallel::LockedFreeList<
					libmaus::bambam::parallel::DecompressedBlock,
					libmaus::bambam::parallel::DecompressedBlockAllocator,
					libmaus::bambam::parallel::DecompressedBlockTypeInfo
				> decompfreelist;
				
				std::priority_queue<
					libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type,
					std::vector<libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type>,
					DecompressedBlockReorderHeapComparator
				> parsereorderqueue;
				libmaus::parallel::PosixSpinLock parsereorderqueuelock;
				uint64_t volatile parsereorderqueuenext;
				
				FastQInputDesc(std::istream & rin, uint64_t const numblocks, uint64_t const blocksize, uint64_t const rstreamid)
				: in(rin), blockFreeList(numblocks,libmaus::bambam::parallel::GenericInputBlockAllocator<meta_type>(blocksize)),
				  streamid(rstreamid), stallArray(), stallArraySize(0), blockid(0), eof(false), blocksproduced(0), blockspassed(0),
				  readpendingqueue(), readpendingqueuelock(), readpendingqueuenext(0),
				  readpendingsubqueuenextid(0),
				  decompfreelist(32),
				  parsereorderqueue(),
				  parsereorderqueuelock(),
				  parsereorderqueuenext(0)
				{}
				
				uint64_t getStreamId() const
				{
					return streamid;
				}

				uint64_t getBlockId() const
				{
					return blockid;
				}
				
				void incrementBlockId()
				{
					blockid += 1;
				}
				
				bool getEOF() const
				{
					return eof;
				}
				
				void setEOF(bool const reof)
				{
					eof = reof;
				}
				
				uint64_t getBlocksProduced()
				{
					libmaus::parallel::ScopePosixSpinLock slock(blocksproducedlock);
					return blocksproduced;
				}
				
				void incrementBlocksProduced()
				{
					libmaus::parallel::ScopePosixSpinLock slock(blocksproducedlock);
					blocksproduced += 1;					
				}
				
				uint64_t getBlocksPassed()
				{					
					libmaus::parallel::ScopePosixSpinLock slock(blockspassedlock);
					return blockspassed;
				}

				void incrementBlocksPassed()
				{
					libmaus::parallel::ScopePosixSpinLock slock(blockspassedlock);
					blockspassed += 1;					
				}
			};
			
			struct FastqInputPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef FastqInputPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				FastQInputDesc * data;
			
				FastqInputPackage() : libmaus::parallel::SimpleThreadWorkPackage(), data(0)
				{				
				}		
				FastqInputPackage(uint64_t const rpriority, uint64_t const rdispatcherid, FastQInputDesc * rdata)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata)
				{
				
				}

				char const * getPackageName() const
				{
					return "FastqInputPackage";
				}
			};

			struct FastqInputPackageReturnInterface
			{
				virtual ~FastqInputPackageReturnInterface() {}
				virtual void fastqInputPackageReturn(FastqInputPackage * package) = 0;
			};
			
			struct FastqInputPackageAddPendingInterface
			{
				virtual ~FastqInputPackageAddPendingInterface() {}
				virtual void fastqInputPackageAddPending(FastQInputDescBase::input_block_type::shared_ptr_type) = 0;
			};
			
			struct FastqInputPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FastqInputPackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				FastqInputPackageReturnInterface & packageReturnInterface;
				FastqInputPackageAddPendingInterface & addPendingInterface;
							
				FastqInputPackageDispatcher(
					FastqInputPackageReturnInterface & rpackageReturnInterface,
					FastqInputPackageAddPendingInterface & raddPendingInterface
				)
				: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface)
				{
				}
			
				void dispatch(
					libmaus::parallel::SimpleThreadWorkPackage * P, 
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					assert ( dynamic_cast<FastqInputPackage *>(P) != 0 );
					FastqInputPackage * BP = dynamic_cast<FastqInputPackage *>(P);

					FastQInputDesc & data = *(BP->data);

					typedef FastQInputDescBase::input_block_type input_block_type;
					typedef FastQInputDescBase::free_list_type free_list_type;
					
					libmaus::parallel::PosixSpinLock & inlock = data.inlock;
					free_list_type & blockFreeList = data.blockFreeList;
					uint64_t const streamid = data.getStreamId();
					libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> & stallArray = data.stallArray;
					uint64_t volatile & stallArraySize = data.stallArraySize;
					std::istream & in = data.in;

					std::vector<input_block_type::shared_ptr_type> fullBlocks;
					
					if ( inlock.trylock() )
					{
						libmaus::parallel::ScopePosixSpinLock slock(inlock,true /* pre locked */);
						
						input_block_type::shared_ptr_type sblock;
			
						while ( 
							(!data.getEOF()) && (sblock=blockFreeList.getIf()) 
						)
						{
							// reset buffer
							sblock->reset();
							// set stream id
							sblock->meta.streamid = streamid;
							// set block id
							sblock->meta.blockid = data.getBlockId();
							// insert stalled data
							sblock->insert(stallArray.begin(),stallArray.begin()+stallArraySize);
			
							// extend if there is no space
							if ( sblock->pe == sblock->A.end() )
								sblock->extend();
							
							// there should be free space now
							assert ( sblock->pe != sblock->A.end() );
			
							// fill buffer
							libmaus::bambam::parallel::GenericInputBlockFillResult P = sblock->fill(
								in, false /* finite */,0 /* dataleft */);
			
							if ( in.bad() )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "Stream error while filling buffer." << std::endl;
								lme.finish();
								throw lme;
							}
			
							data.setEOF(P.eof);
							sblock->meta.eof = P.eof;
							
							// parse bgzf block headers to determine how many full blocks we have				
							libmaus::bambam::parallel::GenericInputBlockSubBlockInfo & meta = sblock->meta;
							uint64_t f = 0;

							bool foundnewline = false;
							uint8_t * const pc = sblock->pc;
							uint8_t * pe = sblock->pe;
							while ( pe != pc )
							{
								uint8_t const c = *(--pe);
								
								if ( c == '\n' )
								{
									foundnewline = true;
									break;
								}
							}
							
							if ( foundnewline )
							{
								pe += 1;
								
								uint8_t * ls[4] = {0,0,0,0};
								
								while ( pe != pc )
								{
									assert ( pe[-1] == '\n' );
									
									pe -= 1;
									while ( pe != pc )
									{
										uint8_t const c = *(--pe);
										
										if ( c == '\n' )
										{
											pe += 1;
											break;
										}
									}
									
									ls[3] = ls[2];
									ls[2] = ls[1];
									ls[1] = ls[0];
									ls[0] = pe;
									
									if ( ls[3] && ls[0][0] == '@' && ls[2][0] == '+' )
										break;
								}							

								if ( ls[3] && ls[0][0] == '@' && ls[2][0] == '+' )
								{
									uint8_t * le = ls[3];
									while ( *le != '\n' )
										++le;
									assert ( *le == '\n' );
									le += 1;

									uint64_t const bs = le - pc;
									meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pc+bs));
									f += 1;
									
									sblock->pc += bs;
									
									if ( sblock->pc == sblock->pe && data.getEOF() )
										sblock->meta.eof = true;
								}
							}

							// empty file? inject empty block so we see eof downstream
							if ( data.getEOF() && (sblock->pc == sblock->pe) && (!f) )
							{
								meta.addBlock(std::pair<uint8_t *,uint8_t *>(sblock->pc,sblock->pe));
								f += 1;
							}
											
							// extract rest of data for next block
							stallArraySize = sblock->extractRest(stallArray);
			
							if ( f )
							{
								if ( fullBlocks.size() >= 1 )
								{
									for ( uint64_t i = 0; i < fullBlocks.size(); ++i )
										addPendingInterface.fastqInputPackageAddPending(fullBlocks[i]);
									fullBlocks.resize(0);
								}
								fullBlocks.push_back(sblock);
								data.incrementBlocksProduced();
								data.incrementBlockId();
							}
							else
							{
								// buffer is too small for a single block
								if ( ! data.getEOF() )
								{
									sblock->extend();
									blockFreeList.put(sblock);
								}
								else if ( sblock->pc != sblock->pe )
								{
									assert ( data.getEOF() );
									// throw exception, block is incomplete at EOF
									libmaus::exception::LibMausException lme;
									lme.getStream() << "Unexpected EOF." << std::endl;
									lme.finish();
									throw lme;
								}
							}
						}			
					}
			
					packageReturnInterface.fastqInputPackageReturn(BP);
			
					for ( uint64_t i = 0; i < fullBlocks.size(); ++i )
						addPendingInterface.fastqInputPackageAddPending(fullBlocks[i]);
				}
			};

			struct FastqParsePackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef FastqParsePackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				FastqToBamControlSubReadPending data;
			
				FastqParsePackage() : libmaus::parallel::SimpleThreadWorkPackage(), data()
				{				
				}		
				FastqParsePackage(uint64_t const rpriority, uint64_t const rdispatcherid, FastqToBamControlSubReadPending & rdata)
				: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid), data(rdata)
				{
				
				}

				char const * getPackageName() const
				{
					return "FastqParsePackage";
				}
			};

			struct FastqParsePackageReturnInterface
			{
				virtual ~FastqParsePackageReturnInterface() {}
				virtual void fastqParsePackageReturn(FastqParsePackage * package) = 0;
			};
			
			struct FastqParsePackageFinishedInterface
			{
				virtual ~FastqParsePackageFinishedInterface() {}
				virtual void fastqParsePackageFinished(FastqToBamControlSubReadPending data) = 0;
			};

			struct FastqParsePackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
			{
				typedef FastqParsePackageDispatcher this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				FastqParsePackageReturnInterface & packageReturnInterface;
				FastqParsePackageFinishedInterface & addPendingInterface;
				libmaus::bambam::BamSeqEncodeTable const seqenc;
				libmaus::fastx::SpaceTable const ST;
				std::string const rgid;
				libmaus::autoarray::AutoArray<uint8_t> rgA;
							
				FastqParsePackageDispatcher(
					FastqParsePackageReturnInterface & rpackageReturnInterface,
					FastqParsePackageFinishedInterface & raddPendingInterface,
					std::string const rrgid
				)
				: packageReturnInterface(rpackageReturnInterface), addPendingInterface(raddPendingInterface), seqenc(), ST(), rgid(rrgid),
				  rgA(rgid.size() ? (rgid.size() + 4) : 0, false)
				{
					if ( rgid.size() )
					{
						rgA[0] = 'R';
						rgA[1] = 'G';
						rgA[2] = 'Z';
						std::copy(rgid.begin(),rgid.end(),rgA.begin()+3);
						rgA[3 + rgid.size()] = 0;						
					}
				}
			
				void dispatch(
					libmaus::parallel::SimpleThreadWorkPackage * P, 
					libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */
				)
				{
					assert ( dynamic_cast<FastqParsePackage *>(P) != 0 );
					FastqParsePackage * BP = dynamic_cast<FastqParsePackage *>(P);
					
					FastqToBamControlSubReadPending & data = BP->data;
					FastQInputDescBase::input_block_type::shared_ptr_type & block = data.block;
					DecompressedBlock::shared_ptr_type & deblock = data.deblock;

					deblock->blockid = data.absid;
					deblock->final = block->meta.eof && ( data.subid+1 == block->meta.blocks.size() );
					deblock->P = deblock->D.begin();
					deblock->uncompdatasize = 0;

					libmaus::fastx::EntityBuffer<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> buffer;
					libmaus::bambam::BamAlignment algn;
					
					std::pair<uint8_t *, uint8_t *> Q = block->meta.blocks[data.subid];
					while ( Q.first != Q.second )
					{
						std::pair<uint8_t *,uint8_t *> ls[4];
						
						for ( unsigned int i = 0; i < 4; ++i )
						{
							uint8_t * s = Q.first;
							while ( Q.first != Q.second && Q.first[0] != '\n' )
								++Q.first;
							if ( Q.first == Q.second )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "Unexpected EOF." << std::endl;
								lme.finish();
								throw lme;
							}
							assert ( Q.first[0] == '\n' );
							ls[i] = std::pair<uint8_t *,uint8_t *>(s,Q.first);
							Q.first += 1;
						}
						
						uint8_t const * name = ls[0].first+1;
						uint32_t const namelen = ls[0].second-ls[0].first-1;
						int32_t const refid = -1;
						int32_t const pos = -1;
						uint32_t const mapq = 0;
						libmaus::bambam::cigar_operation const * cigar = 0;
						uint32_t const cigarlen = 0;
						int32_t const nextrefid = -1;
						int32_t const nextpos = -1;
						uint32_t const tlen = 0;
						uint8_t const * seq = ls[1].first;
						uint32_t const seqlen = ls[1].second-ls[1].first;
						uint8_t const * qual = ls[3].first;
						int const qualshift = 33;

						NameInfo const NI(
							name,namelen,ST,fastq_name_scheme_generic
						);
						
						uint32_t flags = libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FUNMAP;
						if ( NI.ispair )
						{
							flags |= libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FPAIRED;
							flags |= libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FMUNMAP;
							if ( NI.isfirst )
								flags |= libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
							else
								flags |= libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2;
						}
						
						std::pair<uint8_t const *,uint8_t const *> NP = NI.getName();

						libmaus::bambam::BamAlignmentEncoderBase::encodeAlignment
						(
							buffer,seqenc,NP.first,NP.second-NP.first,refid,pos,mapq,flags,cigar,cigarlen,
							nextrefid,nextpos,tlen,seq,seqlen,qual,qualshift
						);
						
						if ( rgA.size() )
							for ( uint64_t i = 0; i < rgA.size(); ++i )
								buffer.bufferPush(rgA[i]);
						
						algn.blocksize = buffer.swapBuffer(algn.D);
						
						deblock->pushData      (algn.D.begin(), algn.blocksize);
					}
					
					addPendingInterface.fastqParsePackageFinished(BP->data);
					packageReturnInterface.fastqParsePackageReturn(BP);
				}
			};

		}
	}
}
#endif

#include <libmaus/parallel/NumCpus.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/bambam/parallel/GenericInputControlCompressionPending.hpp>
#include <libmaus/bambam/parallel/GenericInputControlCompressionPendingHeapComparator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBaseTypeInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlockCompressionWorkPackageDispatcher.hpp>
#include <libmaus/parallel/LockedGrowingFreeList.hpp>
#include <libmaus//bambam/parallel/GenericInputControlBlockWritePackageDispatcher.hpp>

#include <libmaus/lz/BgzfDeflateZStreamBaseAllocator.hpp>
#include <libmaus/lz/BgzfDeflateZStreamBaseTypeInfo.hpp>
#include <libmaus/bambam/parallel/GenericInputControlBlocksWritePackageDispatcher.hpp>

// GenericInputControlBlockCompressionWorkPackageReturnInterface & packageReturnInterface;
// GenericInputControlBlockCompressionFinishedInterface & blockCompressedInterface;
// GenericInputControlGetCompressorInterface & getCompressorInterface;
// GenericInputControlPutCompressorInterface & putCompressorInterface;

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{

			struct FastqToBamControl :
				public FastqInputPackageReturnInterface,
				public FastqInputPackageAddPendingInterface,
				public FastqParsePackageReturnInterface,
				public FastqParsePackageFinishedInterface,
				public GenericInputControlGetCompressorInterface,
				public GenericInputControlPutCompressorInterface,
				public GenericInputControlBlockCompressionWorkPackageReturnInterface,
				public GenericInputControlBlockCompressionFinishedInterface,
				public GenericInputControlBlockWritePackageReturnInterface,
				public GenericInputControlBlockWritePackageBlockWrittenInterface,
				public GenericInputControlBlocksWritePackageReturnInterface
			{
				typedef FastqToBamControl this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				std::istream & in;
				std::ostream & out;
				
				libmaus::parallel::SimpleThreadPool & STP;
				FastQInputDesc desc;

				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::FastqInputPackage> readWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::FastqParsePackage> parseWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage> compressionWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlockWritePackage> writeWorkPackages;
				libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<libmaus::bambam::parallel::GenericInputControlBlocksWritePackage> writevWorkPackages;
								
				FastqInputPackageDispatcher FIPD;
				uint64_t const FIPDid;
				FastqParsePackageDispatcher FPPD;
				uint64_t const FPPDid;
				GenericInputControlBlockCompressionWorkPackageDispatcher GICBCWPD;
				uint64_t const GICBCWPDid;
				GenericInputControlBlockWritePackageDispatcher GICBWPD;
				uint64_t const GICBWPDid;
				GenericInputControlBlocksWritePackageDispatcher GICBsWPD;
				uint64_t const GICBsWPDid;
				
				bool volatile readingFinished;
				libmaus::parallel::PosixSpinLock readingFinishedLock;
				
				bool volatile parsingFinished;
				libmaus::parallel::PosixSpinLock parsingFinishedLock;
				
				uint64_t volatile nextcompblockid;
				libmaus::parallel::PosixSpinLock nextcompblockidlock;
				
				std::deque<GenericInputControlCompressionPending> compresspendingqueue;
				libmaus::parallel::PosixSpinLock compresspendingqueuelock;
				
				std::map < int64_t, uint64_t > compressionunfinished;
				libmaus::parallel::PosixSpinLock compressionunfinishedlock;
				
				std::map < int64_t, libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type > compressionactive;
				libmaus::parallel::PosixSpinLock compressionactivelock;

				libmaus::parallel::LockedFreeList<
					libmaus::lz::BgzfDeflateOutputBufferBase,
					libmaus::lz::BgzfDeflateOutputBufferBaseAllocator,
					libmaus::lz::BgzfDeflateOutputBufferBaseTypeInfo
				> zbufferfreelist;
				// libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type
				
				libmaus::parallel::LockedGrowingFreeList<
					libmaus::lz::BgzfDeflateZStreamBase,
					libmaus::lz::BgzfDeflateZStreamBaseAllocator,
					libmaus::lz::BgzfDeflateZStreamBaseTypeInfo
				> compfreelist;
				
				std::priority_queue<
					GenericInputControlCompressionPending,
					std::vector<GenericInputControlCompressionPending>,
					GenericInputControlCompressionPendingHeapComparator
				> writependingqueue;
				libmaus::parallel::PosixSpinLock writependingqueuelock;
				uint64_t volatile writependingqueuenext;
				
				bool volatile compressionfinished;
				libmaus::parallel::PosixSpinLock compressionfinishedlock;

				libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type genericInputControlGetCompressor()
				{
					return compfreelist.get();
				}
				
				void genericInputControlPutCompressor(libmaus::lz::BgzfDeflateZStreamBase::shared_ptr_type comp)
				{
					compfreelist.put(comp);
				}

				void fastqParsePackageReturn(FastqParsePackage * package)
				{
					parseWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockWritePackageReturn(libmaus::bambam::parallel::GenericInputControlBlockWritePackage * package)
				{
					writeWorkPackages.returnPackage(package);
				}
				
				void genericInputControlBlocksWritePackageReturn(libmaus::bambam::parallel::GenericInputControlBlocksWritePackage * package)
				{
					writevWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockWritePackageBlockWritten(libmaus::bambam::parallel::GenericInputControlCompressionPending GICCP)
				{
					zbufferfreelist.put(GICCP.outblock);
					checkCompressionPendingQueue();
					
					{
					libmaus::parallel::ScopePosixSpinLock slock(writependingqueuelock);
					writependingqueuenext += 1;
					}				
					
					checkWritePendingQueue();
				}

				void checkWritePendingQueue()
				{
					std::vector < GenericInputControlCompressionPending > Q;
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(writependingqueuelock);
						
						while ( 
							writependingqueue.size() && 
							writependingqueue.top().absid == (writependingqueuenext + Q.size())
						)
						{
							GenericInputControlCompressionPending GICCP = writependingqueue.top();
							writependingqueue.pop();
							
							if ( GICCP.final )
							{
								compressionfinishedlock.lock();
								compressionfinished = true;
								compressionfinishedlock.unlock();	
							}
							
							Q.push_back(GICCP);

						}
					}

					libmaus::bambam::parallel::GenericInputControlBlocksWritePackage * package = writevWorkPackages.getPackage();
					*package = libmaus::bambam::parallel::GenericInputControlBlocksWritePackage(0/*prio*/,GICBsWPDid,Q,&out);
					STP.enque(package);
				}
				
				void genericInputControlBlockCompressionFinished(GenericInputControlCompressionPending GICCP)
				{
					uint64_t const blockid = GICCP.blockid;
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(compressionunfinishedlock);						
						assert ( compressionunfinished.find(blockid) != compressionunfinished.end() );
						if ( ! --compressionunfinished[blockid] )
						{
							compressionunfinished.erase(compressionunfinished.find(blockid));

							libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type deblock;
							
							{
							libmaus::parallel::ScopePosixSpinLock llock(compressionactivelock);
							assert ( compressionactive.find(blockid) != compressionactive.end() );
							deblock = compressionactive.find(blockid)->second;
							}

							desc.decompfreelist.put(deblock);
							checkSubReadPendingQueue();			
						}
					}
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(writependingqueuelock);
						writependingqueue.push(GICCP);
					}
					
					checkWritePendingQueue();
				}
			
				void checkCompressionPendingQueue()
				{
					std::vector<GenericInputControlCompressionPending> readylist;
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(compresspendingqueuelock);
						libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type obuf;
						
						while ( 
							compresspendingqueue.size() 
							&&
							(obuf = zbufferfreelist.getIf())
						)
						{
							GenericInputControlCompressionPending GICCP = compresspendingqueue.front();
							compresspendingqueue.pop_front();
							GICCP.outblock = obuf;
							
							readylist.push_back(GICCP);
						}
					}

					for ( uint64_t i = 0; i < readylist.size(); ++i )
					{
						GenericInputControlCompressionPending GICCP = readylist[i];
						
						libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage * package = 
							compressionWorkPackages.getPackage();
						*package = libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage(0, GICBCWPDid, GICCP);
						
						STP.enque(package);
					}
				}
				
				void checkParseReorderQueue()
				{
					bool finished = false;
				
					{
						libmaus::parallel::ScopePosixSpinLock slock(desc.parsereorderqueuelock);
						
						while ( desc.parsereorderqueue.size() && desc.parsereorderqueue.top()->blockid == desc.parsereorderqueuenext )
						{
							libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type deblock = desc.parsereorderqueue.top();
							desc.parsereorderqueue.pop();
														
							uint64_t const maxblocksize = libmaus::lz::BgzfConstants::getBgzfMaxBlockSize();
							uint64_t const tnumblocks = (deblock->uncompdatasize + maxblocksize - 1)/maxblocksize;
							uint64_t const bytesperblock = tnumblocks ? ((deblock->uncompdatasize+tnumblocks-1)/tnumblocks) : 0;
							uint64_t const numblocks = bytesperblock ? ((deblock->uncompdatasize+bytesperblock-1)/bytesperblock) : 0;
							
							uint64_t const blockid = deblock->blockid;

							{
								libmaus::parallel::ScopePosixSpinLock llock(compressionunfinishedlock);
								compressionunfinished[blockid] += numblocks;
							}
							
							{
								libmaus::parallel::ScopePosixSpinLock llock(compressionactivelock);
								compressionactive[blockid] = deblock;
							}
		
							for ( uint64_t i = 0; i < numblocks; ++i )
							{
								uint64_t const low = i*bytesperblock;
								uint64_t const high = std::min(low + bytesperblock, deblock->uncompdatasize);

								nextcompblockidlock.lock();
								uint64_t const absid = nextcompblockid++;
								nextcompblockidlock.unlock();
								
								GenericInputControlCompressionPending GICCP(
									blockid,
									i,
									absid,
									deblock->final && (i+1 == numblocks),
									std::pair<uint8_t *,uint8_t *>(
										reinterpret_cast<uint8_t *>(deblock->D.begin()) + low, 
										reinterpret_cast<uint8_t *>(deblock->D.begin()) + high
									)
								);
																
								{
									libmaus::parallel::ScopePosixSpinLock llock(compresspendingqueuelock);
									compresspendingqueue.push_back(GICCP);
								}		
							}

							if ( deblock->final )
								finished = true;
																			
							desc.parsereorderqueuenext += 1;
						}
					}
					
					checkCompressionPendingQueue();

					if ( finished )
					{
						parsingFinishedLock.lock();
						parsingFinished = true;
						parsingFinishedLock.unlock();
					}
				}
				
				void fastqParsePackageFinished(FastqToBamControlSubReadPending data)
				{
					if ( data.block->meta.returnBlock() )
					{
						desc.blockFreeList.put(data.block);
						if ( ! desc.getEOF() )
							enqueReadPackage();
					}
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(desc.parsereorderqueuelock);
						desc.parsereorderqueue.push(data.deblock);
					}
					
					checkParseReorderQueue();
				}
									
				FastqToBamControl(
					std::istream & rin, 
					std::ostream & rout,
					libmaus::parallel::SimpleThreadPool & rSTP,
					uint64_t const numblocks, uint64_t const blocksize,
					uint64_t const numoutblocks,
					int const level,
					std::string const rgid
				)
				: in(rin), out(rout), STP(rSTP), desc(in,numblocks,blocksize,0/*streamid*/),
				  FIPD(*this,*this), FIPDid(STP.getNextDispatcherId()),
				  FPPD(*this,*this,rgid), FPPDid(STP.getNextDispatcherId()),
				  GICBCWPD(*this,*this,*this,*this), GICBCWPDid(STP.getNextDispatcherId()),
				  GICBWPD(*this,*this), GICBWPDid(STP.getNextDispatcherId()),
				  GICBsWPD(*this,*this), GICBsWPDid(STP.getNextDispatcherId()),
				  readingFinished(false), readingFinishedLock(),
				  nextcompblockid(0), nextcompblockidlock(),
				  zbufferfreelist(numoutblocks,libmaus::lz::BgzfDeflateOutputBufferBaseAllocator(level)),
				  writependingqueuenext(0),
				  compressionfinished(false)
				{
					STP.registerDispatcher(FIPDid,&FIPD);
					STP.registerDispatcher(FPPDid,&FPPD);
					STP.registerDispatcher(GICBCWPDid,&GICBCWPD);
					STP.registerDispatcher(GICBWPDid,&GICBWPD);
					STP.registerDispatcher(GICBsWPDid,&GICBsWPD);
				}
				
				void enqueReadPackage()
				{
					libmaus::bambam::parallel::FastqInputPackage * package = readWorkPackages.getPackage();
					*package = libmaus::bambam::parallel::FastqInputPackage(
						10/*priority*/,
						FIPDid/*dispid*/,
						&desc
					);
					STP.enque(package);				
				}
				
				void fastqInputPackageReturn(FastqInputPackage * package)
				{
					readWorkPackages.returnPackage(package);
				}

				void genericInputControlBlockCompressionWorkPackageReturn(libmaus::bambam::parallel::GenericInputControlBlockCompressionWorkPackage * package)
				{
					compressionWorkPackages.returnPackage(package);
				}
				
				void checkSubReadPendingQueue()
				{
					std::vector<FastqToBamControlSubReadPending> readyList;

					{
						libmaus::parallel::ScopePosixSpinLock llock(desc.readpendingsubqueuelock);
						libmaus::bambam::parallel::DecompressedBlock::shared_ptr_type deblock;
						
						while ( 
							desc.readpendingsubqueue.size() 
							&&
							(deblock = desc.decompfreelist.getIf())
						)
						{
							FastqToBamControlSubReadPending t = desc.readpendingsubqueue.top();
							desc.readpendingsubqueue.pop();
							t.absid = desc.readpendingsubqueuenextid++;
							t.deblock = deblock;
							readyList.push_back(t);
						}
					}
					
					for ( uint64_t i = 0; i < readyList.size(); ++i )
					{
						FastqToBamControlSubReadPending t = readyList[i];
						FastqParsePackage * package = parseWorkPackages.getPackage();
						*package = FastqParsePackage(0/*prio*/,FPPDid,t);
						STP.enque(package);
					}
				}
				
				void checkReadPendingQueue()
				{
					libmaus::parallel::ScopePosixSpinLock slock(desc.readpendingqueuelock);

					while ( 
						desc.readpendingqueue.size()
						&&
						desc.readpendingqueuenext == desc.readpendingqueue.top()->meta.blockid
					)
					{
						FastQInputDescBase::input_block_type::shared_ptr_type block = desc.readpendingqueue.top();
						desc.readpendingqueue.pop();
						
						{
							libmaus::parallel::ScopePosixSpinLock llock(desc.readpendingsubqueuelock);
							for ( uint64_t i = 0; i < block->meta.blocks.size(); ++i )
								desc.readpendingsubqueue.push(
									FastqToBamControlSubReadPending(block,i)
								);
						}

						
						desc.readpendingqueuenext += 1;
					}
					
					checkSubReadPendingQueue();
				}
				
				void fastqInputPackageAddPending(FastQInputDescBase::input_block_type::shared_ptr_type block)
				{
					assert ( block->meta.blocks.size() <= 1 );
					
					if ( block->meta.blocks.size() )
					{
						std::pair<uint8_t *,uint8_t *> PP(block->meta.blocks[0]);
						
						if ( PP.second != PP.first )
						{
							assert ( PP.second[-1] == '\n' );
							
							std::vector<uint8_t *> PV;
							ptrdiff_t const d = (PP.second-PP.first) / STP.getNumThreads();

							// try to produce work packages
							for ( uint64_t i = 0; i < STP.getNumThreads(); ++i )
							{
								// set start pointer for scanning
								uint8_t * p = PP.first + i*d;
								
								// search next newline if this is not the start of the buffer
								if ( p != PP.first )
								{
									while ( *p != '\n' )
										++p;
									assert ( *p == '\n' );
									++p;
								}
								
								// if start pointer is not the end of the buffer	
								uint8_t * ls[4] = {0,0,0,0};
								if ( p != PP.second )
								{
									while ( p != PP.second )
									{
										ls[0] = ls[1];
										ls[1] = ls[2];
										ls[2] = ls[3];
										ls[3] = p;
										
										if ( ls[0] && ls[0][0] == '@' && ls[2][0] == '+' )
											break;
										
										while ( *p != '\n' )
											++p;
										assert ( *p == '\n' );
										p += 1;	
									};

								}								

								if ( ls[0] && ls[0][0] == '@' && ls[2][0] == '+' )
									PV.push_back(ls[0]);
							}
							
							PV.push_back(PP.second);
							std::vector < std::pair<uint8_t *,uint8_t *> > PPV;
							for ( uint64_t i = 1; i < PV.size(); ++i )
							{
								std::pair<uint8_t *,uint8_t *> P(PV[i-1],PV[i]);
								if ( P.second != P.first )
									PPV.push_back(P);
							}
							
							if ( PPV.size() )
							{
								block->meta.blocks.resize(0);
								for ( uint64_t i = 0; i < PPV.size(); ++i )
									block->meta.blocks.push_back(PPV[i]);
							}
						}
						
					}
					
					{
						libmaus::parallel::ScopePosixSpinLock slock(desc.readpendingqueuelock);
						desc.readpendingqueue.push(block);
					}

					desc.incrementBlocksPassed();
					
					if ( desc.getEOF() && (desc.getBlocksProduced() == desc.getBlocksPassed()) )
					{
						readingFinishedLock.lock();
						readingFinished = true;
						readingFinishedLock.unlock();
					}

					checkReadPendingQueue();
				}
				bool getReadingFinished()
				{
					bool finished;
					
					readingFinishedLock.lock();
					finished = readingFinished;
					readingFinishedLock.unlock();
					
					return finished;
				}
				
				void waitReadingFinished()
				{
					while ( 
						!getReadingFinished()
						&&
						!STP.isInPanicMode() )
						sleep(1);
						
					if ( STP.isInPanicMode() )
						STP.join();
				}

				bool getParsingFinished()
				{
					bool finished = false;
					parsingFinishedLock.lock();
					finished = parsingFinished;
					parsingFinishedLock.unlock();
					return finished;
				}

				void waitParsingFinished()
				{
					while ( 
						!getParsingFinished()
						&&
						!STP.isInPanicMode() 
					)
						sleep(1);
						
					if ( STP.isInPanicMode() )
						STP.join();
				}

				bool getCompressionFinished()
				{
					bool finished;
					compressionfinishedlock.lock();
					finished = compressionfinished;
					compressionfinishedlock.unlock();	
					return finished;
				}

				void waitCompressionFinished()
				{
					while ( 
						!getCompressionFinished()
						&&
						!STP.isInPanicMode() 
					)
						sleep(1);
						
					if ( STP.isInPanicMode() )
						STP.join();
				}
			};
		}
	}
}

struct RgInfo
{
	std::string ID;
	std::string CN;
	std::string DS;
	std::string DT;
	std::string FO;
	std::string KS;
	std::string LB;
	std::string PG; // = fastqtobam
	std::string PI;
	std::string PL;
	std::string PU;
	std::string SM;
	
	RgInfo() {}
	RgInfo(libmaus::util::ArgInfo const & arginfo)
	:
		ID(arginfo.getUnparsedValue("RGID","")),
		CN(arginfo.getUnparsedValue("RGCN","")),
		DS(arginfo.getUnparsedValue("RGDS","")),
		DT(arginfo.getUnparsedValue("RGDT","")),
		FO(arginfo.getUnparsedValue("RGFO","")),
		KS(arginfo.getUnparsedValue("RGKS","")),
		LB(arginfo.getUnparsedValue("RGLB","")),
		PG(arginfo.getUnparsedValue("RGPG","fastqtobam")),
		PI(arginfo.getUnparsedValue("RGPI","")),
		PL(arginfo.getUnparsedValue("RGPL","")),
		PU(arginfo.getUnparsedValue("RGPU","")),
		SM(arginfo.getUnparsedValue("RGSM",""))
	{
		
	}
	
	std::string toString() const
	{
		std::ostringstream ostr;
		
		if ( ID.size() )
		{
			ostr << "@RG\tID:" << ID;
			
			if ( CN.size() ) ostr << "\tCN:" << CN;
			if ( DS.size() ) ostr << "\tDS:" << DS;
			if ( DT.size() ) ostr << "\tDT:" << DT;
			if ( FO.size() ) ostr << "\tFO:" << FO;
			if ( KS.size() ) ostr << "\tKS:" << KS;
			if ( LB.size() ) ostr << "\tLB:" << LB;
			if ( PG.size() ) ostr << "\tPG:" << PG;
			if ( PI.size() ) ostr << "\tPI:" << PI;
			if ( PL.size() ) ostr << "\tPL:" << PL;
			if ( PU.size() ) ostr << "\tPU:" << PU;
			if ( SM.size() ) ostr << "\tSM:" << SM;
			
			ostr << "\n";
		}
		
		return ostr.str();
	}
};

#include <config.h>
#include <libmaus/lz/BgzfDeflate.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);	

		::libmaus::bambam::BamHeader bamheader;
		std::ostringstream headerostr;
		headerostr << "@HD\tVN:1.4\tSO:unknown\n";
		headerostr 
			<< "@PG"<< "\t" 
			<< "ID:" << "fastqtobam" << "\t" 
			<< "PN:" << "fastqtobam" << "\t"
			<< "CL:" << arginfo.commandline << "\t"
			<< "VN:" << std::string(PACKAGE_VERSION)
			<< std::endl;
		RgInfo const rginfo(arginfo);
		headerostr << rginfo.toString();
		bamheader.text = headerostr.str();		


		uint64_t const numlogcpus = arginfo.getValue<int>("threads",libmaus::parallel::NumCpus::getNumLogicalProcessors());
		libmaus::parallel::SimpleThreadPool STP(numlogcpus);
		int const level = Z_DEFAULT_COMPRESSION;
		uint64_t const outblocks = 1024;
		uint64_t const inputblocksize = 1024*1024*64;
		uint64_t const inblocks = 64;
		std::ostream & out = std::cout;
		libmaus::aio::PosixFdInputStream PFIS(STDIN_FILENO);
		libmaus::bambam::parallel::FastqToBamControl FTBC(PFIS,out,STP,inblocks,inputblocksize,outblocks,level,rginfo.ID);

		{
			libmaus::lz::BgzfOutputStream bgzf(out);
			bamheader.serialise(bgzf);
			bgzf.flush();
		}
		
		FTBC.enqueReadPackage();
		FTBC.waitCompressionFinished();
		
		STP.terminate();		
		STP.join();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
