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
#if ! defined(CARTESIANTREE_HPP)
#define CARTESIANTREE_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/BitWriter.hpp>
#include <libmaus/bitio/BitVector.hpp>
#include <ostream>

namespace libmaus
{
	namespace rmq
	{
		template<typename key_type> struct CartesianTree;
		template<typename key_type> ::std::ostream & operator<<(::std::ostream & out, CartesianTree<key_type> const & node);

		template<typename key_type>
		struct CartesianTree
		{
			protected:
			::libmaus::autoarray::AutoArray<uint64_t> leftright;
			::libmaus::autoarray::AutoArray<uint64_t> parent;
			::libmaus::autoarray::AutoArray<key_type> K;
			uint64_t root;
			uint64_t const n;

			uint64_t getSize() const
			{
				return 
					(n<<1)*sizeof(uint64_t) +
					(n   )*sizeof(uint64_t) +
					(n   )*sizeof(key_type) +
					2*sizeof(uint64_t);
			}
			
			inline uint64_t       & getLeft  (uint64_t i)       { return leftright[(i<<1)|0]; }
			inline uint64_t const & getLeft  (uint64_t i) const { return leftright[(i<<1)|0]; }
			inline uint64_t       & getRight (uint64_t i)       { return leftright[(i<<1)|1]; }
			inline uint64_t const & getRight (uint64_t i) const { return leftright[(i<<1)|1]; }
			inline uint64_t       & getParent(uint64_t i)       { return parent[i]; }
			inline uint64_t const & getParent(uint64_t i) const { return parent[i]; }
			
			inline key_type       & getKey(uint64_t i)       { return K[i]; }
			inline key_type const & getKey(uint64_t i) const { return K[i]; }
			
			uint64_t getIndex (uint64_t i) const { return i; }
			
			void allocate()
			{
				leftright = ::libmaus::autoarray::AutoArray<uint64_t>(n<<1,true); for ( uint64_t i = 0; i < (n<<1); ++i ) leftright[i] = n;
				parent = ::libmaus::autoarray::AutoArray<uint64_t>(n,false); for ( uint64_t i = 0; i < n; ++i ) parent[i] = n;
				K = ::libmaus::autoarray::AutoArray<key_type>(n,false);
			}

			void inorder(::std::ostream & out, uint64_t node) const
			{
				out << "(";
				if ( getLeft(node) != n )
					inorder(out,getLeft(node));
				out << getKey(node);
				out << "[";
				out << getIndex(node);
				out << "]";
				if ( getRight(node) != n )
					inorder(out,getRight(node));
				out << ")";
			}
			void inorder(::std::ostream & out) const
			{
				inorder(out,root);
			}

			public:
			template<typename const_iterator>
			CartesianTree(const_iterator a, const_iterator b) : root(0), n(b-a)
			{
				if ( n )
				{
					allocate();
					
					root = 0;
					getKey(root) = *(a++);
					
					uint64_t nextnode = root+1;
					uint64_t pnode = root;
					
					while ( a != b )
					{
						key_type const v = *(a++);

						uint64_t x = nextnode++; 
						getKey(x) = v;
						uint64_t y = pnode;
						
						while ( (y!=n) && (getKey(y) > v) ) // A[l_m] <= A[i+1]
							y = getParent(y);
					
						// y is a node, x becomes a child of y
						if ( (y!=n) ) 
						{
							getParent(x) = y;
							// ::std::cerr << "x=" << x << ", y=" << y << ::std::endl;
							getLeft(x) = getRight(y);
							getRight(y) = x;
						}
						// y is not a node, x becomes the new root
						else
						{
							y = root;
							getLeft(x) = y;
							getParent(y) = x;
							root = x;
						}
						
						pnode = x;
					}			
				}
			}
			template<typename const_iterator>
			CartesianTree(const_iterator a, uint64_t const rn) : root(0), n(rn)
			{
				if ( n )
				{
					allocate();
					
					root = 0;
					getKey(root) = a[0];
					
					uint64_t nextnode = root+1;
					uint64_t pnode = root;
					
					for ( uint64_t zzz = 1; zzz < n; ++zzz )
					{
						key_type const v = a[zzz];

						uint64_t x = nextnode++; 
						getKey(x) = v;
						uint64_t y = pnode;
						
						while ( (y!=n) && (getKey(y) > v) ) // A[l_m] <= A[i+1]
							y = getParent(y);
					
						// y is a node, x becomes a child of y
						if ( (y!=n) ) 
						{
							getParent(x) = y;
							// ::std::cerr << "x=" << x << ", y=" << y << ::std::endl;
							getLeft(x) = getRight(y);
							getRight(y) = x;
						}
						// y is not a node, x becomes the new root
						else
						{
							y = root;
							getLeft(x) = y;
							getParent(y) = x;
							root = x;
						}
						
						pnode = x;
					}			
				}
			}
			
			template<typename writer_type>
			void bps(writer_type & bitout) const
			{
				bps(root,bitout);
			}

			template<typename writer_type>
			void bps(uint64_t node, writer_type & bitout) const
			{
				bitout.writeBit(1);
				
				if ( CartesianTree<key_type>::getLeft(node) != CartesianTree<key_type>::n )
					bps(CartesianTree<key_type>::getLeft(node),bitout);
								
				if ( CartesianTree<key_type>::getRight(node) != CartesianTree<key_type>::n )
					bps(CartesianTree<key_type>::getRight(node),bitout);
				
				bitout.writeBit(0);
			}

			std::string bps() const
			{
				::std::ostringstream ostr;
				::std::ostream_iterator<uint8_t> ostrit(ostr);
				::libmaus::bitio::BitWriterStream8 BWS8(ostrit);
				bps(BWS8);
				BWS8.flush();
				return ostr.str();
			}
			
			::libmaus::bitio::IndexedBitVector::unique_ptr_type bpsVector() const
			{
				::libmaus::bitio::IndexedBitVector::unique_ptr_type IB ( new ::libmaus::bitio::IndexedBitVector(2*n) );
				::libmaus::bitio::BitWriter8 BW8(IB->A.get());
				bps(BW8);
				BW8.flush();
				IB->setupIndex();
				return UNIQUE_PTR_MOVE(IB);
			}
			
			friend ::std::ostream & operator<< <>(::std::ostream & out, CartesianTree<key_type> const & node);
		};

		template<typename key_type>
		::std::ostream & operator<<(::std::ostream & out, CartesianTree<key_type> const & node)
		{
			node.inorder(out);
			out << " root=" << node.root;
			return out;
		}
	}
}
#endif
