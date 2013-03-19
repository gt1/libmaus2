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
#if ! defined(LIBMAUS_FASTX_COMPACTFASTQCONTAINER_HPP)
#define LIBMAUS_FASTX_COMPACTFASTQCONTAINER_HPP

#include <libmaus/fastx/CompactFastQContainerDictionary.hpp>
#include <libmaus/fastx/CompactFastQContext.hpp>
#include <libmaus/fastx/CompactFastQHeader.hpp>
#include <libmaus/fastx/CompactFastQDecoderBase.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct CompactFastQContainer : public ::libmaus::fastx::CompactFastQDecoderBase
		{
                        typedef CompactFastQContainer this_type;
                        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			::libmaus::autoarray::AutoArray<uint8_t> T;
			::libmaus::fastx::CompactFastQContainerDictionary::unique_ptr_type dict;
			::libmaus::fastx::CompactFastQHeader::unique_ptr_type H;
			::libmaus::fastx::CompactFastQContext C;
			
			uint64_t byteSize() const
			{
				return T.byteSize() + dict->byteSize() + H->byteSize() + C.byteSize();
			}
			
			struct GetObject
			{
				uint8_t const * T;
				
				GetObject(uint8_t const * rT) : T(rT) {}
				
				int get() { return *(T++); }
			};
			
			static unique_ptr_type construct(std::istream & textstr)
			{
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(textstr)));
			}
			
			CompactFastQContainer(std::istream & textstr)
			: T(textstr), dict(new ::libmaus::fastx::CompactFastQContainerDictionary(textstr)), H(), C()
			{
				GetObject G(T.begin());
				H = UNIQUE_PTR_MOVE(::libmaus::fastx::CompactFastQHeader::unique_ptr_type(new ::libmaus::fastx::CompactFastQHeader(G)));
			}

			CompactFastQContainer(::libmaus::network::SocketBase * textstr)
			: T(textstr->readMessageInBlocks<uint8_t,::libmaus::autoarray::alloc_type_cxx>()), 
			  dict(new ::libmaus::fastx::CompactFastQContainerDictionary(textstr)), H(), C()
			{
				GetObject G(T.begin());
				H = UNIQUE_PTR_MOVE(::libmaus::fastx::CompactFastQHeader::unique_ptr_type(new ::libmaus::fastx::CompactFastQHeader(G)));
			}

			void serialise(::libmaus::network::SocketBase * socket) const
			{
				socket->writeMessageInBlocks(T);
				dict->serialise(socket);
			}

			static unique_ptr_type load(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				unique_ptr_type ptr(new this_type(istr));
				
				if ( ! istr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "CompactFastQContainer::load(): failed to load file " << filename << std::endl;
					se.finish();
					throw se;
				}
				
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			void serialise(std::ostream & out) const
			{
				T.serialize(out);
				dict->serialise(out);
			}

			void getPattern(pattern_type & pat, uint64_t i)
			{
				GetObject G(T.begin()+(*dict)[i - dict->FI.low]);
				C.nextid = i;
				::libmaus::fastx::CompactFastQDecoderBase::decodePattern<GetObject>(G,*H,C,pat);
				pat.patid = i;
			}

			void getPattern(pattern_type & pat, uint64_t i) const
			{
				GetObject G(T.begin()+(*dict)[i - dict->FI.low]);
				::libmaus::fastx::CompactFastQContext C;
				C.nextid = i;
				::libmaus::fastx::CompactFastQDecoderBase::decodePattern<GetObject>(G,*H,C,pat);
				pat.patid = i;
			}

			void getElement(element_type & pat, uint64_t i)
			{
				GetObject G(T.begin()+(*dict)[i - dict->FI.low]);
				C.nextid = i;
				::libmaus::fastx::CompactFastQDecoderBase::decodeElement<GetObject>(G,*H,C,pat);
			}

			void getElement(element_type & pat, uint64_t i) const
			{
				GetObject G(T.begin()+(*dict)[i - dict->FI.low]);
				::libmaus::fastx::CompactFastQContext C;
				C.nextid = i;
				::libmaus::fastx::CompactFastQDecoderBase::decodeElement<GetObject>(G,*H,C,pat);
			}
			
			element_type operator[](uint64_t const i) const
			{
				element_type element;
				getElement(element,i);
				return element;
			}
			
			uint64_t size() const
			{
				return H->numreads;
			}
		};
	}
}
#endif
