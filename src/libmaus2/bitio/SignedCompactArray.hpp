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

#if ! defined(SIGNEDCOMPACTARRAY_HPP)
#define SIGNEDCOMPACTARRAY_HPP

#include <libmaus2/bitio/CompactArray.hpp>

namespace libmaus2
{
	namespace bitio
	{
		/* signed */
		template<typename _base_type>
		struct SignedCompactArrayTemplate : public _base_type
		{
			typedef _base_type base_type;
			typedef SignedCompactArrayTemplate<base_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef int64_t value_type;

			typedef typename ::libmaus2::util::AssignmentProxy<this_type,value_type> proxy_type;
			typedef typename ::libmaus2::util::AssignmentProxyIterator<this_type,value_type> iterator;
			typedef typename ::libmaus2::util::ConstIterator<this_type,value_type> const_iterator;

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			const_iterator end() const
			{
				return const_iterator(this,base_type::size());
			}
			iterator begin()
			{
				return iterator(this,0);
			}
			iterator end()
			{
				return iterator(this,base_type::size());
			}

			int64_t const nshift;

			static int64_t deserializeSignedNumber(std::istream & in, uint64_t & t)
			{
				int64_t n;
				t += ::libmaus2::serialize::Serialize<int64_t>::deserialize(in, &n);
				return n;
			}
			static int64_t deserializeSignedNumber(std::istream & in)
			{
				uint64_t t;
				return deserializeSignedNumber(in,t);
			}

			uint64_t serialize(std::ostream & out)
			{
				uint64_t t = 0;
				t += base_type::serialize(out);
				t += ::libmaus2::serialize::Serialize<int64_t>::serialize(out, nshift);
				return t;
			}

			template<typename iterator>
			SignedCompactArrayTemplate(iterator a, iterator e, uint64_t const rb)
			: base_type(a,e,rb), nshift((1ll << (base_type::b-1)))
			{
				uint64_t i = 0;
				for ( ; a != e; ++a, ++i )
					set(i,*a);
			}
			SignedCompactArrayTemplate(uint64_t const rn, uint64_t const rb)
			: base_type(rn,rb), nshift((1ll << (base_type::b-1)))
			{
			}
			SignedCompactArrayTemplate(uint64_t const rn, uint64_t const rb, uint64_t * const rD)
			: base_type(rn,rb,rD), nshift((1ll << (base_type::b-1)))
			{
			}
			SignedCompactArrayTemplate(std::istream & in)
			: base_type(in), nshift(deserializeSignedNumber(in))
			{
			}
			SignedCompactArrayTemplate(std::istream & in, uint64_t & t)
			: base_type(in,t), nshift(deserializeSignedNumber(in))
			{
			}

			int64_t get(uint64_t i) const
			{
				return static_cast<int64_t>(base_type::getBits(i*base_type::b)) - nshift;
			}
			void set(uint64_t i, int64_t v)
			{
				base_type::putBits(i*base_type::b, static_cast<uint64_t>(v + nshift));
			}

			int64_t postfixIncrement(uint64_t i) { int64_t const v = get(i); set(i,v+1); return v; }

			unique_ptr_type clone() const
			{
				unique_ptr_type O( new this_type(base_type::n,base_type::b) );
				for ( uint64_t i = 0; i < base_type::n; ++i )
					O->set( i, this->get(i) );
				return O;
			}
		};

		typedef SignedCompactArrayTemplate<CompactArray> SignedCompactArray;
		#if defined(LIBMAUS2_HAVE_SYNC_OPS)
		typedef SignedCompactArrayTemplate<SynchronousCompactArray> SynchronousSignedCompactArray;
		#endif
	}
}
#endif
