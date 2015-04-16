/*
    libmaus2
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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPMERGE_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPMERGE_HPP

#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/gamma/SparseGammaGapConcatDecoder.hpp>
#include <libmaus2/gamma/SparseGammaGapBlockEncoder.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/parallel/PosixSemaphore.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct SparseGammaGapMerge
		{
			struct SparseGammaGapMergeInfo
			{
				typedef SparseGammaGapMergeInfo this_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
				private:
				uint64_t tparts;
				std::string fnpref;
				bool registertempfiles;
				
				std::vector<std::string> fna;
				std::vector<std::string> fnb;
				std::vector<std::string> fno;
				std::vector<uint64_t> sp;
				
				uint64_t next;
				uint64_t finished;
				uint64_t level;

				bool initialised;

				libmaus2::parallel::OMPLock::shared_ptr_type lock;
				libmaus2::parallel::OMPLock::shared_ptr_type initlock;
				
				libmaus2::parallel::OMPLock * semlock;
				std::vector < libmaus2::parallel::PosixSemaphore * > * mergepacksem;
				
				libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder::shared_ptr_type indexa;
				libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder::shared_ptr_type indexb;

				public:
				SparseGammaGapMergeInfo() 
				: tparts(0), fnpref(), registertempfiles(false), next(0), 
				  finished(0), level(0), initialised(false), 
				  lock(new libmaus2::parallel::OMPLock),
				  initlock(new libmaus2::parallel::OMPLock),
				  semlock(0),
				  mergepacksem(0)
				{}
				
				SparseGammaGapMergeInfo(
					std::vector<std::string> rfna,
					std::vector<std::string> rfnb,
					std::vector<std::string> rfno,
					std::vector<uint64_t> rsp	
				) : tparts(rfno.size()), fnpref(), registertempfiles(false), 
				    fna(rfna), fnb(rfnb), fno(rfno), sp(rsp), next(0), finished(0), level(0), initialised(true),
				    lock(new libmaus2::parallel::OMPLock),
				    initlock(new libmaus2::parallel::OMPLock),
				    semlock(0),
				    mergepacksem(0)
				{}
				
				/**
				 * constructor for delayed initialisation
				 **/
				SparseGammaGapMergeInfo(
					std::vector<std::string> const & rfna,
					std::vector<std::string> const & rfnb,
					std::string const rfnpref,
					uint64_t rtparts,
					bool rregisterTempFiles = true
				)
				: tparts(rtparts), fnpref(rfnpref), registertempfiles(rregisterTempFiles), 
				  fna(rfna), fnb(rfnb), fno(), sp(), next(0), finished(0), level(0), initialised(false),
				  lock(new libmaus2::parallel::OMPLock),
				  initlock(new libmaus2::parallel::OMPLock),
				  semlock(0),
				  mergepacksem(0)
				{
				
				}
				
				void setSemaphoreInfo(
					libmaus2::parallel::OMPLock * rsemlock,
					std::vector < libmaus2::parallel::PosixSemaphore * > * rmergepacksem
				)
				{
					semlock = rsemlock;
					mergepacksem = rmergepacksem;
				}
				
				void initialise()
				{
					libmaus2::parallel::ScopeLock slock(*initlock);

					if ( ! indexa )
					{
						libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder::shared_ptr_type tindexa(
							new libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder(fna)
						);
						indexa = tindexa;
					}
					if ( ! indexb )
					{
						libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder::shared_ptr_type tindexb(
							new libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder(fnb)
						);
						indexb = tindexb;
					}
					if ( ! initialised )
					{
					
						uint64_t maxv = 0;
						sp = libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder::getSplitKeys(*indexa,*indexb,tparts,maxv);
						uint64_t const parts = sp.size();
						sp.push_back(maxv+1);
						
						std::vector<std::string> outputfilenames(parts);
						for ( uint64_t p = 0; p < parts; ++p )
						{
							std::ostringstream fnostr;
							fnostr << fnpref << "_" << std::setw(6) << std::setfill('0') << p << std::setw(0);
							outputfilenames[p] = fnostr.str();				
							if ( registertempfiles )
								libmaus2::util::TempFileRemovalContainer::addTempFile(outputfilenames[p]);
						}
						fno = outputfilenames;
						
						if ( semlock && mergepacksem )
						{
							libmaus2::parallel::ScopeLock slock(*semlock);
							for ( uint64_t i = 0; i < fno.size(); ++i )
								for ( uint64_t j = 0; j < mergepacksem->size(); ++j )
									(*mergepacksem)[j]->post();
						}
						
						initialised = true;
					}
				
				}
				
				void dispatch(uint64_t const p)
				{
					initialise();
					
					std::string const & fn = fno.at(p);
					std::string const indexfn = fn + ".idx";
					libmaus2::util::TempFileRemovalContainer::addTempFile(indexfn);
					libmaus2::aio::CheckedOutputStream COS(fn);
					libmaus2::aio::CheckedInputOutputStream indexstr(indexfn.c_str());
					merge(
						/* fna,fnb, */
						*indexa,*indexb,
						sp.at(p),sp.at(p+1),COS,indexstr
					);
					remove(indexfn.c_str());			
				}

				bool dispatchNext()
				{
					initialise();
				
					uint64_t id = fno.size();
					
					{
						libmaus2::parallel::ScopeLock slock(*lock);
						if ( next == fno.size() )
							id = fno.size();
						else
							id = next++;
					}
					
					if ( id < fno.size() )
						dispatch(id);
						
					return id < fno.size();
				}
				
				bool getNextDispatchId(uint64_t & id)
				{
					initialise();
				
					id = fno.size();

					{
						libmaus2::parallel::ScopeLock slock(*lock);
						if ( next == fno.size() )
							id = fno.size();
						else
							id = next++;
					}
						
					return id < fno.size();
				}

				void dispatch()
				{
					while ( dispatchNext() )
					{
						// dispatch(i);
					}
				}
				
				bool incrementFinished()
				{
					libmaus2::parallel::ScopeLock slock(*lock);
					bool const done = ((++finished) >= fno.size());
					return done;
				}
				
				std::vector<std::string> const & getOutputFileNames()
				{
					return fno;
				}
				
				bool isHandoutFinished()
				{
					libmaus2::parallel::ScopeLock slock(*lock);
					return initialised && (next == fno.size());	
				}
				
				bool isFinished()
				{
					libmaus2::parallel::ScopeLock slock(*lock);
					return initialised && (finished == fno.size());	
				}
				
				void setLevel(uint64_t const rlevel)
				{
					level = rlevel;
				}
				
				uint64_t getLevel() const
				{
					return level;
				}
				
				void removeInputFiles()
				{
					for ( uint64_t i = 0; i < fna.size(); ++i )
						remove(fna[i].c_str());
					for ( uint64_t i = 0; i < fnb.size(); ++i )
						remove(fnb[i].c_str());					
				}
			};

			/**
			 * compute parallel merging info
			 **/
			static SparseGammaGapMergeInfo getMergeInfoDelayedInit(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				std::string const fnpref,
				uint64_t tparts,
				bool registerTempFiles = true
			)
			{
				return SparseGammaGapMergeInfo(fna,fnb,fnpref,tparts,registerTempFiles);
			}

			static SparseGammaGapMergeInfo getMergeInfo(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				std::string const fnpref,
				uint64_t tparts,
				bool registerTempFiles = true
			)
			{
				uint64_t maxv = 0;
				std::vector<uint64_t> sp = libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder::getSplitKeys(fna,fnb,tparts,maxv);
				uint64_t const parts = sp.size();
				sp.push_back(maxv+1);
				
				std::vector<std::string> outputfilenames(parts);
				for ( uint64_t p = 0; p < parts; ++p )
				{
					std::ostringstream fnostr;
					fnostr << fnpref << "_" << std::setw(6) << std::setfill('0') << p << std::setw(0);
					outputfilenames[p] = fnostr.str();				
					if ( registerTempFiles )
						libmaus2::util::TempFileRemovalContainer::addTempFile(outputfilenames[p]);
				}
				
				SparseGammaGapMergeInfo SGGMI(fna,fnb,outputfilenames,sp);

				return SGGMI;
			}

			static std::vector<std::string> merge(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				std::string const fnpref,
				uint64_t tparts,
				bool const registerTempFiles = true
			)
			{
				SparseGammaGapMergeInfo SGGMI = getMergeInfo(fna,fnb,fnpref,tparts,registerTempFiles);
				SGGMI.dispatch();
				return SGGMI.getOutputFileNames();
			}
		
			static void merge(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				std::string const & outputfilename
			)
			{
				std::string const indexfilename = outputfilename + ".idx";
				libmaus2::util::TempFileRemovalContainer::addTempFile(indexfilename);
				
				libmaus2::aio::CheckedOutputStream COS(outputfilename);
				libmaus2::aio::CheckedInputOutputStream indexstr(indexfilename.c_str());
				
				libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder indexa(fna);
				libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder indexb(fnb);
				
				merge(indexa,indexb,0,std::numeric_limits<uint64_t>::max(),COS,indexstr);
				
				remove(indexfilename.c_str());
			}
		
			static void merge(
				libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder & indexa,
				libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder & indexb,
				uint64_t const klow,  // inclusive
				uint64_t const khigh, // exclusive
				std::ostream & stream_out,
				std::iostream & index_str
			)
			{
				// true if a contains any relevant keys
				bool const aproc = indexa.hasKeyInRange(klow,khigh);
				// true if b contains any relevant keys
				bool const bproc = indexb.hasKeyInRange(klow,khigh);

				// first key in stream a (or 0 if none)
				uint64_t const firstkey_a = aproc ? libmaus2::gamma::SparseGammaGapConcatDecoder::getNextKey(indexa,klow) : std::numeric_limits<uint64_t>::max();
				// first key in stream b (or 0 if none)
				uint64_t const firstkey_b = bproc ? libmaus2::gamma::SparseGammaGapConcatDecoder::getNextKey(indexb,klow) : std::numeric_limits<uint64_t>::max();
				
				// previous non zero key (or -1 if none)
				int64_t const prevkey_a = libmaus2::gamma::SparseGammaGapConcatDecoder::getPrevKey(indexa,klow);
				int64_t const prevkey_b = libmaus2::gamma::SparseGammaGapConcatDecoder::getPrevKey(indexb,klow);
				int64_t const prevkey_ab = std::max(prevkey_a,prevkey_b);
				
				// set up encoder
				libmaus2::gamma::SparseGammaGapBlockEncoder oenc(stream_out,index_str,prevkey_ab);
				// set up decoders
				libmaus2::gamma::SparseGammaGapConcatDecoder adec(indexa,firstkey_a);
				libmaus2::gamma::SparseGammaGapConcatDecoder bdec(indexb,firstkey_b);

				// current key,value pairs for stream a and b
				std::pair<uint64_t,uint64_t> aval(firstkey_a,adec.p.second);
				std::pair<uint64_t,uint64_t> bval(firstkey_b,bdec.p.second);
				
				// while both streams have keys in range
				while ( aval.second && aval.first < khigh && bval.second && bval.first < khigh )
				{
					if ( aval.first == bval.first )
					{
						oenc.encode(aval.first,aval.second+bval.second);
						aval.first += adec.nextFirst() + 1;
						aval.second = adec.nextSecond();
						bval.first += bdec.nextFirst() + 1;
						bval.second = bdec.nextSecond();
					}
					else if ( aval.first < bval.first )
					{
						oenc.encode(aval.first,aval.second);						
						aval.first += adec.nextFirst() + 1;
						aval.second = adec.nextSecond();
					}
					else // if ( bval.first < aval.first )
					{
						oenc.encode(bval.first,bval.second);						
						bval.first += bdec.nextFirst() + 1;
						bval.second = bdec.nextSecond();
					}				
				}

				// rest keys in stream a
				while ( aval.second && aval.first < khigh )
				{
					oenc.encode(aval.first,aval.second);						
					aval.first += adec.nextFirst() + 1;
					aval.second = adec.nextSecond();				
				}

				// rest keys in stream b
				while ( bval.second && bval.first < khigh )
				{
					oenc.encode(bval.first,bval.second);						
					bval.first += bdec.nextFirst() + 1;
					bval.second = bdec.nextSecond();
				}
				
				oenc.term();
			}
		
			static void merge(
				std::istream & stream_in_a,
				std::istream & stream_in_b,
				std::ostream & stream_out
			)
			{
				libmaus2::aio::SynchronousGenericInput<uint64_t> SGIa(stream_in_a,64*1024);
				libmaus2::aio::SynchronousGenericInput<uint64_t> SGIb(stream_in_b,64*1024);
				libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(stream_out,64*1024);
				
				libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<uint64_t> > adec(SGIa);
				libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<uint64_t> > bdec(SGIb);
				libmaus2::gamma::GammaEncoder< libmaus2::aio::SynchronousGenericOutput<uint64_t> > oenc(SGO);
				
				std::pair<uint64_t,uint64_t> aval;
				std::pair<uint64_t,uint64_t> bval;

				aval.first = adec.decode();
				aval.second = adec.decode();

				bval.first = bdec.decode();
				bval.second = bdec.decode();
				
				int64_t prevkey = -1;
				
				while ( aval.second && bval.second )
				{
					if ( aval.first == bval.first )
					{
						oenc.encode(static_cast<int64_t>(aval.first) - prevkey - 1);
						oenc.encode(aval.second + bval.second);
						
						prevkey = aval.first;
						
						aval.first += adec.decode() + 1;
						aval.second = adec.decode();
						bval.first += bdec.decode() + 1;
						bval.second = bdec.decode();
					}
					else if ( aval.first < bval.first )
					{
						oenc.encode(static_cast<int64_t>(aval.first) - prevkey - 1);
						oenc.encode(aval.second);
						
						prevkey = aval.first;
						
						aval.first += adec.decode() + 1;
						aval.second = adec.decode();
					}
					else // if ( bval.first < aval.first )
					{
						oenc.encode(static_cast<int64_t>(bval.first) - prevkey - 1);
						oenc.encode(bval.second);
						
						prevkey = bval.first;
						
						bval.first += bdec.decode() + 1;
						bval.second = bdec.decode();
					}
				}
				
				while ( aval.second )
				{
					oenc.encode(static_cast<int64_t>(aval.first) - prevkey - 1);
					oenc.encode(aval.second);
						
					prevkey = aval.first;
						
					aval.first += adec.decode() + 1;
					aval.second = adec.decode();				
				}
				
				while ( bval.second )
				{
					oenc.encode(static_cast<int64_t>(bval.first) - prevkey - 1);
					oenc.encode(bval.second);
						
					prevkey = bval.first;
						
					bval.first += bdec.decode() + 1;
					bval.second = bdec.decode();
				}
				
				oenc.encode(0);
				oenc.encode(0);
				oenc.flush();
				SGO.flush();
				stream_out.flush();
			}		
		};
	}
}
#endif
