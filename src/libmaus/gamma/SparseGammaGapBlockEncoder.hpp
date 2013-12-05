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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPBLOCKENCODER_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPBLOCKENCODER_HPP

#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/util/GetFileSize.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapBlockEncoder
		{
			typedef SparseGammaGapBlockEncoder this_type;
			
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef libmaus::aio::SynchronousGenericOutput<uint64_t> stream_type;
			
			// w output stream
			libmaus::aio::CheckedOutputStream::unique_ptr_type SGOCOS;
			// rw index stream
			libmaus::util::unique_ptr<std::fstream>::type indexUP;
			
			std::ostream & SGOout;
			std::iostream & indexout;
			
			libmaus::aio::SynchronousGenericOutput<uint64_t> SGO;
			
			int64_t prevkey;
			libmaus::gamma::GammaEncoder<stream_type> genc;
			
			uint64_t const blocksize;
			uint64_t blockleft;
			
			uint64_t indexentries;

			SparseGammaGapBlockEncoder(
				std::ostream & out,
				std::iostream & rindexout,
				int64_t const rprevkey = -1,
				uint64_t const rblocksize = 64*1024
			)
			:
			  SGOout(out), 
			  indexout(rindexout),
			  SGO(SGOout,8*1024),
			  prevkey(rprevkey), 
			  genc(SGO), 
			  blocksize(rblocksize), 
			  blockleft(0),
			  indexentries(0)
			{
			}
			
			SparseGammaGapBlockEncoder(
				std::string const & filename,
				std::string const & indexfilename,
				uint64_t const rblocksize = 64*1024
			)
			: SGOCOS(new libmaus::aio::CheckedOutputStream(filename)), 
			  indexUP(new std::fstream(indexfilename)),
			  SGOout(*SGOCOS), 
			  indexout(*indexUP),
			  SGO(SGOout,8*1024),
			  prevkey(-1), 
			  genc(SGO), 
			  blocksize(rblocksize), 
			  blockleft(0),
			  indexentries(0)			  
			{
			}
			
			void encode(uint64_t const key, uint64_t const val)
			{
				// start of next block
				if ( ! blockleft )
				{
					uint64_t const ikey = key;
					uint64_t const ibitoff = genc.getOffset();
					
					libmaus::util::NumberSerialisation::serialiseNumber(indexout,ikey);
					libmaus::util::NumberSerialisation::serialiseNumber(indexout,ibitoff);
					indexentries++;
					
					// std::cerr << "ikey=" << ikey << " ibitoff=" << ibitoff << std::endl;

					blockleft = blocksize;
				}
			
				int64_t const dif = (static_cast<int64_t>(key)-prevkey)-1;
				genc.encode(dif);
				prevkey = key;
				assert ( val );
				genc.encode(val);
				--blockleft;
			}
			
			void term()
			{
				genc.encode(0);
				genc.encode(0);
				genc.flush();
				SGO.flush();
				SGOout.flush();
				
				indexout.clear();
				indexout.seekg(0,std::ios::beg);
				
				// uint64_t const indexpos = SGO.getWrittenBytes();
				indexout.clear();
				indexout.seekg(0,std::ios::beg);
				libmaus::util::GetFileSize::copy(indexout,SGOout,2*sizeof(uint64_t)*indexentries);
				libmaus::util::NumberSerialisation::serialiseNumber(SGOout,indexentries ? prevkey : 0); // highest key in file
				libmaus::util::NumberSerialisation::serialiseNumber(SGOout,indexentries);
				
				SGOout.flush();
			}
			
			template<typename it>
			static void encodeArray(it const ita, it const ite, std::ostream & out, std::ostream & indexout)
			{
				std::sort(ita,ite);
				
				this_type enc(out);
				
				it itl = ita;
				
				while ( itl != ite )
				{
					it ith = itl;
					
					while ( ith != ite && *ith == *itl )
						++ith;
					
					enc.encode(*itl,ith-itl);
					
					itl = ith;
				}
				
				enc.term();
				out.flush();
			}
			
			template<typename it>
			static std::vector<std::string> encodeArray(it const gita, it const gite, std::string const & fnprefix, uint64_t const tparts, uint64_t const blocksize = 64*1024)
			{
				std::sort(gita,gite);
				
				uint64_t const partsize = (gite-gita+tparts-1)/(tparts);
				
				std::vector<uint64_t> partstarts;
				it gitc = gita;
				while ( gitc != gite )
				{
					while ( gitc != gita && gitc != gite && (*(gitc-1)) == *gitc )
						++gitc;
						
					assert ( gitc == gita || gitc == gite || ((*(gitc-1)) != (*gitc)) );
					
					if ( gitc != gite )
						partstarts.push_back(gitc-gita);
						
					gitc += std::min(partsize,static_cast<uint64_t>(gite-gitc));
				}
				
				uint64_t const parts = partstarts.size();
				std::vector<std::string> partfn(parts);
				partstarts.push_back(gite-gita);
				
				for ( uint64_t p = 0; p < parts; ++p )
				{
					std::ostringstream fnostr;
					fnostr << fnprefix << "_" << std::setw(6) << std::setfill('0') << p;
					std::string const fn = fnostr.str();
					partfn[p] = fn;
					std::string const indexfn = fn+".idx";
					libmaus::util::TempFileRemovalContainer::addTempFile(indexfn);				
					
					libmaus::aio::CheckedOutputStream COS(fn);
					std::fstream indexstr(indexfn, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
					if ( ! indexstr.is_open() )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "SparseGammaGapBlockEncoder::encodeArray(): failed  to open file " << indexfn << std::endl;
						ex.finish();
						throw ex;
					}

					this_type enc(
						COS,indexstr,
						(p==0)?(-1):gita[partstarts[p]-1],
						blocksize
					);

					it itl = gita + partstarts[p];
					it ite = gita + partstarts[p+1];
				
					while ( itl != ite )
					{
						it ith = itl;
						
						while ( ith != ite && *ith == *itl )
							++ith;
						
						enc.encode(*itl,ith-itl);
						
						itl = ith;
					}
					
					enc.term();
					COS.flush();
					COS.close();
				}
				
				return partfn;
			}

			template<typename it>
			static void encodeArray(it const ita, it const ite, std::string const & fn)
			{
				libmaus::aio::CheckedOutputStream COS(fn);
				std::string const indexfn = fn+".idx";
				libmaus::util::TempFileRemovalContainer::addTempFile(indexfn);
				libmaus::aio::CheckedOutputStream indexCOS(indexfn);
				encodeArray(ita,ite,COS,indexCOS);
				remove(indexfn.c_str());
			}
		};	
	}
}
#endif
