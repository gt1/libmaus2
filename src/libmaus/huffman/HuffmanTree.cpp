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
#include <libmaus/huffman/HuffmanTree.hpp>

::std::ostream & libmaus::huffman::operator<<(::std::ostream & out, libmaus::huffman::HuffmanTree const & H)
{
	H.printRec(out,H.root());
	return out;
}

::std::ostream & libmaus::huffman::operator<<(::std::ostream & out, libmaus::huffman::HuffmanTree::EncodeTable const & E)
{
	for ( int64_t i = E.minsym; i <= E.maxsym; ++i )
		if ( E.hasSymbol(i) )
		{
			out << i << "\t" << E.getCode(i) << "\t" << E.getCodeLength(i) << "\t";
			libmaus::huffman::HuffmanTree::printCode(out,E.getCode(i),E.getCodeLength(i));
			out << "\t";
			for ( unsigned int j = 0; j < E.getCodeLength(i); ++j )
				out << (E.getBitFromTop(i,j)!=0);
			out << std::endl;
		}
	return out;
}
