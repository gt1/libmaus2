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

#include <libmaus2/autoarray/AutoArray.hpp>
#include <deque>
#include <stack>

namespace libmaus2
{
	namespace util
	{
		template<typename _key_type, typename _comparator_type = std::less<_key_type> >
		struct SplayTree
		{
			typedef _key_type key_type;
			typedef _comparator_type comparator_type;
			typedef SplayTree<key_type,comparator_type> this_type;

			struct SplayTreeElement
			{
				int64_t parent;
				int64_t left;
				int64_t right;
				key_type key;
			};

			comparator_type const comparator;

			int64_t root;
			libmaus2::autoarray::AutoArray<SplayTreeElement> Aelements;

			libmaus2::autoarray::AutoArray<uint64_t> Afreelist;
			uint64_t freelisthigh;

			void toStringStruct(std::ostream & out, int64_t node) const
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
				std::stack< std::pair<int64_t,int> > todo;

				if ( root != -1 )
					todo.push(std::pair<int64_t,int>(root,0));

				key_type prev;
				bool prevvalid = false;

				while ( !todo.empty() )
				{
					std::pair<int64_t,int> const P = todo.top();
					todo.pop();

					switch ( P.second )
					{
						case 0:
						{
							todo.push(std::pair<int64_t,int>(P.first,1));
							if ( getLeft(P.first) != -1 )
								todo.push(std::pair<int64_t,int>(getLeft(P.first),0));
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
								todo.push(std::pair<int64_t,int>(getRight(P.first),0));
							break;
						}
						default:
							break;
					}
				}

				return true;
			}

			bool checkConsistency(int64_t const node) const
			{
				std::deque<int64_t> todo;
				if ( node != -1 )
					todo.push_back(node);

				bool ok = true;

				while ( ok && todo.size() )
				{
					int64_t const cur = todo.front();
					todo.pop_front();

					int64_t const left = getLeft(cur);
					int64_t const right = getRight(cur);

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

			int64_t getNewNode()
			{
				if ( !freelisthigh )
				{
					uint64_t const oldsize = Aelements.size();
					Aelements.bump();
					uint64_t const newsize = Aelements.size();
					uint64_t const dif = newsize - oldsize;
					Afreelist.ensureSize(dif);
					for ( uint64_t i = 0; i < dif; ++i )
						Afreelist[freelisthigh++] = i+oldsize;
				}

				int64_t const node = Afreelist[--freelisthigh];

				return node;
			}

			void deleteNode(int64_t node)
			{
				Afreelist.push(freelisthigh,node);
			}

			void freeNode(int64_t node)
			{
				Afreelist.push(freelisthigh,node);
			}

			bool isLeftChild(int64_t const x)
			{
				if ( getLeft(getParent(x)) == x )
					return true;
				else
					return false;
			}

			bool isRightChild(int64_t const x)
			{
				if ( getRight(getParent(x)) == x )
					return true;
				else
					return false;
			}

			void setLeft(int64_t const node, int64_t const left)
			{
				if ( node != -1 )
					Aelements[node].left = left;
			}

			void setRight(int64_t const node, int64_t const right)
			{
				if ( node != -1 )
					Aelements[node].right = right;
			}

			void setKey(int64_t const node, key_type const & key)
			{
				if ( node != -1 )
					Aelements[node].key = key;
			}

			/**
			 * replace child of node (left or right) by newchild
			 **/
			void replaceChild(int64_t node, int64_t child, int64_t newchild)
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
			void setParent(int64_t node, int64_t parent)
			{
				if ( node != -1 )
					Aelements[node].parent = parent;
			}

			void rotateRight(int64_t x)
			{
				assert ( Aelements[x].parent != -1 );

				int64_t const u = getLeft(x);
				int64_t const v = getRight(x);
				int64_t const p = getParent(x);
				int64_t const pp = getParent(p);
				int64_t const w = getRight(p);
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

			void rotateLeft(int64_t x)
			{
				assert ( Aelements[x].parent != -1 );

				int64_t const u = getLeft(x);
				int64_t const v = getRight(x);
				int64_t const p = getParent(x);
				int64_t const pp = getParent(p);
				int64_t const w = getLeft(p);
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

			void splayUp(int64_t x)
			{
				while ( getParent(x) != -1 )
					if ( isLeftChild(x) )
						rotateRight(x);
					else
						rotateLeft(x);
			}

			public:
			SplayTree(comparator_type const & rcomparator = comparator_type())
			: comparator(rcomparator), root(-1), freelisthigh(0)
			{

			}

			int64_t getParent(int64_t const node) const
			{
				if ( node != -1 )
					return Aelements[node].parent;
				else
					return -1;
			}

			int64_t getLeft(int64_t const node) const
			{
				if ( node != -1 )
					return Aelements[node].left;
				else
					return -1;
			}

			int64_t getRight(int64_t const node) const
			{
				if ( node != -1 )
					return Aelements[node].right;
				else
					return -1;
			}

			key_type const & getKey(int64_t const node) const
			{
				return Aelements[node].key;
			}

			void insert(key_type const & v)
			{
				int64_t parent = -1;
				int64_t node = root;

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

					int64_t const child = getNewNode();

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
					root = getNewNode();
					setParent(root,-1);
					setLeft(root,-1);
					setRight(root,-1);
					Aelements[root].key = v;
				}
			}

			/**
			 * search for key and make it the root of the tree if found
			 **/
			int64_t find(key_type const & key)
			{
				int64_t node = root;

				while ( node != -1 )
				{
					key_type const & ref = getKey(node);

					if ( comp(key,ref) )
						node = getLeft(node);
					else if ( comp(ref,key) )
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
			 * search smallest >= key
			 **/
			int64_t lowerBound(key_type const & key)
			{
				// tree empty?
				if ( root == -1 )
					return -1;

				// parent of node
				int64_t parent = -1;
				// current node
				int64_t node = root;

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
				int64_t node = root;

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
						int64_t parent = -1;
						int64_t cur = getLeft(node);

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
						int64_t parent = -1;
						int64_t cur = getRight(node);

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
						deleteNode(node);
						// quit loop
						node = -1;
					}
				}
			}

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

			std::vector<key_type> extract() const
			{
				std::vector<key_type> V;
				extract(V);
				return V;
			}

			void extract(std::vector<key_type> & V) const
			{
				std::stack< std::pair<int64_t,int> > todo;

				if ( root != -1 )
					todo.push(std::pair<int64_t,int>(root,0));

				while ( !todo.empty() )
				{
					std::pair<int64_t,int> const P = todo.top();
					todo.pop();

					switch ( P.second )
					{
						case 0:
						{
							todo.push(std::pair<int64_t,int>(P.first,1));
							if ( getLeft(P.first) != -1 )
								todo.push(std::pair<int64_t,int>(getLeft(P.first),0));
							break;
						}
						case 1:
						{
							V.push_back(getKey(P.first));
							if ( getRight(P.first) != -1 )
								todo.push(std::pair<int64_t,int>(getRight(P.first),0));
							break;
						}
						default:
							break;
					}
				}
			}
		};
	}
}
#endif
