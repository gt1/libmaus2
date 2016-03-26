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
#if !defined(LIBMAUS2_RMQ_RMMTREE_HPP)
#define LIBMAUS2_RMQ_RMMTREE_HPP

#include <libmaus2/util/ImpCompactNumberArray.hpp>
#include <libmaus2/util/Array864.hpp>

namespace libmaus2
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
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef libmaus2::util::ImpCompactNumberArray C_type;
			// typedef libmaus2::util::Array864 C_type;
			typedef C_type::unique_ptr_type C_ptr_type;

			static unsigned int const k = (1u << klog);
			static uint64_t const kmask = (k-1);

			base_layer_type const & B;
			uint64_t const n;
			unsigned int const numlevels;
			libmaus2::autoarray::AutoArray< libmaus2::bitio::CompactArray::unique_ptr_type > I;
			libmaus2::autoarray::AutoArray< C_ptr_type > C;
			libmaus2::autoarray::AutoArray< uint64_t > S;

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
				libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,numlevels);

				libmaus2::util::NumberSerialisation::serialiseNumber(out,I.size());
				libmaus2::util::NumberSerialisation::serialiseNumber(out,C.size());

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
				n(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				numlevels(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				I(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				C(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				S(in)
			{
				for ( uint64_t i = 0; i < I.size(); ++i )
				{
					libmaus2::bitio::CompactArray::unique_ptr_type tIi(new libmaus2::bitio::CompactArray(in));
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
				libmaus2::aio::InputStreamInstance CIS(fn);
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
			static libmaus2::util::Histogram::unique_ptr_type fillSubHistogram(
				in_iterator in_it,
				uint64_t const in,
				uint64_t const numthreads
			)
			{
				libmaus2::util::Histogram::unique_ptr_type phist(new libmaus2::util::Histogram);
				libmaus2::util::Histogram & hist = *phist;
				libmaus2::parallel::PosixSpinLock lock;

				uint64_t const full = in >> klog;
				uint64_t const rest = in-full*k;

				uint64_t const fullperthread = (full + numthreads - 1)/numthreads;
				uint64_t const numpacks = full ? ((full + fullperthread - 1)/fullperthread) : 0;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const ilow = t * fullperthread;
					uint64_t const ihigh = std::min(ilow + fullperthread,full);

					in_iterator it_a = in_it + ilow*k;
					in_iterator it = it_a;
					libmaus2::util::Histogram lhist;

					for ( uint64_t i = ilow; i < ihigh; ++i )
					{
						uint64_t vmin = *(it++);
						for ( unsigned int j = 1; j < k; ++j )
						{
							uint64_t const vj = *(it++);
							if ( vj < vmin )
								vmin = vj;
						}

						lhist(libmaus2::math::bitsPerNum(vmin));
					}

					assert ( (it - in_it) % k == 0 );
					assert ( (it - it_a) == (ihigh-ilow)*k );

					{
						libmaus2::parallel::ScopePosixSpinLock slock(lock);
						hist.merge(lhist);
					}
				}

				#if 0
				for ( uint64_t i = 0; i < full; ++i )
				{
					uint64_t vmin = *(in_it++);
					for ( unsigned int j = 1; j < k; ++j )
					{
						uint64_t const vj = *(in_it++);
						if ( vj < vmin )
							vmin = vj;
					}

					hist(libmaus2::math::bitsPerNum(vmin));
				}
				#endif

				if ( rest )
				{
					in_iterator it = in_it + full*k;

					uint64_t vmin = *(it++);
					for ( unsigned int j = 1; j < rest; ++j )
					{
						uint64_t const vj = *(it++);
						if ( vj < vmin )
							vmin = vj;
					}

					hist(libmaus2::math::bitsPerNum(vmin));
				}

				return UNIQUE_PTR_MOVE(phist);
			}

			template<typename in_iterator>
			static libmaus2::huffman::HuffmanTreeNode::shared_ptr_type fillSubNode(
				in_iterator in_it, uint64_t in, uint64_t const numthreads
			)
			{
				libmaus2::util::Histogram::unique_ptr_type phist = UNIQUE_PTR_MOVE(fillSubHistogram(in_it,in,numthreads));
				std::map<int64_t,uint64_t> const probs = phist->getByType<int64_t>();
				libmaus2::huffman::HuffmanTreeNode::shared_ptr_type snode = libmaus2::huffman::HuffmanBase::createTree(probs);
				return snode;
			}

			template<typename in_iterator, typename C_generator_type>
			static void fillSubArrays(
				in_iterator in_it,
				uint64_t const in,
				libmaus2::bitio::CompactArray & I,
				C_generator_type & impgen,
				uint64_t const numthreads,
				libmaus2::autoarray::AutoArray<uint64_t> & L,
				libmaus2::autoarray::AutoArray<uint8_t> & J
			)
			{
				assert ( L.size() == J.size() );
				assert ( L.size() != 0 );

				uint64_t const full = in >> klog;
				uint64_t const rest = in-full*k;

				uint64_t const blocksize = L.size();
				uint64_t const numblocks = (full + blocksize-1)/blocksize;

				for ( uint64_t bb = 0; bb < numblocks; ++bb )
				{
					uint64_t const ilow = bb * blocksize;
					uint64_t const ihigh = std::min(ilow+blocksize,full);
					uint64_t const irange = (ihigh-ilow);
					assert ( irange );
					uint64_t const iperthread = (irange + numthreads - 1) / numthreads;
					uint64_t const numpacks = (irange + iperthread - 1)/iperthread;

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( uint64_t t = 0; t < numpacks; ++t )
					{
						uint64_t const tlow = ilow + t*iperthread;
						uint64_t const thigh = std::min(tlow+iperthread,ihigh);
						uint64_t const trange = thigh-tlow;

						in_iterator const it_a = in_it + tlow * k;
						in_iterator it = it_a;

						for ( uint64_t i = tlow; i < thigh; ++i )
						{
							uint64_t vmin = *(it++);
							unsigned int jmin = 0;
							for ( unsigned int j = 1; j < k; ++j )
							{
								uint64_t const vj = *(it++);
								if ( vj < vmin )
								{
									vmin = vj;
									jmin = j;
								}
							}

							J[i-ilow] = jmin;
							L[i-ilow] = vmin;
						}

						assert ( it == it_a + (trange*k) );
					}

					for ( uint64_t i = ilow; i < ihigh; ++i )
					{
						I.set(i,J[i-ilow]);
						impgen.add(L[i-ilow]);
					}
				}

				#if 0
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
				#endif

				if ( rest )
				{
					in_iterator it = in_it + full*k;
					uint64_t vmin = *(it++);
					unsigned int jmin = 0;
					for ( unsigned int j = 1; j < rest; ++j )
					{
						uint64_t const vj = *(it++);
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

			RMMTree(base_layer_type const & rB, uint64_t const rn, uint64_t const numthreads, uint64_t const blocksize, std::ostream * logstr)
			: B(rB), n(rn), numlevels(computeNumLevels(n)), I(numlevels), C(numlevels), S(numlevels+1)
			{
				uint64_t in = n;
				unsigned int level = 0;

				libmaus2::autoarray::AutoArray<uint64_t> L(blocksize,false);
				libmaus2::autoarray::AutoArray<uint8_t> J(blocksize,false);

				while ( in > 1 )
				{
					libmaus2::timing::RealTimeClock rtc;
					rtc.start();

					uint64_t const out = (in+k-1) >> klog;

					// minimal indices for next level
					libmaus2::bitio::CompactArray::unique_ptr_type tIlevel(
                                                new libmaus2::bitio::CompactArray(out,klog));
					I[level] = UNIQUE_PTR_MOVE(tIlevel);

					libmaus2::util::Histogram::unique_ptr_type subhist;

					if ( level == 0 )
					{
						libmaus2::util::Histogram::unique_ptr_type tsubhist(fillSubHistogram(B.begin(),in,numthreads));
						subhist = UNIQUE_PTR_MOVE(tsubhist);
					}
					else
					{
						libmaus2::util::Histogram::unique_ptr_type tsubhist(fillSubHistogram(C[level-1]->begin(),in,numthreads));
						subhist = UNIQUE_PTR_MOVE(tsubhist);
					}

					C_type::generator_type impgen(*subhist);

					if ( level == 0 )
						fillSubArrays(B.begin(),in,*(I[level]),impgen,numthreads,L,J);
					else
						fillSubArrays(C[level-1]->begin(),in,*(I[level]),impgen,numthreads,L,J);

					C_ptr_type tClevel(impgen.createFinal());
					C[level] = UNIQUE_PTR_MOVE(tClevel);

					if ( logstr )
					{
						std::cerr << "[V] rmm level " << level << " size " << in << " full time " << rtc.getElapsedSeconds() << std::endl;
					}

					in = out;
					++level;
				}

				S[0] = n;
				for ( uint64_t i = 0; i < numlevels; ++i )
					S[i+1] = I[i]->size();

				if ( 1 || rmmtreedebug )
				{
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
