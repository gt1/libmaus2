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

#if ! defined(EDGELIST_HPP)
#define EDGELIST_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/graph/TripleEdge.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/network/Socket.hpp>
#include <libmaus/math/MetaLog.hpp>
#include <limits>

namespace libmaus
{
	namespace graph
	{
		struct EdgeListBase
		{
			typedef uint32_t edge_count_type;
			typedef uint64_t edge_target_type;
			typedef uint16_t edge_weight_type;
			
			static unsigned int const maxweight;
			static edge_target_type const edge_list_term;
		};
			
		template<bool bininsert>
		struct EdgeListTemplate : public EdgeListBase
		{
			typedef EdgeListTemplate<bininsert> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t edgelow;
			uint64_t edgehigh;
			uint64_t maxedges;
			::libmaus::autoarray::AutoArray<edge_target_type> edges;
			::libmaus::autoarray::AutoArray<edge_weight_type> weights;
			
			static uint64_t memPerEdge()
			{
				return sizeof(edge_target_type) + sizeof(edge_weight_type);
			}
			
			EdgeListTemplate(uint64_t const redgelow, uint64_t const redgehigh, uint64_t const rmaxedges)
			: edgelow(redgelow), edgehigh(redgehigh), maxedges(rmaxedges), 
			  edges( (edgehigh-edgelow)*maxedges, false ),
			  weights( (edgehigh-edgelow)*maxedges, false )
			{
				#if ! defined(_OPENMP)
				::std::fill ( edges.get(), edges.get()+edges.getN(), edge_list_term );
				::std::fill ( weights.get(), weights.get()+weights.getN(), 0 );
				#else
				
				#pragma omp parallel for
				for ( int64_t i = 0; i < static_cast<int64_t>(edges.getN()); ++i )
					edges[i] = edge_list_term;

				#pragma omp parallel for
				for ( int64_t i = 0; i < static_cast<int64_t>(weights.getN()); ++i )
					weights[i] = 0;
				
				#endif
			}
			
			unsigned int getNumEdges(uint64_t const src) const
			{
				edge_target_type const * const pa = edges.get() + (src-edgelow)*maxedges;
				edge_target_type const * const pe = pa + maxedges;

				if ( ! bininsert )
				{
					edge_target_type const * pc = pa;
					
					while ( (pc != pe) && (*pc != edge_list_term) )
						++pc;
					
					return pc-pa;
				}
				else
				{
					return ::std::lower_bound(pa,pe,edge_list_term) - pa;
				}
			}
			
			std::map<unsigned int,uint64_t> getNumEdgeDist() const
			{
				::libmaus::autoarray::AutoArray<uint64_t> D256(256);
				std::map < unsigned int, uint64_t > D;
				
				for ( uint64_t i = edgelow; i < edgehigh; ++i )
				{
					uint64_t const numedges = getNumEdges(i);
					
					if ( numedges < 256 )
						D256[numedges]++;
					else
						D[numedges]++;
				}
				
				for ( uint64_t i = 0; i < D256.size(); ++i )
					if ( D256[i] )
						D[i] += D256[i];
				
				return D;
			}
			
			template<typename a, typename b>
			static std::string serialiseMap(std::map<a,b> const & M)
			{
				std::ostringstream out;
				
				::libmaus::serialize::Serialize<uint64_t>::serialize(out,M.size());
				
				for ( typename std::map<a,b>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					::libmaus::serialize::Serialize<a>::serialize(out,ita->first);
					::libmaus::serialize::Serialize<b>::serialize(out,ita->second);
				}
				
				return out.str();
			}
			
			std::string getSerialisedNumEdgeDist() const
			{
				std::map<unsigned int, uint64_t> M = getNumEdgeDist();
				return serialiseMap(M);
			}
			
			template<typename a, typename b>
			static std::map<a,b> deserialiseMap(std::string const & s)
			{
				std::istringstream in(s);
				return deserialiseMap<a,b>(in);
			}

			template<typename a, typename b>
			static std::map<a,b> deserialiseMap(std::istream & in)
			{
				uint64_t m;
				::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&m);
				std::map<a,b> M;
				
				for ( uint64_t i = 0; i < m; ++i )
				{
					a A;
					b B;

					::libmaus::serialize::Serialize<a>::deserialize(in,&A);
					::libmaus::serialize::Serialize<b>::deserialize(in,&B);
					
					M[A] = B;
				}
				
				return M;
			}
			
			static std::map<unsigned int, uint64_t> getDeserialisedNumEdgeDist(std::istream & in)
			{
				return deserialiseMap<unsigned int, uint64_t>(in);
			}
			static std::map<unsigned int, uint64_t> getDeserialisedNumEdgeDist(std::string const & s)
			{
				return deserialiseMap<unsigned int, uint64_t>(s);
			}
			
			static std::map<unsigned int,uint64_t> mergeNumEdgeDists(
				std::map<unsigned int,uint64_t> const & M0,
				std::map<unsigned int,uint64_t> const & M1
			)
			{
				std::map<unsigned int,uint64_t> M2;
				
				for ( std::map<unsigned int,uint64_t>::const_iterator ita = M0.begin(); ita != M0.end(); ++ita )
					M2[ita->first] += ita->second;
				for ( std::map<unsigned int,uint64_t>::const_iterator ita = M1.begin(); ita != M1.end(); ++ita )
					M2[ita->first] += ita->second;
					
				return M2;
			}

			template<typename output_type>
			uint64_t writeNumEdgeStream(output_type & out) const
			{
				uint64_t numedges = 0;
				for ( uint64_t i = edgelow; i < edgehigh; ++i )
				{
					uint64_t const lnumedges = getNumEdges(i);
					out.put( lnumedges );
					numedges += lnumedges;
				}
						
				out.flush();		
				
				return numedges;	
			}

			uint64_t writeNumEdgeFile(std::string const & filename) const
			{
				::libmaus::aio::SynchronousGenericOutput<edge_count_type> out(filename,8*1024);
				return writeNumEdgeStream(out);				
			}
			
			uint64_t writeNumEdgeSocket(
				::libmaus::network::SocketBase * const socket
			)
			{
				::libmaus::network::SocketOutputBuffer<edge_count_type> SOB(socket,64*1024);
				uint64_t const numedges = writeNumEdgeStream(SOB);
				socket->writeMessage<uint64_t>(1,0,0);
				return numedges;
			}
			
			template<typename type>
			static void receiveSocket(				
				::libmaus::network::SocketBase * const socket,
				std::string const & filename,
				bool const append
				)
			{
				::libmaus::aio::SynchronousGenericOutput<type> out(filename,8*1024);
				::libmaus::network::SocketInputBuffer<type> SIB(socket,::std::numeric_limits<uint64_t>::max(),1);
				type v;
				while ( SIB.get(v) )
					out.put(v);
				out.flush();
			}

			template<typename output_type>
			void writeEdgeTargetsStream(output_type & out) const
			{
				for ( uint64_t src = edgelow; src != edgehigh; ++src )
				{
					edge_target_type const * const pa = edges.get() + (src-edgelow)*maxedges;
					edge_target_type const * pc = pa;
					edge_target_type const * const pe = pc + maxedges;
				
					while ( (pc != pe) && (*pc != edge_list_term) )
						out.put( *(pc++) );
				}
				
				out.flush();			
			}
			
			void writeEdgeTargets(std::string const & filename) const
			{
				::libmaus::aio::SynchronousGenericOutput<edge_target_type> out(filename,8*1024);
				writeEdgeTargetsStream(out);	
			}

			void writeEdgeTargetsSocket(
				::libmaus::network::SocketBase * const socket
			)
			{
				::libmaus::network::SocketOutputBuffer<edge_target_type> SOB(socket,64*1024);
				writeEdgeTargetsStream(SOB);
				socket->writeMessage<uint64_t>(1,0,0);
			}

			template<typename output_type>
			void writeEdgeWeightStream(output_type & out) const
			{
				for ( uint64_t src = edgelow; src != edgehigh; ++src )
				{
					edge_target_type const * const pa = edges.get() + (src-edgelow)*maxedges;
					edge_target_type const * pc = pa;
					edge_target_type const * const pe = pc + maxedges;
					edge_weight_type const * wc = weights.get() + (src-edgelow)*maxedges;
				
					while ( (pc != pe) && (*pc != edge_list_term) )
					{
						out.put( *(wc++) );
						pc++;
					}
				}
				
				out.flush();			
			}			

			void writeEdgeWeights(std::string const & filename) const
			{
				::libmaus::aio::SynchronousGenericOutput<unsigned char> out(filename,8*1024);
				writeEdgeWeightStream(out);
			}

			void writeEdgeWeightsSocket(
				::libmaus::network::SocketBase * const socket
			)
			{
				::libmaus::network::SocketOutputBuffer<edge_weight_type> SOB(socket,64*1024);
				writeEdgeWeightStream(SOB);
				socket->writeMessage<uint64_t>(1,0,0);
			}
			
			void operator()(uint64_t const src, uint64_t const dst, unsigned int weight)
			{
				assert ( src >= edgelow );
				assert ( src < edgehigh );
			
				edge_target_type * const pa = edges.get() + (src-edgelow)*maxedges;
				edge_target_type * const pe = pa + maxedges;
				
				if ( ! bininsert )
				{
					edge_target_type * pc = pa;
				
					while ( (pc != pe) && (*pc != static_cast<edge_target_type>(dst)) && (*pc != edge_list_term) )
						++pc;
				
					if ( pc != pe )
					{
						*pc = dst;
						weights [ pc-edges.get() ] = std::min(maxweight,weights[pc-edges.get()] + weight);
					}
				}
				else
				{
					// where do we need to insert the element?
					edge_target_type * pc = ::std::lower_bound(pa,pe,dst);
					
					// post end?
					if ( pc != pe )
					{
						// insert index
						uint64_t const off = pc-pa;
						
						// element already present?
						if ( *pc != dst )
						{
							edge_target_type * cb = ::std::lower_bound(pa,pe,edge_list_term);
						
							// still space
							if ( cb != pe )
							{
								uint64_t const eoff = cb-pa;
								edge_weight_type * wa = weights.get() + (src-edgelow)*maxedges;
							
								::std::copy_backward(pc,cb,cb+1);
								::std::copy_backward(
									wa+off,
									wa+eoff,
									wa+eoff+1);

								wa[off] = std::min(weight,maxweight);
								*pc = dst;

							}
							// no more space
							// we would need to remove an element
							#if 0
							else
							{
								edge_weight_type * wa = weights.get() + (src-edgelow)*maxedges;
								
								// std::cerr << "Pushout copy" << std::endl;
								::std::copy_backward(pc,cb-1,cb);
								::std::copy_backward(
									wa+off,
									wa+(weights.size()-1),
									wa+weights.size()
								);

								wa[off] = std::min(weight,maxweight);
								*pc = dst;
							}
							#else
							else
							{
								// std::cerr << "source " << src << " is full." << std::endl;
							}
							#endif
						}
						else
						{
							edge_weight_type * wa = weights.get() + (src-edgelow)*maxedges;
							wa[off] = std::min(wa[off]+weight,maxweight);
						}
					}
				}
				
				/*
				print(std::cerr);
				std::cerr << "---" << std::endl;
				*/
			}

			void operator()(::libmaus::graph::TripleEdge const & T)
			{
				(*this)(T.a,T.b,T.c);
			}
			
			void operator()(::libmaus::graph::TripleEdge const * T, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					(*this)(T[i]);
			}
			
			void print(std::ostream & out) const
			{
				for ( uint64_t i = 0; i < edgehigh-edgelow; ++i )
				{
					edge_target_type const * p = edges.get() + i * maxedges;
					edge_weight_type const * w = weights.get() + i*maxedges;
					
					for ( uint64_t j = 0; j < maxedges && p[j] != edge_list_term; ++j )
						out << (i+edgelow) << "," << p[j] << "," << static_cast<int>(w[j]) << std::endl;
				}
			}
		};
		
		typedef EdgeListTemplate<true> EdgeList;
	}
}
#endif
