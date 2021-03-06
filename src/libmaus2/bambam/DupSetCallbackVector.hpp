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
#if ! defined(LIBMAUS2_BAMBAM_DUPSETCALLBACKVECTOR_HPP)
#define LIBMAUS2_BAMBAM_DUPSETCALLBACKVECTOR_HPP

#include <libmaus2/bambam/DupSetCallback.hpp>
#include <libmaus2/bambam/DuplicationMetrics.hpp>
#include <libmaus2/util/unique_ptr.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct DupSetCallbackVector : public ::libmaus2::bambam::DupSetCallback
		{
			typedef std::map<uint64_t,::libmaus2::bambam::DuplicationMetrics> map_type;
			typedef libmaus2::util::unique_ptr<map_type>::type map_ptr_type;

			::libmaus2::bitio::BitVector B;
			map_ptr_type Pmetrics;
			map_type & metrics;

			void serialise(std::ostream & out) const
			{
				B.serialise(out);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,metrics.size());
				for (
					std::map<uint64_t,::libmaus2::bambam::DuplicationMetrics>::const_iterator ita = metrics.begin();
					ita != metrics.end();
					++ita
				)
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,ita->first);
					ita->second.serialise(out);
				}
			}

			DupSetCallbackVector(std::istream & in)
			: B(in), Pmetrics(new map_type), metrics(*Pmetrics)
			{
				uint64_t const nummet = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				for ( uint64_t i = 0; i < nummet; ++i )
				{
					uint64_t const j = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					::libmaus2::bambam::DuplicationMetrics met(in);
					metrics[j] = met;
				}
			}

			DupSetCallbackVector(
				uint64_t const n,
				std::map<uint64_t,::libmaus2::bambam::DuplicationMetrics> & rmetrics
			) : B(n), metrics(rmetrics) /* unpairedreadduplicates(), readpairduplicates(), metrics(rmetrics) */ {}

			void operator()(::libmaus2::bambam::ReadEnds const & A)
			{
				B.set(A.getRead1IndexInFile(),true);

				if ( A.isPaired() )
				{
					B.set(A.getRead2IndexInFile(),true);
					metrics[A.getLibraryId()].readpairduplicates++;
				}
				else
				{
					metrics[A.getLibraryId()].unpairedreadduplicates++;
				}
			}

			uint64_t getNumDups() const
			{
				uint64_t dups = 0;
				for ( uint64_t i = 0; i < B.size(); ++i )
					if ( B.get(i) )
						dups++;

				return dups;
			}
			void addOpticalDuplicates(uint64_t const libid, uint64_t const count)
			{
				metrics[libid].opticalduplicates += count;
			}

			bool isMarked(uint64_t const i) const
			{
				return B[i];
			}

			void flush(uint64_t const)
			{

			}
		};
	}
}
#endif
