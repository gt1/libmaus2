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
#if ! defined(LIBMAUS_BAMBAM_READENDS_HPP)
#define LIBMAUS_BAMBAM_READENDS_HPP

#include <libmaus/bambam/ReadEndsBase.hpp>
#include <map>
#include <cstring>
#include <sstream>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * ReadEnds class (used for duplicate marking); this class contains partial information from
		 * a pair of reads
		 **/
		struct ReadEnds : public ReadEndsBase
		{
			//! first alignment if copied
			BamAlignment::shared_ptr_type p;
			//! second alignment if copied
			BamAlignment::shared_ptr_type q;
			
			/**
			 * constructor for empty/invalid empty
			 **/
			ReadEnds() : ReadEndsBase()
			{
			}
			
			/**
			 * reset object to invalid/empty state
			 **/
			void reset()
			{
				ReadEndsBase::reset();
				p.reset();
				q.reset();
			}
			
			/**
			 * recode object (run thorugh an encode and decode cycle; used for debugging, 
			 * recoded object should be identical to original object)
			 *
			 * @return recoded object
			 **/
			ReadEnds recode() const
			{
				std::ostringstream ostr;
				put(ostr);
				std::istringstream istr(ostr.str());
				ReadEnds RE;
				RE.get(istr);
				return RE;
			}

			/**
			 * decode ReadEnds object from stream G
			 *
			 * @param G input stream
			 **/
			template<typename get_type>
			void get(get_type & G)
			{
				ReadEndsBase::get(G);
				
				uint64_t numal = G.get();
				
				if ( numal > 0 )
					p = BamAlignment::shared_ptr_type(new BamAlignment(G));
				if ( numal > 1 )
					q = BamAlignment::shared_ptr_type(new BamAlignment(G));
			}

			/**
			 * encode ReadEnds object to output stream P
			 *
			 * @param P output stream
			 **/
			template<typename put_type>
			void put(put_type & P) const
			{
				ReadEndsBase::put(P);

				unsigned int const havep = ((p.get() != 0) ? 1 : 0);
				unsigned int const haveq = ((q.get() != 0) ? 1 : 0);
				uint64_t const numal = havep+haveq;
				P.put(static_cast<uint8_t>(numal));
					
				if ( havep )
					p->serialise(P);
				if ( haveq )
					q->serialise(P);
			}

			/**
			 * constructor for fragment type ReadEnds object
			 *
			 * @param p alignment
			 * @param header BAM header
			 * @param copyAlignment copy alignment to object
			 **/
			ReadEnds(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamHeader const & header,
				bool const copyAlignment = false,
				uint64_t const rtagId = 0
			)
			{
				reset();
				fillFrag(p,header,*this,rtagId);
				if ( copyAlignment )
					 this->p = p.sclone();
			}

			/**
			 * constructor for fragment type ReadEnds object
			 *
			 * @param pD alignment block
			 * @param pblocksize alignment block length
			 * @param header BAM header
			 * @param copyAlignment copy alignment to object
			 **/
			ReadEnds(
				uint8_t const * pD,
				uint64_t const pblocksize,
				::libmaus::bambam::BamHeader const & header,
				bool const copyAlignment = false,
				uint64_t const rtagId = 0
			)
			{
				reset();
				fillFrag(pD,pblocksize,header,*this,rtagId);
				if ( copyAlignment )
				{
					BamAlignment::shared_ptr_type salgn(new libmaus::bambam::BamAlignment);
					salgn->copyFrom(pD,pblocksize);
					this->p = salgn;
				}
			}

			/**
			 * constructor for pair type ReadEnds object
			 *
			 * @param p first alignment
			 * @param q second alignment
			 * @param header BAM header
			 * @param copyAlignment copy alignment to object
			 **/
			ReadEnds(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamAlignment const & q, 
				::libmaus::bambam::BamHeader const & header,
				bool const copyAlignment = false,
				uint64_t const rtagId = 0
			)
			{
				reset();
				fillFragPair(p,q,header,*this,rtagId);
				if ( copyAlignment )
				{
					 this->p = p.sclone();
					 this->q = q.sclone();
				}
			}	

			/**
			 * constructor for pair type ReadEnds object
			 *
			 * @param pD first alignment block
			 * @param pblocksize first alignment block size
			 * @param qD second alignment block
			 * @param qblocksize second alignment block size
			 * @param header BAM header
			 * @param copyAlignment copy alignment to object
			 **/
			ReadEnds(
				uint8_t const * pD, 
				uint64_t const pblocksize,
				uint8_t const * qD, 
				uint64_t const qblocksize,
				::libmaus::bambam::BamHeader const & header,
				bool const copyAlignment = false,
				uint64_t const rtagId = 0
			)
			{
				reset();
				fillFragPair(pD,pblocksize,qD,qblocksize,header,*this,rtagId);
				if ( copyAlignment )
				{
					BamAlignment::shared_ptr_type palgn(new libmaus::bambam::BamAlignment);
					palgn->copyFrom(pD,pblocksize);
					BamAlignment::shared_ptr_type qalgn(new libmaus::bambam::BamAlignment);
					qalgn->copyFrom(qD,qblocksize);
					this->p = palgn;
					this->q = qalgn;
				}
			}	
		};		
	}
}

/**
 * format orientation for output stream
 *
 * @param out output stream
 * @param reo read ends orientation
 * @return out
 **/
std::ostream & operator<<(std::ostream & out, libmaus::bambam::ReadEnds::read_end_orientation reo);
/**
 * format read ends object for output stream
 *
 * @param out output stream
 * @param RE read ends object
 * @return out
 **/
std::ostream & operator<<(std::ostream & out, libmaus::bambam::ReadEnds const & RE);
#endif
