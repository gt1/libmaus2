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
#if !defined(LIBMAUS_RMQ_RMMTREE_HPP)
#define LIBMAUS_RMQ_RMMTREE_HPP

#include <libmaus/util/ImpCompactNumberArray.hpp>
#include <libmaus/util/Array864.hpp>

namespace libmaus
{
	namespace rmq
	{
		template<typename _base_layer_type, unsigned int _klog, bool _rmmtreedebug = false>
		struct RMMTree
		{
			typedef _base_layer_type base_layer_type;
			static unsigned int const klog = _klog;
			static bool const rmmtreedebug = _rmmtreedebug;
			typedef RMMTree<base_layer_type,klog,rmmtreedebug>  this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			typedef libmaus::util::ImpCompactNumberArray C_type;
			// typedef libmaus::util::Array864 C_type;
			typedef C_type::unique_ptr_type C_ptr_type;

			static unsigned int const k = (1u << klog);
			static uint64_t const kmask = (k-1);
			
			base_layer_type const & B;
			uint64_t const n;
			unsigned int const numlevels;
			libmaus::autoarray::AutoArray< libmaus::bitio::CompactArray::unique_ptr_type > I;
			libmaus::autoarray::AutoArray< C_ptr_type > C;
			libmaus::autoarray::AutoArray< uint64_t > S;
			
			uint64_t byteSize() const
			{
				uint64_t s =
					sizeof(uint64_t) +
					sizeof(unsigned int) +
					I.byteSize() +
					C.byteSize() +
					S.byteSize();
					
				for ( uint64_t i = 0; i < I.size(); ++i )
					if ( I[i] )
						s += I[i]->byteSize();
				for ( uint64_t i = 0; i < C.size(); ++i )
					if ( C[i] )
						s += C[i]->byteSize();
						
				return s;
			}
			
			template<typename stream_type>
			void serialise(stream_type & out)
			{
				libmaus::util::NumberSerialisation::serialiseNumber(out,n);
				libmaus::util::NumberSerialisation::serialiseNumber(out,numlevels);

				libmaus::util::NumberSerialisation::serialiseNumber(out,I.size());
				libmaus::util::NumberSerialisation::serialiseNumber(out,C.size());

				S.serialize(out);
				
				for ( uint64_t i = 0; i < I.size(); ++i )
					I[i]->serialize(out);
				for ( uint64_t i = 0; i < C.size(); ++i )
					C[i]->serialise(out);
			}
			
			template<typename stream_type>
			RMMTree(stream_type & in, base_layer_type const & rB)
			: 
				B(rB),
				n(libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numlevels(libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				I(libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				C(libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				S(in)
			{
				for ( uint64_t i = 0; i < I.size(); ++i )
				{
					libmaus::bitio::CompactArray::unique_ptr_type tIi(new libmaus::bitio::CompactArray(in));
					I[i] = UNIQUE_PTR_MOVE(tIi);
				}
				for ( uint64_t i = 0; i < C.size(); ++i )
				{
					C_ptr_type Ci(C_type::load(in));					
					C[i] = UNIQUE_PTR_MOVE(Ci);
				}
			}
			
			static unique_ptr_type load(base_layer_type const & B, std::string const & fn)
			{
				libmaus::aio::CheckedInputStream CIS(fn);
				unique_ptr_type ptr(
                                                new this_type(CIS,B)
                                        );
				return UNIQUE_PTR_MOVE(ptr);
			}

			uint64_t operator()(unsigned int const level, uint64_t const i) const
			{
				if ( level == 0 )
					return B[i];
				else
					return C[level-1]->get(i);
			}
			
			static unsigned int computeNumLevels(uint64_t const n)
			{
				uint64_t in = n;
				unsigned int level = 0;
				
				while ( in > 1 )
				{
					in = ( in + k - 1 ) >> klog;
					level += 1;
				}

				return level;	
			}
			
			template<typename in_iterator>
			static libmaus::util::Histogram::unique_ptr_type fillSubHistogram(
				in_iterator in_it,
				uint64_t const in
			)
			{
				libmaus::util::Histogram::unique_ptr_type phist(new libmaus::util::Histogram);
				libmaus::util::Histogram & hist = *phist;

				uint64_t const full = in >> klog;
				uint64_t const rest = in-full*k;
			
				for ( uint64_t i = 0; i < full; ++i )
				{
					uint64_t vmin = *(in_it++);
					for ( unsigned int j = 1; j < k; ++j )
					{
						uint64_t const vj = *(in_it++);
						if ( vj < vmin )
							vmin = vj;
					}
					
					hist(libmaus::math::bitsPerNum(vmin));
				}
				if ( rest )
				{
					uint64_t vmin = *(in_it++);
					for ( unsigned int j = 1; j < rest; ++j )
					{
						uint64_t const vj = *(in_it++);
						if ( vj < vmin )
							vmin = vj;
					}
					
					hist(libmaus::math::bitsPerNum(vmin));
				}
				
				return UNIQUE_PTR_MOVE(phist);
			}

			template<typename in_iterator>
			static libmaus::huffman::HuffmanTreeNode::shared_ptr_type fillSubNode(
				in_iterator in_it, uint64_t in
			)
			{
				libmaus::util::Histogram::unique_ptr_type phist = UNIQUE_PTR_MOVE(fillSubHistogram(in_it,in));
				std::map<int64_t,uint64_t> const probs = phist->getByType<int64_t>();
				libmaus::huffman::HuffmanTreeNode::shared_ptr_type snode = libmaus::huffman::HuffmanBase::createTree(probs);
				return snode;
			}

			template<typename in_iterator, typename C_generator_type>
			static void fillSubArrays(
				in_iterator in_it,
				uint64_t const in,
				libmaus::bitio::CompactArray & I,
				C_generator_type & impgen
			)
			{
				uint64_t const full = in >> klog;
				uint64_t const rest = in-full*k;
			
				for ( uint64_t i = 0; i < full; ++i )
				{
					uint64_t vmin = *(in_it++);
					unsigned int jmin = 0;
					for ( unsigned int j = 1; j < k; ++j )
					{
						uint64_t const vj = *(in_it++);
						if ( vj < vmin )
						{
							vmin = vj;
							jmin = j;
						}
					}
					
					I.set(i,jmin);
					impgen.add(vmin);
				}
				if ( rest )
				{
					uint64_t vmin = *(in_it++);
					unsigned int jmin = 0;
					for ( unsigned int j = 1; j < rest; ++j )
					{
						uint64_t const vj = *(in_it++);
						if ( vj < vmin )
						{
							vmin = vj;
							jmin = j;
						}
					}
					
					I.set(full,jmin);
					impgen.add(vmin);
				}		
			}

			RMMTree(base_layer_type const & rB, uint64_t const rn)
			: B(rB), n(rn), numlevels(computeNumLevels(n)), I(numlevels), C(numlevels), S(numlevels+1)
			{
				uint64_t in = n;
				unsigned int level = 0;
				
				while ( in > 1 )
				{
					uint64_t const out = (in+k-1) >> klog;
					
					// minimal indices for next level
					libmaus::bitio::CompactArray::unique_ptr_type tIlevel(
                                                new libmaus::bitio::CompactArray(out,klog));
					I[level] = UNIQUE_PTR_MOVE(tIlevel);

					libmaus::util::Histogram::unique_ptr_type subhist;

					if ( level == 0 )
					{
						libmaus::util::Histogram::unique_ptr_type tsubhist(fillSubHistogram(B.begin(),in));
						subhist = UNIQUE_PTR_MOVE(tsubhist);
					}
					else
					{
						libmaus::util::Histogram::unique_ptr_type tsubhist(fillSubHistogram(C[level-1]->begin(),in));
						subhist = UNIQUE_PTR_MOVE(tsubhist);
					}
					
					C_type::generator_type impgen(*subhist);

					if ( level == 0 )
						fillSubArrays(B.begin(),in,*(I[level]),impgen);
					else
						fillSubArrays(C[level-1]->begin(),in,*(I[level]),impgen);
						
					C_ptr_type tClevel(impgen.createFinal());
					C[level] = UNIQUE_PTR_MOVE(tClevel);
					
					in = out;
					++level;
				}
				
				S[0] = n;
				for ( uint64_t i = 0; i < numlevels; ++i )
					S[i+1] = I[i]->size();
				
				if ( rmmtreedebug )
					for ( uint64_t kk = k, level = 0; kk < n; kk *= k, ++level )
					{
						uint64_t low = 0;
						uint64_t z = 0;
						while ( low < n )
						{
							uint64_t const high = std::min(low+kk,n);
							
							uint64_t minv = B[low];
							uint64_t mini = low;
							for ( uint64_t i = low+1; i < high; ++i )
								if ( B[i] < minv )
								{
									minv = B[i];
									mini = i;
								}
								
							assert ( (*C[level])[z] == minv );
							assert ( (*I[level])[z] == ((mini-low)*k)/kk );
						
							++z;	
							low = high;
						}
					}
			}
			
			/*
			 * position of next smaller value after index i (or n if there is no such position)
			 */
			uint64_t nsv(uint64_t const i) const
			{
				// reference value
				uint64_t const rval = B[i];

				// tree position of next smaller value (if any)
				unsigned int nlevel = 0;
				uint64_t ni = i;
				uint64_t nval = rval;

				uint64_t ii = i;
				
				for ( unsigned int level = 0; (nval == rval) && level < I.size(); ++level )
				{
					while ( ++ii & kmask && ii < S[level] )
					{
						uint64_t t;
						if ( (t=(*this)(level,ii)) < rval )
						{
							nlevel = level;
							nval = t;
							ni = ii;
							goto nsvloopdone;
						}
					}
					--ii;

					ii >>= klog;
				}
				nsvloopdone:
				
				// if there is no next smaller value
				if ( nval == rval )
					ni = n;
				// otherwise go down tree and search for the nsv
				else
					while ( nlevel-- )
					{
						ni <<= klog;
						
						while ( (*this)(nlevel,ni) >= rval )
							++ni;
					}
					
				if ( rmmtreedebug )
				{
					uint64_t dni = n;
					for ( uint64_t z = i+1; (dni==n) && z < n; ++z )
						if ( B[z] < rval )
							dni = z;
				
					if ( dni != ni )
						std::cerr << "i=" << i << " dni=" << dni << " ni=" << ni << " rval=" << rval << std::endl;
				}
				
				return ni;
			}

			/*
			 * position of next smaller than rval after index i (or n if there is no such position)
			 */
			uint64_t nsv(uint64_t const i, uint64_t const rval) const
			{
				// tree position of next smaller value (if any)
				unsigned int nlevel = 0;
				uint64_t ni = i;
				uint64_t nval = rval;

				uint64_t ii = i;
				
				for ( unsigned int level = 0; (nval == rval) && level < I.size(); ++level )
				{
					while ( ++ii & kmask && ii < S[level] )
					{
						uint64_t t;
						if ( (t=(*this)(level,ii)) < rval )
						{
							nlevel = level;
							nval = t;
							ni = ii;
							goto nsvloopdone;
						}
					}
					--ii;

					ii >>= klog;
				}
				nsvloopdone:
				
				// if there is no next smaller value
				if ( nval == rval )
					ni = n;
				// otherwise go down tree and search for the nsv
				else
					while ( nlevel-- )
					{
						ni <<= klog;
						
						while ( (*this)(nlevel,ni) >= rval )
							++ni;
					}
					
				if ( rmmtreedebug )
				{
					uint64_t dni = n;
					for ( uint64_t z = i+1; (dni==n) && z < n; ++z )
						if ( B[z] < rval )
							dni = z;
				
					if ( dni != ni )
						std::cerr << "i=" << i << " dni=" << dni << " ni=" << ni << " rval=" << rval << std::endl;
				}
				
				return ni;
			}

			/*
			 * position of previous smaller value before index i (or -1 if there is no such position)
			 */
			int64_t psv(uint64_t const j) const
			{
				// reference value
				uint64_t const rval = B[j];

				// tree position of previous smaller value (if any)
				unsigned int nlevel = 0;
				int64_t nj = j;
				uint64_t nval = rval;

				uint64_t jj = j;
				
				for ( unsigned int level = 0; (nval == rval) && level < I.size(); ++level )
				{
					while ( jj-- & kmask )
					{
						uint64_t t;
						if ( (t=(*this)(level,jj)) < rval )
						{
							nlevel = level;
							nval = t;
							nj = jj;
							goto psvloopdone;
						}
					}

					++jj;
					jj >>= klog;
				}
				psvloopdone:
				
				// if there is no next smaller value
				if ( nval == rval )
					nj = -1;
				// otherwise go down tree and search for the nsv
				else
					while ( nlevel-- )
					{
						nj = (nj<<klog) + (k-1);
						
						while ( (*this)(nlevel,nj) >= rval )
							--nj;
					}
				
				if ( rmmtreedebug )
				{
					int64_t dnj = -1;
					for ( int64_t z = static_cast<int64_t>(j)-1; (dnj==-1) && z >= 0; --z )
						if ( B[z] < rval )
							dnj = z;
				
					if ( dnj != nj )
						std::cerr << "j=" << j << " dnj=" << dnj << " nj=" << nj << " rval=" << rval << std::endl;
				}			
				
				return nj;
			}

			/*
			 * position of previous smaller value before index i (or 0 if there is no such position)
			 */
			int64_t psvz(uint64_t const j) const
			{
				// reference value
				uint64_t const rval = B[j];

				// tree position of previous smaller value (if any)
				unsigned int nlevel = 0;
				int64_t nj = j;
				uint64_t nval = rval;

				uint64_t jj = j;
				
				for ( unsigned int level = 0; (nval == rval) && level < I.size(); ++level )
				{
					while ( jj-- & kmask )
					{
						uint64_t t;
						if ( (t=(*this)(level,jj)) < rval )
						{
							nlevel = level;
							nval = t;
							nj = jj;
							goto psvloopdone;
						}
					}

					++jj;
					jj >>= klog;
				}
				psvloopdone:
				
				// if there is no next smaller value
				if ( nval == rval )
					nj = 0;
				// otherwise go down tree and search for the nsv
				else
					while ( nlevel-- )
					{
						nj = (nj<<klog) + (k-1);
						
						while ( (*this)(nlevel,nj) >= rval )
							--nj;
					}
				
				if ( rmmtreedebug )
				{
					int64_t dnj = 0;
					for ( int64_t z = static_cast<int64_t>(j)-1; (dnj==-1) && z >= 0; --z )
						if ( B[z] < rval )
							dnj = z;
				
					if ( dnj != nj )
						std::cerr << "j=" << j << " dnj=" << dnj << " nj=" << nj << " rval=" << rval << std::endl;
				}			
				
				return nj;
			}

			/**
			 * return position of the leftmost minimum in the interval
			 * of indices [i,j] (including both)
			 **/
			uint64_t rmq(uint64_t const i, uint64_t const j) const
			{
				unsigned int lminlevel = 0, rminlevel = 0;
				uint64_t lminv = (*this)(0,i), rminv = (*this)(0,j);
				uint64_t lmini = i, rmini = j;

				uint64_t ii = i;
				uint64_t jj = j;
				
				for ( 
					unsigned int level = 0;
					true;
					++level
				)
				{
					uint64_t const ip = ii>>klog;
					uint64_t const jp = jj>>klog;
					
					// if we have reached the lowest common ancestor node of i and j
					if ( ip == jp )
					{
						for ( 
							int64_t z = static_cast<int64_t>(jj)-1; 
							z >= static_cast<int64_t>(ii+1); 
							--z 
						)
						{
							uint64_t t;
							if ( (t=(*this)(level,z)) <= rminv )
							{
								rmini = z;
								rminv = t;
								rminlevel = level;
							}
						}
						break;
					}
					// otherwise follow the two paths upward
					else
					{
						while ( ++ii & kmask )
						{
							uint64_t t;
							if ( (t=(*this)(level,ii)) < lminv )
							{
								lmini = ii;
								lminv = t;
								lminlevel = level;
							}
						}

						while ( jj-- & kmask )
						{
							uint64_t t;
							if ( (t=(*this)(level,jj)) <= rminv )
							{
								rmini = jj;
								rminv = t;
								rminlevel = level;
							}
						}
						
						ii = ip;
						jj = jp;
					}
				}
				
				uint64_t minv;
				uint64_t mini;
				unsigned int minlevel;
				
				if ( rminv < lminv )
				{
					minv = rminv;
					mini = rmini;
					minlevel = rminlevel;
				}
				else
				{
					minv = lminv;
					mini = lmini;
					minlevel = lminlevel;
				}
				
				// #define RMMTREEDEBUG
				
				if ( rmmtreedebug )
					assert ( (*this)(minlevel,mini) == minv );

				// follow path to the position of the minimum
				while ( minlevel != 0 )
				{
					mini = (mini << klog) + I[--minlevel]->get(mini);
						
					if ( rmmtreedebug )
						assert ( (*this)(minlevel,mini) == minv );
				}
				
				if ( rmmtreedebug )
				{
					uint64_t dminv = std::numeric_limits<uint64_t>::max();
					uint64_t dmini = std::numeric_limits<uint64_t>::max();
					for ( uint64_t z = i; z <= j; ++ z )
						if ( (*this)(0,z) < dminv )
						{
							dminv = (*this)(0,z);
							dmini = z;
						}
					
					assert ( minv == dminv );
					assert ( mini == dmini );
				}
								
				return mini;
			}
		};
	}
}
#endif
