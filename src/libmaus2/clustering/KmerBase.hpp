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
#if ! defined(LIBMAUS2_CLUSTERING_KMERBASE_HPP)
#define LIBMAUS2_CLUSTERING_KMERBASE_HPP

#include <libmaus2/clustering/HashCreationBase.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>

namespace libmaus2
{
	namespace clustering
	{
		struct KmerBase
		{
			typedef KmerBase this_type;
			typedef ::libmaus2::clustering::HashCreationBase hash_creation;
			typedef ::libmaus2::fastx::SingleWordDNABitBuffer single_word_buffer_type;

			::libmaus2::autoarray::AutoArray<unsigned int> const E;
			::libmaus2::autoarray::AutoArray<uint8_t> const S;
			::libmaus2::autoarray::AutoArray<uint8_t> const R;

			KmerBase()
			: E(hash_creation::createErrorMap()), S(hash_creation::createSymMap()), R(hash_creation::createRevSymMap())
			{

			}

			template<typename pattern_iterator_type, typename callback_type>
			void kmerCallback(
				pattern_iterator_type const pattern,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				single_word_buffer_type & reve,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k )
				{
					forw.reset();
					reve.reset();

					char const * sequence = pattern;
					char const * fsequence = sequence;
					char const * rsequence = (sequence + k);

					// number of indeterminate bases in current kmer
					unsigned int e = 0;
					// fill in first kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(sequence++);
						e += E [ base ];
						forw.pushBackUnmasked( S [ base ]  );

						char const rbase = *(--rsequence);
						reve.pushBackUnmasked( R [ rbase ] );
					}

					rsequence = pattern + k;

					// iterate over kmers
					for ( unsigned int z = 0; z < l-k+1; )
					{
						if ( e < 1 )
						{
							uint64_t const fword = forw.buffer;
							uint64_t const rword = reve.buffer;

							if ( fword <= rword )
							{
								if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
									callback(fword);
							}
							else
							{
								if ( (rword>>hashshift) >= minhash && (rword>>hashshift) < maxhash )
									callback(rword);
							}
						}

						// compute next kmer data if there are more bases
						if ( ++z < l-k+1 )
						{
							e -= E[*(fsequence++)];

							char const base = *(sequence++);
							forw.pushBackMasked( S[base] );
							reve.pushFront( R[base] );

							e += E[base];
						}
					}
				}

			}

			template<
				typename pattern_iterator_type,
				typename callback_type
			>
			void kmerCallbackPos(
				pattern_iterator_type const pattern,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				single_word_buffer_type & reve,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k )
				{
					forw.reset();
					reve.reset();

					char const * sequence = pattern;
					char const * fsequence = sequence;
					char const * rsequence = (sequence + k);

					// number of indeterminate bases in current kmer
					unsigned int e = 0;
					// fill in first kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(sequence++);
						e += E [ base ];
						forw.pushBackUnmasked( S [ base ]  );

						char const rbase = *(--rsequence);
						reve.pushBackUnmasked( R [ rbase ] );
					}

					rsequence = pattern + k;

					// iterate over kmers
					for ( unsigned int z = 0; z < l-k+1; )
					{
						if ( e < 1 )
						{
							uint64_t const fword = forw.buffer;
							uint64_t const rword = reve.buffer;

							if ( fword <= rword )
							{
								if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
									callback(fword,z,false);
							}
							else
							{
								if ( (rword>>hashshift) >= minhash && (rword>>hashshift) < maxhash )
									callback(rword,z,true);
							}
						}

						// compute next kmer data if there are more bases
						if ( ++z < l-k+1 )
						{
							e -= E[*(fsequence++)];

							char const base = *(sequence++);
							forw.pushBackMasked( S[base] );
							reve.pushFront( R[base] );

							e += E[base];
						}
					}
				}
			}

			template<
				typename pattern_iterator_type,
				typename callback_type
			>
			void kmerCallbackPosFail(
				pattern_iterator_type const pattern,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				single_word_buffer_type & reve,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k )
				{
					forw.reset();
					reve.reset();

					char const * sequence = pattern;
					char const * fsequence = sequence;
					char const * rsequence = (sequence + k);

					// number of indeterminate bases in current kmer
					unsigned int e = 0;
					// fill in first kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(sequence++);
						e += E [ base ];
						forw.pushBackUnmasked( S [ base ]  );

						char const rbase = *(--rsequence);
						reve.pushBackUnmasked( R [ rbase ] );
					}

					rsequence = pattern + k;

					// iterate over kmers
					for ( unsigned int z = 0; z < l-k+1; )
					{
						if ( e < 1 )
						{
							uint64_t const fword = forw.buffer;
							uint64_t const rword = reve.buffer;

							if ( fword <= rword )
							{
								if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
									callback(fword,z,false);
							}
							else
							{
								if ( (rword>>hashshift) >= minhash && (rword>>hashshift) < maxhash )
									callback(rword,z,true);
							}
						}
						else
						{
							callback.fail(z);
						}

						// compute next kmer data if there are more bases
						if ( ++z < l-k+1 )
						{
							e -= E[*(fsequence++)];

							char const base = *(sequence++);
							forw.pushBackMasked( S[base] );
							reve.pushFront( R[base] );

							e += E[base];
						}
					}
				}
			}

			template<
				typename pattern_iterator_type,
				typename callback_type
			>
			void kmerCallbackPosForwardOnly(
				pattern_iterator_type const pattern_a,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k )
				{
					forw.reset();

					pattern_iterator_type sequence = pattern_a;
					pattern_iterator_type fsequence = sequence;

					// number of indeterminate bases in current kmer
					unsigned int e = 0;
					// fill in first kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(sequence++);
						e += E [ base ];
						forw.pushBackUnmasked( S [ base ]  );
					}

					// iterate over kmers
					for ( unsigned int z = 0; z < l-k+1; )
					{
						if ( e < 1 )
						{
							uint64_t const fword = forw.buffer;

							if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
								callback(fword,z);
						}

						// compute next kmer data if there are more bases
						if ( ++z < l-k+1 )
						{
							e -= E[*(fsequence++)];

							char const base = *(sequence++);
							forw.pushBackMasked( S[base] );

							e += E[base];
						}
					}
				}
			}

			template<
				typename pattern_iterator_type,
				typename callback_type
			>
			void kmerCallbackPosForwardOnlyPreMapped(
				pattern_iterator_type const pattern_a,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k )
				{
					forw.reset();

					pattern_iterator_type sequence = pattern_a;
					pattern_iterator_type fsequence = sequence;

					// number of indeterminate bases in current kmer
					unsigned int e = 0;
					// fill in first kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(sequence++);
						e += (base>3);
						forw.pushBackUnmasked( base );
					}

					// iterate over kmers
					for ( unsigned int z = 0; z < l-k+1; )
					{
						if ( e < 1 )
						{
							uint64_t const fword = forw.buffer;

							if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
								callback(fword,z);
						}

						// compute next kmer data if there are more bases
						if ( ++z < l-k+1 )
						{
							e -= (*(fsequence++)) > 3;

							char const base = *(sequence++);
							forw.pushBackMasked( base );

							e += (base > 3);
						}
					}
				}
			}

			template<
				typename pattern_iterator_type,
				typename callback_type
			>
			void kmerCallbackPosForwardOnlyReverseCircular(
				pattern_iterator_type const pattern_a,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k )
				{
					forw.reset();

					// error bit mask
					uint64_t const emask = libmaus2::math::lowbits(k);
					// error bit vector
					uint64_t e = 0;

					// end of sequence
					pattern_iterator_type psequence = pattern_a + l;

					pattern_iterator_type primseq = pattern_a + k;
					// fill in kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(--primseq);
						e = (e << 1) | (E [ base ] ? 1ull : 0ull);
						forw.pushFront( S [ base ]  );
					}

					// iterate over kmers
					for ( unsigned int z = 0; z < l; ++z )
					{
						char const base = *(--psequence);
						e = ((e << 1) & emask) | (E [ base ] ? 1ull : 0ull);
						forw.pushFront( S[base] );

						if ( ! e )
						{
							uint64_t const fword = forw.buffer;

							if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
								callback(fword,-static_cast<int>(z)-1);
						}
					}
				}
			}

			template<
				typename pattern_iterator_type,
				typename callback_type
			>
			void kmerCallbackPosForwardOnlyForwardCircular(
				pattern_iterator_type const pattern_a,
				pattern_iterator_type const pattern_e,
				unsigned int const l,
				callback_type & callback,
				single_word_buffer_type & forw,
				unsigned int const k,
				uint64_t const minhash = 0,
				uint64_t const maxhash = std::numeric_limits<uint64_t>::max(),
				unsigned int const hashshift = 0
			) const
			{
				if ( l >= k && pattern_e != pattern_a )
				{
					forw.reset();

					pattern_iterator_type sequence = pattern_a;
					pattern_iterator_type fsequence = sequence;

					// number of indeterminate bases in current kmer
					unsigned int e = 0;
					// fill in first kmer
					for ( unsigned int i = 0; i < k; ++i )
					{
						char const base = *(sequence++);
						if ( sequence == pattern_e )
							sequence = pattern_a;
						e += E [ base ];
						forw.pushBackUnmasked( S [ base ]  );
					}

					// iterate over kmers
					for ( unsigned int z = 0; z < l; ++z )
					{
						if ( e < 1 )
						{
							uint64_t const fword = forw.buffer;

							if ( (fword>>hashshift) >= minhash && (fword>>hashshift) < maxhash )
								callback(fword,z);
						}

						// compute next kmer data if there are more bases
						e -= E[*(fsequence++)];
						if ( fsequence == pattern_e )
							fsequence = pattern_a;

						char const base = *(sequence++);
						if ( sequence == pattern_e )
							sequence = pattern_a;
						forw.pushBackMasked( S[base] );

						e += E[base];
					}
				}
			}
		};
	}
}
#endif
