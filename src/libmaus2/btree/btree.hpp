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


#if ! defined(BTREE_HPP)
#define BTREE_HPP

#include <ostream>
#include <vector>
#include <algorithm>
#include <stack>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace btree
	{

		template<typename key_type, unsigned int k >
		struct BTreeNode
		{
			BTreeNode<key_type,k> * parent;
			key_type keys[2*k];
			BTreeNode<key_type,k> * pointers[2*k+1];
			unsigned int n;

			BTreeNode() : parent(0), n(0)
			{
				for ( uint64_t i = 0; i < 2*k+1; ++i )
					pointers[i] = 0;
			}
			BTreeNode(key_type key, BTreeNode<key_type, k> * left, BTreeNode<key_type, k> * right) : parent(0), n(1)
			{
				for ( uint64_t i = 0; i < 2*k+1; ++i )
					pointers[i] = 0;
				keys[0] = key;
				pointers[0] = left;
				pointers[1] = right;
			}

			~BTreeNode()
			{
				for ( uint64_t i = 0; i < 2*k+1; ++i )
				{
					delete pointers[i];
					pointers[i] = 0;
				}
			}
			
			void insertKeySpaceLeaf(key_type key, unsigned int i)
			{
				// shift keys
				while ( i < n )
				{
					key_type tempkey = keys[i];
					keys[i] = key;
					key = tempkey;
					i += 1;
				}			
					
				keys[i] = key;
				n += 1;		
			}

			std::pair < key_type, BTreeNode<key_type,k> *> insertKeyFullLeaf(key_type key, unsigned int i)
			{
				key_type tempkeys[2*k+1];
				
				unsigned int ii = 0;
				for ( unsigned int j = 0; j < i; ++j )
					tempkeys[ii++] = keys[j];

				tempkeys[ii++] = key;
				
				for ( unsigned int j = i; j < n; ++j )
					tempkeys[ii++] = keys[j];
								
				BTreeNode<key_type,k> * nleft = this, * nright = 0;
				
				try
				{
					nright = new BTreeNode;
					nright->parent = nleft->parent;
					
					for ( unsigned int i = 0; i < k; ++i )
						nleft->keys[i] = tempkeys[i];
					nleft->n = k;

					for ( unsigned int i = 0; i < k; ++i )
						nright->keys[i] = tempkeys[k+1+i];
					nright->n = k;

					return std::pair < key_type, BTreeNode<key_type,k> *>(tempkeys[k], nright);
				}
				catch(...)
				{
					delete nright;
					nright = 0;
					throw;
				}
			}

			void insertKeySpaceNode(key_type key, unsigned int i, BTreeNode<key_type,k> * newnode)
			{
				BTreeNode<key_type,k> ** pp = (&pointers[0]) + i + 1;
				
				// shift keys
				while ( i < n )
				{
					key_type tempkey = keys[i];
					keys[i] = key;
					key = tempkey;
					
					BTreeNode<key_type,k> * tempptr = *pp;
					*pp = newnode;
					newnode = tempptr;
					
					i += 1;
					pp += 1;
				}			
				
				keys[i] = key;
				*pp = newnode;
				n += 1;
			}

			std::pair < key_type, BTreeNode<key_type,k> *> insertKeyFullNode(
				key_type key, unsigned int i, BTreeNode<key_type,k> * newnode
			)
			{
				key_type tempkeys[2*k+1];
				BTreeNode<key_type,k> * temppointers[2*k+2];
				
				unsigned int ii = 0;
				unsigned int pp = 0;
				for ( unsigned int j = 0; j < i; ++j )
				{
					tempkeys[ii++] = keys[j];
					temppointers[pp++] = pointers[j];
				}
				
				tempkeys[ii++] = key;
				temppointers[pp++] = pointers[i];
				temppointers[pp++] = newnode;
				
				for ( unsigned int j = i; j < n; ++j )
				{
					tempkeys[ii++] = keys[j];
					temppointers[pp++] = pointers[j+1];
				}
				
				BTreeNode<key_type,k> * nleft = this, * nright = 0;
				
				try
				{
					nright = new BTreeNode;
					nright->parent = nleft->parent;
					
					for ( unsigned int j = 0; j < k; ++j )
						nleft->keys[j] = tempkeys[j];
					nleft->n = k;

					for ( unsigned int j = 0; j < k; ++j )
						nright->keys[j] = tempkeys[k+1+j];
					nright->n = k;
					
					for ( unsigned int j = 0; j < k+1; ++j )
						nleft->pointers[j] = temppointers[j];
					for ( unsigned int j = k+1; j < 2*k+1; ++j )
						nleft->pointers[j] = 0;
					for ( unsigned int j = 0; j < k+1; ++j )
						nright->pointers[j] = temppointers[k+1+j];

					return std::pair < key_type, BTreeNode<key_type,k> *>(tempkeys[k],nright);
				}
				catch(...)
				{
					delete nright;
					nright = 0;
					throw;
				}
			}
			
		};

		template<typename key_type, unsigned int k>
		std::ostream & operator << (std::ostream & out, BTreeNode<key_type, k> const & node)
		{
			out << "BTreeNode(n=" << node.n;
			
			unsigned int i = 0;
			for ( ; i < node.n; ++i )
			{
				out << ",";
				if ( node.pointers[i] )
					out << *(node.pointers[i]);
				else
					out << "*";
				out << "," << node.keys[i];
			}
			
			out << ",";
			if ( node.pointers[i] )
				out << *(node.pointers[i]);
			else
				out << "*";
				
			out << ")";
			
			return out;
		}
		
		struct BTreeKeyNodeContainedException : public std::exception
		{
			virtual char const * what() const throw()
			{
				return "BTreeKeyNodeContainedException()";
			}
		};

		template<typename key_type, unsigned int k, typename comp_type = std::less< key_type > >
		struct BTree
		{
			BTreeNode<key_type, k> * root;
			comp_type comp;
			
			BTree() : root(0)
			{}
			~BTree()
			{
				delete root;
				root = 0;
			}
			
			void clear()
			{
				delete root;
				root = 0;
			}
			
			bool insert(key_type key)
			{
				if ( ! root )
					root = new BTreeNode<key_type,k>;
				
				BTreeNode< key_type, k > * insnode;
				bool const inserted = insert(root,key,insnode);

				// root was split
				if ( insnode )
				{
					root = new BTreeNode<key_type, k>(key,root,insnode);
					root->pointers[0]->parent = root;
					root->pointers[1]->parent = root;
				}
				
				return inserted;
			}
			
			key_type const & operator[](key_type const key)
			{
				if ( ! root )
					throw BTreeKeyNodeContainedException();
				else
					return findNode(root,key);
			}

			key_type const & findNode(BTreeNode<key_type, k> const * node, key_type const key) const
			{
				while ( true )
				{
					bool const isleaf = (node->pointers[0] == 0);

					if ( isleaf )
					{
						for ( unsigned int i = 0; i < node->n; ++i )
							if ( 
								(!(comp(key,node->keys[i])))
								&&
								(!(comp(node->keys[i],key)))
							) // comp
								return node->keys[i];

						throw BTreeKeyNodeContainedException();
					}
					else // inner node
					{
						BTreeNode<key_type,k> const * recnode = 0;
						
						for ( unsigned int i = 0; i < node->n; ++i )
							if ( 
								(!(comp(key, node->keys[i])))
								&&
								(!(comp(node->keys[i], key)))
							) // comp
								return node->keys[i];
							else if ( comp(key, node->keys[i]) )
							{
								recnode = node->pointers[i];
								break;
							}

						if ( ! recnode )
							recnode = node->pointers[node->n];

						node = recnode;
					}
				}
			}
			
			bool contains(key_type const key) const
			{
				if ( ! root )
					return false;
				else
				{
					try
					{
						findNode(root,key);
						return true;
					}
					catch(BTreeKeyNodeContainedException const &)
					{
						return false;
					}
				}
			}

			enum walkAction 
			{ 
				PROCESS_CHILDREN,
				PROCESS_ELEMENT	
			};
			
			struct WalkElement
			{
				walkAction action;
				BTreeNode<key_type, k> const * node;
				int i;
				
				WalkElement() {}
				WalkElement(
					walkAction raction,
					BTreeNode<key_type, k> const * rnode,
					int ri
					)
				: action(raction), node(rnode), i(ri)
				{}
			};
			
			struct ToVectorCallback
			{
				std::vector < key_type > & V;
				
				ToVectorCallback(std::vector < key_type > & rV) : V(rV) {}
				
				void operator()(key_type const & key)
				{
					V.push_back(key);
				}
			};
			
			void toVector(std::vector<key_type> & V) const
			{
				V.resize(0);
				ToVectorCallback cb(V);
				inorderWalk(cb);
			}
			
			template<typename callback_type>
			void inorderWalk(callback_type & callback) const
			{
				std::stack < WalkElement > S;
				
				if ( root )
					S.push( WalkElement(PROCESS_CHILDREN, root, 0)  );
				
				while ( ! S.empty() )
				{
					WalkElement el = S.top(); S.pop();
					
					switch ( el.action )
					{
						case PROCESS_CHILDREN:
							if ( el.node->pointers[el.node->n] )
								S.push(WalkElement(PROCESS_CHILDREN, el.node->pointers[el.node->n], 0) );
							
							for ( int i = el.node->n - 1; i >= 0; --i )
							{
								S.push(WalkElement(PROCESS_ELEMENT, el.node, i) );
								if ( el.node->pointers[i] )
									S.push(WalkElement(PROCESS_CHILDREN, el.node->pointers[i], 0) );
							}
							
							break;
						case PROCESS_ELEMENT:
							callback(el.node->keys[el.i]);
							// V.push_back(el.node->keys[el.i]);
							break;
					}
				}
			}

			uint64_t size() const
			{
				std::stack < BTreeNode<key_type,k> const * > S;
				if ( root )
					S.push(root);
				
				uint64_t s = 0;
				while ( ! S.empty() )
				{
					BTreeNode<key_type,k> const * curnode = S.top(); S.pop();
					
					s += curnode->n;
					
					for ( unsigned int i = 0; i < curnode->n + 1u; ++i )
						if ( curnode->pointers[i] )
							S.push(curnode->pointers[i]);
				}
				
				return s;
			}

			bool insert(BTreeNode<key_type,k> * node, key_type & key, BTreeNode<key_type,k> * & insnode)
			{
				insnode = 0;
				
				std::stack < BTreeNode<key_type,k> * > S;
				S.push(node);
				
				while ( S.top()->pointers[0] )
				{
					BTreeNode<key_type,k> * curnode = S.top();

					unsigned int j = curnode->n;
					for ( unsigned int i = 0; i < curnode->n; ++i )
						// key is already present
						if ( 
							(!(comp(key, curnode->keys[i]))) &&
							(!(comp(curnode->keys[i], key)))
						) // comp
						{
							return false;
						}
						else if ( comp(key, curnode->keys[i]) )
						{
							j = i;
							break;
						}

					S.push ( curnode->pointers[j] );
				}
				
				assert ( ! S.empty() );
				assert ( ! S.top()->pointers[0] );

				// BTreeNode < key_type,k > * insnode = 0;
				
				while ( ! S.empty() )
				{
					BTreeNode<key_type,k> * curnode = S.top(); S.pop();
					
					// leaf
					if ( curnode->pointers[0] == 0 )
					{
						unsigned int i  = 0;

						// count elements smaller than key
						while ( (i < curnode->n) && (comp(curnode->keys[i], key)) )
							++i;

						// check if key is already present
						if ( 
							!( (i < curnode->n) && 
							(!(comp(key, curnode->keys[i]))) &&
							(!(comp(curnode->keys[i], key))) )
						) // comp
						{
							if ( curnode->n < 2*k )
							{
								curnode->insertKeySpaceLeaf(key,i);

								insnode = 0;
								key = 0;
							}
							else
							{
								std::pair < key_type, BTreeNode<key_type,k> *> insinf =
									 curnode->insertKeyFullLeaf(key,i);
								
								key = insinf.first;
								insnode = insinf.second;
							}
						}
						// key present
						else
						{
							return false;
						}
					}
					else if ( insnode )
					{
						unsigned int i  = 0;

						// count elements smaller than key
						while ( (i < curnode->n) && (comp(curnode->keys[i], key)) )
							++i;

						if ( curnode->n < 2 * k )
						{
							curnode->insertKeySpaceNode(key, i, insnode);
							insnode = 0;
						}
						else
						{
							std::pair < key_type, BTreeNode<key_type,k> *> insinf =
								 curnode->insertKeyFullNode(key, i, insnode);
							key = insinf.first;
							insnode = insinf.second;
						}
					}
				}

				return true;
			}

		};

		template<typename key_type, unsigned int k>
		std::ostream & operator << (std::ostream & out, BTree<key_type, k> const & node)
		{
			out << "BTree(" << (*(node.root)) << ")";
			return out;
		}
	}
}
#endif
