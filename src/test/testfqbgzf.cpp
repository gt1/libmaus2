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
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/fastx/SocketFastQReader.hpp>
#include <libmaus2/util/PushBuffer.hpp>
#include <libmaus2/lz/BgzfDeflateParallel.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>

struct PatternBuffer : public libmaus2::util::PushBuffer<libmaus2::fastx::SocketFastQReader::pattern_type>
{
	PatternBuffer()
	{
	
	}
	
	libmaus2::fastx::SocketFastQReader::pattern_type & getNextPattern()
	{
		return get();
	}	
};

#include <libmaus2/clustering/KMeans.hpp>

int main(int argc, char * argv[])
{
	try
	{
		{
		std::vector<double> V;
		V.push_back(1);
		V.push_back(2);
		V.push_back(3);
		V.push_back(4);
		V.push_back(5);
		std::vector<double> C = libmaus2::clustering::KMeans::kmeans(V.begin(),V.size(),2);
		for ( uint64_t i = 0; i < C.size(); ++i )
			std::cerr << C[i] << std::endl;
		}
		#if 0
			template<typename iterator, typename dissimilarity_type = Dissimilary>
			static std::vector<double> kmeans(
				iterator V, 
				uint64_t const n,
				uint64_t const k, 
				bool const pp = true,
				uint64_t const iterations = 10, 
				uint64_t const maxloops = 16*1024, double const ethres = 1e-6,
				dissimilarity_type dissimilary = dissimilarity_type()
			)
		#endif
		
	
		libmaus2::util::ArgInfo const arginfo(argc,argv);
		
		if ( !arginfo.hasArg("index") )
		{
			libmaus2::exception::LibMausException se;
			se.getStream() << "Please set the index key (e.g. index=file.idx)" << std::endl;
			se.finish();
			throw se;
		}

		std::string const indexfilename = arginfo.getUnparsedValue("index","file.idx" /* unused */);
		std::string const deftmp = arginfo.getDefaultTmpFileName();
		std::string const fifilename = deftmp + ".fi";
		std::string const bgzfidxfilename = deftmp + ".bgzfidx";
		libmaus2::util::TempFileRemovalContainer::addTempFile(fifilename);
		libmaus2::util::TempFileRemovalContainer::addTempFile(bgzfidxfilename);
	
		::libmaus2::network::SocketBase fdin(STDIN_FILENO);
		typedef ::libmaus2::fastx::SocketFastQReader reader_type;
		reader_type reader(&fdin,0 /* q offset */);
		
		libmaus2::aio::OutputStreamInstance::unique_ptr_type bgzfidoutstr(
			new libmaus2::aio::OutputStreamInstance(bgzfidxfilename)
		);
		libmaus2::aio::OutputStreamInstance::unique_ptr_type fioutstr(
			new libmaus2::aio::OutputStreamInstance(fifilename)
		);
		libmaus2::lz::BgzfDeflateParallel::unique_ptr_type 
			bgzfenc(
				new libmaus2::lz::BgzfDeflateParallel(
					std::cout,32,128,Z_DEFAULT_COMPRESSION,
					bgzfidoutstr.get()
				)
			);
		
		typedef reader_type::pattern_type pattern_type;
		
		PatternBuffer buf;
		uint64_t fqacc = 0;
		uint64_t patacc = 0;
		uint64_t numblocks = 0;
		libmaus2::autoarray::AutoArray<char> outbuf(libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize(),false);
	
		while ( reader.getNextPatternUnlocked(buf.getNextPattern()) )
		{
			uint64_t const patid = buf.f-1;
			pattern_type & pattern = buf.A[patid];
		
			uint64_t const qlen = pattern.spattern.size();
			uint64_t const nlen = pattern.sid.size();
			uint64_t const fqlen =  
				1 + nlen + 1 + 
				qlen + 1 + 
				1 + 1 + 
				qlen + 1
			;

			if ( fqacc + fqlen <= libmaus2::lz::BgzfConstants::getBgzfMaxBlockSize() )
			{
				fqacc += fqlen;
			}
			else
			{
				char * outp = outbuf.begin();
				uint64_t lnumsyms = 0;
				uint64_t minlen = std::numeric_limits<uint64_t>::max();
				uint64_t maxlen = 0;
				
				for ( uint64_t i = 0; i < patid; ++i )
				{
					*(outp)++ = '@';
					std::copy(buf.A[i].sid.begin(), buf.A[i].sid.end(),outp);
					outp += buf.A[i].sid.size();
					*(outp++) = '\n';

					std::copy(buf.A[i].spattern.begin(), buf.A[i].spattern.end(),outp);
					outp += buf.A[i].spattern.size();
					lnumsyms += buf.A[i].spattern.size();
					minlen = std::min(minlen,static_cast<uint64_t>(buf.A[i].spattern.size()));
					maxlen = std::max(maxlen,static_cast<uint64_t>(buf.A[i].spattern.size()));
					*(outp++) = '\n';

					*(outp)++ = '+';
					*(outp++) = '\n';

					std::copy(buf.A[i].quality.begin(), buf.A[i].quality.end(),outp);
					outp += buf.A[i].quality.size();
					*(outp++) = '\n';
				}
				
				//std::cerr << "expect " << fqacc << " got " << outp-outbuf.begin() << std::endl;
				
				assert ( outp - outbuf.begin() == static_cast<ptrdiff_t>(fqacc) );
				
				// std::cout.write(outbuf.begin(),outp-outbuf.begin());
				bgzfenc->writeSynced(outbuf.begin(),outp-outbuf.begin());

				uint64_t const fqlow  = patacc;
				uint64_t const fqhigh = fqlow + patid;
				uint64_t const fqfileoffset = 0;
				uint64_t const fqfileoffsethigh = 0;
				uint64_t const fqnumsyms = lnumsyms;
				uint64_t const fqminlen = minlen;
				uint64_t const fqmaxlen = maxlen;
				
				libmaus2::fastx::FastInterval FI(
					fqlow,fqhigh,fqfileoffset,fqfileoffsethigh,
					fqnumsyms,fqminlen,fqmaxlen
				);
				
				// std::cerr << "*" << FI << std::endl;
				
				(*fioutstr) << FI.serialise();
				
				numblocks++;

				fqacc = fqlen;
				buf.A[0] = buf.A[patid];
				buf.f = 1;
				
				patacc += patid;
				
				// std::cerr << "flushed." << std::endl;
			}
		}
		
		buf.f -= 1;

		char * outp = outbuf.begin();
		uint64_t lnumsyms = 0;
		uint64_t minlen = std::numeric_limits<uint64_t>::max();
		uint64_t maxlen = 0;
		
		for ( uint64_t i = 0; i < buf.f; ++i )
		{
			*(outp)++ = '@';
			std::copy(buf.A[i].sid.begin(), buf.A[i].sid.end(),outp);
			outp += buf.A[i].sid.size();
			*(outp++) = '\n';

			std::copy(buf.A[i].spattern.begin(), buf.A[i].spattern.end(),outp);
			outp += buf.A[i].spattern.size();
			lnumsyms += buf.A[i].spattern.size();
			minlen = std::min(minlen,static_cast<uint64_t>(buf.A[i].spattern.size()));
			maxlen = std::max(maxlen,static_cast<uint64_t>(buf.A[i].spattern.size()));
			*(outp++) = '\n';

			*(outp)++ = '+';
			*(outp++) = '\n';

			std::copy(buf.A[i].quality.begin(), buf.A[i].quality.end(),outp);
			outp += buf.A[i].quality.size();
			*(outp++) = '\n';
		}
		
		// std::cerr << "expecting " << fqacc << " got " << outp - outbuf.begin() << std::endl;
		
		// std::cout.write(outbuf.begin(),outp-outbuf.begin());
		bgzfenc->writeSynced(outbuf.begin(),outp-outbuf.begin());

		uint64_t const fqlow  = patacc;
		uint64_t const fqhigh = fqlow + buf.f;
		uint64_t const fqfileoffset = 0;
		uint64_t const fqfileoffsethigh = 0;
		uint64_t const fqnumsyms = lnumsyms;
		uint64_t const fqminlen = minlen;
		uint64_t const fqmaxlen = maxlen;
		
		libmaus2::fastx::FastInterval FI(
			fqlow,fqhigh,fqfileoffset,fqfileoffsethigh,
			fqnumsyms,fqminlen,fqmaxlen
		);

		// std::cerr << "*" << FI << std::endl;

		(*fioutstr) << FI.serialise();

		numblocks++;

		assert ( outp - outbuf.begin() == static_cast<ptrdiff_t>(fqacc) );
				
		fqacc = 0;
		buf.f = 0;
		
		bgzfenc->flush();
		bgzfenc.reset();
		bgzfidoutstr->flush();
		bgzfidoutstr.reset();
		fioutstr->flush();
		fioutstr.reset();
		
		libmaus2::aio::InputStreamInstance fiin(fifilename);
		libmaus2::aio::InputStreamInstance bgzfidxin(bgzfidxfilename);
		libmaus2::fastx::FastInterval rFI;
		
		uint64_t filow = 0;
		uint64_t fihigh = 0;
		
		libmaus2::aio::OutputStreamInstance indexCOS(indexfilename);
		uint64_t const combrate = 4;
		::libmaus2::util::NumberSerialisation::serialiseNumber(indexCOS,(numblocks+combrate-1)/combrate);
		std::vector < libmaus2::fastx::FastInterval > FIV;
		
		for ( uint64_t i = 0; i < numblocks; ++i )
		{
			rFI = libmaus2::fastx::FastInterval::deserialise(fiin);
			/* uint64_t const uncomp = */ libmaus2::util::UTF8::decodeUTF8(bgzfidxin);
			uint64_t const comp = libmaus2::util::UTF8::decodeUTF8(bgzfidxin);
			
			fihigh += comp;
			
			rFI.fileoffset = filow;
			rFI.fileoffsethigh = fihigh;
			
			// std::cerr << rFI << std::endl;
			
			FIV.push_back(rFI);
			
			if ( FIV.size() == combrate )
			{
				indexCOS << libmaus2::fastx::FastInterval::merge(FIV.begin(),FIV.end()).serialise();
				FIV.clear();
			}
			// indexCOS << rFI.serialise();
			
			filow = fihigh;
		}
		
		if ( FIV.size() )
		{
			indexCOS << libmaus2::fastx::FastInterval::merge(FIV.begin(),FIV.end()).serialise();
			FIV.clear();	
		}
		
		indexCOS.flush();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
