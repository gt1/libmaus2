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

#if ! defined(HUFFMANTREENODE_HPP)
#define HUFFMANTREENODE_HPP

#include <map>
#include <vector>
#include <memory>

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/getBits.hpp>
#include <libmaus2/bitio/BitWriter.hpp>

namespace libmaus2
{
	namespace huffman
	{
		struct HuffmanTreeInnerNode;
		struct HuffmanTreeLeaf;

		struct HuffmanTreeNode
		{
			typedef HuffmanTreeNode this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			virtual bool isLeaf() const = 0;
			virtual uint64_t getFrequency() const = 0;
			virtual ~HuffmanTreeNode() {}
			virtual void fillParentMap(::std::map < HuffmanTreeNode *, HuffmanTreeInnerNode * > & M) = 0;
			virtual void fillLeafMap(::std::map < int64_t, HuffmanTreeLeaf * > & M) = 0;
			virtual void structureVector(::std::vector < bool > & B) const = 0;
			virtual void symbolVector(::std::vector < int64_t > & B) const = 0;
                        virtual void depthVector(::std::vector < uint64_t > & B, uint64_t depth = 0) const = 0;
                        virtual uint64_t depth() const = 0;
                        virtual void symbolDepthVector(::std::vector < std::pair < int64_t, uint64_t > > & B, uint64_t depth = 0) const = 0;
                        virtual uint64_t byteSize() const = 0;

			virtual void addPrefix(uint64_t prefix, uint64_t shift) = 0;

			virtual void square(
				HuffmanTreeNode * original,
				uint64_t shift)
				= 0;
			virtual void square(uint64_t shift)
			{
				if ( isLeaf() )
					throw ::std::runtime_error("Cannot square unary alphabet tree.");
				::libmaus2::util::shared_ptr < HuffmanTreeNode >::type  acloned ( clone() );
				square( acloned.get(), shift );
			}

			virtual HuffmanTreeNode * clone() const = 0;
			virtual ::libmaus2::util::shared_ptr < HuffmanTreeNode >::type aclone() const
			{
				return ::libmaus2::util::shared_ptr < HuffmanTreeNode >::type(clone());
			}

			virtual ::libmaus2::autoarray::AutoArray<uint64_t> structureArray() const
			{
				::std::vector < bool > B;
				structureVector(B);

				::libmaus2::autoarray::AutoArray<uint64_t> A( (B.size() + 63) / 64 );
				bitio::BitWriter8 W(A.get());
				for ( uint64_t i = 0; i < B.size(); ++i )
				{
					W.writeBit(B[i]);
				}
				W.flush();

				return A;
			}
			virtual ::libmaus2::autoarray::AutoArray<int64_t> symbolArray() const
			{
				::std::vector < int64_t > B;
				symbolVector(B);
				::libmaus2::autoarray::AutoArray<int64_t> A(B.size());
				::std::copy ( B.begin(), B.end(), A.get());
				return A;
			}

			virtual uint64_t numsyms() const
			{
				return symbolArray().size();
			}

			virtual ::libmaus2::autoarray::AutoArray<uint64_t> depthArray() const
			{
				::std::vector < uint64_t > B;
				depthVector(B);
				::libmaus2::autoarray::AutoArray<uint64_t> A(B.size());
				::std::copy ( B.begin(), B.end(), A.get());
				return A;
			}
			virtual ::libmaus2::autoarray::AutoArray<std::pair<int64_t,uint64_t> > symbolDepthArray() const
			{
				::std::vector < std::pair<int64_t,uint64_t> > B;
				symbolDepthVector(B);
				::libmaus2::autoarray::AutoArray< std::pair<int64_t,uint64_t> > A(B.size());
				::std::copy ( B.begin(), B.end(), A.get());
				return A;
			}
			uint64_t serialize(::std::ostream & out) const
			{
				::libmaus2::autoarray::AutoArray<uint64_t> struc = structureArray();
				::libmaus2::autoarray::AutoArray<int64_t> symb = symbolArray();
				uint64_t s = 0;
				s += struc.serialize(out);
				s += symb.serialize(out);
				out.flush();
				return s;
			}
			static ::libmaus2::util::shared_ptr < HuffmanTreeNode >::type deserialize(::std::istream & in)
			{
				uint64_t s = 0;
				return deserialize(in,s);
			}
			static ::libmaus2::util::shared_ptr < HuffmanTreeNode >::type deserialize(::std::istream & in,uint64_t & s)
			{
				::libmaus2::autoarray::AutoArray<uint64_t> struc;
				::libmaus2::autoarray::AutoArray<int64_t> symb;
				s += struc.deserialize(in);
				s += symb.deserialize(in);

				if ( !symb.getN() )
					return ::libmaus2::util::shared_ptr < HuffmanTreeNode >::type();
				else
				{
					uint64_t istruc = 0;
					uint64_t isymb = 0;
					return HuffmanTreeNode::shared_ptr_type (
						deserialize(struc.get(),istruc,symb.get(),isymb)
					);
				}
			}
			static HuffmanTreeNode * deserialize(
				uint64_t const * const struc,
				uint64_t & istruc,
				int64_t const * const symb,
				uint64_t & isymb
				);

			virtual void toDot(::std::ostream & out) const = 0;

			virtual void  fillIdMap(::std::map<  HuffmanTreeNode const *, uint64_t > & idmap, uint64_t & cur) const = 0;
			void fillIdMap(::std::map<  HuffmanTreeNode const *, uint64_t > & idmap) const
			{
				uint64_t cur = 0;
				fillIdMap(idmap,cur);
			}
			::std::map<  HuffmanTreeNode const *, uint64_t > getIdMap() const
			{
				::std::map<  HuffmanTreeNode const *, uint64_t > idmap;
				fillIdMap(idmap);
				return idmap;
			}
			void lineSerialise(::std::ostream & out) const
			{
				::std::map<  HuffmanTreeNode const *, uint64_t > idmap = getIdMap();
				out << "HuffmanTreeNode" << "\t" << idmap.size() << "\n";
				lineSerialise(out,idmap);
			}
			virtual void lineSerialise(::std::ostream & out, ::std::map<  HuffmanTreeNode const *, uint64_t > const & idmap) const = 0;

			static ::libmaus2::util::shared_ptr < HuffmanTreeNode >::type simpleDeserialise(::std::istream & in);
		};
	}
}
#endif
