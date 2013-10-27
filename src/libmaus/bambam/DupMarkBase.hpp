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
#if ! defined(LIBMAUS_BAMBAM_DUPMARKBASE_HPP)
#define LIBMAUS_BAMBAM_DUPMARKBASE_HPP

#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/DupSetCallback.hpp>
#include <libmaus/math/iabs.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct DupMarkBase
		{
			// #define DEBUG
			
			#if defined(DEBUG)
			#define MARKDUPLICATEPAIRSDEBUG
			#define MARKDUPLICATEFRAGSDEBUG
			#define MARKDUPLICATECOPYALIGNMENTS
			#endif

			struct MarkDuplicateProjectorIdentity
			{
				static libmaus::bambam::ReadEnds const & deref(libmaus::bambam::ReadEnds const & object)
				{
					return object;
				}
			};

			struct MarkDuplicateProjectorPointerDereference
			{
				static libmaus::bambam::ReadEnds const & deref(libmaus::bambam::ReadEnds const * object)
				{
					return *object;
				}
			};

			template<typename iterator, typename projector = MarkDuplicateProjectorIdentity>
			static uint64_t markDuplicatePairs(
				iterator const lfrags_a,
				iterator const lfrags_e,
				::libmaus::bambam::DupSetCallback & DSC,
				unsigned int const optminpixeldif = 100
			)
			{
				if ( lfrags_e-lfrags_a > 1 )
				{
					#if defined(MARKDUPLICATEPAIRSDEBUG)
					std::cerr << "[V] --- checking for duplicate pairs ---" << std::endl;
					for ( iterator lfrags_c = lfrags_a; lfrags_c != lfrags_e; ++lfrags_c )
						std::cerr << "[V] " << projector::deref(*lfrags_c) << std::endl;
					#endif
				
					uint64_t maxscore = projector::deref(*lfrags_a).getScore();
					
					iterator lfrags_m = lfrags_a;
					for ( iterator lfrags_c = lfrags_a+1; lfrags_c != lfrags_e; ++lfrags_c )
						if ( projector::deref(*lfrags_c).getScore() > maxscore )
						{
							maxscore = projector::deref(*lfrags_c).getScore();
							lfrags_m = lfrags_c;
						}

					for ( iterator lfrags_c = lfrags_a; lfrags_c != lfrags_e; ++lfrags_c )
						if ( lfrags_c != lfrags_m )
							DSC(projector::deref(*lfrags_c));
				
					// check for optical duplicates
					std::sort ( lfrags_a, lfrags_e, ::libmaus::bambam::OpticalComparator() );
					
					for ( iterator low = lfrags_a; low != lfrags_e; )
					{
						iterator high = low+1;
						
						// search top end of tile
						while ( 
							high != lfrags_e && 
							projector::deref(*high).getReadGroup() == projector::deref(*low).getReadGroup() &&
							projector::deref(*high).getTile() == projector::deref(*low).getTile() )
						{
							++high;
						}
						
						if ( high-low > 1 && projector::deref(*low).getTile() )
						{
							#if defined(DEBUG)
							std::cerr << "[D] Range " << high-low << " for " << projector::deref(lfrags[low]) << std::endl;
							#endif
						
							std::vector<bool> opt(high-low,false);
							bool haveoptdup = false;
							
							for ( iterator i = low; i+1 != high; ++i )
							{
								for ( 
									iterator j = i+1; 
									j != high && projector::deref(*j).getX() - projector::deref(*low).getX() <= optminpixeldif;
									++j 
								)
									if ( 
										::libmaus::math::iabs(
											static_cast<int64_t>(projector::deref(*i).getY())
											-
											static_cast<int64_t>(projector::deref(*j).getY())
										)
										<= optminpixeldif
									)
								{	
									opt [ j - low ] = true;
									haveoptdup = true;
								}
							}
							
							if ( haveoptdup )
							{
								unsigned int const lib = projector::deref(*low).getLibraryId();
								uint64_t numopt = 0;
								for ( uint64_t i = 0; i < opt.size(); ++i )
									if ( opt[i] )
										numopt++;
										
								DSC.addOpticalDuplicates(lib,numopt);	
							}
						}
						
						low = high;
					}	
				}
				else
				{
					#if defined(MARKDUPLICATEPAIRSDEBUG)
					std::cerr << "[V] --- singleton pair set ---" << std::endl;
					for ( iterator i = lfrags_a; i != lfrags_e; ++i )
						std::cerr << "[V] " << projector::deref(*i) << std::endl;
					#endif
				
				}
				
				uint64_t const lfragssize = lfrags_e-lfrags_a;
				// all but one are duplicates
				return lfragssize ? 2*(lfragssize - 1) : 0;
			}


			static uint64_t markDuplicatePairsVector(std::vector< ::libmaus::bambam::ReadEnds > & lfrags, ::libmaus::bambam::DupSetCallback & DSC, unsigned int const optminpixeldif = 100)
			{
				return markDuplicatePairs<std::vector<libmaus::bambam::ReadEnds>::iterator,MarkDuplicateProjectorIdentity>(lfrags.begin(),lfrags.end(),DSC,optminpixeldif);
			}

			template<typename iterator>
			static uint64_t markDuplicatePairsPointers(iterator const lfrags_a, iterator const lfrags_e, ::libmaus::bambam::DupSetCallback & DSC, unsigned int const optminpixeldif = 100)
			{
				return markDuplicatePairs<iterator,MarkDuplicateProjectorPointerDereference>(lfrags_a,lfrags_e,DSC,optminpixeldif);
			}


			template<typename iterator, typename projector = MarkDuplicateProjectorIdentity>
			static uint64_t markDuplicateFrags(iterator const lfrags_a, iterator const lfrags_e, ::libmaus::bambam::DupSetCallback & DSC)
			{
				uint64_t const lfragssize = lfrags_e - lfrags_a;

				if ( lfragssize > 1 )
				{
					#if defined(MARKDUPLICATEFRAGSDEBUG)
					std::cerr << "[V] --- frag set --- " << std::endl;
					for ( iterator lfrags_c = lfrags_a; lfrags_c != lfrags_e; ++lfrags_c )
						std::cerr << "[V] " << projector::deref(*lfrags_c) << std::endl;
					#endif
				
					bool containspairs = false;
					bool containsfrags = false;
					
					for ( iterator lfrags_c = lfrags_a; lfrags_c != lfrags_e; ++lfrags_c )
						if ( projector::deref(*lfrags_c).isPaired() )
							containspairs = true;
						else
							containsfrags = true;
					
					// if there are any single fragments
					if ( containsfrags )
					{
						// mark single ends as duplicates if there are pairs
						if ( containspairs )
						{
							#if defined(MARKDUPLICATEFRAGSDEBUG)
							std::cerr << "[V] there are pairs, marking single ends as duplicates" << std::endl;
							#endif
						
							uint64_t dupcnt = 0;
							// std::cerr << "Contains pairs." << std::endl;
							for ( iterator lfrags_c = lfrags_a; lfrags_c != lfrags_e; ++lfrags_c )
								if ( ! (projector::deref(*lfrags_c).isPaired()) )
								{
									DSC(projector::deref(*lfrags_c));
									dupcnt++;
								}
								
							return dupcnt;	
						}
						// if all are single keep highest score only
						else
						{
							#if defined(MARKDUPLICATEFRAGSDEBUG)
							std::cerr << "[V] there are only fragments, keeping best one" << std::endl;
							#endif
							// std::cerr << "Frags only." << std::endl;
						
							uint64_t maxscore = projector::deref(*lfrags_a).getScore();
							iterator lfrags_m = lfrags_a;
							
							for ( iterator lfrags_c = lfrags_a+1; lfrags_c != lfrags_e; ++lfrags_c )
								if ( projector::deref(*lfrags_c).getScore() > maxscore )
								{
									maxscore = projector::deref(*lfrags_c).getScore();
									lfrags_m = lfrags_c;
								}

							for ( iterator lfrags_c = lfrags_a; lfrags_c != lfrags_e; ++lfrags_c )
								if ( lfrags_c != lfrags_m )
									DSC(projector::deref(*lfrags_c));

							return lfragssize-1;
						}			
					}
					else
					{
						#if defined(MARKDUPLICATEFRAGSDEBUG)
						std::cerr << "[V] group does not contain unpaired reads." << std::endl;
						#endif
					
						return 0;
					}
				}	
				else
				{
					return 0;
				}
			}

			static uint64_t markDuplicateFrags(std::vector< ::libmaus::bambam::ReadEnds > const & lfrags, ::libmaus::bambam::DupSetCallback & DSC)
			{
				return markDuplicateFrags<std::vector< ::libmaus::bambam::ReadEnds >::const_iterator,MarkDuplicateProjectorIdentity>(lfrags.begin(),lfrags.end(),DSC);
			}

			template<typename iterator>
			static uint64_t markDuplicateFragsPointers(iterator const lfrags_a, iterator const lfrags_e, ::libmaus::bambam::DupSetCallback & DSC)
			{
				return markDuplicateFrags<iterator,MarkDuplicateProjectorPointerDereference>(lfrags_a,lfrags_e,DSC);
			}

			static bool isDupPair(::libmaus::bambam::ReadEnds const & A, ::libmaus::bambam::ReadEnds const & B)
			{
				bool const notdup = 
					A.getLibraryId()       != B.getLibraryId()       ||
					A.getRead1Sequence()   != B.getRead1Sequence()   ||
					A.getRead1Coordinate() != B.getRead1Coordinate() ||
					A.getOrientation()     != B.getOrientation()     ||
					A.getRead2Sequence()   != B.getRead2Sequence()   ||
					A.getRead2Coordinate() != B.getRead2Coordinate()
				;
				
				return ! notdup;
			}

			static bool isDupFrag(::libmaus::bambam::ReadEnds const & A, ::libmaus::bambam::ReadEnds const & B)
			{
				bool const notdup = 
					A.getLibraryId()      != B.getLibraryId()       ||
					A.getRead1Sequence()   != B.getRead1Sequence()   ||
					A.getRead1Coordinate() != B.getRead1Coordinate() ||
					A.getOrientation()     != B.getOrientation()
				;
				
				return ! notdup;
			}
		};
	}
}
#endif
