/**
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
**/
#if ! defined(LIBMAUS_BAMBAM_DUPLICATIONMETRICS_HPP)
#define LIBMAUS_BAMBAM_DUPLICATIONMETRICS_HPP

#include <libmaus/types/types.hpp>
#include <ostream>
#include <cmath>

namespace libmaus
{
	namespace bambam
	{
		struct DuplicationMetrics
		{
			uint64_t unmapped;
			uint64_t unpaired;
			uint64_t readpairsexamined;
			uint64_t unpairedreadduplicates;
			uint64_t readpairduplicates;
			uint64_t opticalduplicates;
			
			DuplicationMetrics()
			:
				unmapped(0),
				unpaired(0),
				readpairsexamined(0),
				unpairedreadduplicates(0),
				readpairduplicates(0),
				opticalduplicates(0)
			{
				
			}

			/**
			 * Code imported from picard for library size estimation
			 *
			 * Estimates the size of a library based on the number of paired end molecules observed
			 * and the number of unique pairs ovserved.
			 *
			 * Based on the Lander-Waterman equation that states:
			 *     C/X = 1 - exp( -N/X )
			 * where
			 *     X = number of distinct molecules in library
			 *     N = number of read pairs
			 *     C = number of distinct fragments observed in read pairs
			 */
			static int64_t estimateLibrarySize(int64_t const readPairs, int64_t const uniqueReadPairs) 
			{
				int64_t const readPairDuplicates = readPairs - uniqueReadPairs;

				if (readPairs > 0 && readPairDuplicates > 0) 
				{
					int64_t const n = readPairs;
					int64_t const c = uniqueReadPairs;

					double m = 1.0, M = 100.0;

					if (c >= n || f(m*c, c, n) < 0) 
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "[E] Invalid values for pairs and unique pairs: " << n << ", " << c << std::endl;
						se.finish();
						throw se;
					}

					while( f(M*c, c, n) >= 0 ) M *= 10.0;

					for ( int i=0; i < 40; ++i ) 
					{
						double const r = (m+M)/2.0;
						double const u = f( r * c, c, n );
						if ( u == 0 ) break;
						else if ( u > 0 ) m = r;
						else if ( u < 0 ) M = r;
					}

					return static_cast<int64_t> (c * (m+M)/2.0);
				}
				else 
				{
					return -1;
				}
			}

			/** Method that is used in the computation of estimated library size. */
			static double f(double const x, double const c, double const n) 
			{
				return c/x - 1 + ::std::exp(-n/x);
			}

			/**
			 * Estimates the ROI (return on investment) that one would see if a library was sequenced to
			 * x higher coverage than the observed coverage.
			 *
			 * @param estimatedLibrarySize the estimated number of molecules in the library
			 * @param x the multiple of sequencing to be simulated (i.e. how many X sequencing)
			 * @param pairs the number of pairs observed in the actual sequencing
			 * @param uniquePairs the number of unique pairs observed in the actual sequencing
			 * @return a number z <= x that estimates if you had pairs*x as your sequencing then you
			 *         would observe uniquePairs*z unique pairs.
			 */
			 static double estimateRoi(int64_t estimatedLibrarySize, double x, int64_t pairs, int64_t uniquePairs) 
			 {
			 	return estimatedLibrarySize * ( 1 - ::std::exp(-(x*pairs)/estimatedLibrarySize) ) / uniquePairs;
			}

                        static std::ostream & printFormatHeader(std::string const & CL, std::ostream & out)
			{
                                out << "# " << CL << std::endl << std::endl << "##METRICS" << std::endl;
                                out << "LIBRARY\tUNPAIRED_READS_EXAMINED\tREAD_PAIRS_EXAMINED\tUNMAPPED_READS\tUNPAIRED_READ_DUPLICATES\tREAD_PAIR_DUPLICATES\tREAD_PAIR_OPTICAL_DUPLICATES\tPERCENT_DUPLICATION\tESTIMATED_LIBRARY_SIZE\n";
				return out;
			}
		
			/**
			 * Calculates a histogram using the estimateRoi method to estimate the effective yield
			 * doing x sequencing for x=1..10.
			 */
			std::map<unsigned int,double> calculateRoiHistogram()  const
			{
				std::map<unsigned int, double> H;
				
				try
				{
					int64_t const ESTIMATED_LIBRARY_SIZE = estimateLibrarySize(readpairsexamined - opticalduplicates, readpairsexamined - readpairduplicates);
					
					if ( ESTIMATED_LIBRARY_SIZE < 0 )
						return H;

					int64_t const uniquePairs = readpairsexamined - readpairduplicates;
						
					for ( double x = 1.0; x <= 100.0; x+=1.0 )
						H[x] += estimateRoi(ESTIMATED_LIBRARY_SIZE, x, readpairsexamined, uniquePairs);
					
					return H;
				}
				catch(...)
				{
					return H;
				}
			}
			
			std::ostream & printHistogram(std::ostream & out) const
			{
				std::map<unsigned int,double> const H = calculateRoiHistogram();
				
				for ( std::map<unsigned int,double>::const_iterator ita = H.begin(); ita != H.end(); ++ita )
					if ( ita->second )
						out << ita->first << "\t" << ita->second << std::endl;
						
				return out;
			}

			std::ostream & format(std::ostream & out, std::string const libraryName) const
			{
				int64_t const ESTIMATED_LIBRARY_SIZE = estimateLibrarySize(readpairsexamined - opticalduplicates, readpairsexamined - readpairduplicates);                           
				double const PERCENT_DUPLICATION = (unpairedreadduplicates + 2*readpairduplicates) / static_cast<double> (unpaired + readpairsexamined*2);
                                                                          
				out 
					<< libraryName << "\t"
					<< unpaired << "\t"
					<< readpairsexamined << "\t"
					<< unmapped << "\t"
					<< unpairedreadduplicates << "\t"
					<< readpairduplicates << "\t"
					<< opticalduplicates << "\t"
					<< PERCENT_DUPLICATION << "\t"
					<< ESTIMATED_LIBRARY_SIZE << "\n";
				return out;
			}
		};

		inline std::ostream & operator<<(std::ostream & out, DuplicationMetrics const & M)
		{
			out << "unmapped\t" << M.unmapped << std::endl;
			out << "unpaired\t" << M.unpaired << std::endl;
			out << "readpairsexamined\t" << M.readpairsexamined << std::endl;
			out << "unpairedreadduplicates\t" << M.unpairedreadduplicates << std::endl;
			out << "readpairduplicates\t" << M.readpairduplicates << std::endl;
			out << "opticalduplicates\t" << M.opticalduplicates << std::endl;
			// out << "unmapped\t" << M.unmapped << std::endl;
			return out;
		}
	}
}
#endif
