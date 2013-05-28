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

#if ! defined(TRIPLEEDGECOMPAT_HPP)
#define TRIPLEEDGECOMPAT_HPP

#include <libmaus/aio/ReorderConcatGenericInput.hpp>
#include <libmaus/aio/GenericOutput.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/graph/TripleEdge.hpp>
#include <libmaus/util/Histogram.hpp>
#include <iostream>

namespace libmaus
{
	namespace graph
	{
		struct TripleEdgeCompact
		{
			static uint64_t getNumTotalEdges(std::string const numlistconcatrequestname)
			{
				uint64_t const bufsize = 64*1024;
				::libmaus::aio::ReorderConcatGenericInput<uint32_t>::unique_ptr_type 
					numedgefile = ::libmaus::aio::ReorderConcatGenericInput<uint32_t>::openConcatFile(numlistconcatrequestname, bufsize);
				
				uint32_t numedges;
				uint64_t numtotaledges = 0;
				while ( numedgefile->getNext(numedges) )
					numtotaledges += numedges;
				
				return numtotaledges;
			}
		
			template <
				typename numlistfiletype,
				typename edgetargetfiletype,
				typename edgeweightfiletype>
			static ::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > decompat(
				numlistfiletype & numedgefile,
				edgetargetfiletype & edgetargetfile,
				edgeweightfiletype & edgeweightfile,
				uint64_t const numtotaledges,
				uint64_t srcbase
				)
			{
				::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > edges(numtotaledges,false);
				::libmaus::graph::TripleEdge * edgesp = edges.begin();

				uint32_t numedges;
				while ( numedgefile->getNext(numedges) )
				{
					for ( uint64_t j = 0; j < numedges; ++j )
					{
						uint32_t edgetarget = 0; uint8_t edgeweight = 0;
						
						bool const tok = edgetargetfile->getNext(edgetarget);	
						bool const wok = edgeweightfile->getNext(edgeweight);
						assert ( tok );
						assert ( wok );
					
						*(edgesp++) = ::libmaus::graph::TripleEdge(srcbase,edgetarget,edgeweight);
					}
					
					srcbase++;
				}
				
				assert ( edgesp == edges.end() );
				
				return edges;
			}
			
			template<
				typename numedgefiletype,
				typename edgetargetfiletype,
				typename edgeweightfiletype
			>
			struct TripleEdgeDecompacterBase
			{
				typedef typename numedgefiletype::value_type numedgetype;
				typedef typename edgetargetfiletype::value_type edgetargettype;
				typedef typename edgeweightfiletype::value_type edgeweighttype;
				
				numedgefiletype & numedgefile;
				edgetargetfiletype & edgetargetfile;
				edgeweightfiletype & edgeweightfile;
				
				numedgetype numedges;
				int64_t src;
				
				TripleEdgeDecompacterBase(
					numedgefiletype & rnumedgefile,
					edgetargetfiletype & redgetargetfile,
					edgeweightfiletype & redgeweightfile,
					int64_t rsrclow
				)
				:
					numedgefile ( rnumedgefile ),
					edgetargetfile ( redgetargetfile ),
					edgeweightfile ( redgeweightfile ),
					numedges(0),
					src(rsrclow-1)
				{
				
				}
			
				bool getNext ( ::libmaus::graph::TripleEdge & e )
				{
					while ( ! numedges )
					{
						if ( ! numedgefile.getNext(numedges) )
							return false;
						src++;						
					}

					assert ( numedges );

					edgetargettype edgetarget = 0; edgeweighttype edgeweight = 0;
						
					bool const tok = edgetargetfile.getNext(edgetarget);	
					bool const wok = edgeweightfile.getNext(edgeweight);
					assert ( tok );
					assert ( wok );
					
					e.a = src;
					e.b = edgetarget;
					e.c = edgeweight;
					
					numedges--;
					
					// std::cerr << "ok." << std::endl;
					
					return true;
				}
			};
			
			struct TripleEdgeFileDecompacterFiles
			{
				typedef ::libmaus::aio::ReorderConcatGenericInput<uint32_t> numedgefiletype;
				typedef ::libmaus::aio::ReorderConcatGenericInput<uint32_t> edgetargetfiletype;
				typedef ::libmaus::aio::ReorderConcatGenericInput<uint8_t> edgeweightfiletype;
				
				numedgefiletype::unique_ptr_type numedgefile;
				edgetargetfiletype::unique_ptr_type edgetargetfile;
				edgeweightfiletype::unique_ptr_type edgeweightfile;

				TripleEdgeFileDecompacterFiles(
					std::string const numlistconcatrequestname,
					std::string const edgetargetsconcatequestname,
					std::string const edgeweightsconcatrequestname
				)
				:
					numedgefile ( numedgefiletype::openConcatFile(numlistconcatrequestname) ),
					edgetargetfile ( edgetargetfiletype::openConcatFile(edgetargetsconcatequestname) ),
					edgeweightfile ( edgeweightfiletype::openConcatFile(edgeweightsconcatrequestname) )
				{}
			};

			struct TripleEdgeFileDecompacter :
				public TripleEdgeFileDecompacterFiles, 
				public TripleEdgeDecompacterBase<
					::libmaus::aio::ReorderConcatGenericInput<uint32_t>,
					::libmaus::aio::ReorderConcatGenericInput<uint32_t>,
					::libmaus::aio::ReorderConcatGenericInput<uint8_t>
				>
			{
				TripleEdgeFileDecompacter(
					std::string const numlistconcatrequestname,
					std::string const edgetargetsconcatequestname,
					std::string const edgeweightsconcatrequestname,
					int64_t rsrclow
				)
				: TripleEdgeFileDecompacterFiles(numlistconcatrequestname,edgetargetsconcatequestname,edgeweightsconcatrequestname),
				  TripleEdgeDecompacterBase<
					::libmaus::aio::ReorderConcatGenericInput<uint32_t>,
					::libmaus::aio::ReorderConcatGenericInput<uint32_t>,
					::libmaus::aio::ReorderConcatGenericInput<uint8_t>
				  >( 
				  	*(TripleEdgeFileDecompacterFiles::numedgefile),
				  	*(TripleEdgeFileDecompacterFiles::edgetargetfile),
				  	*(TripleEdgeFileDecompacterFiles::edgeweightfile),
				  	rsrclow
				)
				{
				
				}
			};

			static ::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > decompat(
				std::string const numlistconcatrequestname,
				std::string const edgetargetsconcatequestname,
				std::string const edgeweightsconcatrequestname,
				uint64_t srcbase
				)
			{
				uint64_t const numtotaledges = getNumTotalEdges(numlistconcatrequestname);
				uint64_t const bufsize = 64*1024;
				::libmaus::aio::ReorderConcatGenericInput<uint32_t>::unique_ptr_type 
					numedgefile = ::libmaus::aio::ReorderConcatGenericInput<uint32_t>::openConcatFile(numlistconcatrequestname, bufsize);
				::libmaus::aio::ReorderConcatGenericInput<uint32_t>::unique_ptr_type 
					edgetargetfile = ::libmaus::aio::ReorderConcatGenericInput<uint32_t>::openConcatFile(edgetargetsconcatequestname, bufsize);
				::libmaus::aio::ReorderConcatGenericInput<uint8_t>::unique_ptr_type 
					edgeweightfile = ::libmaus::aio::ReorderConcatGenericInput<uint8_t>::openConcatFile(edgeweightsconcatrequestname, bufsize);
				return decompat(numedgefile,edgetargetfile,edgeweightfile,numtotaledges,srcbase);
			}

			static ::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > decompatDecompacter(
				std::string const numlistconcatrequestname,
				std::string const edgetargetsconcatequestname,
				std::string const edgeweightsconcatrequestname,
				uint64_t srcbase
				)
			{
				uint64_t const numtotaledges = getNumTotalEdges(numlistconcatrequestname);
				TripleEdgeFileDecompacter decompacter(numlistconcatrequestname,edgetargetsconcatequestname,edgeweightsconcatrequestname,srcbase);
				::libmaus::autoarray::AutoArray< ::libmaus::graph::TripleEdge > edges(numtotaledges,false);
				for ( uint64_t i = 0; i < numtotaledges; ++i )
				{
					bool const ok = decompacter.getNext(edges[i]);
					assert ( ok );
				}
				
				::libmaus::graph::TripleEdge edge;
				assert ( ! decompacter.getNext(edge) );
				
				return edges;
			}

			template<typename stream_type, typename _value_type>
			struct ByteStreamInputType
			{
				typedef _value_type value_type;
				
				stream_type & stream;
				
				ByteStreamInputType(stream_type & rstream)
				: stream(rstream)
				{
				
				}
				
				int get()
				{
					value_type v;
					if (  getNext(v) )
						return v;
					else
						return -1;
				}
				
				bool getNext(value_type & v)
				{
					v = 0;
					for ( uint64_t i = 0; i < sizeof(value_type); ++i )
					{
						v <<= 8;
						int const vi = stream.get();
						if ( vi < 0 )
							return false;
						v |= static_cast<value_type>(static_cast<uint8_t>(vi));
					}
					return true;
				}
			};
			
			template<typename stream_type>
			struct SingleStreamDecompacterStreams
			{
				ByteStreamInputType<stream_type,uint16_t> numedgein;
				ByteStreamInputType<stream_type,uint32_t> edgetargetin;
				ByteStreamInputType<stream_type,uint8_t> edgeweightin;
				
				SingleStreamDecompacterStreams(stream_type & stream)
				: numedgein(stream), edgetargetin(stream), edgeweightin(stream)
				{
				
				}
			};
			
			template<typename stream_type>
			struct SingleStreamDecompacter :
				public SingleStreamDecompacterStreams<stream_type>,
				public TripleEdgeDecompacterBase<		
					ByteStreamInputType<stream_type,uint16_t>,
					ByteStreamInputType<stream_type,uint32_t>,
					ByteStreamInputType<stream_type,uint8_t>
					>
			{
				typedef SingleStreamDecompacter<stream_type> this_type;
				typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
				typedef SingleStreamDecompacterStreams<stream_type> streambasetype;
				typedef TripleEdgeDecompacterBase<		
					ByteStreamInputType<stream_type,uint16_t>,
					ByteStreamInputType<stream_type,uint32_t>,
					ByteStreamInputType<stream_type,uint8_t>
					> decbasetype;
			
				SingleStreamDecompacter(stream_type & stream, uint64_t const srcbase)
				: 
					SingleStreamDecompacterStreams<stream_type>(stream),
					TripleEdgeDecompacterBase<		
						ByteStreamInputType<stream_type,uint16_t>,
						ByteStreamInputType<stream_type,uint32_t>,
						ByteStreamInputType<stream_type,uint8_t>
					>( streambasetype::numedgein, streambasetype::edgetargetin, streambasetype::edgeweightin, srcbase )
				{
				
				}
			};

			template<typename stream_type>
			static ::libmaus::autoarray::AutoArray < ::libmaus::graph::TripleEdge > decompatDecompacterSingle(
				stream_type & stream,
				uint64_t const numtotaledges,
				uint64_t const srcbase
				)
			{
				SingleStreamDecompacter<stream_type> SSD(stream,srcbase);

				::libmaus::autoarray::AutoArray< ::libmaus::graph::TripleEdge > edges(numtotaledges,false);
				for ( uint64_t i = 0; i < numtotaledges; ++i )
				{
					bool const ok = SSD.getNext(edges[i]);
					assert ( ok );
				}
								
				return edges;
			}

			template < 
				typename iterator_type,
				typename output_type_list,
				typename output_type_target,
				typename output_type_weight
			>
			static void compact(
				iterator_type A,
				uint64_t const n,
				output_type_list & numlistout,
				output_type_target & edgetargetout,
				output_type_weight & edgeweightout,
				uint64_t srclow,
				uint64_t const mod = 16ull*1024ull*1024ull
			)
			{
				if ( n )
				{
					uint64_t i = 0;
					uint64_t l = 0;
					
					while ( i < n )
					{
						uint32_t const src = A[i].a;
						
						uint64_t j = i;
						while ( j != n && A[j].a == src )
							++j;
						
						uint64_t const numsrcedges = j-i;
						
						while ( srclow != src )
						{
							numlistout.put(static_cast<uint16_t>(0)), srclow++;
						}

						numlistout.put(static_cast<uint16_t>(numsrcedges));

						for ( uint64_t k = i; k < j; ++k )
						{
							assert ( A[k].a == src );
							edgetargetout.put(static_cast<uint32_t>(A[k].b));
							edgeweightout.put(static_cast<uint8_t>(A[k].c));
						}
						
						// next
						i = j;
						srclow = src+1;
						
						// uint64_t const mod = 16*1024*1024;
						if ( i-l >= mod )
						{
							std::cerr << "(" << (n-i) << ")";
							l = i;
						}
					}
					std::cerr << "(" << 0 << ")";
					
					numlistout.flush();
					edgetargetout.flush();
					edgeweightout.flush();
				}
			}
			
			template<typename stream_type>
			struct ByteStreamOutputType
			{
				stream_type & stream;
				
				ByteStreamOutputType(stream_type & rstream) : stream(rstream) {}
				
				void put(uint8_t v)
				{
					stream.put(v);
				}
				void put(uint16_t v)
				{
					put( static_cast<uint8_t> (v >> 8) );
					put( static_cast<uint8_t> (v & 0xFF) );
				}
				void put(uint32_t v)
				{
					put( static_cast<uint16_t> ( v >> 16 ) );
					put( static_cast<uint16_t> ( v & 0xFFFF ) );
				}
				
				void flush()
				{
					stream.flush();
				}
			};
			
			
			template<typename iterator_type, typename stream_type>
			static void singleStreamCompact(
				iterator_type const & A,
				uint64_t const n,
				stream_type & stream,
				uint64_t srclow,
				uint64_t const mod = 16ull*1024ull*1024ull)
			{
				ByteStreamOutputType<stream_type> BSOT(stream);
				compact(A,n,BSOT,BSOT,BSOT,srclow,mod);
			}
			
			template<typename type>
			struct HistOutputType : public ::libmaus::util::Histogram
			{
				HistOutputType()
				: Histogram()
				{}
				
				void put(type const & n)
				{
					::libmaus::util::Histogram::operator()(n);
				}
				void flush()
				{
				}
			};

			template<typename N>
			struct NullOutputType
			{
				NullOutputType() {}
				
				void put(N const &)
				{

				}
				void flush()
				{
				
				}
			};

			template<typename N, typename writer_type, typename huffman_type>
			struct HuffmanOutputType
			{
				writer_type & writer;
				huffman_type & canon;
				
				HuffmanOutputType(writer_type & rwriter, huffman_type & rcanon)
				: writer(rwriter), canon(rcanon)
				{
				
				}
				
				void put(N const & v)
				{
					canon.encode(writer,v);
				}
				
				void flush()
				{
					writer.flush();
				}
			};

			template<typename N, typename writer_type>
			struct BitWriterType
			{
				writer_type & writer;
				unsigned int const numbits;
				
				BitWriterType(writer_type & rwriter, unsigned int const rnumbits = 32)
				: writer(rwriter), numbits(rnumbits)
				{
				
				}
				
				void put(N const & v)
				{
					writer.write(v,numbits);
				}
				
				void flush()
				{
					writer.flush();
				}
			};
			template<typename type>
			struct SynchronousHistOutputType : public ::libmaus::util::Histogram, public ::libmaus::aio::SynchronousGenericOutput<type>
			{
				SynchronousHistOutputType(std::string const & filename, uint64_t const bufsize = 64*1024)
				: Histogram(), ::libmaus::aio::SynchronousGenericOutput<type>(filename,bufsize)
				{}
				
				void put(type const & n)
				{
					::libmaus::aio::SynchronousGenericOutput<type>::put(n);
					::libmaus::util::Histogram::operator()(n);
				}
				void flush()
				{
					::libmaus::aio::SynchronousGenericOutput<type>::flush();
				}
			};
		};
	}
}
#endif
