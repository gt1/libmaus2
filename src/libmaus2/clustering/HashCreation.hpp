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

#if ! defined(HASHCREATION_HPP)
#define HASHCREATION_HPP

#include <libmaus2/fastx/ComputeFastXIntervals.hpp>
#include <libmaus2/clustering/HashCreationBase.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>
#include <libmaus2/graph/TripleEdgeOutputSet.hpp>
#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/aio/SynchronousOutputFile8Array.hpp>

namespace libmaus2
{
	namespace clustering
	{
		struct HashCreationTables : public HashCreationBase
		{
			#if 0
			static unsigned int const maxhashlen = 31;
			static unsigned int const maxhashbits = 26;
			#else
			#define maxhashlen 31u
			#define maxhashbits 26u
			#endif
		
			unsigned int const k;
			unsigned int const hashlen;
			unsigned int const hashbits;
			uint64_t const hashtablesize;
			unsigned int const hashshift;

			::libmaus2::autoarray::AutoArray<uint8_t> const S;
			::libmaus2::autoarray::AutoArray<uint8_t> const R;
			::libmaus2::autoarray::AutoArray<unsigned int> const E;

			void serialise(std::ostream & out) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,k);
			}
			
			void serialise(std::string const & filename) const
			{
				std::ofstream ostr(filename.c_str(), std::ios::binary);
				serialise(ostr);
				ostr.flush();
				assert ( ostr );
				ostr.close();
			}
			
			HashCreationTables(std::istream & in)
			:
			        k(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			        hashlen(std::min(k,maxhashlen)),
			        hashbits(std::min(maxhashbits, 2*hashlen)), hashtablesize(1ull<<hashbits),
			        hashshift(2*hashlen-hashbits), 
			        S(createSymMap()), R(createRevSymMap()), E(createErrorMap())
                        {}
			
			HashCreationTables(unsigned int const rk)
			: k(rk), 
			  hashlen(std::min(k,maxhashlen)),
			  hashbits(std::min(maxhashbits, 2*hashlen)), hashtablesize(1ull<<hashbits),
			  hashshift(2*hashlen-hashbits), 
			  S(createSymMap()), R(createRevSymMap()), E(createErrorMap())
			{
				#if 0
				std::cerr << "hashlen " << hashlen << " hashbits " << hashbits 
					<< " hash shift " << hashshift << std::endl;
				std::cerr << "table size " << hashtablesize / (1024*1024) << std::endl;
				#endif
			}

			template<typename pattern_iterator_type>
			void handleReadHistogram(
				pattern_iterator_type pattern,
				uint64_t const l,
				::libmaus2::fastx::SingleWordDNABitBuffer & forw,
				::libmaus2::fastx::SingleWordDNABitBuffer & reve,
				uint64_t * const O
				) const
			{
				forw.reset();
				reve.reset();
		
				if ( l < k )
					return;
		
				unsigned int const localk = std::min(static_cast<uint64_t>(k), l);
		
				pattern_iterator_type sequence = pattern;
				pattern_iterator_type fsequence = sequence;
				pattern_iterator_type rsequence = (sequence + localk);
		
				unsigned int forwe = 0;
				for ( unsigned int i = 0; i < hashlen; ++i )
				{
					char const base = *(sequence++);
					forwe += E [ base ];
					forw.pushBackUnmasked( S [ base ]  );
		
					char const rbase = *(--rsequence);
					reve.pushBackUnmasked( R [ rbase ] );
				}
		
				unsigned int e = forwe;
		
				if ( localk > hashlen )
				{
					pattern_iterator_type tsequence = sequence;
					for ( unsigned int i = hashlen; i < localk; ++i )
						e += E[ *(tsequence++) ];
				}
		
				rsequence = pattern + localk;
			
				for ( unsigned int z = 0; z < l-localk+1; )
				{
					if ( e < 1 )
					{
						if ( reve.buffer < forw.buffer )
						{
							uint64_t const rhash = (reve.buffer) >> hashshift;
							assert ( rhash < hashtablesize );
							O[rhash]++;
						}
						else
						{
							uint64_t const fhash = (forw.buffer) >> hashshift;
							assert ( fhash < hashtablesize );
							O[fhash]++;
						}
		
						#if 0
						std::cerr << pattern.pattern << std::endl;
		
						for ( unsigned int i = 0; i < z; ++i )
							std::cerr << " ";
						std::cerr << decodeBuffer(forw) << std::endl;
		
						for ( unsigned int i = 0; i < z + localk - hashlen; ++i )
							std::cerr << " ";
						std::cerr << decodeReverseBuffer(reve) << std::endl;
						#endif
					}
		
					if ( ++z < l-localk+1 )
					{
						e -= E[*(fsequence++)];
		
						char const base = *(sequence++);
						assert ( base >= 0 );
						forw.pushBackMasked( S[base] );
		
						char const rbase = *(rsequence++);
						assert ( rbase >= 0 );
						reve.pushFront( R[rbase] );
		
						e += E[rbase];
					}
				}
		
			}

			template<typename reader_param_type>
			void computeHashFrequencyBlock(uint64_t * const O, reader_param_type & patfile) const
			{
				::libmaus2::fastx::SingleWordDNABitBuffer forw(hashlen);
				::libmaus2::fastx::SingleWordDNABitBuffer reve(hashlen);
		
				typename reader_param_type::pattern_type pattern;
		
				while ( patfile.getNextPatternUnlocked(pattern) )
					handleReadHistogram(
						pattern.pattern,
						pattern.getPatternLength(),
						forw,reve,O
					);
			}

			::libmaus2::autoarray::AutoArray < std::pair<uint64_t, uint64_t> > getHashIntervals(
				uint64_t const maxmem,
				uint64_t const numthreads,
				::libmaus2::autoarray::AutoArray<uint64_t> const & O
			) const
			{
				uint64_t const bytesperkmerdata = multi_word_buffer_type::getNumberOfBufferBytes(k);
				uint64_t const bytesperkmer = bytesperkmerdata + sizeof(uint64_t);
				// uint64_t const maxinternalkmermem = 100*(1024ull*1024ull*1024ull);
				uint64_t const maxinternalkmermem = maxmem;
				uint64_t const maxmemperthread = maxinternalkmermem / numthreads;
		
				std::vector < std::pair<uint64_t, uint64_t> > V;
		
				uint64_t hashlow = 0;
				uint64_t hashhigh = 0;
		
				while ( hashlow < O.getN() )
				{
					uint64_t memused = 0;
					uint64_t numkmersproc = 0;
		
					memused += bytesperkmer * O[hashhigh] + sizeof(uint64_t);
					numkmersproc += O[hashhigh];
					hashhigh += 1;
		
					while ( hashhigh < O.getN() && 
						((memused + bytesperkmer * O[hashhigh] + sizeof(uint64_t)) < maxmemperthread) )
					{
						memused += bytesperkmer * O[hashhigh] + sizeof(uint64_t);
						numkmersproc += O[hashhigh];
						hashhigh++;
					}
		
					V.push_back( std::pair<uint64_t, uint64_t>(hashlow,hashhigh) );
		
					hashlow = hashhigh;
				}
				
				::libmaus2::autoarray::AutoArray <  std::pair<uint64_t, uint64_t> > A(V.size());
				std::copy ( V.begin(), V.end(), A.get() );
		
				return A;
			}
		
			template<typename pattern_type, typename output_file_array_type, bool report_id>
			inline void handlePatternPresortFiles(
				multi_word_buffer_type & forw,
				multi_word_buffer_type & reve,
				pattern_type const & pattern,
				output_file_array_type & OF,
				uint64_t const hlow,
				uint64_t const hhigh
				) const
			{
				unsigned int const l = pattern.getPatternLength();
		
				if ( l < k )
				{
					// std::cerr << "Read of length " << l << " is too short." << std::endl;
					return;
				}
		
				forw.reset();
				reve.reset();
		
				unsigned int const localk = std::min(k, l);
		
				char const * sequence = pattern.pattern;
				char const * fsequence = sequence;
				char const * rsequence = (sequence + localk);
		
				unsigned int e = 0;
				for ( unsigned int i = 0; i < localk; ++i )
				{
					char const base = *(sequence++);
					e += E [ base ];
					forw.pushBack( S [ base ]  );
		
					char const rbase = *(--rsequence);
					reve.pushBack( R [ rbase ] );
				}
		
				rsequence = pattern.pattern + localk;
		
				for ( unsigned int z = 0; z < l-localk+1; )
				{
					if ( e < 1 )
					{
						uint64_t const fval = forw.getFrontSymbols(hashlen);
						uint64_t const rval = reve.getFrontSymbols(hashlen);
											
						if ( rval < fval )
						{
							uint64_t const rhash = rval >> hashshift;
							assert ( rhash < hashtablesize );
		
							if ( rhash >= hlow && rhash < hhigh )
							{
								typename output_file_array_type::buffer_type & O =
									OF.getBuffer(rhash);
			
								for ( unsigned int j = 0; j < reve.buffers.getN(); ++j )
									O.put(reve.buffers[j]);
								if ( report_id )
									O.put(pattern.getPatID());
								
								#if 0
								for ( uint64_t j = 0; j < reve.buffers.getN(); ++j )
									std::cerr << rhash << " " << j  << " " << reve.buffers[j] << std::endl;
								#endif
							}
						}
						else
						{
							uint64_t const fhash = fval >> hashshift;
							assert ( fhash < hashtablesize );
		
							if ( fhash >= hlow && fhash < hhigh )
							{
								typename output_file_array_type::buffer_type & O =
									OF.getBuffer(fhash);
			
								for ( unsigned int j = 0; j < forw.buffers.getN(); ++j )
									O.put(forw.buffers[j]);
								if ( report_id )
									O.put(pattern.getPatID());

								#if 0
								for ( uint64_t j = 0; j < forw.buffers.getN(); ++j )
									std::cerr << fhash << " " << j << " " << forw.buffers[j] << std::endl;
								#endif
							}
						}
		
						#if 0
						std::cerr << "P" << pattern.pattern << std::endl;
		
						std::cerr << "F";
						for ( unsigned int i = 0; i < z; ++i )
							std::cerr << " ";
						std::cerr << decodeBuffer(forw) << std::endl;
		
						std::cerr << "R";
						for ( unsigned int i = 0; i < z; ++i )
							std::cerr << " ";
						std::cerr << decodeReverseBuffer(reve) << std::endl;
						
						assert ( decodeBuffer(forw) == decodeReverseBuffer(reve) );
						assert ( decodeBuffer(forw) == pattern.spattern.substr(z,k) );
						#endif
					}
		
					if ( ++z < l-localk+1 )
					{
						e -= E[*(fsequence++)];
		
						char const base = *(sequence++);
						forw.pushBack( S[base] );
						reve.pushFront( R[base] );
		
						e += E[base];
					}
				}
			}
		};
		
		template<typename reader_type>
		struct HashCreation : public HashCreationTables
		{
			typedef HashCreationTables base_type;
		
			/* ---------------------------------------------------------------- */

			HashCreation(std::istream & in) : HashCreationTables(in) {}			
			HashCreation(unsigned int const rk) : HashCreationTables(rk) {}

			static void computeReadLengthDistributionBlock(
				::libmaus2::fastx::FastInterval const & FI,
				std::vector<std::string> const & filenames,
				::std::map<uint64_t,uint64_t> & high
				)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> low(1024);
		
				reader_type patfile(filenames, FI);
				typename reader_type::pattern_type pattern;
		
				while ( patfile.getNextPatternUnlocked(pattern) )
				{
					uint64_t const patlen = pattern.getPatternLength();
					if ( patlen < low.size() )
						low[patlen]++;
					else
						high[patlen]++;
				}
				
				for ( uint64_t i = 0; i < low.size(); ++i )
					if ( low[i] )
						high[i] += low[i];
			}

			void computeHashFrequencyBlock(
				::libmaus2::fastx::FastInterval const & FI,
				std::vector<std::string> const & filenames,
				uint64_t * const O
				) const
			{
				reader_type patfile(filenames, FI);
				base_type::computeHashFrequencyBlock(O,patfile);
			}
		
			::libmaus2::autoarray::AutoArray<uint64_t> computeHashFrequencies(
				std::vector<std::string> const & filenames,
				std::vector< ::libmaus2::fastx::FastInterval> const & V
				) const
			{
				// mutex for std err and histogram accumulation
				::libmaus2::parallel::OMPLock cerrlock;
				
				uint64_t const numthreads = getMaxThreads();
		
				// typedef typename reader_type::block_type block_type;
		
				typedef ::libmaus2::autoarray::AutoArray<uint64_t> hashtabletype;
				typedef hashtabletype::unique_ptr_type hashtableptrtype;
				::libmaus2::autoarray::AutoArray< hashtableptrtype > hashtables(numthreads);

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif				
				for ( int64_t h = 0; h < static_cast<int64_t>(numthreads); ++h )
				{
					hashtableptrtype thashtablesh(new hashtabletype(hashtablesize));
					hashtables[h] = UNIQUE_PTR_MOVE(thashtablesh);
				}
		
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( int64_t zz = 0; zz < static_cast<int64_t>(V.size()); ++zz )
				{
					::libmaus2::autoarray::AutoArray<uint64_t> & O = *(hashtables[getThreadNum()]);
		
					cerrlock.lock();
					std::cerr << "thread " << getThreadNum() << " got packet " << V[zz] << std::endl;
					cerrlock.unlock();
					
					computeHashFrequencyBlock(
						V[zz],filenames,O.get()
					);
				}
		
				for ( uint64_t h = 1; h < numthreads; ++h )
				{
					hashtabletype & O0 = *(hashtables[0]);
					hashtabletype & Oh = *(hashtables[h]);
					for ( uint64_t i = 0; i < hashtablesize; ++i )
						O0[i] += Oh[i];
				}
				
				hashtabletype OG = *(hashtables[0]);
		
				return OG;
			}
		
		
			inline void handleKmerBlock(
				uint64_t const * pp,
				uint64_t const numocc,
				uint64_t const wordsperkmer,
				::libmaus2::graph::TripleEdgeOutputSet & tos,
				uint64_t const kmeroccthres,
				uint64_t const * const readKillList
				) const
			{
				assert ( numocc );
				
				#if 0
				{
					multi_word_buffer_type forw(k);
					for ( uint64_t i = 0; i < wordsperkmer; ++i )
						forw.buffers[i] = pp[i];
					std::string const sforw = decodeBuffer(forw);
					std::cerr << "Handling block for kmer " 
						<< sforw << " occurence count " << numocc << std::endl;
				}
				#endif
				
				#if 0
				if ( numocc == 1 )
				{
					multi_word_buffer_type forw(k);
					for ( uint64_t i = 0; i < wordsperkmer; ++i )
						forw.buffers[i] = pp[i];
					std::string const sforw = decodeBuffer(forw);
					std::string const sreve = decodeReverseBuffer(forw);
					std::cerr << "single occurence forw=" << sforw << " reverse=" << sreve << std::endl;
				}
				#endif
		
				if ( numocc <= kmeroccthres )
				{
					uint64_t const * ppe = pp + numocc*(wordsperkmer+1);
			
					#if defined(KMERDEBUG)
					std::map<uint64_t,uint64_t> debmap0;
					std::map<uint64_t,uint64_t> debmap1;
			
					uint64_t const * debpp = pp;
					for ( uint64_t i = 0; i < numocc; ++i )
					{
						debpp += wordsperkmer;
						uint64_t const curid = *(debpp++);
						debmap0[curid]++;
					}
			
					std::vector< Triple > debvec0;
					std::vector< Triple > debvec1;
					for ( 
						std::map<uint64_t,uint64_t>::const_iterator ita = debmap0.begin(); 
						ita != debmap0.end(); 
						++ita )
					{
						std::map<uint64_t,uint64_t>::const_iterator tita = ita;
						tita++;
			
						for ( ; tita != debmap0.end(); ++tita )
						{
							debvec0.push_back (
								Triple(ita->first,tita->first,std::min(ita->second,tita->second)) );
						}
					}
					#endif
			
					pp += wordsperkmer;
					uint64_t curid = *(pp++);
					uint64_t numcur = 1;
			
					for ( uint64_t i = 1; i < numocc; ++i )
					{
						pp += wordsperkmer;
						uint64_t const nextid =	*(pp++);
			
						if ( nextid != curid )
						{
							#if defined(KMERDEBUG)
							debmap1[curid] = numcur;
							assert ( debmap0.find(curid) != debmap0.end() );
							assert ( debmap0.find(curid)->second = numcur );
							#endif
			
							uint64_t numsec = 1;
							uint64_t secid = nextid;
							uint64_t const * secptr = pp; 
			
							while ( secptr != ppe )
							{
								secptr += wordsperkmer;
								uint64_t const nextsecid = *(secptr++);
			
								if ( nextsecid != secid )
								{
									bool const curlive = 
										!::libmaus2::bitio::getBit ( readKillList , curid ); 
									bool const seclive = 
										!::libmaus2::bitio::getBit ( readKillList , secid ); 

									if ( curlive && seclive )
									{
										::libmaus2::graph::TripleEdge
											trip(curid,secid,std::min(numcur,numsec));
										tos.write(trip);
									}
		
									#if defined(KMERDEBUG)
									assert ( debmap0.find(secid) != debmap0.end() );
									assert ( debmap0.find(secid)->second == numsec );
			
									debvec1.push_back(trip);
									#endif
			
									secid = nextsecid;
									numsec = 1;
								}
								else
								{
									numsec++;
								}
							}
							{
								bool const curlive = 
									!::libmaus2::bitio::getBit ( readKillList , curid ); 
								bool const seclive = 
									!::libmaus2::bitio::getBit ( readKillList , secid ); 

								if ( curlive && seclive )
								{
									::libmaus2::graph::TripleEdge 
										trip(curid,secid,std::min(numcur,numsec));
									tos.write(trip);
								}	

								#if defined(KMERDEBUG)
								assert ( debmap0.find(secid) != debmap0.end() );
								assert ( debmap0.find(secid)->second == numsec );
			
								debvec1.push_back(trip);
								#endif
							}
			
							curid = nextid;
							numcur = 1;
						}
						else
						{
							numcur++;
						}
					}
					{
						#if defined(KMERDEBUG)
						debmap1[curid] = numcur;
						#endif
					}
			
					#if defined(KMERDEBUG)
					assert ( debmap0 == debmap1 );
					assert ( debvec0 == debvec1 );
					#endif
				}
			}

			inline void handleKmerBlock(
				uint64_t const * pp,
				uint64_t const numocc,
				uint64_t const wordsperkmer,
				::libmaus2::graph::TripleEdgeOutputSet & tos,
				uint64_t const kmeroccthres
				) const
			{
				assert ( numocc );
				
				#if 0
				{
					multi_word_buffer_type forw(k);
					for ( uint64_t i = 0; i < wordsperkmer; ++i )
						forw.buffers[i] = pp[i];
					std::string const sforw = decodeBuffer(forw);
					std::cerr << "Handling block for kmer " 
						<< sforw << " occurence count " << numocc << std::endl;
				}
				#endif
				
				#if 0
				if ( numocc == 1 )
				{
					multi_word_buffer_type forw(k);
					for ( uint64_t i = 0; i < wordsperkmer; ++i )
						forw.buffers[i] = pp[i];
					std::string const sforw = decodeBuffer(forw);
					std::string const sreve = decodeReverseBuffer(forw);
					std::cerr << "single occurence forw=" << sforw << " reverse=" << sreve << std::endl;
				}
				#endif
		
				if ( numocc <= kmeroccthres )
				{
					uint64_t const * ppe = pp + numocc*(wordsperkmer+1);
			
					#if defined(KMERDEBUG)
					std::map<uint64_t,uint64_t> debmap0;
					std::map<uint64_t,uint64_t> debmap1;
			
					uint64_t const * debpp = pp;
					for ( uint64_t i = 0; i < numocc; ++i )
					{
						debpp += wordsperkmer;
						uint64_t const curid = *(debpp++);
						debmap0[curid]++;
					}
			
					std::vector< Triple > debvec0;
					std::vector< Triple > debvec1;
					for ( 
						std::map<uint64_t,uint64_t>::const_iterator ita = debmap0.begin(); 
						ita != debmap0.end(); 
						++ita )
					{
						std::map<uint64_t,uint64_t>::const_iterator tita = ita;
						tita++;
			
						for ( ; tita != debmap0.end(); ++tita )
						{
							debvec0.push_back (
								Triple(ita->first,tita->first,std::min(ita->second,tita->second)) );
						}
					}
					#endif
			
					pp += wordsperkmer;
					uint64_t curid = *(pp++);
					uint64_t numcur = 1;
			
					for ( uint64_t i = 1; i < numocc; ++i )
					{
						pp += wordsperkmer;
						uint64_t const nextid =	*(pp++);
			
						if ( nextid != curid )
						{
							#if defined(KMERDEBUG)
							debmap1[curid] = numcur;
							assert ( debmap0.find(curid) != debmap0.end() );
							assert ( debmap0.find(curid)->second = numcur );
							#endif
			
							uint64_t numsec = 1;
							uint64_t secid = nextid;
							uint64_t const * secptr = pp; 
			
							while ( secptr != ppe )
							{
								secptr += wordsperkmer;
								uint64_t const nextsecid = *(secptr++);
			
								if ( nextsecid != secid )
								{
									::libmaus2::graph::TripleEdge tripforw(curid,secid,std::min(numcur,numsec));
									tos.write(tripforw);
									::libmaus2::graph::TripleEdge tripreve(secid,curid,std::min(numcur,numsec));
									tos.write(tripreve);
		
									#if defined(KMERDEBUG)
									assert ( debmap0.find(secid) != debmap0.end() );
									assert ( debmap0.find(secid)->second == numsec );
			
									debvec1.push_back(trip);
									#endif
			
									secid = nextsecid;
									numsec = 1;
								}
								else
								{
									numsec++;
								}
							}
							{
								::libmaus2::graph::TripleEdge trip(curid,secid,std::min(numcur,numsec));
								tos.write(trip);
								::libmaus2::graph::TripleEdge tripreve(secid,curid,std::min(numcur,numsec));
								tos.write(tripreve);

								#if defined(KMERDEBUG)
								assert ( debmap0.find(secid) != debmap0.end() );
								assert ( debmap0.find(secid)->second == numsec );
			
								debvec1.push_back(trip);
								#endif
							}
			
							curid = nextid;
							numcur = 1;
						}
						else
						{
							numcur++;
						}
					}
					{
						#if defined(KMERDEBUG)
						debmap1[curid] = numcur;
						#endif
					}
			
					#if defined(KMERDEBUG)
					assert ( debmap0 == debmap1 );
					assert ( debvec0 == debvec1 );
					#endif
				}
			}

			template<typename iterator_type, typename callback_type>
			static inline void handleKmerBlockIdOnlyCallback(
				iterator_type pp,
				uint64_t const numocc,
				callback_type & cb,
				uint64_t const kmeroccthres
				)
			{
				assert ( numocc );
				
				if ( numocc <= kmeroccthres )
				{
					iterator_type ppe = pp + numocc;
			
					uint64_t curid = *(pp++);
					uint64_t numcur = 1;
			
					/* iterate over occurences */
					for ( uint64_t i = 1; i < numocc; ++i )
					{
						/* next id */
						uint64_t const nextid =	*(pp++);
			
						/* id is different */
						if ( nextid != curid )
						{
							/* initialize secondary loop */
							uint64_t numsec = 1;
							uint64_t secid = nextid;
							iterator_type secptr = pp; 
			
							while ( secptr != ppe )
							{
								uint64_t const nextsecid = *(secptr++);
			
								if ( nextsecid != secid )
								{
									::libmaus2::graph::TripleEdge tripforw(curid,secid,std::min(numcur,numsec));
									cb(tripforw);
									::libmaus2::graph::TripleEdge tripreve(secid,curid,std::min(numcur,numsec));
									cb(tripreve);
		
									secid = nextsecid;
									numsec = 1;
								}
								else
								{
									numsec++;
								}
							}
							{
								::libmaus2::graph::TripleEdge tripforw(curid,secid,std::min(numcur,numsec));
								cb(tripforw);
								::libmaus2::graph::TripleEdge tripreve(secid,curid,std::min(numcur,numsec));
								cb(tripreve);
							}
			
							curid = nextid;
							numcur = 1;
						}
						else
						{
							numcur++;
						}
					}
				}
			}

			template<typename iterator_type>
			static inline void handleKmerBlockIdOnly(
				iterator_type pp,
				uint64_t const numocc,
				::libmaus2::graph::TripleEdgeOutputSet & tos,
				uint64_t const kmeroccthres
				)
			{
				assert ( numocc );
				
				if ( numocc <= kmeroccthres )
				{
					iterator_type ppe = pp + numocc;
			
					uint64_t curid = *(pp++);
					uint64_t numcur = 1;
			
					/* iterate over occurences */
					for ( uint64_t i = 1; i < numocc; ++i )
					{
						/* next id */
						uint64_t const nextid =	*(pp++);
			
						/* id is different */
						if ( nextid != curid )
						{
							/* initialize secondary loop */
							uint64_t numsec = 1;
							uint64_t secid = nextid;
							iterator_type secptr = pp; 
			
							while ( secptr != ppe )
							{
								uint64_t const nextsecid = *(secptr++);
			
								if ( nextsecid != secid )
								{
									::libmaus2::graph::TripleEdge tripforw(curid,secid,std::min(numcur,numsec));
									tos.write(tripforw);
									::libmaus2::graph::TripleEdge tripreve(secid,curid,std::min(numcur,numsec));
									tos.write(tripreve);
		
									secid = nextsecid;
									numsec = 1;
								}
								else
								{
									numsec++;
								}
							}
							{
								::libmaus2::graph::TripleEdge trip(curid,secid,std::min(numcur,numsec));
								tos.write(trip);
								::libmaus2::graph::TripleEdge tripreve(secid,curid,std::min(numcur,numsec));
								tos.write(tripreve);
							}
			
							curid = nextid;
							numcur = 1;
						}
						else
						{
							numcur++;
						}
					}
				}
			}
			
			template<typename reader_param_type, typename output_file_array_type>
			void computeUnsortedKmerFileBlock(
				reader_param_type & patfile,
				output_file_array_type & OBA,
				uint64_t const hlow = 0,
				uint64_t const hhigh = std::numeric_limits<uint64_t>::max(),
				bool report_id = true
				) const
			{
				typedef typename reader_param_type::pattern_type pattern_type;
				pattern_type pattern;
				multi_word_buffer_type forw(k);
				multi_word_buffer_type reve(k);
		
				if ( report_id )
					while ( patfile.getNextPatternUnlocked(pattern) )
						handlePatternPresortFiles<pattern_type,output_file_array_type,true>(forw,reve,pattern,OBA,hlow,hhigh);
				else
					while ( patfile.getNextPatternUnlocked(pattern) )
						handlePatternPresortFiles<pattern_type,output_file_array_type,false>(forw,reve,pattern,OBA,hlow,hhigh);				
			}

			template<typename output_file_array_type>
			void computeUnsortedKmerFileBlock(
				::std::vector<std::string> const & filenames,
				::libmaus2::fastx::FastInterval const & FI,
				output_file_array_type & OBA,
				uint64_t const hlow = 0,
				uint64_t const hhigh = std::numeric_limits<uint64_t>::max()
				) const
			{
				reader_type patfile(filenames, FI);		
				computeUnsortedKmerFileBlock(patfile,OBA,hlow,hhigh);
			}
					
			std::vector < std::vector < std::string > > computeUnsortedKmerFiles(
				std::string const & tempfileprefix,
				::libmaus2::util::TempFileNameGenerator * rtmpgen,
				::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t > > const & HI,
				std::vector< ::libmaus2::fastx::FastInterval> const & V,
				std::vector<std::string> const & filenames
				) const
			{
				uint64_t const numthreads = getMaxThreads();
			
				::libmaus2::parallel::OMPLock cerrlock;
		
				typedef ::libmaus2::aio::SynchronousOutputFile8Array output_file_array_type;
				::libmaus2::autoarray::AutoArray < output_file_array_type::unique_ptr_type > bufferarrays(numthreads); 
				
				::libmaus2::util::TempFileNameGenerator::unique_ptr_type ptmpgen;
				::libmaus2::util::TempFileNameGenerator * tmpgen = 0;
				
				if ( ! rtmpgen )
				{
					::libmaus2::util::TempFileNameGenerator::unique_ptr_type tptmpgen(
                                                new ::libmaus2::util::TempFileNameGenerator(tempfileprefix + "_tmpdir" , 4 ));
					ptmpgen = UNIQUE_PTR_MOVE(tptmpgen);
					tmpgen = ptmpgen.get();
				}
				else
				{
					tmpgen = rtmpgen;
				}

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(numthreads); ++i )
				{
					output_file_array_type::unique_ptr_type tbufferarraysi(
                                                new output_file_array_type(HI,*tmpgen));
					bufferarrays[i] = UNIQUE_PTR_MOVE(tbufferarraysi);
				}
		
				/**
				 * static scheduling is necessary for Phusion2 compatibility
				 **/
				#if defined(_OPENMP)
				// #pragma omp parallel for schedule(dynamic,1)
				#pragma omp parallel for schedule(static)
				#endif
				for ( int64_t zz = 0; zz < static_cast<int64_t>(V.size()); ++zz )
				{
		
					cerrlock.lock();
					std::cerr << "{" << getThreadNum() << "}" << " writing kmers for interval " << V[zz] << std::endl;
					cerrlock.unlock();
		
					computeUnsortedKmerFileBlock(filenames,V[zz],*(bufferarrays[getThreadNum()]));

					cerrlock.lock();
					std::cerr << "{" << getThreadNum() << "}" << " wrote kmers for interval " << V[zz] << std::endl;
					cerrlock.unlock(); 
				}
		
				for ( int64_t i = 0; i < static_cast<int64_t>(bufferarrays.getN()); ++i )
					bufferarrays[i]->flush();
		
				std::vector < std::vector < std::string > > kmerfiles;
		
				for ( uint64_t h = 0; h < HI.size(); ++h )
				{
					std::vector < std::string > hfiles;
		
					for ( uint64_t t = 0; t < bufferarrays.getN(); ++t )
						hfiles.push_back(bufferarrays[t]->filenames[h]);
		
					kmerfiles.push_back(hfiles);
				}
		
				for ( uint64_t i = 0; i < numthreads; ++i )
					bufferarrays[i].reset();
				
				return kmerfiles;
			}

			static uint64_t getKmerOccThres(std::map<std::string,std::string> const & params)
			{
				std::map<std::string,std::string>::const_iterator it = params.find("kmeroccthres");
				
				if ( it == params.end() )
					return 28;
				else
				{
					std::string const & sval = it->second;
					std::istringstream istr(sval);
					uint64_t kmeroccthres;
					istr >> kmeroccthres;
					
					if ( ! istr )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Unable to parse " << sval << " as kmer occurence threshold.";
						se.finish();
						throw se;
					}
					
					return kmeroccthres;
				}
			}

			static uint64_t getComponentSizeThreshold(std::map<std::string,std::string> const & params)
			{
				std::map<std::string,std::string>::const_iterator it = params.find("compsizethres");
				
				if ( it == params.end() )
					return 10;
				else
				{
					std::string const & sval = it->second;
					std::istringstream istr(sval);
					uint64_t compsizethres;
					istr >> compsizethres;
					
					if ( ! istr )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Unable to parse " << sval << " as component size threshold.";
						se.finish();
						throw se;
					}
					
					return compsizethres;
				}
			}

			static uint64_t getEdgeWeightThreshold(std::map<std::string,std::string> const & params)
			{
				std::map<std::string,std::string>::const_iterator it = params.find("edgeweightthres");
				
				if ( it == params.end() )
					return 5;
				else
				{
					std::string const & sval = it->second;
					std::istringstream istr(sval);
					uint64_t edgeweightthres;
					istr >> edgeweightthres;
					
					if ( ! istr )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Unable to parse " << sval << " as edge weight threshold.";
						se.finish();
						throw se;
					}
					
					return edgeweightthres;
				}
			}

		};
	}
}
#endif
