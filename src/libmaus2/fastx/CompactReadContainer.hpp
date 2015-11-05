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

#if ! defined(COMPACTREADCONTAINER_HPP)
#define COMPACTREADCONTAINER_HPP

#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/fastx/Pattern.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/fastx/CompactFastDecoder.hpp>
#include <libmaus2/bitio/BitWriter.hpp>
#include <libmaus2/util/GetObject.hpp>
#if ! defined(_WIN32)
#include <libmaus2/network/Interface.hpp>
#include <libmaus2/network/UDPSocket.hpp>
#endif

namespace libmaus2
{
	namespace fastx
	{
		struct CompactReadContainer
		{
			typedef CompactReadContainer this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::rank::ERank222B rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			typedef rank_type::writer_type writer_type;
			typedef writer_type::data_type data_type;
			typedef ::libmaus2::fastx::Pattern pattern_type;

			::libmaus2::fastx::FastInterval FI;
			uint64_t numreads;
			::libmaus2::autoarray::AutoArray < data_type > designators;
			rank_ptr_type designatorrank;
			::libmaus2::autoarray::AutoArray< uint16_t > shortpointers;
			::libmaus2::autoarray::AutoArray< uint64_t > longpointers;
			::libmaus2::autoarray::AutoArray< uint8_t > text;
			::libmaus2::fastx::CompactFastDecoderBase CFDB;

			void setupRankDictionary()
			{
				rank_ptr_type tdesignatorrank(new rank_type(designators.get(), designators.size()*64));
				designatorrank = UNIQUE_PTR_MOVE(tdesignatorrank);
			}

			void serialise(std::ostream & out) const
			{
				::libmaus2::fastx::FastInterval::serialise(out,FI);
				designators.serialize(out);
				shortpointers.serialize(out);
				longpointers.serialize(out);
				text.serialize(out);
			}

			#if ! defined(_WIN32)
			void serialise(::libmaus2::network::SocketBase * socket) const
			{
				socket->writeString(FI.serialise());
				socket->writeMessageInBlocks(designators);
				socket->writeMessageInBlocks(shortpointers);
				socket->writeMessageInBlocks(longpointers);
				socket->writeMessageInBlocks(text);
			}

			void broadcastSend(
				::libmaus2::network::Interface const & interface,
				unsigned short const broadcastport,
				::libmaus2::autoarray::AutoArray < ::libmaus2::network::ClientSocket::unique_ptr_type > & secondarysockets,
				unsigned int const packsize = 508
			) const
			{
				std::cerr << "Writing FI...";
				for ( uint64_t i = 0; i < secondarysockets.size(); ++i )
					secondarysockets[i]->writeString(FI.serialise());
				std::cerr << "done.";

				std::cerr << "Broadcasting designators...";
				::libmaus2::network::UDPSocket::sendArrayBroadcast(interface,broadcastport,
					secondarysockets,designators.get(),designators.size(),packsize);
				std::cerr << "done.";

				std::cerr << "Broadcasting shortpointers...";
				::libmaus2::network::UDPSocket::sendArrayBroadcast(interface,broadcastport,
					secondarysockets,shortpointers.get(),shortpointers.size(),packsize);
				std::cerr << "done.";

				std::cerr << "Broadcasting longpointers...";
				::libmaus2::network::UDPSocket::sendArrayBroadcast(interface,broadcastport,
					secondarysockets,longpointers.get(),longpointers.size(),packsize);
				std::cerr << "done.";

				std::cerr << "Broadcasting text...";
				::libmaus2::network::UDPSocket::sendArrayBroadcast(interface,broadcastport,
					secondarysockets,text.get(),text.size(),packsize);
				std::cerr << "done.";
			}
			#endif

			std::string operator[](uint64_t const i) const
			{
				pattern_type pat;
				getPattern(pat,i);
				return pat.spattern;
			}

			void getPattern(pattern_type & pat, uint64_t i) const
			{
				assert ( i >= FI.low && i < FI.high );

				uint64_t const j = i-FI.low;
				uint64_t const offsetbase = longpointers [ designatorrank->rank1(j) ];
				uint64_t const codepos = offsetbase + shortpointers[j];
				uint8_t const * code = text.begin()+codepos;
				::libmaus2::util::GetObject<uint8_t const *> G(code);
				::libmaus2::parallel::SynchronousCounter<uint64_t> nextid(i);
				CompactFastDecoderBase::decode(pat,G,nextid);
			}

			static ::libmaus2::fastx::FastInterval deserialiseInterval(std::string const & s)
			{
				std::istringstream istr(s);
				return ::libmaus2::fastx::FastInterval::deserialise(istr);
			}

			#if ! defined(_WIN32)
			CompactReadContainer(
				::libmaus2::network::UDPSocket::unique_ptr_type & broadcastsocket,
				::libmaus2::network::SocketBase::unique_ptr_type & parentsocket
			)
			: FI( deserialiseInterval(parentsocket->readString()) ),
			  numreads(FI.high-FI.low),
			  designators ( ::libmaus2::network::UDPSocket::receiveArrayBroadcast<data_type>(broadcastsocket,parentsocket) ),
			  shortpointers ( ::libmaus2::network::UDPSocket::receiveArrayBroadcast<uint16_t>(broadcastsocket,parentsocket) ),
			  longpointers ( ::libmaus2::network::UDPSocket::receiveArrayBroadcast<uint64_t>(broadcastsocket,parentsocket) ),
			  text ( ::libmaus2::network::UDPSocket::receiveArrayBroadcast<uint8_t>(broadcastsocket,parentsocket) ),
			  CFDB()
			{
				setupRankDictionary();
			}


			CompactReadContainer(::libmaus2::network::SocketBase * socket)
			{
				std::cerr << "Receiving FI...";
				FI = ::libmaus2::fastx::FastInterval(deserialiseInterval(socket->readString()));
				std::cerr << "done, FI=" << FI << std::endl;

				numreads = FI.high-FI.low;
				std::cerr << "numreads=" << numreads << std::endl;

				std::cerr << "Receiving designators...";
				#if 0
				designators = socket->readMessage<data_type>();
				#else
				designators = socket->readMessageInBlocks<data_type,::libmaus2::autoarray::alloc_type_cxx>();
				#endif
				std::cerr << "done." << std::endl;

				std::cerr << "Receiving short pointers...";
				#if 0
				shortpointers =socket->readMessage<uint16_t>();
				#else
				shortpointers =socket->readMessageInBlocks<uint16_t,::libmaus2::autoarray::alloc_type_cxx>();
				#endif
				std::cerr << "done." << std::endl;

				std::cerr << "Receiving long pointers...";
				#if 0
				longpointers = socket->readMessage<uint64_t>();
				#else
				longpointers = socket->readMessageInBlocks<uint64_t,::libmaus2::autoarray::alloc_type_cxx>();
				#endif
				std::cerr << "done." << std::endl;

				std::cerr << "Receiving text...";
				#if 0
				text = socket->readMessage<uint8_t>();
				#else
				text = socket->readMessageInBlocks<uint8_t,::libmaus2::autoarray::alloc_type_cxx>();
				#endif
				std::cerr << "done." << std::endl;

				std::cerr << "Setting up rank dict...";
				setupRankDictionary();
				std::cerr << "done." << std::endl;
			}
			#endif

			CompactReadContainer(std::istream & in)
			:
			  FI(::libmaus2::fastx::FastInterval::deserialise(in)),
			  numreads(FI.high-FI.low),
			  designators(in),
			  shortpointers(in),
			  longpointers(in),
			  text(in),
			  CFDB()
			{
				setupRankDictionary();
			}

			CompactReadContainer(
				std::vector<std::string> const & filenames,
				::libmaus2::fastx::FastInterval const & rFI,
				bool const verbose = false
			)
			: FI(rFI), numreads(FI.high-FI.low), designators( (numreads+63)/64 ), shortpointers(numreads,false), longpointers(), text(FI.fileoffsethigh-FI.fileoffset,false)
			{
				typedef ::libmaus2::fastx::CompactFastConcatDecoder reader_type;
				// typedef reader_type::pattern_type pattern_type;
				reader_type CFD(filenames,FI);

				uint64_t codepos = 0;
				uint64_t offsetbase = 0;

				// bool const verbose = true;

				uint64_t const mod = std::max((numreads+50)/100,static_cast<uint64_t>(1));
				uint64_t const bmod = libmaus2::math::nextTwoPow(mod);
				uint64_t const bmask = bmod-1;

				if ( verbose )
				{
					if ( isatty(STDERR_FILENO) )
						std::cerr << "Computing designators/pointers...";
					else
						std::cerr << "Computing designators/pointers..." << std::endl;
				}

				std::vector < uint64_t > prelongpointers;
				prelongpointers.push_back(0);
				writer_type W(designators.get());
				for ( uint64_t i = 0; i < numreads; ++i )
				{
					if (
						(
							codepos-offsetbase
							>
							static_cast<uint64_t>(std::numeric_limits<uint16_t>::max())
						)
					)
					{
						W.writeBit(1);
						offsetbase = codepos;
						prelongpointers.push_back(offsetbase);
					}
					else
					{
						W.writeBit(0);
					}
					shortpointers[i] = codepos-offsetbase;

					CFD.skipPattern(codepos);

					if ( verbose && ((i & (bmask)) == 0) )
					{
						if ( isatty(STDERR_FILENO) )
							std::cerr << "(" << i/static_cast<double>(numreads)  << ")";
						else
							std::cerr << "Finished " << i/static_cast<double>(numreads)  << std::endl;
					}
				}
				W.flush();

				longpointers = ::libmaus2::autoarray::AutoArray< uint64_t >(prelongpointers.size(),false);
				std::copy(prelongpointers.begin(),prelongpointers.end(),longpointers.begin());

				if ( verbose )
					std::cerr << "Done." << std::endl;

				if ( verbose )
					std::cerr << "Loading text...";
				std::vector < libmaus2::aio::FileFragment > const frags =
					::libmaus2::fastx::CompactFastDecoder::getDataFragments(filenames);
				::libmaus2::aio::ReorderConcatGenericInput<uint8_t> RCGI(frags,64*1024,text.size(),FI.fileoffset);
				uint64_t const textread = RCGI.read(text.begin(),text.size());

				if ( textread != text.size() )
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "Failed to read text in CompactReadContainer." << std::endl;
					se.finish();
					throw se;
				}
				if ( verbose )
					std::cerr << "done." << std::endl;

				if ( verbose )
					std::cerr << "Setting up rank dictionary for designators...";
				setupRankDictionary();
				if ( verbose )
					std::cerr << "done." << std::endl;

				#if 0
				std::cerr << "Checking dict...";
				reader_type CFD2(filenames,FI);
				for ( uint64_t i = 0; i < numreads; ++i )
				{
					if ( CFD2.istr.getptr != longpointers [ designatorrank->rank1(i) ] + shortpointers[i] )
					{
						std::cerr << "Failure for i=" << i << std::endl;
						std::cerr << "Ptr is " << CFD2.istr.getptr << std::endl;
						std::cerr << "Expected " << longpointers [ designatorrank->rank1(i) ] + shortpointers[i] << std::endl;
						assert ( CFD2.istr.getptr == longpointers [ designatorrank->rank1(i) ] + shortpointers[i] );
					}
					::libmaus2::fastx::Pattern pattern;
					CFD2.getNextPatternUnlocked(pattern);
				}
				std::cerr << "done." << std::endl;
				#endif
			}
		};
	}
}
#endif
