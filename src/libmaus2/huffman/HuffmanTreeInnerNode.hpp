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

#if ! defined(HUFFMANTREEINNERNODE_HPP)
#define HUFFMANTREEINNERNODE_HPP

#include <libmaus2/huffman/HuffmanTreeNode.hpp>
#include <libmaus2/huffman/HuffmanTreeLeaf.hpp>

#include <map>
#include <deque>
#include <stack>

namespace libmaus2
{
        namespace huffman
        {
                struct HuffmanTreeInnerNode : public HuffmanTreeNode
                {
                        HuffmanTreeNode * left;
                        HuffmanTreeNode * right;
                        uint64_t const frequency;
                        
                        HuffmanTreeInnerNode(
                                HuffmanTreeNode * const rleft, 
                                HuffmanTreeNode * const rright, 
                                uint64_t const rfrequency
                        )
                        : left(rleft), right(rright), frequency(rfrequency) {}
                        ~HuffmanTreeInnerNode() { delete left; delete right; }

                        bool isLeaf() const { return false; }
                        uint64_t getFrequency() const {
                                return frequency;
                        }
                        
                        uint64_t byteSize() const
                        {
                        	uint64_t s = 0;
                        	
                        	s += 2*sizeof(HuffmanTreeNode *) + sizeof(uint64_t);
                        	if ( left )
                        		s += left->byteSize();
                        	if ( right )
                        		s += right->byteSize();
                        		
				return s;
                        }
                        
                        void fillParentMap(::std::map < HuffmanTreeNode *, HuffmanTreeInnerNode * > & M)
                        {
                                if ( left )
                                {
                                        M[left] = this;
                                        if ( dynamic_cast<HuffmanTreeInnerNode *>(left) )
                                                dynamic_cast<HuffmanTreeInnerNode *>(left)->fillParentMap(M);
                                }
                                if ( right )
                                {
                                        M[right] = this;
                                        if ( dynamic_cast<HuffmanTreeInnerNode *>(right) )
                                                dynamic_cast<HuffmanTreeInnerNode *>(right)->fillParentMap(M);
                                }
                        }
                        virtual void fillLeafMap(::std::map < int64_t, HuffmanTreeLeaf * > & M)
                        {
                                if ( left )
                                        left->fillLeafMap(M);
                                if ( right )
                                        right->fillLeafMap(M);
                        }
                        virtual void structureVector(::std::vector < bool > & B) const
                        {
                                B.push_back ( 0 );
                                
                                if ( left )
                                        left->structureVector(B);
                                if ( right )
                                        right->structureVector(B);

                                B.push_back ( 1 );
                        }
                        virtual void symbolVector(::std::vector < int64_t > & B) const
                        {
                                if ( left )
                                        left->symbolVector(B);
                                if ( right )
                                        right->symbolVector(B);
                        }
                        virtual void depthVector(::std::vector < uint64_t > & B, uint64_t depth = 0) const
                        {
                                if ( left )
                                        left->depthVector(B,depth+1);
                                if ( right )
                                        right->depthVector(B,depth+1);
                        }
                        virtual void symbolDepthVector(::std::vector < std::pair < int64_t, uint64_t > > & B, uint64_t depth = 0) const
                        {
                                if ( left )
                                        left->symbolDepthVector(B,depth+1);
                                if ( right )
                                        right->symbolDepthVector(B,depth+1);                        
                        }
                        
                        HuffmanTreeNode * clone() const
                        {
                                return new HuffmanTreeInnerNode(left->clone(), right->clone(), frequency);
                        }
                        
                        void addPrefix(uint64_t prefix, uint64_t shift)
                        {
                                left->addPrefix(prefix,shift);
                                right->addPrefix(prefix,shift);
                        }
                        virtual void square(HuffmanTreeNode * original, uint64_t shift)
                        {
                                if ( dynamic_cast<HuffmanTreeInnerNode *>(left) )
                                        left->square(original,shift);
                                else
                                {
                                        HuffmanTreeLeaf * leaf = dynamic_cast<HuffmanTreeLeaf *>(left);
                                        int64_t const subsym = leaf->symbol;
                                        HuffmanTreeNode * clone = original->clone();
                                        clone->addPrefix(subsym,shift);
                                        left = clone;
                                        delete leaf;
                                }
                                if ( dynamic_cast<HuffmanTreeInnerNode *>(right) )
                                        right->square(original,shift);
                                else
                                {
                                        HuffmanTreeLeaf * leaf = dynamic_cast<HuffmanTreeLeaf *>(right);
                                        int64_t const subsym = leaf->symbol;
                                        HuffmanTreeNode * clone = original->clone();
                                        clone->addPrefix(subsym,shift);
                                        right = clone;
                                        delete leaf;
                                }
                        }
                        
                        enum Visit { First, Second, Third };
                        
                        virtual void toDot(::std::ostream & out) const
                        {
                                out << "digraph vcsn {\n";
                                out << "graph[rankdir=LR];\n";
                                out << "node [shape=circle];\n";

                                ::std::stack < ::std::pair < HuffmanTreeNode const *, Visit > > S;
                                S.push( ::std::pair < HuffmanTreeNode const *, Visit > (this,First) );
                                
                                ::std::map < HuffmanTreeInnerNode const *, uint64_t > nodeToId;
                                ::std::map < HuffmanTreeLeaf const *, uint64_t > leafToId;
                                ::std::vector < HuffmanTreeInnerNode const * > idToNode;
                                
                                uint64_t leafid = 0;

                                while ( ! S.empty() )
                                {
                                        HuffmanTreeNode const * node = S.top().first; 
                                        Visit const firstvisit = S.top().second;
                                        S.pop();
                                        
                                        if ( node->isLeaf() )
                                        {
                                                HuffmanTreeLeaf const * leaf = dynamic_cast<HuffmanTreeLeaf const *>(node);
                                                out << "leaf"<<leafid<<" [label=\""<<static_cast<char>(leaf->symbol)<< ":" << leaf->frequency <<"\"];\n";
                                                leafToId[leaf] = leafid++;
                                        }
                                        else 
                                        {
                                                HuffmanTreeInnerNode const * inode = dynamic_cast<HuffmanTreeInnerNode const *>(node);
                                                
                                                if ( firstvisit == First )
                                                {
                                                        uint64_t const id = idToNode.size();
                                                        nodeToId[inode] = id;
                                                        idToNode.push_back(inode);
                                                        
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Second) );
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->left,First) );

                                                        out << "inner"<<id<<" [label=\""<< id <<"\"];\n";
                                                }
                                                else if ( firstvisit == Second )
                                                {
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Third) );
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->right,First) );
                                                }
                                                else
                                                {
                                                }
                                        }
                                }

                                S.push( ::std::pair < HuffmanTreeNode const *, Visit > (this,First) );

                                while ( ! S.empty() )
                                {
                                        HuffmanTreeNode const * node = S.top().first; 
                                        Visit const firstvisit = S.top().second;
                                        S.pop();
                                        
                                        if ( node->isLeaf() )
                                        {
                                        }
                                        else 
                                        {
                                                HuffmanTreeInnerNode const * inode = dynamic_cast<HuffmanTreeInnerNode const *>(node);
                                                
                                                if ( firstvisit == First )
                                                {
                                                        HuffmanTreeNode const * lleft = inode->left;
                                                        HuffmanTreeNode const * lright = inode->right;
                                                        
                                                        if ( lleft->isLeaf() )
                                                                out << "inner" << nodeToId.find(inode)->second << " -> leaf" << leafToId.find(dynamic_cast<HuffmanTreeLeaf const *>(lleft))->second << " [label=\"0\"]\n";
                                                        else
                                                                out << "inner" << nodeToId.find(inode)->second << " -> inner" << nodeToId.find(dynamic_cast<HuffmanTreeInnerNode const *>(lleft))->second << " [label=\"0\"]\n";

                                                        if ( lright->isLeaf() )
                                                                out << "inner" << nodeToId.find(inode)->second << " -> leaf" << leafToId.find(dynamic_cast<HuffmanTreeLeaf const *>(lright))->second << " [label=\"1\"]\n";
                                                        else
                                                                out << "inner" << nodeToId.find(inode)->second << " -> inner" << nodeToId.find(dynamic_cast<HuffmanTreeInnerNode const *>(lright))->second << " [label=\"1\"]\n";
                                                
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Second) );
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->left,First) );
                                                }
                                                else if ( firstvisit == Second )
                                                {
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Third) );
                                                        S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->right,First) );
                                                }
                                                else
                                                {
                                                }
                                        }
                                }


                                #if 0
                                ::std::map < HuffmanTreeNode const *, uint64_t > nodeIds;
                                ::std::deque < HuffmanTreeNode const * > Q;
                                Q.push_back(this);
                                uint64_t inode = 0;
                                
                                // assign node ids
                                while ( ! Q.empty() )
                                {
                                        HuffmanTreeNode const * cur = Q.front();
                                        Q.pop_front();

                                        uint64_t const id = nodeIds.size();
                                        nodeIds[cur] = id;
                                        
                                        if ( cur->isLeaf() )
                                        {
                                                HuffmanTreeLeaf const * leaf = dynamic_cast<HuffmanTreeLeaf const *>(cur);
                                                out << "state"<<id<<" [label=\""<<static_cast<char>(leaf->symbol)<< ":" << leaf->frequency <<"\"];\n";			
                                        }
                                        else
                                        {
                                                HuffmanTreeInnerNode const * inner = dynamic_cast<HuffmanTreeInnerNode const *>(cur);
                                                Q.push_back(inner->left);
                                                Q.push_back(inner->right);
                                                out << "state"<<id<<" [label=\""<< (inode++)  <<"\"];\n";			
                                        }
                                }

                                Q.push_back(this);
                                
                                // assign node ids
                                while ( ! Q.empty() )
                                {
                                        HuffmanTreeNode const * cur = Q.front();
                                        Q.pop_front();
                                        
                                        if ( !cur->isLeaf() )
                                        {
                                                HuffmanTreeInnerNode const * inner = dynamic_cast<HuffmanTreeInnerNode const *>(cur);
                                                Q.push_back(inner->left);
                                                Q.push_back(inner->right);
                                                
                                                assert ( nodeIds.find(cur) != nodeIds.end() );
                                                assert ( nodeIds.find(inner->left) != nodeIds.end() );
                                                assert ( nodeIds.find(inner->right) != nodeIds.end() );
                                                
                                                out << "state" << nodeIds.find(cur)->second << " -> state" << nodeIds.find(inner->left)->second << " [label=\"0\"]\n";
                                                out << "state" << nodeIds.find(cur)->second << " -> state" << nodeIds.find(inner->right)->second << " [label=\"1\"]\n";
                                        }
                                }
                                #endif

                                out << "}\n";                
                        }

                        void  fillIdMap(::std::map<  HuffmanTreeNode const *, uint64_t > & idmap, uint64_t & cur) const
                        {
                                idmap[this] = cur++;
                                if ( left )
                                        left->fillIdMap(idmap,cur);
                                if ( right )
                                        right->fillIdMap(idmap,cur);
                        }

                        virtual void lineSerialise(::std::ostream & out, ::std::map<  HuffmanTreeNode const *, uint64_t > const & idmap) const
                        {
                                if ( left )
                                        left->lineSerialise(out,idmap);
                                if ( right )
                                        right->lineSerialise(out,idmap);
                                
                                out 
                                        << idmap.find(this)->second << "\t"
                                        << "inner" << "\t"
                                        << (left ? idmap.find(left)->second : 0) << "\t"
                                        << (right ? idmap.find(right)->second : 0) << "\n";
                        }
                        
                        uint64_t depth() const
                        {
                                return 1ull + std::max(left->depth(),right->depth());
                        }
                };
        }
}
#endif
