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

#if ! defined(HUFFMANSORTING_HPP)
#define HUFFMANSORTING_HPP

#include <iostream>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/ReverseTable.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/bitio/getBits.hpp>
#include <libmaus/bitio/putBit.hpp>
#include <libmaus/huffman/huffman.hpp>

namespace libmaus
{
	namespace huffman
	{
		struct HuffmanSorting
		{
			
			struct SymBitPair
			{
				bool bit;
				char sym;
				
				SymBitPair()
				: bit(false), sym(0)
				{}
				SymBitPair(bool const rbit, char const rsym)
				: bit(rbit), sym(rsym)
				{
					
				}
				
				bool operator< (SymBitPair const & o) const
				{
					return bit < o.bit;
				}
			};

			/*
			 * find first entry with most significant bit set
			 *
			 * @param A code
			 * @param offset bit offset in code
			 * @param length number of symbols to consider
			 * @param dectable
			 * @param enctable
			 */
			template<unsigned int words>
			static ::std::pair < uint64_t, uint64_t > findFirstBit(
				uint64_t const * const A,
				uint64_t const offset,
				uint64_t const length,
				huffman::DecodeTable const & dectable,
				huffman::EncodeTable<words> const & enctable,
				bool const bit
				)
			{
				uint64_t decoded_bits = 0;
				uint64_t decoded_symbols = 0;
				uint64_t read_offset = offset;
				unsigned int tableid = 0;
				libmaus::uint::UInt<words> comp(static_cast<uint64_t>(bit));
				
				while ( decoded_symbols < length )
				{
					uint64_t const bits = bitio::getBits ( A, read_offset, dectable.lookupbits );
					
					huffman::DecodeTableEntry const & entry = dectable(tableid,bits);
					
					for ( unsigned int i = 0; i < entry.symbols.size(); ++i )
					{
						::std::pair < libmaus::uint::UInt < words >, unsigned int > const & code = enctable[entry.symbols[i]];
						
						if (
							(code.first >> (code.second-1)) == comp
						)
						{
							return ::std::pair < uint64_t, uint64_t > (offset + decoded_bits, decoded_symbols );
						}
							
						decoded_bits += code.second;
						decoded_symbols += 1;
						
						if ( decoded_symbols == length )
							return ::std::pair < uint64_t, uint64_t > (offset + decoded_bits, decoded_symbols );
					}
					
					tableid = entry.nexttable;
					read_offset += dectable.lookupbits;
				}
				
				return ::std::pair < uint64_t, uint64_t > (offset + decoded_bits, decoded_symbols );
			}	

			template<unsigned int words>
			static uint64_t getCodeOffset(
				uint64_t const * const A,
				uint64_t const offset,
				uint64_t const length,
				huffman::DecodeTable const & dectable,
				huffman::EncodeTable<words> const & enctable
				)
			{
				uint64_t decoded_bits = 0;
				uint64_t decoded_symbols = 0;
				uint64_t read_offset = offset;
				unsigned int tableid = 0;
				
				while ( decoded_symbols < length )
				{
					uint64_t const bits = bitio::getBits ( A, read_offset, dectable.lookupbits );
					
					huffman::DecodeTableEntry const & entry = dectable(tableid,bits);
					
					for ( unsigned int i = 0; i < entry.symbols.size(); ++i )
					{
						::std::pair < libmaus::uint::UInt < words >, unsigned int > const & code = enctable[entry.symbols[i]];
						
						decoded_bits += code.second;
						decoded_symbols += 1;
						
						if ( decoded_symbols == length )
							return offset + decoded_bits;
					}
					
					tableid = entry.nexttable;
					read_offset += dectable.lookupbits;
				}
				
				return offset + decoded_bits;
			}

			static void revertSlow(
				uint64_t * const A,
				uint64_t const l,
				uint64_t const h)
			{
				uint64_t const length = h-l;
				
				for ( uint64_t i = 0; i < length/2; ++i )
				{
					bool const v0 = bitio::getBit(A,l+i);
					bool const v1 = bitio::getBit(A,h-i-1);
					bitio::putBit(A,l+i,v1);
					bitio::putBit(A,h-i-1,v0);
				}
			}

			static void revert(
				uint64_t * const A,
				uint64_t const l,
				uint64_t const h,
				bitio::ReverseTable const & revtable)
			{
				uint64_t const length = h-l;
				uint64_t const doublewords = length / (revtable.b<<1);
				
				for ( uint64_t i = 0; i < doublewords; ++i )
				{
					uint64_t a = revtable ( revtable.b, bitio::getBits ( A, l + i*revtable.b, revtable.b ));
					uint64_t b = revtable ( revtable.b, bitio::getBits ( A, h - (i+1)*revtable.b, revtable.b ));
					bitio::putBits ( A, l+i*revtable.b, revtable.b, b );
					bitio::putBits ( A, h-(i+1)*revtable.b, revtable.b, a );		
				}
				
				uint64_t restl = l + doublewords * revtable.b;
				uint64_t resth = h - doublewords * revtable.b;
				uint64_t restlength2 = (resth-restl)/2;
				
				uint64_t a = revtable ( restlength2, bitio::getBits ( A, restl, restlength2 ));
				uint64_t b = revtable ( restlength2, bitio::getBits ( A, resth - restlength2, restlength2 ) );
				
				bitio::putBits ( A, restl, restlength2, b );
				bitio::putBits ( A, resth-restlength2, restlength2, a );
				
				assert ( restlength2 <= revtable.b );
			}
				
			template<unsigned int words>
			static bool mergeScan(
				uint64_t * const A,
				uint64_t const offset,
				uint64_t const length,
				huffman::DecodeTable const & dectable,
				huffman::EncodeTable<words> const & enctable,
				bitio::ReverseTable const & revtable
				)
			{
				uint64_t loffset = offset;
				uint64_t decoded_syms = 0;
				uint64_t numloops = 0;
				
				while ( decoded_syms != length )
				{
					::std::pair < uint64_t , uint64_t > Pt0 = findFirstBit(A,loffset,length-decoded_syms,dectable,enctable,true);
					decoded_syms += Pt0.second;
					
					::std::pair < uint64_t , uint64_t > Pt1 = findFirstBit(A,Pt0.first,length-decoded_syms,dectable,enctable,false);
					decoded_syms += Pt1.second;

					::std::pair < uint64_t , uint64_t > Pt2 = findFirstBit(A,Pt1.first,length-decoded_syms,dectable,enctable,true);
					decoded_syms += Pt2.second;

					::std::pair < uint64_t , uint64_t > Pt3 = findFirstBit(A,Pt2.first,length-decoded_syms,dectable,enctable,false);
					decoded_syms += Pt3.second;
					
					// revert inner parts
					revert(A, Pt0.first, Pt1.first, revtable);
					revert(A, Pt1.first, Pt2.first, revtable);
					// revert all inner
					revert(A, Pt0.first, Pt2.first, revtable);
					
					loffset = Pt3.first;
					numloops++;
				}
				
				return numloops <= 1;
			}

			template<unsigned int words>
			static void mergeScanSort(
				uint64_t * const A,
				uint64_t const offset,
				uint64_t const length,
				huffman::DecodeTable const & dectable,
				huffman::EncodeTable<words> const & enctable,
				bitio::ReverseTable const & revtable
				)
			{
				while ( !mergeScan ( A, offset, length, dectable, enctable, revtable ) )
				{}
			}

			template<unsigned int words>
			static void merge(
				uint64_t * const A,
				uint64_t const offset,
				uint64_t const length,
				uint64_t const mergelen,
				huffman::DecodeTable const & dectable,
				huffman::EncodeTable<words> const & enctable,
				bitio::ReverseTable const & revtable
				)
			{
				// double merge length
				uint64_t const dmergelen = mergelen << 1;
				// number of full merge loops
				uint64_t const fullloops = length / dmergelen;
				
				// bit offset of length merge part
				uint64_t loffset = offset;
				
				for ( uint64_t z = 0; z < fullloops; ++z )
				{
					// bit offset of right merge part
					uint64_t const roffset = getCodeOffset(A,loffset,mergelen,dectable,enctable);
					// bit offset of next left part
					uint64_t const nextloffset = getCodeOffset(A,roffset,mergelen,dectable,enctable);

					// find first code MSB in left part
					uint64_t const t0 = findFirstBit(A,loffset,mergelen,dectable,enctable,true).first;
					// find first code MSB in right part
					uint64_t const t1 = findFirstBit(A,roffset,mergelen,dectable,enctable,true).first;

					// revert inner parts
					revert(A,t0,roffset,revtable);
					revert(A,roffset,t1,revtable);
					// revert all inner
					revert(A,t0,t1,revtable);

					// set next left offset
					loffset = nextloffset;
				}
				
				// unhandled rest?
				if ( fullloops *  dmergelen + mergelen < length )
				{
					uint64_t const rest = length - (fullloops *  dmergelen + mergelen);
					
					// ::std::cerr << "length=" << length << " mergelen=" << mergelen << " rest = " << rest << ::std::endl;

					uint64_t const roffset = getCodeOffset(A,loffset,mergelen,dectable,enctable);
					uint64_t const nextloffset = getCodeOffset(A,roffset,rest,dectable,enctable);

					uint64_t const t0 = findFirstBit(A,loffset,mergelen,dectable,enctable,true).first;
					uint64_t const t1 = findFirstBit(A,roffset,rest,dectable,enctable,true).first;

					// revert inner parts
					revert(A,t0,roffset,revtable);
					revert(A,roffset,t1,revtable);
					// revert all inner
					revert(A,t0,t1,revtable);

					loffset = nextloffset;
				}
			}

			template<unsigned int words>
			static void mergeSort(
				uint64_t * const A,
				uint64_t const offset,
				uint64_t const length,
				huffman::DecodeTable const & dectable,
				huffman::EncodeTable<words> const & enctable,
				uint64_t mergelen = 1)
			{
				while ( mergelen < length )
				{
					merge(A,offset,length,mergelen,dectable,enctable);
					mergelen *= 2;
				}
			}


			template<unsigned int words>
			static void checkCodeOffset(
				huffman::EncodeTable<words> const & enctable,
				huffman::DecodeTable const & dectable,
				::std::string const & s,
				uint64_t const n,
				uint64_t const * const acode
			)
			{
				::std::vector < uint64_t > cl;
				for ( uint64_t i = 0; i < n; ++i )
					cl.push_back ( enctable [ s[i] ].second );

				::std::vector < uint64_t > ca;
				uint64_t sum = 0;
				for ( uint64_t i = 0; i < n; ++i )
				{
					uint64_t const t = cl[i];
					ca.push_back ( sum );
					sum += t;
				}
				
				for ( uint64_t i = 0; i < n; ++i )
					for ( uint64_t j = i; j < n; ++j )
						assert ( getCodeOffset(acode, ca[i], j-i, dectable, enctable) == ca[j] );
			}

			template<unsigned int words>
			static void printEncoded(
				huffman::EncodeTable<words> const & enctable,
				::std::string const & s,
				uint64_t const n
			)
			{
				for ( uint64_t i = 0; i < n; ++i )
					::std::cerr << huffman::EncodeTable<words>::printCode (
						enctable [ s[i] ].first,
						enctable[s[i]].second )
						<< " ";
				::std::cerr << ::std::endl;
			}

			template<unsigned int words>
			static void printDecoded(
				huffman::EncodeTable<words> const & /* enctable */,
				huffman::DecodeTable const & dectable,
				::std::string const & /* s */,
				uint64_t const n,
				uint64_t const * const acode,
				uint64_t const codelength
			)
			{
				unsigned int nodeid = 0;
				uint64_t decsyms = 0;
				for ( uint64_t i = 0; i < (codelength+dectable.lookupbits-1)/dectable.lookupbits; ++i )
				{
					huffman::DecodeTableEntry entry = dectable(nodeid,bitio::getBits(acode,dectable.lookupbits*i,dectable.lookupbits));

					for ( unsigned int j = 0; j < entry.symbols.size() && decsyms < n; ++j, ++decsyms )
						::std::cerr << static_cast<char>(entry.symbols[j]);
					
					nodeid = entry.nexttable;
				}
				::std::cerr << ::std::endl;
			}

			template<unsigned int lookupwords>
			static uint64_t compressHuffmanCoded(
				uint64_t * const acode,
				uint64_t const offset,
				uint64_t const n,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable,
				huffman::HuffmanTreeNode const * root
				)
			{	
				assert ( !root->isLeaf() );
				uint64_t const firsthighsym = findFirstBit(acode, offset /*offset bits*/, n/*length symbols*/, dectable, enctable, true).second;

				huffman::CompressTable<lookupwords> comptable ( root, dectable.lookupbits );

				/** compress **/
				uint64_t nodeid = 0;
				uint64_t decsyms = 0;
				uint64_t encbits = 0;
				uint64_t allcodebits = 0;
				for ( uint64_t i = 0; decsyms != n; ++i )
				{
					huffman::CompressTableEntry<lookupwords> const & entry = 
						comptable(nodeid,bitio::getBits(acode,offset + comptable.lookupbits*i,comptable.lookupbits));

					if ( decsyms + entry.symbols.size() < n )
					{
						entry.compressed.serialize(acode, offset + encbits, entry.bitswritten);
						encbits += entry.bitswritten;
						decsyms += entry.symbols.size();
						
						for ( uint64_t j = 0; j < entry.symbols.size(); ++j )
							allcodebits += enctable[ entry.symbols[j] ].second;
					}
					else
					{
						libmaus::uint::UInt<lookupwords> comp = entry.compressed;
						comp >>= (entry.bitswritten - entry.thresholds[n-decsyms-1]);
						
						comp.serialize(
							acode, 
							offset + encbits, 
							entry.thresholds[n-decsyms-1]
						);
						encbits += entry.thresholds [ n-decsyms-1 ];

						for ( uint64_t j = 0; decsyms != n; ++j )
						{
							allcodebits += enctable[ entry.symbols[j] ].second;
							decsyms += 1;
						}

						break;
					}
					
					nodeid = entry.nexttable;
				}
					
				/** copy back **/
				uint64_t const copybits = 32;
				for ( uint64_t i = 0; i < encbits / copybits; ++i )
					bitio::putBits ( acode, offset + allcodebits - (i+1)*copybits, copybits, bitio::getBits ( acode, offset + encbits - (i+1)*copybits, copybits ) );
				if ( encbits % copybits )
					bitio::putBits ( acode, offset + n, encbits % copybits, bitio::getBits ( acode, offset + 0, encbits % copybits ));
				// erase free bits	
				for ( uint64_t i = 0; i < n; ++i )
					bitio::putBit ( acode, offset + i, 0 );
					
				return firsthighsym;
			}

			template<unsigned int lookupwords>
			static void uncompressHuffmanCoded(
				uint64_t * const acode,
				uint64_t const rreencodeoffset,
				uint64_t const n,
				uint64_t const firsthighsym,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & /* dectable */,
				huffman::HuffmanTreeNode const * root,
				huffman::DecodeTable const & dectable0,
				huffman::DecodeTable const & dectable1,
				huffman::EncodeTable<lookupwords> const & enctable0,
				huffman::EncodeTable<lookupwords> const & enctable1
				)
			{
				assert ( ! root->isLeaf() );
				bool const leftIsLeaf = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->left->isLeaf();
				bool const rightIsLeaf = dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->right->isLeaf();
				
				/*
				huffman::DecodeTable dectable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->left, dectable.lookupbits );
				huffman::DecodeTable dectable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->right, dectable.lookupbits );
				huffman::EncodeTable<lookupwords> enctable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->left );
				huffman::EncodeTable<lookupwords> enctable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->right );
				*/

				uint64_t nodeid = 0;
				uint64_t decsyms = 0;
				uint64_t reencodeoffset = rreencodeoffset;
				uint64_t lowdecoffset = reencodeoffset + n;
				uint64_t highdecoffset = lowdecoffset;
				
				if ( leftIsLeaf )
				{
					while ( decsyms != firsthighsym )
					{
						bitio::putBit ( acode, reencodeoffset, 0 );
						reencodeoffset++;
						decsyms++;
					}
				}
				else
				{
					for ( uint64_t i = 0; decsyms != firsthighsym; ++i )
					{
						huffman::DecodeTableEntry const & entry = dectable0(nodeid,bitio::getBits(acode,lowdecoffset+dectable0.lookupbits*i,dectable0.lookupbits));

						for ( uint64_t j = 0; j < entry.symbols.size() && decsyms != firsthighsym; ++j )
						{
							int const sym = entry.symbols[j];
							highdecoffset += enctable0[sym].second;
							
							decsyms++;
							
							enctable [ sym ].first.serialize ( acode , reencodeoffset, enctable[sym].second );
							reencodeoffset += enctable[sym].second;
						}

						nodeid = entry.nexttable;
					}
				}

				nodeid = 0;
				uint64_t alldecoffset = highdecoffset;
				
				if ( rightIsLeaf )
				{
					while ( decsyms != n )
					{
						bitio::putBit ( acode, reencodeoffset, 1 );
						reencodeoffset++;
						decsyms++;		
					}	
				}
				else
				{
					for ( uint64_t i = 0; decsyms != n; ++i )
					{
						huffman::DecodeTableEntry const & entry = dectable1(nodeid,bitio::getBits(acode,highdecoffset+dectable1.lookupbits*i,dectable1.lookupbits));

						for ( uint64_t j = 0; j < entry.symbols.size() && decsyms != n; ++j )
						{
							int const sym = entry.symbols[j];
							alldecoffset += enctable1[sym].second;

							decsyms++;

							enctable [ sym ].first.serialize ( acode , reencodeoffset, enctable[sym].second );
							reencodeoffset += enctable[sym].second;
						}

						nodeid = entry.nexttable;
					}
				}
			}

			template<unsigned int lookupwords>
			static ::std::pair < uint64_t , uint64_t > getDecodableSyms(
				uint64_t const * const acode,
				uint64_t const offset,
				uint64_t const numbits,
				uint64_t const maxdecsyms,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable,
				uint64_t & msb,
				uint64_t & msb_decoded_bits,
				uint64_t & lsb,
				uint64_t & lsb_decoded_bits
				)
			{
				msb = 0;
				lsb = 0;
				msb_decoded_bits = 0;
				lsb_decoded_bits = 0;

				uint64_t decoded_bits = 0;
				uint64_t decoded_syms = 0;
				
				if ( ! maxdecsyms )
					return ::std::pair < uint64_t, uint64_t > (decoded_syms, offset + decoded_bits);

				uint64_t nodeid = 0;
				uint64_t i = 0;
					
				while ( true )
				{
					huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode,offset + dectable.lookupbits*i,dectable.lookupbits));
					
					for ( uint64_t j = 0; j < entry.symbols.size(); ++j )
					{
						// if we cannot decode another symbol, return current count
						if ( decoded_bits + enctable[ entry.symbols[j] ].second > numbits )
						{
							return ::std::pair < uint64_t, uint64_t > (decoded_syms, offset + decoded_bits);
						}
						// otherwise decode symbol
						else
						{
							::std::pair < libmaus::uint::UInt < lookupwords >, unsigned int > const & code = enctable[entry.symbols[j]];
						
							if ( (code.first >> (code.second-1)) == libmaus::uint::UInt<lookupwords>(1ull) )
							{
								msb++;
								msb_decoded_bits += enctable[ entry.symbols[j] ].second;
							}
							else
							{
								lsb++;
								lsb_decoded_bits += enctable[ entry.symbols[j] ].second;
							}
						
							decoded_bits += enctable[ entry.symbols[j] ].second;
							decoded_syms += 1;

							if ( decoded_syms == maxdecsyms )
								return ::std::pair < uint64_t, uint64_t > (decoded_syms, offset + decoded_bits);
						}
					}
					
					i++;
					nodeid = entry.nexttable;
				}
			}

			template<unsigned int lookupwords>
			static uint64_t getDecodeBound(
				uint64_t const * const acode,
				uint64_t const offset,
				uint64_t const length,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable)
			{
				uint64_t decoded_bits = 0;
				uint64_t decoded_syms = 0;
				
				if ( ! length )
					return offset;

				uint64_t nodeid = 0;
				uint64_t i = 0;
					
				while ( decoded_syms != length )
				{
					huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode,offset + dectable.lookupbits*i,dectable.lookupbits));
					
					for ( uint64_t j = 0; j < entry.symbols.size() && decoded_syms != length; ++j )
					{
						decoded_bits += enctable[ entry.symbols[j] ].second;
						decoded_syms += 1;
					}
					
					i++;
					nodeid = entry.nexttable;
				}
				
				return offset + decoded_bits;
			}

			template<unsigned int lookupwords>
			static uint64_t getNextOffset(
				uint64_t const * const acode,
				uint64_t const offset,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable)
			{
				uint64_t nodeid = 0;
				uint64_t i = 0;
					
				while ( true )
				{
					huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode,offset + dectable.lookupbits*i,dectable.lookupbits));
					
					if ( entry.symbols.size() )
						return offset + enctable[ entry.symbols[0] ].second;
					
					i++;
					nodeid = entry.nexttable;
				}
			}

			template<unsigned int lookupwords>
			static void countingSort(
				uint64_t * const acode,
				uint64_t const freeoffset,
				uint64_t const numfreebits,
				uint64_t const dataoffset,
				uint64_t const length,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable)
			{
				uint64_t decoded_symbols = 0;
				uint64_t next_decode_position = dataoffset;
				
				while ( decoded_symbols != length )
				{
					uint64_t msb;
					uint64_t msb_decoded_bits;
					uint64_t lsb;
					uint64_t lsb_decoded_bits;
					
					::std::pair < uint64_t, uint64_t > const decodable_data = getDecodableSyms(
						acode,next_decode_position,numfreebits,length-decoded_symbols,enctable,dectable,
						msb, msb_decoded_bits,lsb,lsb_decoded_bits);

					// ::std::cerr << "Decoding " << decodable_data.first << ::std::endl;

					/**
					 * we have space for some symbols, sort them
					 **/
					if ( decodable_data.first )
					{
						uint64_t lfreeoffset = freeoffset;
						uint64_t mfreeoffset = lfreeoffset + lsb_decoded_bits;
						uint64_t localdecodedsymbols = 0;
						
						uint64_t i = 0;
						uint64_t nodeid = 0;
						while ( localdecodedsymbols != decodable_data.first )
						{
							huffman::DecodeTableEntry const & entry = dectable(
								nodeid,bitio::getBits(acode,next_decode_position + dectable.lookupbits*i,dectable.lookupbits));
					
							for ( uint64_t j = 0; j < entry.symbols.size() && localdecodedsymbols != decodable_data.first; ++j )
							{
								::std::pair < libmaus::uint::UInt < lookupwords >, unsigned int > const & code = 
									enctable[entry.symbols[j]];
						
								// msb
								if ( (code.first >> (code.second-1)) == libmaus::uint::UInt<lookupwords>(1ull) )
								{
									code.first.serialize(acode,mfreeoffset,code.second);
									mfreeoffset += code.second;
								}
								// lsb
								else
								{
									code.first.serialize(acode,lfreeoffset,code.second);
									lfreeoffset += code.second;					
								}
								
								localdecodedsymbols++;
							}
					
							i++;
							nodeid = entry.nexttable;	
						}
						
						/** copy sorted portion back into code stream **/
						uint64_t const copyportion = 32;
						uint64_t const copylength = mfreeoffset-freeoffset;
						uint64_t const copywords = copylength / copyportion;
						uint64_t const copyrest = copylength - copywords*copyportion;
						uint64_t const copysrc = freeoffset;
						uint64_t const copydst = next_decode_position;
						
						for ( uint64_t i = 0; i < copywords; ++i )
							bitio::putBits ( acode, copydst + i*copyportion, copyportion, bitio::getBits ( acode, copysrc + i*copyportion, copyportion ) );
						if ( copyrest )
							bitio::putBits ( acode, copydst + copywords * copyportion, copyrest, bitio::getBits(acode,copysrc+copywords*copyportion,copyrest) );
							
						// ::std::cerr << "Decodable: " << decodable_data.first << ::std::endl;
						decoded_symbols += decodable_data.first;
						next_decode_position = decodable_data.second;
					}
					/**
					 * we do not have space for this symbol. 
					 * leave it as it is, one symbol is sorted
					 **/
					else
					{
						decoded_symbols ++;
						next_decode_position = getNextOffset(acode,next_decode_position,enctable,dectable);
					}
				}
			}

			template<unsigned int lookupwords>
			static void huffmanSortRecursive(
				uint64_t * const acode,
				uint64_t const offset,
				uint64_t const n,
				huffman::HuffmanTreeNode const * root,
				huffman::EncodeTable<lookupwords> const & enctable,
				huffman::DecodeTable const & dectable,
				bitio::ReverseTable const & revtable,
				huffman::DecodeTable const & dectable0,
				huffman::DecodeTable const & dectable1,
				huffman::EncodeTable<lookupwords> const & enctable0,
				huffman::EncodeTable<lookupwords> const & enctable1
				)
			{
				if ( n <= 1 )
					return;
				
				// ::std::cerr << "entering huffmanSortRecursive(" << n << ")" << ::std::endl;
				
				uint64_t const lowhalf = (n+1)/2;
				// recursively sort lower half
				huffmanSortRecursive ( acode, offset, lowhalf, root, enctable, dectable, revtable, dectable0,dectable1,enctable0,enctable1 );

				// ::std::cerr << "returned to huffmanSortRecursive(" << n << ")" << ::std::endl;

				// ::std::cerr << "getting decode bound...";
				uint64_t const dataoffset = getDecodeBound(acode, offset, lowhalf, enctable, dectable );
				// ::std::cerr << "done." << ::std::endl;

				// ::std::cerr << "compress...";
				uint64_t const firsthighsym = compressHuffmanCoded(acode, offset, lowhalf, enctable, dectable, root );
				// ::std::cerr << "done." << ::std::endl;

				// ::std::cerr << "counting sort...";
				countingSort(
					acode, 
					offset, /* freeoffset */
					lowhalf, /* num free bits */
					dataoffset, /* data offset */
					n-lowhalf, /* length */
					enctable,
					dectable);
				// ::std::cerr << "done." << ::std::endl;

				// ::std::cerr << "merge scan sort...";
				mergeScanSort(acode, dataoffset, n-lowhalf, dectable, enctable, revtable);
				// ::std::cerr << "." << ::std::endl;

				// ::std::cerr << "uncompress...";
				uncompressHuffmanCoded(acode, offset /* bit offset */, lowhalf /* syms */, firsthighsym, enctable,
					dectable,root,dectable0,dectable1,enctable0,enctable1);
				// ::std::cerr << "done." << ::std::endl;

				// ::std::cerr << "merge scan...";
				bool const mergeok = mergeScan ( acode , offset , n, dectable, enctable, revtable );
				// ::std::cerr << "done." << ::std::endl;
				
				assert ( mergeok );
			}

			static void huffmanSortRecursive(uint64_t * const acode, uint64_t const offset, uint64_t const n, huffman::HuffmanTreeNode const * root)
			{
				if ( root->isLeaf() )
					return;

				// build encoding table
				unsigned int const lookupwords = 2;
				huffman::EncodeTable<lookupwords> enctable ( root );

				unsigned int const lookupbits = 8;
				#if 0
				unsigned int const lookupbytes = lookupbits / 8;
				#endif
				huffman::DecodeTable dectable ( root, lookupbits );

				huffman::DecodeTable dectable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->left, dectable.lookupbits );
				huffman::DecodeTable dectable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->right, dectable.lookupbits );
				huffman::EncodeTable<lookupwords> enctable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->left );
				huffman::EncodeTable<lookupwords> enctable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root)->right );

				bitio::ReverseTable revtable(8);

				huffmanSortRecursive(acode, offset, n, root, enctable, dectable, revtable, dectable0, dectable1, enctable0, enctable1);

			}

			static void hufRegressionTest(
				::std::string const & s, 
				huffman::HuffmanTreeNode const * const root,
				uint64_t const * const acode,
				uint64_t bitoffset = 0,
				uint64_t symoffset = 0
				)
			{

				// build encoding table
				unsigned int const lookupwords = 2;
				huffman::EncodeTable<lookupwords> enctable ( root );

				unsigned int const lookupbits = 8;
				#if 0
				unsigned int const lookupbytes = lookupbits / 8;
				#endif
				huffman::DecodeTable dectable ( root, lookupbits );
				{
					::std::vector < SymBitPair > SBPV;
					for ( uint64_t i = symoffset; i < s.size(); ++i )
					{
						char const sym = s[i];
						bool const msb = (enctable [ sym ].first >> (enctable [ sym ].second-1))!=libmaus::uint::UInt<lookupwords>(0);
						SBPV.push_back ( SymBitPair ( msb, sym ) );
					}
					::std::stable_sort ( SBPV.begin(), SBPV.end() );

					uint64_t const todec = s.size() - symoffset;

					unsigned int nodeid = 0;
					uint64_t decsyms = 0;
					for ( uint64_t i = 0; decsyms != todec; ++i )
					{
						huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode,bitoffset + lookupbits*i,lookupbits));

						for ( uint64_t j = 0; j < entry.symbols.size() && decsyms != todec; ++j, ++decsyms )
							assert ( SBPV[decsyms].sym == entry.symbols[j] );
						
						nodeid = entry.nexttable;
					}
					// ::std::cerr << "verified." << ::std::endl;
				}

			}

			static void testMergeScanSort ( ::std::string const & s )
			{
				uint64_t const n = s.size();

				// build tree
				::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type root = huffman::HuffmanBase::createTree ( s.begin(), s.end() );
				
				if ( root->isLeaf() )
					return;
				
				// build encoding table
				unsigned int const lookupwords = 2;
				huffman::EncodeTable<lookupwords> enctable ( root.get() );
				// enctable.print();

				// encode
				uint64_t const codelength = enctable.getCodeLength(s.begin(),s.end());
				uint64_t const arraylength = (codelength + 63)/64+1;
				::libmaus::autoarray::AutoArray<uint64_t> acode(arraylength);
				bitio::FastWriteBitWriter8 fwbw8(acode.get());

				for ( uint64_t i = 0; i < n; ++i )
					enctable [ s[i] ].first.serialize ( fwbw8 , enctable[s[i]].second );
				fwbw8.flush();
				
				unsigned int const lookupbits = 8;
				#if 0
				unsigned int const lookupbytes = lookupbits / 8;
				#endif
				huffman::DecodeTable dectable ( root.get(), lookupbits );

				// printEncoded(enctable,s,n);
				// checkCodeOffset(enctable,dectable,s,n,acode.get());
				// printDecoded(enctable, dectable, s, n, acode.get(), codelength);

				// mergeSort(acode.get(), 0, n, dectable, enctable);
				bitio::ReverseTable revtable(8);
				mergeScanSort(acode.get(), 0, n, dectable, enctable, revtable);

				::std::vector < SymBitPair > SBPV;
				for ( uint64_t i = 0; i < n; ++i )
				{
					char const sym = s[i];
					bool const msb = (enctable [ sym ].first >> (enctable [ sym ].second-1))!=libmaus::uint::UInt<lookupwords>(0);
					SBPV.push_back ( SymBitPair ( msb, sym ) );
				}
				::std::stable_sort ( SBPV.begin(), SBPV.end() );

				unsigned int nodeid = 0;
				uint64_t decsyms = 0;
				for ( uint64_t i = 0; i < (codelength+lookupbits-1)/lookupbits; ++i )
				{
					huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode.get(),lookupbits*i,lookupbits));

					for ( uint64_t j = 0; j < entry.symbols.size() && decsyms < n; ++j, ++decsyms )
						assert ( SBPV[decsyms].sym == entry.symbols[j] );
					
					nodeid = entry.nexttable;
				}
			}

			static void testHufRecursive(::std::string const & s)
			{
				uint64_t const n = s.size();

				// build tree
				::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type root = huffman::HuffmanBase::createTree ( s.begin(), s.end() );
				
				if ( root->isLeaf() )
					return;
					
				#if 0
				for ( uint64_t z = 0; z < n; ++z )
				{
					::std::cerr << "*";

					// build encoding table
					unsigned int const lookupwords = 2;
					huffman::EncodeTable<lookupwords> enctable ( root.get() );
					// enctable.print();

					// encode
					uint64_t const codelength = enctable.getCodeLength(s.begin(),s.end());
					uint64_t const arraylength = (codelength + 63)/64+1;
					::libmaus::autoarray::AutoArray<uint64_t> acode(arraylength);

					uint64_t bitoffset = 0;
					for ( uint64_t i = 0; i < z; ++i )
						bitoffset += enctable [ s[i] ] . second;
				
					bitio::FastWriteBitWriter8 fwbw8(acode.get());
				
					for ( uint64_t i = 0; i < n; ++i )
						enctable [ s[i] ].first.serialize ( fwbw8 , enctable[s[i]].second );
					fwbw8.flush();

					huffmanSortRecursive(acode.get(), bitoffset /*offset*/, n - z, root.get());
					hufRegressionTest(s,root.get(),acode.get(),bitoffset,z);
				}
				#endif

				::libmaus::autoarray::AutoArray < uint64_t > acode = hufEncodeString(s.begin(),s.end(),root.get());

				huffmanSortRecursive(acode.get(), 0 /*offset*/, n, root.get());
				hufRegressionTest(s,root.get(),acode.get());
			}

			static void testHuf(::std::string const & s)
			{
				uint64_t const n = s.size();

				// build tree
				::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type root = huffman::HuffmanBase::createTree ( s.begin(), s.end() );
				
				if ( root->isLeaf() )
				{
					return;
				}
				
				// build encoding table
				unsigned int const lookupwords = 2;
				huffman::EncodeTable<lookupwords> enctable ( root.get() );
				// enctable.print();

				// encode
				uint64_t const codelength = enctable.getCodeLength(s.begin(),s.end());
				uint64_t const arraylength = (codelength + 63)/64+1;
				::libmaus::autoarray::AutoArray<uint64_t> acode(arraylength);
				bitio::FastWriteBitWriter8 fwbw8(acode.get());

				for ( uint64_t i = 0; i < n; ++i )
					enctable [ s[i] ].first.serialize ( fwbw8 , enctable[s[i]].second );
				fwbw8.flush();
				
				unsigned int const lookupbits = 8;
				#if 0
				unsigned int const lookupbytes = lookupbits / 8;
				#endif
				huffman::DecodeTable dectable ( root.get(), lookupbits );

				huffman::DecodeTable dectable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root.get())->left, dectable.lookupbits );
				huffman::DecodeTable dectable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root.get())->right, dectable.lookupbits );
				huffman::EncodeTable<lookupwords> enctable0 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root.get())->left );
				huffman::EncodeTable<lookupwords> enctable1 ( dynamic_cast<huffman::HuffmanTreeInnerNode const *>(root.get())->right );

				bitio::ReverseTable revtable(8);

				uint64_t lowhalf = (n+1)/2;

				mergeScanSort(acode.get(), 0, lowhalf, dectable, enctable, revtable);

				uint64_t const dataoffset = getDecodeBound(acode.get(), 0 /* offset */, lowhalf, enctable, dectable );

				uint64_t const firsthighsym = compressHuffmanCoded(acode.get(), 0 /* offset */, lowhalf, enctable, dectable, root.get() );

				countingSort(
					acode.get(), 
					0, /* freeoffset */
					lowhalf, /* num free bits */
					dataoffset, /* data offset */
					n-lowhalf, /* length */
					enctable,
					dectable);

				mergeScanSort(acode.get(), dataoffset, n-lowhalf, dectable, enctable, revtable);

				uncompressHuffmanCoded(acode.get(), 0 /* bit offset */, lowhalf /* syms */, firsthighsym, enctable,dectable,root.get(),dectable0,dectable1,enctable0,enctable1);

				bool const mergeok = mergeScan ( acode.get() , 0, n, dectable, enctable, revtable );
				assert ( mergeok );
				
				// ::std::cerr << "mergeok: " << mergeok << ::std::endl;
				
				{
					::std::vector < SymBitPair > SBPV;
					for ( uint64_t i = 0; i < n; ++i )
					{
						char const sym = s[i];
						bool const msb = (enctable [ sym ].first >> (enctable [ sym ].second-1))!=libmaus::uint::UInt<lookupwords>(0);
						SBPV.push_back ( SymBitPair ( msb, sym ) );
					}
					::std::stable_sort ( SBPV.begin(), SBPV.end() );

					unsigned int nodeid = 0;
					uint64_t decsyms = 0;
					for ( uint64_t i = 0; decsyms != n; ++i )
					{
						huffman::DecodeTableEntry const & entry = dectable(nodeid,bitio::getBits(acode.get(),lookupbits*i,lookupbits));

						for ( uint64_t j = 0; j < entry.symbols.size() && decsyms < n; ++j, ++decsyms )
							assert ( SBPV[decsyms].sym == entry.symbols[j] );
						
						nodeid = entry.nexttable;
					}
					// ::std::cerr << "verified." << ::std::endl;
				}
			}

			static void testHufRecursive()
			{
				for ( uint64_t j = 0; j < 1000; ++j )
				{
					::std::cerr << ".";

					uint64_t const n = 1 + (rand() % (1ull << 12));
					::std::string s(n,' ');
					unsigned int const alphabet_size = 1 + (rand() % 64);
					
					for ( uint64_t i = 0; i < n; ++i )
						s[i] = 'a' + rand() % alphabet_size;

					// ::std::cerr << s << ::std::endl;

					#if 0
					double const bef = clock();
					#endif
					testHufRecursive(s);
					#if 0
					double const aft = clock();
					#endif
					
					#if 0
					::std::cerr << "time for n=" << n << " is " << (aft-bef)/CLOCKS_PER_SEC 
						<< " per mb " <<  ((aft-bef)/CLOCKS_PER_SEC) / (n/(1024.0*1024.0)) << ::std::endl;
					#endif
				}
				::std::cerr << ::std::endl;
			}

			static void testHuf()
			{
				uint64_t const n = 512;
				::std::string s(n,' ');
				unsigned int const alphabet_size = 7;
				
				for ( uint64_t j = 0; j < 1000; ++j )
				{
					::std::cerr << ".";
					for ( uint64_t i = 0; i < n; ++i )
						s[i] = 'a' + rand() % alphabet_size;

					// ::std::cerr << s << ::std::endl;

					testMergeScanSort(s);
					testHuf(s);
				}
				::std::cerr << ::std::endl;
			}
			
			static void test()
			{
				::std::cerr << "Testing sorting of Huffman coded sequences...";
				::std::cerr << "Testing non-recursive...";
				testHuf();
				::std::cerr << "Testing recursive...";
				testHufRecursive();
				::std::cerr << "Huffman sorting check finished." << ::std::endl;
			}


		};
	}
}
#endif
