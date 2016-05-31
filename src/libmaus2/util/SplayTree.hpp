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
#if ! defined(LIBMAUS2_UTIL_SPLAYTREE_HPP)
#define LIBMAUS2_UTIL_SPLAYTREE_HPP

#include <deque>
#include <stack>
#include <libmaus2/util/ContainerElementFreeList.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _key_type, typename _node_id_type = int64_t >
		struct SplayTreeElement
		{
			typedef _key_type key_type;
			typedef _node_id_type node_id_type;
			typedef SplayTreeElement<key_type,node_id_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			node_id_type parent;
			node_id_type left;
			node_id_type right;
			key_type key;
		};

		template<typename _key_type, typename _comparator_type = std::less<_key_type>, typename _node_id_type = int64_t >
		struct SplayTree
		{
			typedef _key_type key_type;
			typedef _comparator_type comparator_type;
			typedef _node_id_type node_id_type;
			typedef SplayTree<key_type,comparator_type,node_id_type> this_type;

			typedef SplayTreeElement<key_type,node_id_type> tree_element_type;
			typedef ContainerElementFreeList<tree_element_type> base_type;
			typedef typename base_type::unique_ptr_type base_pointer_type;

			base_pointer_type Pbase;
			base_type & base;

			// tree nodes
			libmaus2::autoarray::AutoArray<tree_element_type> & Aelements;

			// key comparator
			comparator_type const comparator;

			// tree root
			node_id_type root;

			void toStringStruct(std::ostream & out, node_id_type node) const
			{
				out << "(";

				if ( getLeft(node) != -1 )
					toStringStruct(out,getLeft(node));

				out << getKey(node);

				if ( getRight(node) != -1 )
					toStringStruct(out,getRight(node));

				out << ")";
			}

			bool checkOrder() const
			{
				std::stack< std::pair<node_id_type,int> > todo;

				if ( root != -1 )
					todo.push(std::pair<node_id_type,int>(root,0));

				key_type prev = key_type();
				bool prevvalid = false;

				while ( !todo.empty() )
				{
					std::pair<node_id_type,int> const P = todo.top();
					todo.pop();

					switch ( P.second )
					{
						case 0:
						{
							todo.push(std::pair<node_id_type,int>(P.first,1));
							if ( getLeft(P.first) != -1 )
								todo.push(std::pair<node_id_type,int>(getLeft(P.first),0));
							break;
						}
						case 1:
						{
							if ( prevvalid )
							{
								if ( ! comparator(prev,getKey(P.first)) )
								{
									#if 0
									std::cerr << prev << "," << getKey(P.first) << std::endl;
									std::cerr << toStringStruct() << std::endl;
									assert ( false );
									#endif
									return false;
								}
							}

							prev = getKey(P.first);
							prevvalid = true;

							if ( getRight(P.first) != -1 )
								todo.push(std::pair<node_id_type,int>(getRight(P.first),0));
							break;
						}
						default:
							break;
					}
				}

				return true;
			}

			bool checkConsistency(node_id_type const node) const
			{
				std::deque<node_id_type> todo;
				if ( node != -1 )
					todo.push_back(node);

				bool ok = true;

				while ( ok && todo.size() )
				{
					node_id_type const cur = todo.front();
					todo.pop_front();

					node_id_type const left = getLeft(cur);
					node_id_type const right = getRight(cur);

					if ( left != -1 )
					{
						ok = ok && comparator(Aelements[left].key,Aelements[cur].key);
						ok = ok && getParent(left) == cur;
						todo.push_back(left);
					}
					if ( right != -1 )
					{
						ok = ok && comparator(Aelements[cur].key,Aelements[right].key);
						ok = ok && getParent(right) == cur;
						todo.push_back(right);
					}
				}

				return ok;
			}

			bool checkConsistency() const
			{
				return checkConsistency(root);
			}


			bool isLeftChild(node_id_type const x)
			{
				if ( getLeft(getParent(x)) == x )
					return true;
				else
					return false;
			}

			bool isRightChild(node_id_type const x)
			{
				if ( getRight(getParent(x)) == x )
					return true;
				else
					return false;
			}

			void setLeft(node_id_type const node, node_id_type const left)
			{
				if ( node != -1 )
					Aelements[node].left = left;
			}

			void setRight(node_id_type const node, node_id_type const right)
			{
				if ( node != -1 )
					Aelements[node].right = right;
			}

			void setKey(node_id_type const node, key_type const & key)
			{
				if ( node != -1 )
					Aelements[node].key = key;
			}

			/**
			 * replace child of node (left or right) by newchild
			 **/
			void replaceChild(node_id_type node, node_id_type child, node_id_type newchild)
			{
				if ( node != -1 )
				{
					if ( getLeft(node) == child )
						setLeft(node,newchild);
					else if ( getRight(node) == child )
						setRight(node,newchild);
				}
			}

			/**
			 * set parent of node to parent
			 **/
			void setParent(node_id_type node, node_id_type parent)
			{
				if ( node != -1 )
					Aelements[node].parent = parent;
			}

			/**
			 * right rotation around parent of x for moving x up the tree
			 **/
			void rotateRight(node_id_type x)
			{
				assert ( Aelements[x].parent != -1 );

				node_id_type const u = getLeft(x);
				node_id_type const v = getRight(x);
				node_id_type const p = getParent(x);
				node_id_type const pp = getParent(p);
				node_id_type const w = getRight(p);
				bool const pisroot = getParent(p) == -1;

				setLeft(x,u);
				setParent(u,x);
				setRight(x,p);
				setParent(p,x);

				setLeft(p,v);
				setParent(v,p);
				setRight(p,w);
				setParent(w,p);

				setParent(x,pp);
				replaceChild(pp,p,x);

				if ( pisroot )
					root = x;
			}

			/**
			 * left rotation around parent of x for moving x up the tree
			 **/
			void rotateLeft(node_id_type x)
			{
				assert ( Aelements[x].parent != -1 );

				node_id_type const u = getLeft(x);
				node_id_type const v = getRight(x);
				node_id_type const p = getParent(x);
				node_id_type const pp = getParent(p);
				node_id_type const w = getLeft(p);
				bool const pisroot = getParent(p) == -1;

				setLeft(p,w);
				setParent(w,p);
				setRight(p,u);
				setParent(u,p);

				setLeft(x,p);
				setParent(p,x);
				setRight(x,v);
				setParent(v,x);

				setParent(x,pp);
				replaceChild(pp,p,x);

				if ( pisroot )
					root = x;
			}

			/**
			 * make x the root of the tree by rotations
			 **/
			void splayUp(node_id_type x)
			{
				while ( getParent(x) != -1 )
					if ( isLeftChild(x) )
						rotateRight(x);
					else
						rotateLeft(x);
			}

			public:
			SplayTree(comparator_type const & rcomparator = comparator_type())
			: Pbase(new base_type()), base(*Pbase), Aelements(base.getElementsArray()), comparator(rcomparator), root(-1) {}

			SplayTree(base_type & rbase, comparator_type const & rcomparator = comparator_type())
			: Pbase(), base(rbase), Aelements(base.getElementsArray()), comparator(rcomparator), root(-1) {}

			node_id_type getParent(node_id_type const node) const
			{
				if ( node != -1 )
					return Aelements[node].parent;
				else
					return -1;
			}

			node_id_type getLeft(node_id_type const node) const
			{
				if ( node != -1 )
					return Aelements[node].left;
				else
					return -1;
			}

			node_id_type getRight(node_id_type const node) const
			{
				if ( node != -1 )
					return Aelements[node].right;
				else
					return -1;
			}

			key_type const & getKey(node_id_type const node) const
			{
				return Aelements[node].key;
			}

			/**
			 * insert key v into the tree and make it the tree root.
			 * If key is already in the tree then it is replaced by v
			 **/
			void insert(key_type const & v)
			{
				node_id_type parent = -1;
				node_id_type node = root;

				while ( node != -1 )
				{
					// get key of current node
					key_type const & c = getKey(node);

					// v smaller than c, go to left sub tree
					if ( comparator(v,c) )
					{
						parent = node;
						node = getLeft(node);
					}
					// v larger than c, go to right sub tree
					else if ( comparator(c,v) )
					{
						parent = node;
						node = getRight(node);
					}
					// key is already in tree, bring it to the root and quit
					else
					{
						setKey(node,v);
						splayUp(node);
						return;
					}
				}

				assert ( node == -1 );

				if ( parent != -1 )
				{
					// get key of parent node
					key_type const c = getKey(parent);

					node_id_type const child = base.getNewNode();

					setKey(child,v);
					setLeft(child,-1);
					setRight(child,-1);
					setParent(child,parent);

					if ( comparator(v,c) )
						setLeft(parent,child);
					else
						setRight(parent,child);

					splayUp(child);
				}
				else
				{
					root = base.getNewNode();
					setParent(root,-1);
					setLeft(root,-1);
					setRight(root,-1);
					Aelements[root].key = v;
				}
			}

			/**
			 * search for key and make it the root of the tree if found. Returns -1 if value not found
			 **/
			node_id_type find(key_type const & key)
			{
				node_id_type node = root;

				while ( node != -1 )
				{
					key_type const & ref = getKey(node);

					if ( comparator(key,ref) )
						node = getLeft(node);
					else if ( comparator(ref,key) )
						node = getRight(node);
					else
					{
						splayUp(node);
						return node;
					}
				}

				return -1;
			}

			/**
			 * search smallest >= key, return node id or -1 (none found)
			 **/
			node_id_type lowerBound(key_type const & key)
			{
				// tree empty?
				if ( root == -1 )
					return -1;

				// parent of node
				node_id_type parent = -1;
				// current node
				node_id_type node = root;

				// follow path to key
				while ( node != -1 )
				{
					parent = node;
					key_type const & ref = getKey(node);

					if ( comparator(key,ref) )
						node = getLeft(node);
					else if ( comparator(ref,key) )
						node = getRight(node);
					// found key
					else
					{
						splayUp(node);
						return node;
					}
				}

				assert ( node == -1 );
				assert ( parent != -1 );

				// follow path up to root until we find a suitable key
				while ( parent != -1 && comparator(key,getKey(parent)) )
					parent = getParent(parent);

				// found one?
				if ( parent != -1 )
					splayUp(parent);

				return parent;
			}

			/**
			 * remove key from tree if it exists
			 **/
			void erase(key_type const & key)
			{
				// search for key
				node_id_type node = root;

				while ( node != -1 )
				{
					key_type const & ref = getKey(node);

					if ( comparator(key,ref) )
						node = getLeft(node);
					else if ( comparator(ref,key) )
						node = getRight(node);
					else
						break;
				}

				// if we found a matching node
				while ( node != -1 )
				{
					// if node has a left child
					if ( getLeft(node) != -1 )
					{
						// look for rightmost node in left subtree of node
						node_id_type parent = -1;
						node_id_type cur = getLeft(node);

						while ( cur != -1 )
						{
							parent = cur;
							cur = getRight(cur);
						}

						assert ( cur == -1 );
						assert ( parent != -1 );

						// copy key
						setKey(node,getKey(parent));
						// delete node further down in the tree
						node = parent;
					}
					// otherwise if node has right child
					else if ( getRight(node) != -1 )
					{
						// look for leftmost node in right subtree of node
						node_id_type parent = -1;
						node_id_type cur = getRight(node);

						while ( cur != -1 )
						{
							parent = cur;
							cur = getLeft(cur);
						}

						assert ( cur == -1 );
						assert ( parent != -1 );

						// copy key
						setKey(node,getKey(parent));
						// delete node further down in the tree
						node = parent;
					}
					// no children, remove node
					else
					{
						assert ( getLeft(node) == -1 );
						assert ( getRight(node) == -1 );

						// erase in parent
						replaceChild(getParent(node),node,-1);
						// erase root if node was root
						if ( node == root )
							root = -1;

						// add to free list
						base.deleteNode(node);
						// quit loop
						node = -1;
					}
				}
			}

			/**
			 * check consistency of tree (order and forward/reverse link consistency)
			 **/
			bool check() const
			{
				return checkOrder() && checkConsistency();
			}

			void toStringStruct(std::ostream & out) const
			{
				if ( root != -1 )
					toStringStruct(out,root);
			}

			std::string toStringStruct() const
			{
				std::ostringstream ostr;
				toStringStruct(ostr);
				return ostr.str();
			}

			void toString(std::ostream & out) const
			{
				std::vector<key_type> const V = extract();
				for ( uint64_t i = 0; i < V.size(); ++i )
					out << V[i] << ";";
			}

			std::string toString() const
			{
				std::ostringstream ostr;
				toString(ostr);
				return ostr.str();
			}

			/**
			 * extract key set and return it as a vector
			 **/
			std::vector<key_type> extract() const
			{
				std::vector<key_type> V;
				extract(V);
				return V;
			}

			/**
			 * extract keys in ascending order into V. Keys are appended to V, i.e. V is not emptied before adding
			 **/
			void extract(std::vector<key_type> & V) const
			{
				std::stack< std::pair<node_id_type,int> > todo;

				if ( root != -1 )
					todo.push(std::pair<node_id_type,int>(root,0));

				while ( !todo.empty() )
				{
					std::pair<node_id_type,int> const P = todo.top();
					todo.pop();

					switch ( P.second )
					{
						case 0:
						{
							todo.push(std::pair<node_id_type,int>(P.first,1));
							if ( getLeft(P.first) != -1 )
								todo.push(std::pair<node_id_type,int>(getLeft(P.first),0));
							break;
						}
						case 1:
						{
							V.push_back(getKey(P.first));
							if ( getRight(P.first) != -1 )
								todo.push(std::pair<node_id_type,int>(getRight(P.first),0));
							break;
						}
						default:
							break;
					}
				}
			}

			node_id_type folowLeft(node_id_type node) const
			{
				node_id_type parent = -1;

				while ( node != -1 )
				{
					parent = node;
					node = getLeft(node);
				}

				assert ( node == -1 );

				return parent;
			}

			/**
			 * clear the tree
			 **/
			void clear()
			{
				std::stack< std::pair<node_id_type,int> > todo;

				if ( root != -1 )
					todo.push(std::pair<node_id_type,int>(root,0));

				while ( !todo.empty() )
				{
					std::pair<node_id_type,int> const P = todo.top();
					todo.pop();

					switch ( P.second )
					{
						case 0:
						{
							todo.push(std::pair<node_id_type,int>(P.first,1));
							if ( getLeft(P.first) != -1 )
								todo.push(std::pair<node_id_type,int>(getLeft(P.first),0));
							break;
						}
						case 1:
						{
							node_id_type const right = getRight(P.first);

							base.deleteNode(P.first);

							if ( right != -1 )
								todo.push(std::pair<node_id_type,int>(right,0));
							break;
						}
						default:
							break;
					}
				}

				root = -1;
			}

			node_id_type getNext(node_id_type const node) const
			{
				if ( getRight(node) != -1 )
				{
					node_id_type cur = getRight(node);
					node_id_type parent = -1;

					while ( cur != -1 )
					{
						parent = cur;
						cur = getLeft(cur);
					}

					assert ( cur == -1 );
					assert ( parent != -1 );
					assert ( comparator(getKey(node),getKey(parent)) );

					return parent;
				}
				else
				{
					// follow path to the root until cur is a left child
					// or we can no longer go up
					node_id_type cur = node;
					node_id_type parent = getParent(cur);

					while ( parent != -1 && getLeft(parent) != cur )
					{
						cur = parent;
						parent = getParent(parent);
					}

					if ( parent != -1 )
						assert ( comparator(getKey(node),getKey(parent)) );

					return parent;
				}
			}

			node_id_type getPrev(node_id_type const node) const
			{
				if ( getLeft(node) != -1 )
				{
					node_id_type cur = getLeft(node);
					node_id_type parent = -1;

					while ( cur != -1 )
					{
						parent = cur;
						cur = getRight(cur);
					}

					assert ( cur == -1 );
					assert ( parent != -1 );
					assert ( comparator(getKey(parent),getKey(node)) );

					return parent;
				}
				else
				{
					// follow path to the root until cur is a right child
					// or we can no longer go up
					node_id_type cur = node;
					node_id_type parent = getParent(cur);

					while ( parent != -1 && getRight(parent) != cur )
					{
						cur = parent;
						parent = getParent(parent);
					}

					if ( parent != -1 )
						assert ( comparator(getKey(parent),getKey(node)) );

					return parent;
				}
			}

			node_id_type chain(key_type const & key)
			{
				// tree empty?
				if ( root == -1 )
					return -1;

				// parent of node
				node_id_type parent = -1;
				// current node
				node_id_type node = root;

				// follow path to key
				while ( node != -1 )
				{
					parent = node;
					key_type const & ref = getKey(node);

					if ( comparator(key,ref) ) // key < ref? go left
						node = getLeft(node);
					else if ( comparator(ref,key) ) // key > ref? go right
						node = getRight(node);
					// found key
					else
					{
						splayUp(node);
						return node;
					}
				}

				assert ( node == -1 );

				while ( parent != -1 )
				{
					// node < key
					if ( comparator(getKey(parent),key) )
					{
						splayUp(parent);
						return parent;
					}

					parent = getParent(parent);
				}

				return -1;
			}
		};
	}
}
#endif
