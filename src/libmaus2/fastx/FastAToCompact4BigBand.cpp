/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/fastx/FastAToCompact4BigBand.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <algorithm>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/fastx/StreamFastAReader.hpp>
#include <libmaus2/lz/BufferedGzipStream.hpp>
#include <libmaus2/util/OutputFileNameTools.hpp>
#include <libmaus2/bitio/CompactArrayWriterFile.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/lz/IsGzip.hpp>

/*
 * produce 2 bit representation plus meta data from (compressed) fasta file
 */

static std::string formatBytes(uint64_t n)
{
	char const * units[] = { "", "k", "m", "g", "t", "p", "e", "z", "y" };

	std::vector < std::string > parts;
	unsigned int uindex = 0;

	for ( ; n; ++uindex )
	{
		std::ostringstream ostr;
		ostr << (n%1024) << units[uindex];
		parts.push_back(ostr.str());

		n /= 1024;
	}

	std::reverse(parts.begin(),parts.end());

	std::ostringstream ostr;
	for ( uint64_t i = 0; i < parts.size(); ++i )
	{
		ostr << parts[i];
		if ( i+1 < parts.size() )
			ostr << " ";
	}

	return ostr.str();
}

static std::string basename(std::string const s)
{
	uint64_t l = s.size();
	for ( uint64_t i = 0; i < s.size(); ++i )
		if ( s[i] == '/' )
			l = i;

	if ( l != s.size() )
		return s.substr(l+1);
	else
		return s;
}

static std::string stripAfterDot(std::string const s)
{
	for ( uint64_t i = 0; i < s.size(); ++i )
		if ( s[i] == '.' )
			return s.substr(0,i);

	return s;
}

int libmaus2::fastx::FastAToCompact4BigBand::fastaToCompact4BigBand(
	std::vector<std::string> const & inputfilenames,
	// add reverse complement
	bool const rc,
	// add reverse complement to replace FastA file
	bool const replrc,
	std::ostream * logstr,
	std::string outputfilename
)
{
	std::string const inlcp = libmaus2::util::OutputFileNameTools::lcp(inputfilenames);

	std::string defout = inlcp;
	defout = libmaus2::util::OutputFileNameTools::clipOff(defout,".gz");
	defout = libmaus2::util::OutputFileNameTools::clipOff(defout,".fasta");
	defout = libmaus2::util::OutputFileNameTools::clipOff(defout,".fa");
	if ( ! outputfilename.size() )
		outputfilename = defout + ".compact";

	std::string const metaoutputfilename = outputfilename + ".meta";
	std::string const repfastaoutputfilename = outputfilename + ".repl.fasta";
	libmaus2::autoarray::AutoArray<char> B(8*1024,false);
	libmaus2::bitio::CompactArrayWriterFile compactout(outputfilename,2 /* bits per symbol */);

	libmaus2::aio::OutputStreamInstance::unique_ptr_type replOSI(new libmaus2::aio::OutputStreamInstance(repfastaoutputfilename));

	if ( ! rc )
		std::cerr << "[V] not storing reverse complements" << std::endl;

	// forward mapping table
	libmaus2::autoarray::AutoArray<uint8_t> ftable(256,false);
	// rc mapping for mapped symbols
	libmaus2::autoarray::AutoArray<uint8_t> ctable(256,false);
	// backward mapping table
	libmaus2::autoarray::AutoArray<uint8_t> rtable(256,false);

	std::fill(ftable.begin(),ftable.end(),4);
	std::fill(ctable.begin(),ctable.end(),4);
	std::fill(rtable.begin(),rtable.end(),'N');
	ftable['a'] = ftable['A'] = 0;
	ftable['c'] = ftable['C'] = 1;
	ftable['g'] = ftable['G'] = 2;
	ftable['t'] = ftable['T'] = 3;
	uint64_t insize = 0;

	ctable[0] = 3; // A->T
	ctable[1] = 2; // C->G
	ctable[2] = 1; // G->C
	ctable[3] = 0; // T->A

	rtable[0] = 'A';
	rtable[1] = 'C';
	rtable[2] = 'G';
	rtable[3] = 'T';

	libmaus2::aio::OutputStreamInstance::unique_ptr_type metaOut(new libmaus2::aio::OutputStreamInstance(metaoutputfilename));
	libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,0);
	uint64_t nseq = 0;
	std::vector<uint64_t> lvec;
	std::vector<uint64_t> ovec;
	uint64_t nsum = 0;

	std::string const rcfile = outputfilename + ".rctmp";

	for ( unsigned int rci = 0; rci < (rc?2:1); ++rci )
	{
		libmaus2::aio::OutputStreamInstance::unique_ptr_type rctmpOSI;
		libmaus2::aio::InputStreamInstance::unique_ptr_type rctmpISI;
		if ( rci == 0 )
		{
			libmaus2::util::TempFileRemovalContainer::addTempFile(rcfile);
			libmaus2::aio::OutputStreamInstance::unique_ptr_type trctmpOSI(new libmaus2::aio::OutputStreamInstance(rcfile));
			rctmpOSI = UNIQUE_PTR_MOVE(trctmpOSI);
		}
		if ( rci == 1 )
		{
			libmaus2::aio::InputStreamInstance::unique_ptr_type rrctmpISI(new libmaus2::aio::InputStreamInstance(rcfile));
			rctmpISI = UNIQUE_PTR_MOVE(rrctmpISI);
		}

		for ( uint64_t i = 0; i < inputfilenames.size(); ++i )
		{
			std::string const fn = inputfilenames[i];
			libmaus2::aio::InputStreamInstance CIS(fn);
			libmaus2::lz::BufferedGzipStream::unique_ptr_type BGS;
			std::istream * istr = 0;
			if ( libmaus2::lz::IsGzip::isGzip(CIS) )
			{
				libmaus2::lz::BufferedGzipStream::unique_ptr_type tBGS(
					new libmaus2::lz::BufferedGzipStream(CIS));
				BGS = UNIQUE_PTR_MOVE(tBGS);
				istr = BGS.get();
			}
			else
			{
				istr = &CIS;
			}
			libmaus2::fastx::StreamFastAReaderWrapper fain(*istr);
			libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;

			for ( uint64_t rid = 0; fain.getNextPatternUnlocked(pattern); ++rid )
			{
				if ( logstr )
					*logstr << (i+1) << " " << stripAfterDot(basename(fn)) << " " << pattern.sid << "...";

				ovec.push_back( metaOut->tellp() );

				assert (
					static_cast<int64_t>(metaOut->tellp())
					==
					static_cast<int64_t>(
						sizeof(uint64_t) /* nseq */
						+
						2*sizeof(uint64_t)*rid
						+
						nsum * 2 * sizeof(uint64_t)
					)
				);

				// length of sequence
				libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,pattern.spattern.size());
				lvec.push_back(pattern.spattern.size());
				// number of N areas
				#if ! defined(NDEBUG)
				uint64_t nroff =
				#endif
					metaOut->tellp();
				libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,0);

				// map symbols
				if ( rci )
				{
					for ( uint64_t j = 0; j < pattern.spattern.size(); ++j )
						pattern.spattern[j] = ctable[ftable[static_cast<uint8_t>(pattern.spattern[j])]];
					std::reverse(pattern.spattern.begin(),pattern.spattern.end());
				}
				else
				{
					for ( uint64_t j = 0; j < pattern.spattern.size(); ++j )
						pattern.spattern[j] = ftable[static_cast<uint8_t>(pattern.spattern[j])];
				}

				std::vector < std::pair<uint64_t,uint64_t > > Vreplace;

				// replace blocks of N symbols by random bases
				uint64_t l = 0;
				// number of replaced blocks
				uint64_t nr = 0;
				while ( l < pattern.spattern.size() )
				{
					// skip regular bases
					while ( l < pattern.spattern.size() && pattern.spattern[l] < 4 )
						++l;
					assert ( l == pattern.spattern.size() || pattern.spattern[l] == 4 );

					// go to end of non regular bases block
					uint64_t h = l;
					while ( h < pattern.spattern.size() && pattern.spattern[h] == 4 )
						++h;

					// if non regular block is not empty
					if ( h-l )
					{
						Vreplace.push_back(std::pair<uint64_t,uint64_t>(l,h));

						// write bounds
						libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,l);
						libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,h);
						// add to interval counter
						nr += 1;
					}

					l = h;
				}

				nsum += nr;

				if ( rci == 0 )
				{
					// replace N blocks, back to front
					for ( uint64_t zz = 0; zz < Vreplace.size(); ++zz )
					{
						// index back to front
						uint64_t const z = Vreplace.size()-zz-1;
						// get interval
						std::pair<uint64_t,uint64_t > const P = Vreplace[z];
						uint64_t const l = P.first;
						uint64_t const h = P.second;

						// replace by random bases
						for ( uint64_t j = l; j < h; ++j )
							pattern.spattern[j] = (libmaus2::random::Random::rand8() & 3);
						// write out WC complement back to front
						for ( uint64_t j = 0; j < (h-l); ++j )
							rctmpOSI->put(ctable[pattern.spattern[h-j-1]]);
					}
				}
				else
				{
					// replace N blocks front to back
					for ( uint64_t z = 0; z < Vreplace.size(); ++z )
					{
						// get interval
						std::pair<uint64_t,uint64_t > const P = Vreplace[z];
						uint64_t const l = P.first;
						uint64_t const h = P.second;
						for ( uint64_t j = l; j < h; ++j )
						{
							int const c = rctmpISI->get();
							assert ( c != std::istream::traits_type::eof() );
							pattern.spattern[j] = c;
						}
					}
				}

				// make sure there are no more irregular bases
				for ( uint64_t j = 0; j < pattern.spattern.size(); ++j )
					assert ( pattern.spattern[j] < 4 );

				// go back to start of meta data
				//std::cerr << "[D] before " << metaOut->tellp() << std::endl;
				int64_t const seekoff =
					- (
						static_cast<int64_t>(
							(2*nr+1)
						)
						*
						static_cast<int64_t>(
							sizeof(uint64_t)
						)
				);
				//std::cerr << "[D] seekoff " << seekoff << std::endl;
				metaOut->seekp(seekoff, std::ios::cur );
				//std::cerr << "[D] after " << metaOut->tellp() << std::endl;
				#if ! defined(NDEBUG)
				assert ( static_cast<int64_t>(metaOut->tellp()) == static_cast<int64_t>(nroff) );
				#endif
				// write number of intervals replaced
				libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,nr);
				// skip interval bounds already written
				metaOut->seekp(   static_cast<int64_t>(2*nr  )*sizeof(uint64_t), std::ios::cur );

				// write bases
				compactout.write(pattern.spattern.c_str(),pattern.spattern.size());

				if ( (!rci) || (rci && replrc) )
				{
					// recover human readable bases
					for ( uint64_t j = 0; j < pattern.spattern.size(); ++j )
						pattern.spattern[j] = rtable[pattern.spattern[j]];

					// write fasta
					(*replOSI) << '>' << pattern.sid << '\n';
					(*replOSI).write(pattern.spattern.c_str(),pattern.spattern.size());
					replOSI->put('\n');
				}

				insize += pattern.spattern.size()+1;
				nseq += 1;

				if ( logstr )
					*logstr << "done, input size " << formatBytes(pattern.spattern.size()+1) << " acc " << formatBytes(insize) << std::endl;
			}
		}

		if ( rctmpOSI )
		{
			rctmpOSI->flush();
			rctmpOSI.reset();
		}
		if ( rctmpISI )
		{
			assert ( rctmpISI->get() == std::istream::traits_type::eof() );
		}
	}

	libmaus2::aio::FileRemoval::removeFile(rcfile);

	metaOut->seekp(0);
	libmaus2::util::NumberSerialisation::serialiseNumber(*metaOut,nseq);
	metaOut->flush();
	metaOut.reset();

	libmaus2::aio::InputStreamInstance::unique_ptr_type metaISI(new libmaus2::aio::InputStreamInstance(metaoutputfilename));
	// number of sequences
	#if ! defined(NDEBUG)
	uint64_t const rnseq =
	#endif
		libmaus2::util::NumberSerialisation::deserialiseNumber(*metaISI);
	#if ! defined(NDEBUG)
	assert ( nseq == rnseq );
	#endif
	for ( uint64_t i = 0; i < nseq; ++i )
	{
		// std::cerr << "[V] checking " << i << std::endl;

		assert ( static_cast<int64_t>(metaISI->tellg()) == static_cast<int64_t>(ovec[i]) );

		// length of sequence
		uint64_t const l = libmaus2::util::NumberSerialisation::deserialiseNumber(*metaISI);
		bool const l_ok = (l==lvec[i]);
		if ( ! l_ok )
		{
			std::cerr << "i=" << i << " l=" << l << " != lvec[i]=" << lvec[i] << std::endl;
		}
		assert ( l_ok );
		uint64_t const nr = libmaus2::util::NumberSerialisation::deserialiseNumber(*metaISI);
		// skip replaced intervals
		metaISI->ignore(2*nr*sizeof(uint64_t));
	}
	assert ( metaISI->peek() == std::istream::traits_type::eof() );

	if ( logstr )
		*logstr << "[V] Done, total input size " << insize << std::endl;

	compactout.flush();

	return EXIT_SUCCESS;
}
