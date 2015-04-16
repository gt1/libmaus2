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

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <ctime>

#include <libmaus/btree/btree.hpp>

#include <set>

template<typename key_type, typename comp_type>
void testRandom()
{
	for ( unsigned int i = 0; i < 50; ++i )
	{
		std::set < key_type, comp_type > S;
		libmaus::btree::BTree< key_type, 32, comp_type> tree;
		
		for ( unsigned int j = 0; j < 10000; ++j )
		{
			key_type num = rand();
			S.insert(num);
			tree.insert(num);
		}
		
		for ( 
			typename std::set<key_type, comp_type>::const_iterator ita = S.begin(); 
			ita != S.end(); 
			++ita 
		)
		{
			assert ( tree.contains(*ita) );
			tree[*ita];
		}
			
		assert ( tree.size() == S.size() );
		
		std::vector < key_type > V;
		tree.toVector(V);
		
		std::vector < key_type > V2(S.begin(),S.end());
		assert ( V == V2 );
	}
}

template<typename key_type>
void testRandomType()
{
	testRandom < key_type, std::less <key_type> >();
	testRandom < key_type, std::greater <key_type> >();
}

/* the btree code below does not work for leaf or inner node size < 3, so define a private constructor to prohibit instantiation of the class for leaf_size < 3 */
template<unsigned int leaf_size> struct StaticParameterCheck {};
template<> struct StaticParameterCheck<0> { private: StaticParameterCheck() {} };

template<typename _key_type, unsigned int _leaf_size, unsigned int _inner_node_size, typename _leaf_size_type, typename _inner_node_size_type, typename _order_type = std::less<_key_type> >
struct BTreeAbstractNode : public StaticParameterCheck< ((_leaf_size > 2) && (_inner_node_size > 2)) ? 1 : 0 >
{
	typedef _key_type key_type;
	static unsigned int const leaf_size = _leaf_size;
	static unsigned int const leaf_size_2 = leaf_size/2;
	static unsigned int const inner_node_size = _inner_node_size;
	typedef _leaf_size_type leaf_size_type;
	typedef _inner_node_size_type inner_node_size_type;
	typedef _order_type order_type;
	
	typedef BTreeAbstractNode<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> base_type;
	typedef base_type this_type;

	virtual ~BTreeAbstractNode()
	{
	
	}

	virtual bool insert(key_type const key, base_type * & newleaf, key_type & inskey) = 0;
	virtual uint64_t size() const = 0;
	virtual std::ostream & print(std::ostream & out) const = 0;
	
	virtual void fillVector(std::vector<key_type> & V) const = 0;
	virtual std::vector<key_type> fillVector() const
	{
		std::vector<key_type> V;
		fillVector(V);
		return V;
	}
	virtual std::set<key_type> fillSet() const
	{
		std::vector<key_type> const V = fillVector();
		return std::set<key_type>(V.begin(),V.end());
	}
};


template<typename _key_type, unsigned int _leaf_size, unsigned int _inner_node_size, typename _leaf_size_type, typename _inner_node_size_type, typename _order_type = std::less<_key_type> >
struct BTreeLeaf : public BTreeAbstractNode<_key_type,_leaf_size,_inner_node_size,_leaf_size_type,_inner_node_size_type,_order_type>
{
	typedef _key_type key_type;
	static unsigned int const leaf_size = _leaf_size;
	static unsigned int const leaf_size_2 = leaf_size/2;
	static unsigned int const inner_node_size = _inner_node_size;
	typedef _leaf_size_type leaf_size_type;
	typedef _inner_node_size_type inner_node_size_type;
	typedef _order_type order_type;
	
	typedef BTreeAbstractNode<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> base_type;
	typedef BTreeLeaf<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> this_type;
	
	leaf_size_type k;
	order_type order;
	key_type keys[leaf_size];
	
	BTreeLeaf() : k(0), order() {}

	bool insert(key_type const key, base_type * & newleaf, key_type & inskey)
	{		
		leaf_size_type l = 0;
		
		// std::cerr << this << " "  << "k=" << static_cast<int>(k) << " leaf_size=" << leaf_size << std::endl;
		
		while ( l < k && order(keys[l],key) ) // keys[l] < key == key > keys[l]
			++l;
			
		assert ( l == k || (!(order(keys[l],key))) ); // key < keys[l]
		
		if ( l < k && (!(order(key,keys[l]))) )
			return false;

		newleaf = 0;
		
		if ( k == leaf_size )
		{
			// std::cerr << "leaf is full, creating new leaf." << std::endl;
		
			this_type * p = new this_type;
			newleaf = p;
			
			// inserted value is in left half
			if ( l < leaf_size_2 )
			{
				// copy right half (minus 1 element for passing up into recursion)
				std::copy(&keys[0] + leaf_size_2+1,&keys[0] + leaf_size,&(p->keys[0]));
				p->k = (leaf_size - leaf_size_2 - 1);
				inskey = keys[leaf_size_2];

				// insert new element in lower half
				k = leaf_size_2;
				for ( int64_t i = k; i > l; --i )
					keys[i] = keys[i-1];
					
				keys[l] = key;
				
				k += 1;
			}
			// inserted element is in right half
			else
			{
				// copy right half
				std::copy(&keys[0] + leaf_size_2,&keys[0] + leaf_size,&(p->keys[0]));
				p->k = (leaf_size-leaf_size_2);
				
				// left half keys leaf_size_2-1 elements
				k = leaf_size_2-1;
				inskey = keys[k];
				
				l -= leaf_size_2;
				for ( int64_t i = p->k; i > l; --i )
					p->keys[i] = p->keys[i-1];
				p->keys[l] = key;
				p->k += 1;
			}
		}
		else
		{
			// std::cerr << "not full, l=" << static_cast<int>(l) << " k=" << static_cast<int>(k) << std::endl;
		
			assert ( l == k || order(key,keys[l]) );

			// std::cerr << "move " << l+1 << " to " << k+1 << std::endl;
			for ( int64_t i = k /* +1 */; i > l; --i )
				keys[i] = keys[i-1];
				
			keys[l] = key;

			// std::cerr << "not full *, l=" << static_cast<int>(l) << " k=" << static_cast<int>(k) << std::endl;
			
			k += 1;
			
			// std::cerr << "k=" << static_cast<int>(k) << std::endl;
		}
		
		return true;	
	}
	
	virtual uint64_t size() const
	{
		return k;
	}

	void fillVector(std::vector<key_type> & V) const
	{
		for ( uint64_t i = 0; i < k; ++i )
			V.push_back(keys[i]);
	}

	std::ostream & print(std::ostream & out) const
	{
		out << "BTreeLeaf(";
		
		for ( uint64_t i = 0; i < k; ++i )
		{
			out << keys[i];
			if ( i+1 < k )
				out << ",";
		}
		
		out << ")";

		return out;	
	}
};

template<typename _key_type, unsigned int _leaf_size, unsigned int _inner_node_size, typename _leaf_size_type, typename _inner_node_size_type, typename _order_type = std::less<_key_type> >
struct BTreeInnerNode : public BTreeAbstractNode<_key_type,_leaf_size,_inner_node_size,_leaf_size_type,_inner_node_size_type,_order_type>
{
	typedef _key_type key_type;
	static unsigned int const leaf_size = _leaf_size;
	static unsigned int const leaf_size_2 = leaf_size/2;
	static unsigned int const inner_node_size = _inner_node_size;
	static unsigned int const inner_node_size_2 = _inner_node_size/2;
	typedef _leaf_size_type leaf_size_type;
	typedef _inner_node_size_type inner_node_size_type;
	typedef _order_type order_type;
	
	typedef BTreeAbstractNode<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> base_type;
	typedef BTreeInnerNode<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> this_type;
	
	inner_node_size_type k;
	order_type order;
	
	key_type keys[inner_node_size];
	base_type * children[inner_node_size+1];
	
	BTreeInnerNode() : k(0), order() {}
	~BTreeInnerNode()
	{
		for ( int64_t i = 0; i < k+1; ++i )
		{
			delete children[i];
			children[i] = 0;
		}
	}
	
	bool insert(key_type const key, base_type * & newnode, key_type & inskey)
	{
		leaf_size_type l = 0;
		
		while ( l < k && order(keys[l],key) )
			++l;
		
		assert ( l == k || (!(order(keys[l],key))) );
		
		// check if key is already present
		if ( l < k && (!(order(key,keys[l]))) )
			return false;

		assert ( l == k || key < keys[l] );
		
		// insert key in subtree
		base_type * subnewnode = 0;
		key_type subinskey;
		bool const subins = children[l]->insert(key,subnewnode,subinskey);
		
		// if key was already present, then we are done
		if ( ! subins )
			return false;
			
		newnode = 0;
		
		// if a new node was created
		if ( subnewnode )
		{
			assert ( l == k || inskey < keys[l] );
			
			if ( k == inner_node_size )
			{
				this_type * p = 0;
				
				try
				{
					p = new this_type;
				}
				catch(...)
				{
					delete subnewnode;
					throw;
				}
				
				newnode = p;
				
				// inserted value is in left half
				if ( l < inner_node_size_2 )
				{
					// copy right half (minus 1 element for passing up into recursion)
					std::copy(
						&keys[0] + inner_node_size_2+1,
						&keys[0] + inner_node_size,
						&(p->keys[0])
					);
					std::copy(
						&children[0] + inner_node_size_2+1,
						&children[0] + inner_node_size+1,
						&(p->children[0])
					);
					p->k = (inner_node_size - inner_node_size_2 - 1);
					
					inskey = keys[inner_node_size_2];

					// insert new element in lower half
					k = inner_node_size_2;
					for ( int64_t i = k; i > l; --i )
					{
						keys[i] = keys[i-1];
						children[i+1] = children[i];
					}
					keys[l] = subinskey;
					children[l+1] = subnewnode;
					
					k += 1;
				}
				// inserted element is in right half
				else
				{
					// copy right half
					std::copy(&keys[0] + inner_node_size_2,&keys[0] + inner_node_size,&(p->keys[0]));
					std::copy(&children[0] + inner_node_size_2,&children[0] + inner_node_size+1,&(p->children[0]));
					p->k = (inner_node_size-inner_node_size_2);
					
					// left half keys inner_node_size_2-1 elements
					k = inner_node_size_2-1;

					inskey = keys[k];
					
					l -= inner_node_size_2;
					for ( int64_t i = p->k; i > l; --i )
					{
						p->keys[i] = p->keys[i-1];
						p->children[i+1] = p->children[i];
					}
					p->keys[l] = subinskey;
					p->children[l+1] = subnewnode;
					p->k += 1;
				}
			}
			/* node is not yet full */
			else
			{
				/*
				         k_0     k_1     k_2 ...       k_k
				     c_0     c_1     c_2          c_k      c_{k+1}
				 */
				
				for ( int64_t i = k; i > l; --i )
				{
					keys[i] = keys[i-1];
					children[i+1] = children[i];
				}
				
				keys[l] = subinskey;
				children[l+1] = subnewnode;
			
				k += 1;
			}	
		}
	
		return true;
	}

	virtual uint64_t size() const
	{
		uint64_t s = 0;
		for ( int64_t i = 0; i < k+1; ++i )
			s += children[i]->size();
		s += k;
		
		return s;
	}

	void fillVector(std::vector<key_type> & V) const
	{
		for ( uint64_t i = 0; i < k; ++i )
		{
			children[i]->fillVector(V);
			V.push_back(keys[i]);
		}
		children[k]->fillVector(V);
	}

	std::ostream & print(std::ostream & out) const
	{
		out << "BTreeInnerNode(";
		
		for ( uint64_t i = 0; i < k; ++i )
		{
			children[i]->print(out);
			out << "," << keys[i] << ",";
		}
		children[k]->print(out);
		
		out << ")";

		return out;	
	}
};

template<typename _key_type, unsigned int _leaf_size, unsigned int _inner_node_size, typename _leaf_size_type, typename _inner_node_size_type, typename _order_type>
std::ostream & operator<<(std::ostream & out, BTreeAbstractNode<_key_type,_leaf_size,_inner_node_size,_leaf_size_type,_inner_node_size_type,_order_type> const & node)
{
	return node.print(out);
}

template<typename _key_type, unsigned int _leaf_size, unsigned int _inner_node_size, typename _leaf_size_type, typename _inner_node_size_type, typename _order_type = std::less<_key_type> >
struct BTree
{
	typedef _key_type key_type;
	static unsigned int const leaf_size = _leaf_size;
	static unsigned int const leaf_size_2 = leaf_size/2;
	static unsigned int const inner_node_size = _inner_node_size;
	static unsigned int const inner_node_size_2 = _inner_node_size/2;
	typedef _leaf_size_type leaf_size_type;
	typedef _inner_node_size_type inner_node_size_type;
	typedef _order_type order_type;
	
	typedef BTree<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> this_type;
	typedef BTreeAbstractNode<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> node_type;
	typedef BTreeLeaf<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> leaf_type;
	typedef BTreeInnerNode<key_type,leaf_size,inner_node_size,leaf_size_type,inner_node_size_type,order_type> inner_node_type;
	
	node_type * root;
	
	BTree()
	: root(0)
	{
	
	}
	~BTree()
	{
		delete root;
		root = 0;
	}
	
	uint64_t size() const
	{
		if ( root )
			return root->size();
		else
			return 0;
	}
	
	bool insert(key_type key)
	{
		if ( ! root )
			root = new leaf_type;
			
		node_type * newnode = 0;
		key_type inskey;
		
		bool const inserted = root->insert(key,newnode,inskey);
		
		if ( ! inserted )
			return false;
		
		if ( newnode )
		{
			// std::cerr << "new root, key=" << inskey << std::endl;

			inner_node_type * newroot = 0;
			
			try
			{
				newroot = new inner_node_type;
			}
			catch(...)
			{
				delete newnode;
				throw;
			}
			
			newroot->keys[0] = inskey;
			newroot->children[0] = root;
			newroot->children[1] = newnode;
			newroot->k = 1;
			root = newroot;
		}
		
		return true;
	}

	std::set<key_type> fillSet() const
	{
		if ( root )
			return root->fillSet();
		else
			return std::set<key_type>();
	}
};

template<typename _key_type, unsigned int _leaf_size, unsigned int _inner_node_size, typename _leaf_size_type, typename _inner_node_size_type, typename _order_type>
std::ostream & operator<<(std::ostream & out, BTree<_key_type,_leaf_size,_inner_node_size,_leaf_size_type,_inner_node_size_type,_order_type> const & tree)
{
	out << "BTree(";
	if ( tree.root )
		out << (*(tree.root));
	out << ")";
	return out;
}


template<unsigned int leaf_size>
void testLeafInsert()
{
	for ( unsigned int j = 0; j <= leaf_size; ++ j )
	{
		BTreeLeaf<unsigned int, leaf_size, 4, uint8_t, uint8_t> leaf;
		BTreeAbstractNode<unsigned int, leaf_size, 4, uint8_t, uint8_t> * newnode = 0;
		unsigned int newkey = 0;
	
		for ( unsigned int i = 0; i < leaf_size; ++i )
			leaf.insert(2*i+1,newnode,newkey);
		for ( unsigned int i = 0; i < leaf_size; ++i )
			assert ( leaf.insert(2*i+1,newnode,newkey) == false );
			
		std::cerr << "\n\n ------------------------ \n\n";	
			
		std::cerr << leaf << std::endl;
		
		std::cerr << "insert " << 2*j << std::endl;
		
		leaf.insert(2*j,newnode,newkey);
		
		assert ( newnode );
		
		std::set < unsigned int > S;
		for ( unsigned int i = 0; i < leaf.k; ++i )
			S.insert(leaf.keys[i]);
		S.insert(newkey);
		
		BTreeLeaf<unsigned int, leaf_size, 4, uint8_t, uint8_t> * p = dynamic_cast< BTreeLeaf<unsigned int, leaf_size, 4, uint8_t, uint8_t> * >(newnode);
		for ( unsigned int i = 0; i < p->k; ++i )
			S.insert(p->keys[i]);
			
		for ( unsigned int i = 0; i < leaf_size; ++i )
			assert ( S.find(2*i+1) != S.end() );
		assert ( S.find(2*j) != S.end() );
		
		std::cerr << leaf << std::endl;
		std::cerr << newkey << std::endl;
		std::cerr << *p << std::endl;
		
		assert ( leaf.k + p->k + 1 == leaf_size + 1 );
		
		delete newnode;
	}
}

void testBTreeInsert()
{
	BTree<unsigned int, 8 /* leaf size */, 8 /* inner size */, uint8_t, uint8_t> btree;
	
	for ( unsigned int i = 0; i < 16*1024; ++i )
	{
		// std::cerr << "inserting " << i << std::endl;
		btree.insert(i);
		assert ( btree.size() == i+1 );
		
		// std::cerr << btree << std::endl;

		std::set<unsigned int> S = btree.fillSet();
		assert ( *(S.begin()) == 0 && *(S.rbegin()) == i );
	}
}

int main()
{
	testLeafInsert<32>();	
	testBTreeInsert();

	return 0;

	srand(time(0));
	
	testRandomType<unsigned short>();
	testRandomType<unsigned int>();
	testRandomType<uint64_t>();
}
