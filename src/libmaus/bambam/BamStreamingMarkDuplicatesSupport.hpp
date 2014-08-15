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
#if ! defined(LIBMAUS_BAMBAM_BAMSTREAMINGMARKDUPLICATESSUPPORT_HPP)
#define LIBMAUS_BAMBAM_BAMSTREAMINGMARKDUPLICATESSUPPORT_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamAlignmentDecoder.hpp>
#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/lru/SparseLRUFileBunch.hpp>
#include <libmaus/math/iabs.hpp>
#include <libmaus/sorting/SortingBufferedOutputFile.hpp>
#include <libmaus/uint/uint.hpp>
#include <libmaus/util/FreeList.hpp>
#include <libmaus/util/GrowingFreeList.hpp>

#include <stdint.h>

#if defined(INT32_MIN)
#define LIBMAUS_BAMSTREAMINGMARKDUPLICATESSUPPORT_INT32_MIN INT32_MIN
#else
#define LIBMAUS_BAMSTREAMINGMARKDUPLICATESSUPPORT_INT32_MIN (-2147483647-1)
#endif

namespace libmaus
{
	namespace bambam
	{
		struct BamStreamingMarkDuplicatesSupport
		{
			struct SignCoding
			{
				static int64_t const signshift = LIBMAUS_BAMSTREAMINGMARKDUPLICATESSUPPORT_INT32_MIN;

				static uint32_t signEncode(int32_t const coord)
				{
					return static_cast<int64_t>(coord)-signshift;
				}
				
				static int32_t signDecode(uint32_t const coord)
				{
					return static_cast<int64_t>(coord)+signshift;
				}
			};

			struct PairHashKeyType : public SignCoding
			{
				typedef PairHashKeyType this_type;

				typedef libmaus::uint::UInt<4> key_type;

				key_type key;

				enum pair_orientation_type { pair_orientaton_FF=0, pair_orientaton_FR=1, pair_orientaton_RF=2, pair_orientaton_RR=3 };
				
				PairHashKeyType() : key() {}
				
				PairHashKeyType(uint64_t const rkey[key_type::words])
				{
					std::copy(&rkey[0],&rkey[key_type::words],&key.A[0]);
				}
				
				PairHashKeyType(
					libmaus::bambam::BamAlignment const & algn,
					libmaus::bambam::BamHeader const & header,
					uint64_t tagid
				) : key()
				{
					int64_t const thisref = algn.getRefID();
					int64_t const thiscoord = algn.getCoordinate();
					int64_t const otherref = algn.getNextRefID();
					int64_t const othercoord = algn.getAuxAsNumber<int32_t>("MC");
					
					// is this the left mapping end?
					bool const isleft =
						(thisref < otherref) ||
						(thisref == otherref && thiscoord < othercoord ) ||
						(thisref == otherref && thiscoord == othercoord && algn.isRead1());
					
					// as number for hash key
					uint64_t leftflag = isleft ? 0 : 1;
					
					pair_orientation_type orientation;
					
					// orientation of end pair
					if ( isleft )
					{
						if ( ! algn.isReverse() )
						{
							if ( ! algn.isMateReverse() )
								orientation = pair_orientaton_FF;
							else
								orientation = pair_orientaton_FR;
						}
						else
						{
							if ( ! algn.isMateReverse() )
								orientation = pair_orientaton_RF;
							else
								orientation = pair_orientaton_RR;
						}
					}
					else
					{
						if ( ! algn.isMateReverse() )
						{
							if ( ! algn.isReverse() )
								orientation = pair_orientaton_FF;
							else
								orientation = pair_orientaton_FR;
						}
						else
						{
							if ( ! algn.isReverse() )
								orientation = pair_orientaton_RF;
							else
								orientation = pair_orientaton_RR;
						}
					}
					
					// orientation as number		
					uint64_t uorientation = static_cast<uint64_t>(orientation);

					key.A[0] = 
						(static_cast<uint64_t>(signEncode(thisref)) << 32)
						|
						(static_cast<uint64_t>(signEncode(thiscoord)) << 0)
						;
					key.A[1] = 
						(static_cast<uint64_t>(signEncode(otherref)) << 32)
						|
						(static_cast<uint64_t>(signEncode(othercoord)) << 0)
						;
					key.A[2] = 
						(static_cast<uint64_t>(signEncode(algn.getLibraryId(header))) << 32)
						|
						(leftflag) | (uorientation << 1)
						;
					key.A[3] = tagid; // tag
				}
				
				bool operator==(this_type const & o) const
				{
					return key == o.key;
				}

				int32_t getRefId() const
				{
					return signDecode(key.A[0] >> 32);
				}

				int32_t getCoord() const
				{
					return signDecode(key.A[0] & 0xFFFFFFFFUL);
				}
				
				int32_t getMateRefId() const
				{
					return signDecode(key.A[1] >> 32);
				}

				int32_t getMateCoord() const
				{
					return signDecode(key.A[1] & 0xFFFFFFFFUL);
				}

				int32_t getLibrary() const
				{
					return signDecode(key.A[2] >> 32);
				}

				pair_orientation_type getOrientation() const
				{
					return static_cast<pair_orientation_type>((key.A[1] >> 1) & 0x3);
				}

				int32_t getLeft() const
				{
					return (((key.A[2] & 0xFFFFFFFFUL) & 1) == 0);
				}

				uint64_t getTag() const
				{
					return key.A[3];
				}
			};

			struct PairHashKeyHeapComparator
			{
				bool operator()(PairHashKeyType const & A, PairHashKeyType const & B)
				{
					for ( unsigned int i = 0; i < A.key.words; ++i )
						if ( A.key.A[i] != B.key.A[i] )
							return A.key.A[i] > B.key.A[i];

					return false;
				}
			};

			struct PairHashKeyTypeHashFunction
			{
				size_t operator()(PairHashKeyType const & H) const
				{
					return libmaus::hashing::EvaHash::hash642(H.key.A,PairHashKeyType::key_type::words);
				}
			};

			struct FragmentHashKeyType : public SignCoding
			{
				typedef FragmentHashKeyType this_type;

				typedef libmaus::uint::UInt<3> key_type;

				key_type key;

				enum fragment_orientation_type { fragment_orientation_F=0, fragment_orientation_R = 1 };
				
				FragmentHashKeyType() : key() {}	

				FragmentHashKeyType(
					libmaus::bambam::BamAlignment const & algn,
					libmaus::bambam::BamHeader const & header,
					bool const usemate,
					uint64_t const tagid
				) : key()
				{
					if ( usemate )
					{
						int64_t const thisref = algn.getNextRefID();
						int64_t const thiscoord = algn.getAuxAsNumber<int32_t>("MC");
						fragment_orientation_type fragor = algn.isMateReverse() ? fragment_orientation_R : fragment_orientation_F;
						uint64_t const ufragor = static_cast<uint64_t>(fragor);
						key.A[0] = 
							(static_cast<uint64_t>(signEncode(thisref)) << 32)
							|
							(static_cast<uint64_t>(signEncode(thiscoord)) << 0)
							;
						key.A[1] = (static_cast<uint64_t>(signEncode(algn.getLibraryId(header))) << 32) | ufragor;
						key.A[2] = tagid; // tag		
					}
					else
					{
						int64_t const thisref = algn.getRefID();
						int64_t const thiscoord = algn.getCoordinate();
						fragment_orientation_type fragor = algn.isReverse() ? fragment_orientation_R : fragment_orientation_F;
						uint64_t const ufragor = static_cast<uint64_t>(fragor);
						key.A[0] = 
							(static_cast<uint64_t>(signEncode(thisref)) << 32)
							|
							(static_cast<uint64_t>(signEncode(thiscoord)) << 0)
							;
						key.A[1] = (static_cast<uint64_t>(signEncode(algn.getLibraryId(header))) << 32) | ufragor;
						key.A[2] = tagid; // tag
					}
				}

				bool operator==(this_type const & o) const
				{
					return key == o.key;
				}

				int32_t getRefId() const
				{
					return signDecode(key.A[0] >> 32);
				}

				int32_t getCoord() const
				{
					return signDecode(key.A[0] & 0xFFFFFFFFUL);
				}

				int32_t getLibrary() const
				{
					return signDecode(key.A[1] >> 32);
				}

				fragment_orientation_type getOrientation() const
				{
					return static_cast<fragment_orientation_type>(key.A[1] & 0x1);
				}

				uint64_t getTag() const
				{
					return key.A[2];
				}
			};

			struct FragmentHashKeyHeapComparator
			{
				bool operator()(FragmentHashKeyType const & A, FragmentHashKeyType const & B)
				{
					for ( unsigned int i = 0; i < A.key.words; ++i )
						if ( A.key.A[i] != B.key.A[i] )
							return A.key.A[i] > B.key.A[i];

					return false;
				}

			};

			struct FragmentHashKeyTypeHashFunction
			{
				size_t operator()(FragmentHashKeyType const & H) const
				{
					return libmaus::hashing::EvaHash::hash642(H.key.A,FragmentHashKeyType::key_type::words);
				}
			};

			struct OutputQueueOrder
			{
				bool operator()(
					std::pair<uint64_t,libmaus::bambam::BamAlignment *> const & A, 
					std::pair<uint64_t,libmaus::bambam::BamAlignment *> const & B 
				)
				{
					return A.first > B.first;
				}
			};


			template<typename _writer_type>
			struct OutputQueue
			{
				typedef _writer_type writer_type;
				typedef OutputQueue<writer_type> this_type;

				writer_type & wr;

				libmaus::util::GrowingFreeList<libmaus::bambam::BamAlignment> & BAFL;	
				
				OutputQueueOrder const order;

				std::priority_queue<
					std::pair<uint64_t,libmaus::bambam::BamAlignment *>,
					std::vector< std::pair<uint64_t,libmaus::bambam::BamAlignment *> >,
					OutputQueueOrder
				> OQ;
					
				int64_t nextout;

				uint64_t const entriespertmpfile;

				libmaus::bambam::BamAuxFilterVector tagfilter;

				libmaus::autoarray::AutoArray<libmaus::bambam::BamAlignment *> OL;
				uint64_t olsizefill;
				
				std::string const tmpfileprefix;
				
				std::map<uint64_t,uint64_t> tmpFileFill;
				
				libmaus::lru::SparseLRUFileBunch reorderfiles;

				OutputQueue(
					writer_type & rwr,
					libmaus::util::GrowingFreeList<libmaus::bambam::BamAlignment> & rBAFL,	
					std::string const & rtmpfileprefix,
					std::vector<std::string> const & filtertagvec,
					uint64_t const rentriespertmpfile = 16*1024
				) 
				: wr(rwr), BAFL(rBAFL), order(), OQ(), nextout(0), 
				  entriespertmpfile(rentriespertmpfile), 
				  OL(entriespertmpfile,false), olsizefill(0), tmpfileprefix(rtmpfileprefix),
				  reorderfiles(tmpfileprefix,16)
				{
					tagfilter.set('Z','R');
					for ( uint64_t i = 0; i < filtertagvec.size(); ++i )
						tagfilter.set(filtertagvec[i]);
				}
				
				// flush output list
				void flushOutputList()
				{
					for ( uint64_t i = 0; i < olsizefill; ++i )
					{
						libmaus::bambam::BamAlignment * palgn = OL[i];
						palgn->filterOutAux(tagfilter);
						wr.writeAlignment(*palgn);
						BAFL.put(palgn);
					}
					
					olsizefill = 0;
				}
				
				struct BamAlignmentRankComparator
				{
					bool operator()(
						std::pair<uint64_t,libmaus::bambam::BamAlignment *> const & A, 
						std::pair<uint64_t,libmaus::bambam::BamAlignment *> const & B)
					{
						return A.first < B.first;
					}
				};
				
				void flushTempFile(uint64_t const tmpfileindex)
				{
					libmaus::aio::CheckedInputOutputStream & CIOS = reorderfiles[tmpfileindex];
					CIOS.flush();
					CIOS.clear();
					CIOS.seekg(0,std::ios::beg);
					CIOS.clear();
					
					uint64_t const n = tmpFileFill.find(tmpfileindex)->second;
					libmaus::autoarray::AutoArray< std::pair<uint64_t,libmaus::bambam::BamAlignment *> > algns(n);
					
					for ( uint64_t i = 0; i < n; ++i )
					{
						algns[i].second = BAFL.get();
						libmaus::bambam::BamAlignmentDecoder::readAlignmentGz(CIOS,*(algns[i].second),0,false);
						algns[i].first = algns[i].second->getRank();
					}
					
					std::sort(
						algns.begin(),
						algns.begin()+n,
						BamAlignmentRankComparator()
					);

					for ( uint64_t i = 0; i < n; ++i )
					{
						algns[i].second->filterOutAux(tagfilter);
						wr.writeAlignment(*(algns[i].second));
						BAFL.put(algns[i].second);
					}
					
					nextout += n;
					
					reorderfiles.remove(tmpfileindex);
					tmpFileFill.erase(tmpFileFill.find(tmpfileindex));
					
					// std::cerr << "[V] flushed block " << tmpfileindex << std::endl;
				}
				
				void addTmpFileEntry(libmaus::bambam::BamAlignment * algn)
				{
					addTmpFileEntry(algn, algn->getRank() / entriespertmpfile);
				}
				
				void addTmpFileEntry(
					libmaus::bambam::BamAlignment * algn, uint64_t const tmpfileindex
				)
				{
					// make sure file does not exist before we start adding entries
					if ( tmpFileFill.find(tmpfileindex) == tmpFileFill.end() )
						reorderfiles.remove(tmpfileindex);
				
					libmaus::aio::CheckedInputOutputStream & CIOS = reorderfiles[tmpfileindex];
					algn->serialise(CIOS);
					BAFL.put(algn);
					tmpFileFill[tmpfileindex]++;
					
					// std::cerr << "[V] added to " << tmpfileindex << std::endl;
					
					while ( 
						tmpFileFill.size() 
						&&
						static_cast<uint64_t>(tmpFileFill.begin()->second) == static_cast<uint64_t>(entriespertmpfile)
						&&
						static_cast<uint64_t>(tmpFileFill.begin()->first * entriespertmpfile) == static_cast<uint64_t>(nextout)
					)
					{
						flushTempFile(tmpFileFill.begin()->first);
					}	
				}
				
				void outputListToTmpFiles()
				{
					for ( uint64_t i = 0; i < olsizefill; ++i )
						addTmpFileEntry(OL[i]);
					nextout -= olsizefill;
					olsizefill = 0;	
				}
				
				// flush reorder heap to output list
				void flushInMemQueueInternal()
				{
					while ( 
						OQ.size() && OQ.top().first == static_cast<uint64_t>(nextout)
					)
					{
						libmaus::bambam::BamAlignment * palgn = OQ.top().second;

						OL[olsizefill++] = palgn;
						if ( olsizefill == OL.size() )
							flushOutputList();
						
						nextout += 1;
						OQ.pop();
					}	
				}

				// add alignment
				void push(libmaus::bambam::BamAlignment * algn)
				{
					// tmp file it is assigned to
					uint64_t tmpfileindex;
					
					if ( tmpFileFill.size() && tmpFileFill.find(tmpfileindex=(algn->getRank() / entriespertmpfile)) != tmpFileFill.end() )
					{
						addTmpFileEntry(algn,tmpfileindex);			
					}
					else
					{
						int64_t const rank = algn->getRank();
						assert ( rank >= 0 );
						OQ.push(std::pair<uint64_t,libmaus::bambam::BamAlignment *>(rank,algn));
						flushInMemQueueInternal();
									
						if ( OQ.size() >= 32*1024 )
						{
							outputListToTmpFiles();
							
							while ( OQ.size() )
							{
								addTmpFileEntry(OQ.top().second);
								OQ.pop();
							}
						}
					}
				}
				
				void flush()
				{
					outputListToTmpFiles();

					std::vector<uint64_t> PQ;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = tmpFileFill.begin(); ita != tmpFileFill.end(); ++ita )
						PQ.push_back(ita->first);

					for ( uint64_t i = 0; i < PQ.size(); ++i )
						flushTempFile(PQ[i]);
				}
			};

			struct PairHashKeySimpleHashConstantsType
			{
				typedef PairHashKeyType::key_type key_type;

				PairHashKeyType const unusedValue;
				PairHashKeyType const deletedValue;
				
				static PairHashKeyType computeUnusedValue()
				{
					key_type U;
					key_type Ulow(std::numeric_limits<uint64_t>::max());
					
					for ( unsigned int i = 0; i < key_type::words; ++i )
					{
						U <<= 64;
						U |= Ulow;
					}
					
					PairHashKeyType PHKT;
					PHKT.key = U;
					
					return PHKT;
				}

				static PairHashKeyType computeDeletedValue()
				{
					// get full mask
					PairHashKeyType U = computeUnusedValue();
					// erase top bit
					U.key.setBit(key_type::words*64-1, 0);
					return U;
				}
							
				PairHashKeyType const & unused() const
				{
					return unusedValue;
				}

				PairHashKeyType const & deleted() const
				{
					return deletedValue;
				}

				bool isFree(PairHashKeyType const & v) const
				{
					return (v.key & deletedValue.key) == deletedValue.key;
				}
				
				bool isInUse(PairHashKeyType const & v) const
				{
					return !isFree(v);
				}
				
				PairHashKeySimpleHashConstantsType() 
				: unusedValue(computeUnusedValue()), deletedValue(computeDeletedValue()) {}
				virtual ~PairHashKeySimpleHashConstantsType() {}
			};

			struct PairHashKeySimpleHashComputeType
			{
				typedef PairHashKeyType key_type;
				
				inline static uint64_t hash(key_type const v)
				{
					return libmaus::hashing::EvaHash::hash642(&v.key.A[0],v.key.words);
				}
			};

			struct PairHashKeySimpleHashNumberCastType
			{
				typedef PairHashKeyType key_type;

				static uint64_t cast(key_type const & key)
				{
					return key.key.A[0];
				}
			};

			struct FragmentHashKeySimpleHashConstantsType
			{
				typedef FragmentHashKeyType::key_type key_type;

				FragmentHashKeyType const unusedValue;
				FragmentHashKeyType const deletedValue;
				
				static FragmentHashKeyType computeUnusedValue()
				{
					key_type U;
					key_type Ulow(std::numeric_limits<uint64_t>::max());
					
					for ( unsigned int i = 0; i < key_type::words; ++i )
					{
						U <<= 64;
						U |= Ulow;
					}
					
					FragmentHashKeyType PHKT;
					PHKT.key = U;
					
					return PHKT;
				}

				static FragmentHashKeyType computeDeletedValue()
				{
					// get full mask
					FragmentHashKeyType U = computeUnusedValue();
					// erase top bit
					U.key.setBit(key_type::words*64-1, 0);
					return U;
				}
							
				FragmentHashKeyType const & unused() const
				{
					return unusedValue;
				}

				FragmentHashKeyType const & deleted() const
				{
					return deletedValue;
				}

				bool isFree(FragmentHashKeyType const & v) const
				{
					return (v.key & deletedValue.key) == deletedValue.key;
				}
				
				bool isInUse(FragmentHashKeyType const & v) const
				{
					return !isFree(v);
				}
				
				FragmentHashKeySimpleHashConstantsType() 
				: unusedValue(computeUnusedValue()), deletedValue(computeDeletedValue()) {}
				virtual ~FragmentHashKeySimpleHashConstantsType() {}
			};

			struct FragmentHashKeySimpleHashComputeType
			{
				typedef FragmentHashKeyType key_type;

				inline static uint64_t hash(key_type const v)
				{
					return libmaus::hashing::EvaHash::hash642(&v.key.A[0],v.key.words);
				}
			};

			struct FragmentHashKeySimpleHashNumberCastType
			{
				typedef FragmentHashKeyType key_type;

				static uint64_t cast(key_type const & key)
				{
					return key.key.A[0];
				}
			};

			struct OpticalInfoListNode
			{
				OpticalInfoListNode * next;

				uint16_t readgroup;	
				uint16_t tile;
				uint32_t x;
				uint32_t y;
				
				OpticalInfoListNode()
				: next(0), readgroup(0), tile(0), x(0), y(0)
				{
				
				}
				
				OpticalInfoListNode(OpticalInfoListNode * rnext, uint16_t rreadgroup, uint16_t rtile, uint32_t rx, uint32_t ry)
				: next(rnext), readgroup(rreadgroup), tile(rtile), x(rx), y(ry)
				{
				
				}
				
				bool operator<(OpticalInfoListNode const & o) const
				{
					if ( readgroup != o.readgroup )
						return readgroup < o.readgroup;
					else if ( tile != o.tile )
						return tile < o.tile;
					else if ( x != o.x )
						return x < o.x;
					else
						return y < o.y;
				}
			};

			struct OpticalInfoListNodeComparator
			{
				bool operator()(OpticalInfoListNode const * A, OpticalInfoListNode const * B) const
				{
					return A->operator<(*B);
				}	
			};

			struct OpticalExternalInfoElement
			{
				uint64_t key[PairHashKeyType::key_type::words];
				uint16_t readgroup;
				uint16_t tile;
				uint32_t x;
				uint32_t y;
				
				OpticalExternalInfoElement()
				: readgroup(0), tile(0), x(0), y(0)
				{
					std::fill(&key[0],&key[PairHashKeyType::key_type::words],0);
				}
				
				OpticalExternalInfoElement(PairHashKeyType const & HK, uint16_t const rreadgroup, uint16_t const rtile, uint32_t const rx, uint32_t const ry)
				: readgroup(rreadgroup), tile(rtile), x(rx), y(ry)
				{
					for ( uint64_t i = 0; i < PairHashKeyType::key_type::words; ++i )
						key[i] = HK.key.A[i];
				}

				OpticalExternalInfoElement(PairHashKeyType const & HK, OpticalInfoListNode const & O)
				: readgroup(O.readgroup), tile(O.tile), x(O.x), y(O.y)
				{
					for ( uint64_t i = 0; i < PairHashKeyType::key_type::words; ++i )
						key[i] = HK.key.A[i];
				}
				
				bool operator<(OpticalExternalInfoElement const & o) const
				{
					for ( uint64_t i = 0; i < PairHashKeyType::key_type::words; ++i )
						if ( key[i] != o.key[i] )
							return key[i] < o.key[i];
					
					if ( readgroup != o.readgroup )
						return readgroup < o.readgroup;
					else if ( tile != o.tile )
						return tile < o.tile;
					else if ( x != o.x )
						return x < o.x;
					else
						return y < o.y;
				}
				
				bool operator==(OpticalExternalInfoElement const & o) const
				{
					for ( uint64_t i = 0; i < PairHashKeyType::key_type::words; ++i )
						if ( key[i] != o.key[i] )
							return false;
					
					if ( readgroup != o.readgroup )
						return false;
					else if ( tile != o.tile )
						return false;
					else if ( x != o.x )
						return false;
					else
						return y == o.y;
				}
				
				bool operator!=(OpticalExternalInfoElement const & o) const
				{
					return ! operator==(o);
				}

				bool sameTile(OpticalExternalInfoElement const & o) const
				{
					for ( uint64_t i = 0; i < PairHashKeyType::key_type::words; ++i )
						if ( key[i] != o.key[i] )
							return false;
					
					if ( readgroup != o.readgroup )
						return false;
					else 
						return tile == o.tile;
				}
			};

			struct OpticalInfoList
			{
				OpticalInfoListNode * optlist;
				bool expunged;
				
				OpticalInfoList() : optlist(0), expunged(false) {}
				
				static void markYLevel(
					OpticalInfoListNode ** const b,
					OpticalInfoListNode ** l,
					OpticalInfoListNode ** const t,
					bool * const B,
					unsigned int const optminpixeldif
				)
				{
					for ( OpticalInfoListNode ** c = l+1; c != t; ++c )
						if (
							libmaus::math::iabs(
								static_cast<int64_t>((*l)->y)
								-
								static_cast<int64_t>((*c)->y)
							)
							<= optminpixeldif
						)
						{
							B[c-b] = true;			
						}
				}
				
				static void markXLevel(
					OpticalInfoListNode ** const b,
					OpticalInfoListNode ** l,
					OpticalInfoListNode ** const t,
					bool * const B,
					unsigned int const optminpixeldif
				)
				{
					for ( ; l != t; ++l )
					{
						OpticalInfoListNode ** h = l+1;
						
						while ( (h != t) && ((*h)->x-(*l)->x <= optminpixeldif) )
							++h;
							
						markYLevel(b,l,h,B,optminpixeldif);
					}
				}

				static void markTileLevel(
					OpticalInfoListNode ** const b,
					OpticalInfoListNode ** l,
					OpticalInfoListNode ** const t,
					bool * const B,
					unsigned int const optminpixeldif
				)
				{
					while ( l != t )
					{
						OpticalInfoListNode ** h = l+1;
						
						while ( h != t && (*h)->tile == (*l)->tile )
							++h;
						
						markXLevel(b,l,h,B,optminpixeldif);
							
						l = h;
					}
				}

				static void markReadGroupLevel(
					OpticalInfoListNode ** l,
					OpticalInfoListNode ** const t,
					bool * const B,
					unsigned int const optminpixeldif
				)
				{
					OpticalInfoListNode ** const b = l;
				
					while ( l != t )
					{
						OpticalInfoListNode ** h = l+1;
						
						while ( h != t && (*h)->readgroup == (*l)->readgroup )
							++h;

						markTileLevel(b,l,h,B,optminpixeldif);
							
						l = h;
					}	
				}
				
				uint64_t countOpticalDuplicates(
					libmaus::autoarray::AutoArray<OpticalInfoListNode *> & A, 
					libmaus::autoarray::AutoArray<bool> & B, 
					unsigned int const optminpixeldif = 100
				)
				{
					uint64_t opt = 0;

					if ( ! expunged )
					{
						uint64_t n = 0;
						for ( OpticalInfoListNode * cur = optlist; cur; cur = cur->next )
							++n;
						if ( n > A.size() )
							A = libmaus::autoarray::AutoArray<OpticalInfoListNode *>(n);
						if ( n > B.size() )
							B = libmaus::autoarray::AutoArray<bool>(n);

						n = 0;
						for ( OpticalInfoListNode * cur = optlist; cur; cur = cur->next )
							A[n++] = cur;
							
						std::sort ( A.begin(), A.begin()+n, OpticalInfoListNodeComparator() );
						std::fill ( B.begin(), B.begin()+n, false);
						
						markReadGroupLevel(A.begin(),A.begin()+n,B.begin(),optminpixeldif);
						
						for ( uint64_t i = 0; i < n; ++i )
							opt += B[i];
					}
						
					return opt;
				}

				void addOpticalInfo(
					libmaus::bambam::BamAlignment const & algn,
					libmaus::util::FreeList<OpticalInfoListNode> & OILNFL,
					PairHashKeyType const & HK,
					libmaus::sorting::SortingBufferedOutputFile<OpticalExternalInfoElement> & optSBOF,
					libmaus::bambam::BamHeader const & header
				)
				{
					if ( HK.getLeft() )
					{
						uint16_t tile = 0;
						uint32_t x = 0, y = 0;
						if ( libmaus::bambam::ReadEndsBase::parseOptical(reinterpret_cast<uint8_t const *>(algn.getName()),tile,x,y) )
						{
							int64_t const rg = algn.getReadGroupId(header);
							OpticalInfoListNode const newnode(optlist,rg+1,tile,x,y);
							
							if ( expunged || OILNFL.empty() )
							{
								if ( optlist )
								{
									for ( OpticalInfoListNode * cur = optlist; cur; cur = cur->next )
									{
										OpticalExternalInfoElement E(HK,*cur);
										optSBOF.put(E);
									}
									deleteOpticalInfo(OILNFL);
								}
								
								expunged = true;

								OpticalExternalInfoElement E(HK,newnode);
								optSBOF.put(E);
							}
							else
							{
								OpticalInfoListNode * node = OILNFL.get();
								*node = newnode;
								optlist = node;
							}
						} 
					}
				}

				uint64_t deleteOpticalInfo(libmaus::util::FreeList<OpticalInfoListNode> & OILNFL)
				{
					OpticalInfoListNode * node = optlist;
					uint64_t n = 0;
					
					while ( node )
					{
						OpticalInfoListNode * next = node->next;
						OILNFL.put(node);
						node = next;
						++n;
					}
					
					optlist = 0;
							
					return n;			
				}
			};

			static std::ostream & printOpt(std::ostream & out, PairHashKeyType const & HK, uint64_t const opt)
			{
				bool const isleft = 
					(HK.getRefId()  < HK.getMateRefId()) ||
					(HK.getRefId() == HK.getMateRefId() && HK.getCoord() < HK.getMateCoord())
				;
				
				if ( isleft )
					std::cerr << "\nopt " 
						<< HK.getLibrary()+1 << " "
						<< HK.getRefId()+1 << " "
						<< HK.getCoord()+1 << " "
						<< HK.getMateRefId()+1 << " "
						<< HK.getMateCoord()+1 << " "
						<< opt
						<< std::endl;
				else
					std::cerr << "\nopt " 
						<< HK.getLibrary()+1 << " "
						<< HK.getMateRefId()+1 << " "
						<< HK.getMateCoord()+1 << " "
						<< HK.getRefId()+1 << " "
						<< HK.getCoord()+1 << " "
						<< opt
						<< std::endl;

				return out;
			}
		};

		std::ostream & operator<<(
			std::ostream & out, 
			BamStreamingMarkDuplicatesSupport::PairHashKeyType::pair_orientation_type const & ori
		);
		std::ostream & operator<<(std::ostream & ostr, BamStreamingMarkDuplicatesSupport::PairHashKeyType const & H);
		std::ostream & operator<<(
			std::ostream & out, 
			BamStreamingMarkDuplicatesSupport::FragmentHashKeyType::fragment_orientation_type const & ori
		);
		std::ostream & operator<<(std::ostream & ostr, BamStreamingMarkDuplicatesSupport::FragmentHashKeyType const & H);
		std::ostream & operator<<(std::ostream & out, BamStreamingMarkDuplicatesSupport::OpticalInfoListNode const & O);
		std::ostream & operator<<(std::ostream & out, BamStreamingMarkDuplicatesSupport::OpticalExternalInfoElement const & O);
	}
}
#endif
