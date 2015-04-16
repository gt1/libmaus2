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
#if ! defined(LIBMAUS_BAMBAM_MDNMRECALCULATION_HPP)
#define LIBMAUS_BAMBAM_MDNMRECALCULATION_HPP

#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>
#include <libmaus/fastx/FastAIndex.hpp>
#include <libmaus/fastx/FastABgzfIndex.hpp>
#include <libmaus/fastx/StreamFastAReader.hpp>
#include <libmaus/lz/RAZFDecoder.hpp>
#include <libmaus/util/OutputFileNameTools.hpp>

namespace libmaus 
{
	namespace bambam
	{
		struct MdNmRecalculation
		{
			typedef MdNmRecalculation this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			bool const isgz;
			bool const israzf;
			bool const isbgzf;
			bool const isplain;
			std::string fastaindexfilename;
			libmaus::fastx::FastAIndex::unique_ptr_type Pfaindex;
			bool const havebgzfindex;
			::libmaus::fastx::FastABgzfIndex::unique_ptr_type Pbgzfindex;
			::libmaus::aio::PosixFdInputStream refin;
			::libmaus::lz::RAZFDecoder::unique_ptr_type razfdec;
			libmaus::fastx::StreamFastAReaderWrapper::pattern_type currefpat;

			bool validate;
			
			int64_t loadrefid;
			int64_t prevcheckrefid;
			int64_t prevcheckpos;
			
			uint64_t numrecalc;
			uint64_t numkept;

			::libmaus::bambam::MdStringComputationContext context;

			libmaus::autoarray::AutoArray<uint8_t> vmap;
			libmaus::autoarray::AutoArray<uint64_t> nar;
			libmaus::rank::ERank222B::unique_ptr_type Prank;
			
			bool recompindetonly;
			bool warnchange;
			
			static bool isGzip(std::istream & in)
			{
				int const b0 = in.get();
				int const b1 = in.get();
				in.clear();
				in.seekg(0,std::ios::beg);
				
				return b0 == libmaus::lz::GzipHeaderConstantsBase::ID1 &&
				       b1 == libmaus::lz::GzipHeaderConstantsBase::ID2;
			}
			
			static bool isGzip(std::string const & filename)
			{
				libmaus::aio::CheckedInputStream CIS(filename);
				return isGzip(CIS);
			}
			
			static std::string fastaIndexName(std::string const & reference)
			{
				std::string indexname;
				
				// try appending fai
				if ( libmaus::util::GetFileSize::fileExists(indexname=(reference + ".fai") ) )
				{
				
				}
				// try removing .fa and appending fai
				else if ( 
					libmaus::util::OutputFileNameTools::endsOn(reference,".fa")
					&&
					libmaus::util::GetFileSize::fileExists(
						indexname=(libmaus::util::OutputFileNameTools::clipOff(
							reference,".fa"
						) + ".fai")
					) 
				)
				{
				}
				// try removing .fasta and appending fai
				else if ( 
					libmaus::util::OutputFileNameTools::endsOn(reference,".fasta")
					&&
					libmaus::util::GetFileSize::fileExists(
						indexname=(libmaus::util::OutputFileNameTools::clipOff(
							reference,".fasta"
						) + ".fai")
					) 
				)
				{
				}
				
				return indexname;
			}

			MdNmRecalculation(std::string const & reference, bool const rvalidate, bool const rrecompindetonly, bool const rwarnchange, uint64_t const rioblocksize)
			: 
			  isgz(isGzip(reference)),
			  israzf(isgz && libmaus::lz::RAZFIndex::hasRazfHeader(reference)),
			  isbgzf(isgz && (!israzf)),
			  isplain(!isgz),
			  fastaindexfilename((israzf || isplain) ? fastaIndexName(reference) : std::string() ),
			  Pfaindex(
				fastaindexfilename.size() ? libmaus::fastx::FastAIndex::load(fastaindexfilename) : ::libmaus::fastx::FastAIndex::unique_ptr_type()
			  ),
			  havebgzfindex(isbgzf && libmaus::util::GetFileSize::fileExists(reference+".idx")),
			  Pbgzfindex(havebgzfindex ? ::libmaus::fastx::FastABgzfIndex::load(reference+".idx") : ::libmaus::fastx::FastABgzfIndex::unique_ptr_type() ),
			  refin(reference,rioblocksize), 
			  razfdec(israzf ? (new ::libmaus::lz::RAZFDecoder(refin)) : 0 ),
			  validate(rvalidate), 
			  loadrefid(-1), 
			  prevcheckrefid(-1), 
			  prevcheckpos(-1),
			  numrecalc(0), numkept(0), vmap(256,false),
			  recompindetonly(rrecompindetonly),
			  warnchange(rwarnchange)
			{
				if ( (israzf || isplain) && (!Pfaindex.get()) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "MdNmRecalculation(): reference is plain or RAZF but there is no fai file" << std::endl;
					lme.finish();
					throw lme;
				}
				
				if ( isbgzf && (!Pbgzfindex.get()) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "MdNmRecalculation(): reference is bgzf but there is no idx file" << std::endl;
					lme.finish();
					throw lme;			
				}
			
				std::fill(vmap.begin(),vmap.end(),1);
				vmap['A'] = vmap['C'] = vmap['G'] = vmap['T'] =
				vmap['a'] = vmap['c'] = vmap['g'] = vmap['t'] = 0;
			}
			
			void checkValidity(uint8_t const * D, uint64_t const blocksize)
			{
				if ( validate )
				{
					libmaus::bambam::libmaus_bambam_alignment_validity const validity =
						libmaus::bambam::BamAlignmentDecoderBase::valid(D,blocksize);

					if ( validity != libmaus::bambam::libmaus_bambam_alignment_validity_ok )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "Invalid alignment " << validity << std::endl;
						se.finish();
						throw se;
					}	
				}
			}

			void checkOrder(uint8_t const * D)
			{
				// information for this new alignment
				int64_t const thisrefid = libmaus::bambam::BamAlignmentDecoderBase::getRefID(D);
				int64_t const thispos = libmaus::bambam::BamAlignmentDecoderBase::getPos(D);

				// map negative to maximum positive for checking order
				int64_t const thischeckrefid = (thisrefid >= 0) ? thisrefid : std::numeric_limits<int64_t>::max();
				int64_t const thischeckpos   = (thispos >= 0) ? thispos : std::numeric_limits<int64_t>::max();
				
				// true iff order is ok
				bool const orderok =
					(thischeckrefid > prevcheckrefid)
					||
					(thischeckrefid == prevcheckrefid && thischeckpos >= prevcheckpos);

				// throw exception if alignment stream is not sorted by coordinate
				if ( ! orderok )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "File is not sorted by coordinate." << std::endl;
					se.finish();
					throw se;
				}
			
				prevcheckrefid = thischeckrefid;
				prevcheckpos = thischeckpos;
			}
			
			std::string const & loadReference(uint64_t const id)
			{
				if ( static_cast<int64_t>(id) != loadrefid )
				{
					if ( isbgzf )
					{
						::libmaus::fastx::FastABgzfIndexEntry const & ent = (*Pbgzfindex)[id];
						currefpat.sid = ent.name;
						currefpat.spattern.resize(ent.patlen);
						::libmaus::fastx::FastABgzfDecoder::unique_ptr_type fastr(Pbgzfindex->getStream(refin,id));
						::libmaus::autoarray::AutoArray<char> B(64*1024,false);
						uint64_t p = 0;
						uint64_t todo = ent.patlen;
						
						while ( todo )
						{
							uint64_t const pack = std::min(todo,B.size());
							fastr->read(B.begin(),pack);
							assert ( fastr->gcount() == static_cast<int64_t>(pack) );
							
							std::copy(B.begin(),B.begin()+pack,currefpat.spattern.begin()+p);
							
							p += pack;
							todo -= pack;
						}
						
						std::cerr << "[D] loaded " << ent.name << " of length " << p << std::endl;
						
						loadrefid = id;
					}
					else
					{
						std::istream * in = israzf ? 
							static_cast<std::istream *>(razfdec.get())
							: 
							static_cast<std::istream *>((&refin));
						libmaus::autoarray::AutoArray<char> newrefdata = Pfaindex->readSequence(*in,id);

						currefpat.sid = Pfaindex->sequences[id].name;
						currefpat.spattern = std::string(newrefdata.begin(),newrefdata.end());
						
						loadrefid = id;
							
						#if 0
						while ( loadrefid < static_cast<int64_t>(id) )
						{
							bool const ok = fareader->getNextPatternUnlocked(currefpat);
								
							if ( ! ok )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "[D] Failed to load reference sequence id " << id << std::endl;
								se.finish();
								throw se;		
							}
							
							loadrefid++;
						}
						#endif
					}

					if ( recompindetonly )
					{
						std::string const & seq = currefpat.spattern;
						uint64_t const seqlen = seq.size();
						
						if ( nar.size() < (seqlen+1+63)/64 )
							nar = libmaus::autoarray::AutoArray<uint64_t>((seqlen+1+63)/64,false);
							
						uint8_t const * p = reinterpret_cast<uint8_t const *>(seq.c_str());
						for ( uint64_t i = 0; i < (seqlen/64); ++i )
						{
							uint64_t v = 0;
							for ( uint64_t j = 0; j < 64; ++j )
							{
								v <<= 1;
								v |= vmap[*(p++)];
							}

							nar[i] = v;
						}
						uint64_t const rest = seqlen-((seqlen/64)*64);
						if ( rest )
						{
							uint64_t v = 0;
							for ( uint64_t j = 0; j < rest; ++j )
							{
								v <<= 1;
								v |= vmap[*(p++)];				
							}
							
							nar[seqlen/64] = v << (63-rest);
						}
						
						#if 0
						p = reinterpret_cast<uint8_t const *>(seq.c_str());
						uint64_t nn = 0;
						for ( uint64_t i = 0; i < seqlen; ++i )
						{
							if ( *p == 'N' )
							{
								assert ( libmaus::bitio::getBit(nar.begin(),i) );
								++nn;
							}
							assert ( libmaus::bitio::getBit(nar.begin(),i) == vmap[*(p++)] );
						}
						#endif
						
						libmaus::rank::ERank222B::unique_ptr_type Trank(new libmaus::rank::ERank222B(nar.begin(),64*((seqlen+1+63)/64)));
						Prank = UNIQUE_PTR_MOVE(Trank);
					}
				}
				
				return currefpat.spattern;
			}

			bool calmdnm(uint8_t const * D, uint64_t const blocksize)
			{
				checkValidity(D,blocksize);
				checkOrder(D);
				
				bool recalc = false;
				
				if ( ! libmaus::bambam::BamAlignmentDecoderBase::isUnmap(libmaus::bambam::BamAlignmentDecoderBase::getFlags(D)) )
				{
					int64_t const refid = libmaus::bambam::BamAlignmentDecoderBase::getRefID(D);
					int64_t const pos = libmaus::bambam::BamAlignmentDecoderBase::getPos(D);
					int64_t const refend = pos + libmaus::bambam::BamAlignmentDecoderBase::getReferenceLength(D);
					std::string const & ref = loadReference(refid);
					
					if ( 
						(
							recompindetonly 
							&& 
							(
								(Prank->rankm1(refend)-Prank->rankm1(pos)) 
								|| 
								libmaus::bambam::BamAlignmentDecoderBase::hasNonACGT(D)
							)
						)
						||
						(!recompindetonly)
					)
					{
						numrecalc += 1;
						libmaus::bambam::BamAlignmentDecoderBase::calculateMd(D,blocksize,context,ref.begin() + pos,warnchange);
						if ( context.diff )
							recalc = true;
					}
					else
					{
						numkept += 1;
					}
				}

				return recalc;
			}
			
			bool calmdnm(libmaus::bambam::BamAlignment const & algn)
			{
				return calmdnm(algn.D.begin(),algn.blocksize);
			}
		};
	}
}
#endif
