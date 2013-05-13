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

#if ! defined(HUFFMANTREELEAF_HPP)
#define HUFFMANTREELEAF_HPP

#include <libmaus/huffman/HuffmanTreeNode.hpp>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
        namespace huffman
        {
                struct HuffmanTreeLeaf : public HuffmanTreeNode
                {
                	int64_t symbol;
                        uint64_t frequency;

                        HuffmanTreeLeaf(int64_t rsymbol, uint64_t rfrequency) : symbol(rsymbol), frequency(rfrequency) {}
                        ~HuffmanTreeLeaf() {}
                        
                        uint64_t byteSize() const
                        {
                        	return sizeof(int64_t)+sizeof(uint64_t);
                        }
                                
                        bool isLeaf() const { return true; }
                        uint64_t getFrequency() const {
                                return frequency;
                        }
                        int64_t getSymbol() const {
                                return symbol;
                        }
                        void fillParentMap(::std::map < HuffmanTreeNode *, HuffmanTreeInnerNode * > &)
                        {}
                        virtual void fillLeafMap(::std::map < int64_t, HuffmanTreeLeaf * > & M)
                        {
                                M[symbol] = this;
                        }
                        virtual void structureVector(::std::vector < bool > & B) const
                        {
                                B.push_back(0);
                                B.push_back(1);
                        }
                        virtual void symbolVector(::std::vector < int64_t > & B) const
                        {
                                B.push_back(symbol);
                        }
                        virtual void depthVector(::std::vector < uint64_t > & B, uint64_t depth = 0) const
                        {
                                B.push_back(depth);
                        }
                        virtual void symbolDepthVector(::std::vector < std::pair < int64_t, uint64_t > > & B, uint64_t depth = 0) const
                        {
                                B.push_back(std::pair<int64_t,uint64_t>(symbol,depth));
                        }

                        HuffmanTreeNode * clone() const
                        {
                                return new HuffmanTreeLeaf(symbol,frequency);
                        }
                        void addPrefix(uint64_t prefix, uint64_t shift) 
                        {
                                symbol |= (prefix << shift);
                        }                
                        void square(HuffmanTreeNode *, uint64_t)
                        {
                                throw ::std::runtime_error("Cannot square unary alphabet tree.");
                        }
                        virtual void toDot(::std::ostream & out) const
                        {
                                out << "digraph vcsn {\n";
                                out << "graph[rankdir=LR];\n";
                                out << "node [shape=circle];\n";
                                
                                out << "state"<<0<<" [label=\""<<symbol<<"\"];\n";

                                out << "}\n";
                        }                
                        void  fillIdMap(::std::map<  HuffmanTreeNode const *, uint64_t > & idmap, uint64_t & cur) const
                        {
                                idmap[this] = cur++;
                        }
                        void lineSerialise(::std::ostream & out, ::std::map<  HuffmanTreeNode const *, uint64_t > const & idmap) const 
                        {
                                out 
                                        << idmap.find(this)->second << "\t"
                                        << "leaf" << "\t"
                                        << symbol << "\t"
                                        << frequency << "\n";
                        }
                        
                        uint64_t depth() const
                        {
                                return 0;
                        }
                };
        }
}
#endif
