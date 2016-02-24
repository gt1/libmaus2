/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_FASTX_FASTABPDECODER_HPP)
#define LIBMAUS2_FASTX_FASTABPDECODER_HPP

#include <libmaus2/fastx/FastaBPSequenceDecoder.hpp>
#include <libmaus2/fastx/FastAInfo.hpp>
#include <libmaus2/util/PrefixSums.hpp>

namespace libmaus2
{
	namespace fastx
	{
		template<typename _remap_type>
		struct FastaBPDecoderTemplate
		{
			typedef _remap_type remap_type;
			typedef FastaBPDecoderTemplate<remap_type> this_type;
			typedef libmaus2::fastx::FastaBPSequenceDecoderTemplate<remap_type> sequence_decoder_type;
			typedef typename sequence_decoder_type::unique_ptr_type sequence_decoder_pointer_type;

			private:
			// block size
			uint64_t bs;
			// meta position
			uint64_t metapos;
			// number of sequences
			uint64_t numseq;
			// check crc
			bool checkcrc;

			//! read magic and block size
			uint64_t readBlockSize(std::istream & in) const
			{
				char magic[8];
				in.read(&magic[0],8);
				if ( in.gcount() != 8 )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPDecoderTemplate::readBlockSize(): EOF/error while reading magic" << std::endl;
					lme.finish();
					throw lme;
				}
				if ( std::string(&magic[0],&magic[8]) != std::string("FASTABP\0",8) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPDecoderTemplate::readBlockSize(): magic is wrong, this is not a fab file" << std::endl;
					lme.finish();
					throw lme;
				}

				return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			// read position of meta data
			uint64_t readMetaPos(std::istream & in) const
			{
				in.clear();
				in.seekg(-static_cast<int>(sizeof(uint64_t)),std::ios::end);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			// read number of sequences
			uint64_t readNumSeq(std::istream & in) const
			{
				in.clear();
				in.seekg(metapos,std::ios::beg);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			// read pointer to sequence
			uint64_t readSequencePointer(
				std::istream & in, uint64_t const seq
			) const
			{
				if ( seq >= numseq )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPDecoderTemplate::readSequencePointer: sequence id " << seq << " is out of range." << std::endl;
					lme.finish();
					throw lme;
				}

				in.clear();
				in.seekg(metapos + (1+seq)*sizeof(uint64_t),std::ios::beg);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			libmaus2::fastx::FastAInfo readSequenceInfo(std::istream & in, uint64_t const seq) const
			{
				uint64_t const seqpos = readSequencePointer(in,seq);
				in.clear();
				in.seekg(seqpos);
				libmaus2::fastx::FastAInfo info(in);
				return info;
			}

			uint64_t getNumBlocks(std::istream & in, uint64_t const seq) const
			{
				libmaus2::fastx::FastAInfo info = readSequenceInfo(in,seq);
				return (info.len+bs-1)/bs;
			}

			std::string getSequenceName(std::istream & in, uint64_t const seq) const
			{
				return readSequenceInfo(in,seq).sid;
			}

			uint64_t getSequenceLength(std::istream & in, uint64_t const seq) const
			{
				return readSequenceInfo(in,seq).len;
			}


			uint64_t getBlockPointer(std::istream & in, uint64_t const seq, uint64_t const blockid) const
			{
				uint64_t const numblocks = getNumBlocks(in,seq);

				if ( blockid >= numblocks )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPDecoderTemplate::readSequencePointer: block id " << blockid << " is out of range for sequence " << seq << std::endl;
					lme.finish();
					throw lme;
				}

				in.clear();
				in.seekg(blockid * sizeof(uint64_t), std::ios::cur);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}

			uint64_t getRestSymbols(std::istream & in, uint64_t const seq, uint64_t const blockid) const
			{
				uint64_t const numblocks = getNumBlocks(in,seq);

				if ( blockid >= numblocks )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "FastaBPDecoderTemplate::readSequencePointer: block id " << blockid << " is out of range for sequence " << seq << std::endl;
					lme.finish();
					throw lme;
				}

				return getSequenceLength(in,seq) - blockid * bs;
			}

			void checkBlockPointers(std::istream & in, uint64_t const seq) const
			{
				libmaus2::autoarray::AutoArray<char> Bout(bs,false);
				sequence_decoder_pointer_type ptr(getSequenceDecoder(in,seq,0));

				uint64_t i = 0;

				uint64_t const p = in.tellg();
				uint64_t const bp = getBlockPointer(in,seq,i++);
				assert ( bp == p );
				in.clear();
				in.seekg(p,std::ios::beg);

				uint64_t n = 0;
				while ( (n = ptr->read(Bout.begin(),Bout.size())) )
				{
					if ( ! ptr->eof )
					{
						uint64_t const p = in.tellg();
						uint64_t const bp = getBlockPointer(in,seq,i++);
						assert ( bp == p );
						in.clear();
						in.seekg(p,std::ios::beg);
					}
				}
			}

			public:
			FastaBPDecoderTemplate(std::istream & in, bool const rcheckcrc = false)
			: checkcrc(rcheckcrc)
			{
				bs = readBlockSize(in);
				metapos = readMetaPos(in);
				numseq = readNumSeq(in);
			}

			uint64_t getTotalSequenceLength(std::istream & in) const
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < numseq; ++i )
					s += getSequenceLength(in,i);
				return s;
			}

			sequence_decoder_pointer_type getSequenceDecoder(std::istream & in, uint64_t const seqid, uint64_t const blockid) const
			{
				uint64_t const bp = getBlockPointer(in,seqid,blockid);
				in.clear();
				in.seekg(bp);
				sequence_decoder_pointer_type tptr(new sequence_decoder_type(in,bs,checkcrc));
				return UNIQUE_PTR_MOVE(tptr);
			}


			void checkBlockPointers(std::istream & in) const
			{
				for ( uint64_t i = 0; i < numseq; ++i )
					checkBlockPointers(in,i);
			}

			void decodeSequenceNull(std::istream & in, uint64_t const seq) const
			{
				getSequenceName(in,seq);

				libmaus2::autoarray::AutoArray<char> Bout(bs,false);
				sequence_decoder_pointer_type ptr(getSequenceDecoder(in,seq,0));

				uint64_t n = 0;
				while ( (n = ptr->read(Bout.begin(),Bout.size())) )
				{
				}
			}

			void decodeSequencesNull(std::istream & in) const
			{
				for ( uint64_t i = 0; i < numseq; ++i )
					decodeSequenceNull(in,i);
			}

			void decodeSequencesNullParallel(std::string const & fn, uint64_t const numthreads) const
			{
				assert ( numthreads );

				libmaus2::autoarray::AutoArray<libmaus2::aio::InputStreamInstance::unique_ptr_type> AISI(numthreads);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Tptr(new libmaus2::aio::InputStreamInstance(fn));
					AISI[t] = UNIQUE_PTR_MOVE(Tptr);
				}

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t i = 0; i < numseq; ++i )
				{
					#if defined(_OPENMP)
					uint64_t const t = omp_get_thread_num();
					#else
					uint64_t const t = 0;
					#endif

					decodeSequenceNull(*(AISI[t]),i);
				}
			}

			void printSequence(std::istream & in, std::ostream & out, uint64_t const seq) const
			{
				out << ">" << getSequenceName(in,seq) << "\n";

				libmaus2::autoarray::AutoArray<char> Bout(bs,false);
				sequence_decoder_pointer_type ptr(getSequenceDecoder(in,seq,0));

				uint64_t n = 0;
				while ( (n = ptr->read(Bout.begin(),Bout.size())) )
					std::cout.write(Bout.begin(),n);
				out.put('\n');
			}

			void printSequences(std::istream & in, std::ostream & out) const
			{
				for ( uint64_t i = 0; i < numseq; ++i )
					printSequence(in,out,i);
			}

			libmaus2::autoarray::AutoArray<char> decodeSequence(std::istream & in, uint64_t const seq) const
			{
				uint64_t const seqlen = getSequenceLength(in,seq);
				libmaus2::autoarray::AutoArray<char> A(seqlen,false);
				char * p = A.begin();
				uint64_t n = A.size();
				uint64_t r = 0;

				sequence_decoder_pointer_type ptr(getSequenceDecoder(in,seq,0));
				while ( (r = ptr->read(p,n) ) )
				{
					assert ( r <= n );
					n -= r;
					p += r;
				}

				return A;
			}

			void decodeSequence(std::istream & in, uint64_t const seq, char * p, uint64_t const seqlen) const
			{
				// uint64_t const seqlen = getSequenceLength(in,seq);
				// libmaus2::autoarray::AutoArray<char> A(seqlen,false);
				// char * p = A.begin();
				uint64_t n = seqlen;
				uint64_t r = 0;

				sequence_decoder_pointer_type ptr(getSequenceDecoder(in,seq,0));
				while ( (r = ptr->read(p,n) ) )
				{
					assert ( r <= n );
					n -= r;
					p += r;
				}
			}

			struct SequenceMeta
			{
				uint64_t datastart;
				uint64_t firstbase;
				uint64_t length;
			};

			void decodeSequencesParallel(
				std::string const & fn, uint64_t const numthreads,
				libmaus2::autoarray::AutoArray<char> & Aseq,
				std::vector<SequenceMeta> & Vseqmeta,
				bool const pad = false,
				char padsym = 4
			) const
			{
				assert ( numthreads );

				Vseqmeta.resize(numseq);

				libmaus2::autoarray::AutoArray<libmaus2::aio::InputStreamInstance::unique_ptr_type> AISI(numthreads);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type Tptr(new libmaus2::aio::InputStreamInstance(fn));
					AISI[t] = UNIQUE_PTR_MOVE(Tptr);
				}

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t i = 0; i < numseq; ++i )
				{
					#if defined(_OPENMP)
					uint64_t const t = omp_get_thread_num();
					#else
					uint64_t const t = 0;
					#endif

					Vseqmeta[i].length = getSequenceLength(*(AISI[t]),i);
				}

				uint64_t sum = 0;
				for ( uint64_t i = 0; i < numseq; ++i )
				{
					Vseqmeta[i].datastart = sum;
					Vseqmeta[i].firstbase = Vseqmeta[i].datastart + (pad ? 1 : 0);
					sum += Vseqmeta[i].length + (pad ? 1 : 0);
				}
				sum += (pad ? 1 : 0);

				Aseq.release();
				Aseq = libmaus2::autoarray::AutoArray<char>(sum,false);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t i = 0; i < numseq; ++i )
				{
					#if defined(_OPENMP)
					uint64_t const t = omp_get_thread_num();
					#else
					uint64_t const t = 0;
					#endif

					if ( pad )
						Aseq[Vseqmeta[i].datastart] = padsym;

					decodeSequence(*(AISI[t]),i,Aseq.begin()+Vseqmeta[i].firstbase,Vseqmeta[i].length);
				}

				if ( pad )
					Aseq[sum-1] = padsym;
			}

			uint64_t getNumSeq() const
			{
				return numseq;
			}
		};

		typedef FastaBPDecoderTemplate<FastaBPSequenceDecoderRemap> FastaBPDecoder;
		typedef FastaBPDecoderTemplate<FastaBPSequenceDecoderRemapIdentity> FastaBPDecoderIdentity;
	}
}
#endif
