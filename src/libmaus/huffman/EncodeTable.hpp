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

#if ! defined(ENCODETABLE_HPP)
#define ENCODETABLE_HPP

#include <sstream>
#include <vector>
#include <limits>

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/uint/uint.hpp>

#include <libmaus/huffman/HuffmanTreeNode.hpp>
#include <libmaus/huffman/HuffmanTreeInnerNode.hpp>
#include <libmaus/huffman/HuffmanTreeLeaf.hpp>

namespace libmaus
{
	namespace huffman 
	{
		template<unsigned int words = 1>
		struct EncodeTable
		{
			typedef EncodeTable<words> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			enum Visit { First, Second, Third };
			
			int64_t minsym;
			int64_t maxsym;
			::libmaus::autoarray::AutoArray< ::std::pair < libmaus::uint::UInt < words >, unsigned int > > codes;
			::std::vector<bool> codeused;
		
			bool getBitFromTop(int64_t sym, unsigned int bit) const
			{
				assert ( codeused[sym-minsym] );
				return (*this)[sym].first.getBit((*this)[sym].second-bit-1);
			}
			
			template<typename iterator>
			uint64_t getCodeLength(iterator a, iterator e) const
			{
				uint64_t len = 0;
				for ( ; a != e; ++a )
					len += (*this)[*a].second;
				return len;
			}
			
			bool checkSymbol(int64_t sym) const
			{
				if ( !(sym >= minsym && sym <= maxsym) )
					return false;
				
				return codeused[sym-minsym];
			}
			
			std::vector < int64_t > getSymbols() const
			{
				std::vector<int64_t> V;
				for ( uint64_t i = minsym; i <= maxsym; ++i )
					if ( checkSymbol(i) )
						V.push_back(i);
				return V;
			}
			
			::std::pair < libmaus::uint::UInt < words >, unsigned int > const & operator[] ( int64_t sym ) const
			{
				assert ( checkSymbol(sym) );
				return codes [ sym - minsym ];
			}

			::std::pair < libmaus::uint::UInt < words >, unsigned int > getNonMsbBits ( int64_t sym ) const
			{
				assert ( sym >= minsym && sym <= maxsym );
				assert ( codeused[sym-minsym] );
				::std::pair < libmaus::uint::UInt < words >, unsigned int > cp = codes [ sym - minsym ];
				if ( cp.second )
					cp.second -= 1;
				cp.first.keepLowBits(cp.second);
				return cp;
			}
			
			
			static ::std::string printCode(libmaus::uint::UInt<words> const U, unsigned int const length)
			{
				libmaus::uint::UInt<words> mask(1ull); mask <<= length-1;
				::std::ostringstream ostr;
				
				while ( mask != libmaus::uint::UInt<words>() )
				{
					ostr << 
						((U & mask)!=libmaus::uint::UInt<words>());			
					mask >>= 1;
				}

				return ostr.str();
			}

			::std::string printCode(uint64_t const sym) const
			{
				return printCode (
					(*this)[sym].first,
					(*this)[sym].second );
			}

			::std::pair < ::std::vector < uint64_t >, ::std::vector < uint64_t > > symsOrderedByCode() const
			{
				::std::map < ::std::string , uint64_t > M;
			
				for ( int64_t i = minsym; i <= maxsym; ++i )
					if ( codeused[i-minsym] )
						M [ printCode(i) ] = i;
					
				::std::vector <uint64_t > V;
				::std::vector <uint64_t > R ( maxsym-minsym+1 );
				for ( ::std::map < ::std::string , uint64_t > :: const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					R.at(ita->second - minsym) = V.size();
					V.push_back ( ita->second );
				}
					
				return ::std::pair < ::std::vector < uint64_t >, ::std::vector < uint64_t > >(V,R);
			}
			::std::map<int64_t,uint64_t> symsOrderedByCodeMap() const
			{
				::std::map < ::std::string , uint64_t > M;
			
				for ( int64_t i = minsym; i <= maxsym; ++i )
					if ( codeused[i-minsym] )
						M [ printCode(i) ] = i;
					
				::std::map<int64_t,uint64_t> R;
				for ( ::std::map < ::std::string , uint64_t > :: const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					uint64_t const rank = R.size();
					R[ita->second] = rank;
				}
					
				return R;
			}
			
			static ::std::vector<bool> codeToVector(libmaus::uint::UInt<words> const U, unsigned int const length)
			{
				libmaus::uint::UInt<words> mask(1ull); mask <<= length-1;
				::std::vector<bool> V;
				
				while ( mask != libmaus::uint::UInt<words>() )
				{
					V . push_back ( ((U & mask)!=libmaus::uint::UInt<words>()) );
					mask >>= 1;
				}

				return V;
			}
			
			::std::vector<bool> codeToVector(uint64_t const sym) const
			{
				return codeToVector (
					(*this)[sym].first,
					(*this)[sym].second );
			}

			void print() const
			{
				for ( int64_t i = minsym; i <= maxsym; ++i )
					if ( codeused[i-minsym] )
						::std::cerr << i << "\t" 
							<< printCode( i )
							<< "\t" 
							<< (*this)[i].second << ::std::endl;
			}
			void printChar() const
			{
				for ( int64_t i = minsym; i <= maxsym; ++i )
					if ( codeused[i-minsym] )
						::std::cerr << static_cast<char>(i) << "\t" 
							<< printCode( i )
							<< "\t" 
							<< (*this)[i].second << ::std::endl;
			}

			template<typename iter>
			void print(iter const & R) const
			{
				for ( int64_t i = minsym; i <= maxsym; ++i )
					if ( codeused[i-minsym] )
						::std::cerr << static_cast<char>(R[i]) << "\t" 
							<< printCode( i )
							<< "\t" 
							<< (*this)[i].second << ::std::endl;
			}


			void printSorted() const
			{
				::std::pair < ::std::vector < uint64_t >, ::std::vector < uint64_t > > const VR = symsOrderedByCode();
				::std::vector < uint64_t > const & V = VR.first;
				::std::vector < uint64_t > const & R = VR.second;
				
				for ( uint64_t j = 0; j < V.size(); ++j )
				{
					uint64_t const i = V[j];
					
					if ( codeused[i-minsym] )
						::std::cerr << static_cast<char>(i) << "\t" 
							<< printCode( i )
							<< "\t" 
							<< (*this)[i].second 
							<< "\t"
							<< R[i-minsym]
							<< ::std::endl;
				}
			}

			static void print(HuffmanTreeNode const * node, ::std::vector<bool> & V)
			{
				if ( node->isLeaf() )
				{
					HuffmanTreeLeaf const * lnode = dynamic_cast<HuffmanTreeLeaf const *>(node);
					
					::std::cerr << static_cast<char>(lnode->symbol) << "\t";
					for ( uint64_t i = 0; i < V.size(); ++i )
						::std::cerr << V[i];
					::std::cerr << ::std::endl;			
				}
				else 
				{
					HuffmanTreeInnerNode const * inode = dynamic_cast<HuffmanTreeInnerNode const *>(node);
					
					V.push_back(0);
					print(inode->left,V);
					V.pop_back();
					
					V.push_back(1);
					print(inode->right,V);
					V.pop_back();	
				}
			}	
			
			uint64_t getSymbolRange() const
			{
				return (maxsym-minsym+1);
			}

			EncodeTable(HuffmanTreeNode const * root)
			{
				::std::stack < ::std::pair < HuffmanTreeNode const *, Visit > > S;

				uint64_t numleafs = 0;
				maxsym = ::std::numeric_limits<int64_t>::min();
				minsym = ::std::numeric_limits<int64_t>::max();
				
				S.push( ::std::pair < HuffmanTreeNode const *, Visit > (root,First) );
				
				::std::vector<bool> V;
				while ( ! S.empty() )
				{
					HuffmanTreeNode const * node = S.top().first; 
					Visit const firstvisit = S.top().second;
					S.pop();
					
					if ( node->isLeaf() )
					{
						HuffmanTreeLeaf const * lnode = dynamic_cast<HuffmanTreeLeaf const *>(node);
						
						/*
						::std::cerr << static_cast<char>(lnode->symbol) << "\t";
						for ( uint64_t i = 0; i < V.size(); ++i )
							::std::cerr << V[i];
						::std::cerr << ::std::endl;
						*/
					
						numleafs++;
						maxsym = ::std::max(maxsym,lnode->symbol);
						minsym = ::std::min(minsym,lnode->symbol);
					}
					else 
					{
						HuffmanTreeInnerNode const * inode = dynamic_cast<HuffmanTreeInnerNode const *>(node);
						
						if ( firstvisit == First )
						{
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Second) );
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->left,First) );
							V.push_back(false);
						}
						else if ( firstvisit == Second )
						{
							V.pop_back();
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Third) );
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->right,First) );
							V.push_back(true);
						}
						else
						{
							V.pop_back();
						}
					}
				}
				
				/*
				::std::cerr << "Minimum symbol " << minsym << ::std::endl;
				::std::cerr << "Maximum symbol " << maxsym << ::std::endl;
				*/
				
				codes = ::libmaus::autoarray::AutoArray< ::std::pair < libmaus::uint::UInt < words >, unsigned int > > ( maxsym-minsym+1 );
				codeused.resize( maxsym-minsym+1 );
				for ( uint64_t i = 0; i < static_cast<uint64_t>(maxsym-minsym+1); ++i )
					codeused[i] = 0;

				S.push( ::std::pair < HuffmanTreeNode const *, Visit > (root,First) );
				while ( ! S.empty() )
				{
					HuffmanTreeNode const * node = S.top().first; 
					Visit const firstvisit = S.top().second;
					S.pop();
					
					if ( node->isLeaf() )
					{
						HuffmanTreeLeaf const * lnode = dynamic_cast<HuffmanTreeLeaf const *>(node);
						assert ( V.size() <= words*64 );
						
						libmaus::uint::UInt<words> U;
						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							U <<= 1;
							U |= libmaus::uint::UInt<words>(static_cast<uint64_t>(V[i]));
						}
						
						codes [ lnode->symbol - minsym ]  = ::std::pair < libmaus::uint::UInt<words>, unsigned int > (U,V.size());
						codeused [ lnode->symbol - minsym ] = true;
					}
					else 
					{
						HuffmanTreeInnerNode const * inode = dynamic_cast<HuffmanTreeInnerNode const *>(node);
						
						if ( firstvisit == First )
						{
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Second) );
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->left,First) );
							V.push_back(false);
						}
						else if ( firstvisit == Second )
						{
							V.pop_back();
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (node,Third) );
							S.push( ::std::pair < HuffmanTreeNode const *, Visit > (inode->right,First) );
							V.push_back(true);
						}
						else
						{
							V.pop_back();
						}
					}
				}
				// print();
			}
		};
	}
}
#endif
