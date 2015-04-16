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

#if ! defined(FASTINTERVAL_HPP)
#define FASTINTERVAL_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/IntervalTree.hpp>
#include <vector>
#include <limits>

namespace libmaus
{
	namespace fastx
	{
		struct FastInterval
		{
			uint64_t low;
			uint64_t high;
			uint64_t fileoffset;
			uint64_t fileoffsethigh;
			uint64_t numsyms;
			uint64_t minlen;
			uint64_t maxlen;
			
			uint64_t byteSize() const
			{
				return sizeof(FastInterval);
			}

			bool operator!=(FastInterval const & o) const
			{
				return !((*this)==o);
			}

			bool operator==(FastInterval const & o) const
			{
				return
					low == o.low &&
					high == o.high &&
					fileoffset == o.fileoffset &&
					fileoffsethigh == o.fileoffsethigh &&
					numsyms == o.numsyms &&
					minlen == o.minlen &&
					maxlen == o.maxlen;
			}
			
			static void append(
				std::vector<FastInterval> & V0,
				std::vector<FastInterval> const & V1
				)
			{
				uint64_t const add = V0.size() ? V0.back().high : 0;
				uint64_t const fadd = V0.size() ? V0.back().fileoffsethigh : 0;
				
				for ( uint64_t i = 0; i < V1.size(); ++i )
				{
					FastInterval FI = V1[i];
					FI.low += add;
					FI.high += add;
					FI.fileoffset += fadd;
					FI.fileoffsethigh += fadd;
					V0.push_back(FI);
				}
			}
			
			template<typename iterator>
			static FastInterval merge(iterator a, iterator e)
			{
				if ( e != a )
				{
					uint64_t numsyms = 0;
					uint64_t minlen = std::numeric_limits<uint64_t>::max();
					uint64_t maxlen = 0;
					for ( iterator i = a; i != e; ++i )
					{
						numsyms += i->numsyms;
						minlen = std::min(minlen,i->minlen);
						maxlen = std::max(maxlen,i->maxlen);
					}
					return FastInterval ( 
						a->low,
						(e-1)->high,
						a->fileoffset,
						(e-1)->fileoffsethigh,
						numsyms,
						minlen,
						maxlen
						);
				}
				else
				{
					return FastInterval(0,0,0,0,0,0,0);
				}
			}

			template<typename iterator>
			static FastInterval shallowMerge(iterator a, iterator e)
			{
				if ( e != a )
				{
					return FastInterval ( 
						a->low,
						(e-1)->high,
						a->fileoffset,
						(e-1)->fileoffsethigh,
						0,0,0
						);
				}
				else
				{
					return FastInterval(0,0,0,0,0,0,0);
				}
			}
			
			static std::vector < FastInterval > combine(std::vector < FastInterval > const & index, uint64_t const c)
			{
				std::vector < FastInterval > newindex;
				
				for ( uint64_t i = 0; i < index.size(); i += c )
				{
					uint64_t const low = i;
					uint64_t const high = std::min(i+c,static_cast<uint64_t>(index.size()));
					newindex.push_back ( merge ( index.begin()+low, index.begin()+high) );
				}
				
				return newindex;
			}

			FastInterval unsplit() const
			{
				FastInterval FI = *this;
				FI.low /= 2;
				FI.high /= 2;
				return FI;
			}

			FastInterval() : low(0), high(0), fileoffset(0), fileoffsethigh(0), numsyms(0), minlen(0), maxlen(0) {}
			FastInterval(
				uint64_t const rlow,
				uint64_t const rhigh,
				uint64_t const rfileoffset,
				uint64_t const rfileoffsethigh,
				uint64_t const rnumsyms,
				uint64_t const rminlen,
				uint64_t const rmaxlen)
			: low(rlow), high(rhigh), fileoffset(rfileoffset), fileoffsethigh(rfileoffsethigh), numsyms(rnumsyms), 
			  minlen(rminlen), maxlen(rmaxlen) {}

			static std::string serialise(FastInterval const & F)
			{
				std::ostringstream ostr;
				serialise(ostr,F);
				return ostr.str();
			}
			
			std::string serialise() const
			{
				return serialise(*this);
			}

			static void serialise(std::ostream & out, FastInterval const & F)
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.low);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.high);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.fileoffset);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.fileoffsethigh);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.numsyms);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.minlen);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,F.maxlen);
			}
			static FastInterval deserialise(std::istream & in)
			{
				uint64_t const low = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const high = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const fileoffset = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const fileoffsethigh = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const numsyms = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const minlen = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const maxlen = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				return FastInterval(low,high,fileoffset,fileoffsethigh,numsyms,minlen,maxlen);
			}
			
			static FastInterval deserialise(std::string const & s)
			{
				std::istringstream istr(s);
				return deserialise(istr);
			}
			
			static void serialiseVector(std::ostream & out, std::vector < FastInterval > const & V)
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,V.size());
				for ( uint64_t i = 0; i < V.size(); ++i )
					serialise(out,V[i]);
			}
			static std::string serialiseVector(std::vector < FastInterval > const & V)
			{
				std::ostringstream ostr;
				serialiseVector(ostr,V);
				return ostr.str();
			}
			static std::vector < FastInterval > deserialiseVector(std::istream & in)
			{
				std::vector < FastInterval > V;
				uint64_t const n = ::libmaus::util::NumberSerialisation::deserialiseNumber(in);
				for ( uint64_t i = 0; i < n; ++i )
					V.push_back ( deserialise(in) );
				return V;
			}
			static std::vector < FastInterval > deserialiseVector(std::string const & s)
			{
				std::istringstream istr(s);
				return deserialiseVector(istr);
			}

			static ::libmaus::util::IntervalTree::unique_ptr_type toIntervalTree(std::vector < FastInterval > const & V)
			{
				libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > H(V.size());
				for ( uint64_t i = 0; i < V.size(); ++i )
					H[i] = std::pair<uint64_t,uint64_t>(V[i].low,V[i].high);
				::libmaus::util::IntervalTree::unique_ptr_type PIT(new ::libmaus::util::IntervalTree(H,0,H.size()));
				return UNIQUE_PTR_MOVE(PIT);
			}
		};

		inline std::ostream & operator<<(std::ostream & out, FastInterval const & I)
		{
			out << "FastInterval([" << I.low << "," << I.high << "),fileoffset=" << I.fileoffset << ",fileoffsethigh=" 
				<< I.fileoffsethigh << ",numsyms=" << I.numsyms 
				<< ",minlen=" << I.minlen
				<< ",maxlen=" << I.maxlen
				<< ")";
			return out;
		}
		inline bool operator==(std::vector<FastInterval> const & A, std::vector<FastInterval> const & B)
		{
			if ( A.size() != B.size() )
				return false;
			for ( uint64_t i = 0; i < A.size(); ++i )
				if ( A[i] != B[i] )
					return false;
			return true;
		}
	}
}
#endif
