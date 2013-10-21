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
#if ! defined(LIBMAUS_BAMBAM_BAMMERGE_HPP)
#define LIBMAUS_BAMBAM_BAMMERGE_HPP

#include <libmaus/bambam/BamCatHeader.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * class for merging BAM input
		 **/
		struct BamMerge : public BamAlignmentDecoder
		{
			private:
			struct BamMergeHeapComparator
			{
				libmaus::bambam::BamAlignment ** algns;
				
				BamMergeHeapComparator(libmaus::bambam::BamAlignment ** ralgns) : algns(ralgns) {}
			
				bool operator()(uint64_t const a, uint64_t const b) const
				{
					libmaus::bambam::BamAlignment const * A = algns[a];
					libmaus::bambam::BamAlignment const * B = algns[b];
				
					uint32_t const refida = A->getRefID();
					uint32_t const refidb = B->getRefID();
					
					if ( refida != refidb )
						return refida > refidb;
					
					uint32_t const posa = A->getPos();
					uint32_t const posb = B->getPos();
					
					if ( posa != posb )
						return posa > posb;
						
					return a > b;
				}
			};

			std::vector<std::string> const filenames;
			libmaus::bambam::BamCatHeader const header;
			libmaus::autoarray::AutoArray<libmaus::bambam::BamDecoder::unique_ptr_type> decoders;
			libmaus::autoarray::AutoArray<libmaus::bambam::BamAlignment *> algns;
			BamMergeHeapComparator comp;
			std::priority_queue < uint64_t, std::vector< uint64_t >, BamMergeHeapComparator > Q;
			
			void tryLoad(uint64_t const id)
			{
				if ( decoders[id]->readAlignment() )
				{
					header.updateAlignment(id,*algns[id]);
					Q.push(id);
				}
			}
			
			public:
			BamMerge(std::vector<std::string> const & rfilenames, bool const putrank = false) 
			: BamAlignmentDecoder(putrank), filenames(rfilenames), header(filenames), decoders(filenames.size()), algns(filenames.size()), comp(algns.begin()), Q(comp)
			{
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					libmaus::bambam::BamDecoder::unique_ptr_type tdecoder(new libmaus::bambam::BamDecoder(filenames[i]));
					decoders[i] = UNIQUE_PTR_MOVE(tdecoder);
					algns[i] = &(decoders[i]->getAlignment());
					tryLoad(i);
				}
			}

			/**
			 * input method
			 *
			 * @return bool if alignment input was successfull and a new alignment was stored
			 **/
			virtual bool readAlignmentInternal(bool const delayPutRank = false)
			{
				if ( expect_true(!Q.empty()) )
				{
					uint64_t const id = Q.top();
					Q.pop();
					
					alignment.swap(*algns[id]);

					if ( ! delayPutRank )
						putRank();

					tryLoad(id);
					
					return true;
				}
				else
				{
					return false;
				}
			}
			
			virtual libmaus::bambam::BamHeader const & getHeader() const
			{
				return *(header.bamheader);
			}
		};
	}
}
#endif
