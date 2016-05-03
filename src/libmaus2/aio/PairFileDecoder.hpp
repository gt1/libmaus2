/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_AIO_PAIRFILEDECODER_HPP)
#define LIBMAUS2_AIO_PAIRFILEDECODER_HPP

#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/util/iterator.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct PairFileDecoder
		{
			typedef PairFileDecoder this_type;
			typedef libmaus2::util::ConstIterator<this_type,std::pair<uint64_t,uint64_t> > const_iterator;
		
			mutable libmaus2::aio::InputStreamInstance::unique_ptr_type ISI;
			mutable libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type SGI;
			uint64_t const n;
			mutable std::pair<uint64_t,uint64_t> peekslot;
			mutable bool peekslotactive;
			
			uint64_t getLength() const
			{
				uint64_t const p = ISI->tellg();
				
				ISI->clear();
				ISI->seekg(0,std::ios::end);
				
				uint64_t const l = ISI->tellg();
				
				assert ( l % (2*sizeof(uint64_t)) == 0 );
				
				ISI->clear();
				ISI->seekg(p);
				
				return l / (2*sizeof(uint64_t));
			}
			
			void seekg(uint64_t const offset) const
			{
				ISI->clear();
				ISI->seekg(offset*2*sizeof(uint64_t));
				SGI->clearBuffer();
			}
			
			PairFileDecoder(std::string const & fn)
			: ISI(new libmaus2::aio::InputStreamInstance(fn)), SGI(new libmaus2::aio::SynchronousGenericInput<uint64_t>(*ISI,4096)), n(getLength()), peekslotactive(false)
			{
			}
			
			const_iterator begin()
			{
				return const_iterator(this,0);
			}
			
			const_iterator end()
			{
				return const_iterator(this,n);
			}
			
			void setupFirst(uint64_t const r)
			{
				struct PairFirstComparator
				{
					bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
					{
						return A.first < B.first;
					}
				};
				
				const_iterator it = ::std::lower_bound(begin(),end(),std::pair<uint64_t,uint64_t>(r,0),PairFirstComparator());
				uint64_t const off = it - begin();
				seekg(off);
			}
			
			bool peekNext(std::pair<uint64_t,uint64_t> & P) const
			{
				if ( ! peekslotactive )
					peekslotactive = getNext(peekslot);
				P = peekslot;
				return peekslotactive;
			}
			
			bool getNext(std::pair<uint64_t,uint64_t> & P) const
			{
				if ( peekslotactive )
				{
					P = peekslot;
					peekslotactive = false;
					return true;
				}
				else
				{
					bool ok = SGI->getNext(P.first);
					ok = ok && SGI->getNext(P.second);
					return ok;
				}
			}
			
			std::pair<uint64_t,uint64_t> get(uint64_t const i) const
			{
				seekg(i);
				std::pair<uint64_t,uint64_t> P;
				bool const ok = getNext(P);
				assert ( ok );
				return P;
			}
			
			std::pair<uint64_t,uint64_t> operator[](uint64_t const i) const
			{
				return get(i);
			}
		};
	}
}
#endif
