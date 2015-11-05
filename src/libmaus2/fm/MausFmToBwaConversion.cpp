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
#include <libmaus2/fm/MausFmToBwaConversion.hpp>
#include <libmaus2/util/OutputFileNameTools.hpp>
#include <libmaus2/util/NumberMapSerialisation.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/gamma/GammaRLDecoder.hpp>
#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>
#include <set>

#define HUFRL

#if defined(HUFRL)
typedef ::libmaus2::huffman::RLDecoder rl_decoder;
#else
typedef ::libmaus2::gamma::GammaRLDecoder rl_decoder;
#endif

/*
 * load cumulative symbol frequences for 5 symbol alphabet (4 regular symbols + 1 terminator symbol)
 */
::libmaus2::autoarray::AutoArray<uint64_t> libmaus2::fm::MausFmToBwaConversion::loadL2T(std::string const & infn)
{
	std::string const inhist = ::libmaus2::util::OutputFileNameTools::clipOff(infn,".bwt") + ".hist";

	::libmaus2::aio::InputStreamInstance histCIS(inhist);
	std::map<uint64_t,uint64_t> chist = ::libmaus2::util::NumberMapSerialisation::deserialiseMap<std::istream,uint64_t,uint64_t>(histCIS);

	if ( ! chist.size() )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: symbol histogram is empty" << std::endl;
		se.finish();
		throw se;
	}
	if ( chist.rbegin()->first > 4 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: symbol histogram contains invalid symbols: " << chist.rbegin()->first << std::endl;
		se.finish();
		throw se;
	}
	::libmaus2::autoarray::AutoArray<uint64_t> L2(6);
	for ( std::map<uint64_t,uint64_t>::const_iterator ita = chist.begin();
		ita != chist.end(); ++ita )
		L2 [ ita->first ] = ita->second;
	L2.prefixSums();

	return L2;
}

/**
 * load cumulative symbol frequences for 5 symbol alphabet (1 terminator symbol + 4 regular symbols)
 * and remove terminator symbol
 **/
::libmaus2::autoarray::AutoArray<uint64_t> libmaus2::fm::MausFmToBwaConversion::loadL2(std::string const & infn)
{
	// load and shrink symbol histogram
	::libmaus2::autoarray::AutoArray<uint64_t> L2 = loadL2T(infn);
	if ( L2.size() != 6 || L2[0] != 0 || L2[1] != 1 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: symbol histogram is invalid." << std::endl;
		se.finish();
		throw se;
	}
	for ( uint64_t i = 0; i+1 < L2.size(); ++i )
		L2[i] = L2[i+1]-L2[i];
	L2[L2.size()-1] = 0;
	for ( uint64_t i = 1; i < L2.size(); ++i )
		L2[i-1] = L2[i];
	L2.prefixSums();
	::libmaus2::autoarray::AutoArray<uint64_t> L2N(L2.size()-1,false);
	std::copy(L2.begin(),L2.begin()+L2N.size(),L2N.begin());
	L2 = L2N;

	return L2;
}


/**
 * load ISA[0]
 **/
uint64_t libmaus2::fm::MausFmToBwaConversion::loadPrimary(std::string const & inisa)
{
	::libmaus2::aio::InputStreamInstance isaCIS(inisa);

	uint64_t isasamplingrate = 0;
	::libmaus2::serialize::Serialize<uint64_t>::deserialize(isaCIS,&isasamplingrate);
	uint64_t nisa = 0;
	::libmaus2::serialize::Serialize<uint64_t>::deserialize(isaCIS,&nisa);

	::libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(isaCIS,64);

	int64_t const iprimary = SGI.get();

	if ( iprimary < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: failed to read isa[0]" << std::endl;
		se.finish();
		throw se;
	}

	return iprimary;
}

void libmaus2::fm::MausFmToBwaConversion::rewriteBwt(std::string const & infn, std::ostream & out)
{
	// load ISA[0]
	std::string const inisa = ::libmaus2::util::OutputFileNameTools::clipOff(infn,".bwt") + ".isa";
	uint64_t const primary = loadPrimary(inisa);
	// load cumulative symbol freqs
	::libmaus2::autoarray::AutoArray<uint64_t> L2 = loadL2(infn);

	// write ISA[0] and cumulative symbol freqs
	::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO64(out,64);
	SGO64.put(primary);
	for ( uint64_t i = 1; i < L2.size(); ++i )
		SGO64.put(L2[i]);
	SGO64.flush();

	// write bwt without terminator symbol
	rl_decoder GD(std::vector<std::string>(1,infn));
	::libmaus2::aio::SynchronousGenericOutput<uint32_t> SGO32(out,16*1024);
	::libmaus2::aio::SynchronousGenericOutput<uint32_t>::iterator_type it32(SGO32);
	::libmaus2::bitio::FastWriteBitWriterBuffer32Sync FWBW(it32);

	std::pair<int64_t,uint64_t> run;
	while ( (run=GD.decodeRun()).first >= 0 )
		if ( run.first != 0 )
			for ( uint64_t i = 0; i < run.second; ++i )
				FWBW.write(run.first-1,2);

	FWBW.flush();
	SGO32.flush();
	out.flush();
}

void libmaus2::fm::MausFmToBwaConversion::rewriteSa(std::string const & infn, std::ostream & out)
{
	std::string const inisa = ::libmaus2::util::OutputFileNameTools::clipOff(infn,".bwt") + ".isa";
	uint64_t const primary = loadPrimary(inisa);

	::libmaus2::autoarray::AutoArray<uint64_t> L2 = loadL2(infn);

	uint64_t const n = rl_decoder::getLength(infn);

	std::cerr << "[D] n=" << n << std::endl;

	std::string const insa = ::libmaus2::util::OutputFileNameTools::clipOff(infn,".bwt") + ".sa";
	::libmaus2::aio::InputStreamInstance saCIS(insa);

	// read SA header
	uint64_t sasamplingrate = 0;
	::libmaus2::serialize::Serialize<uint64_t>::deserialize(saCIS,&sasamplingrate);
	if ( ! sasamplingrate )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: suffix array sampling rate is 0" << std::endl;
		se.finish();
		throw se;
	}

	uint64_t nsa = 0;
	::libmaus2::serialize::Serialize<uint64_t>::deserialize(saCIS,&nsa);
	if ( ! nsa )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: sampled suffix array is empty" << std::endl;
		se.finish();
		throw se;
	}

	// skip over SA[0]
	saCIS.ignore(sizeof(uint64_t));

	uint64_t const nsa_in = (n+sasamplingrate-1)/sasamplingrate;
	uint64_t const nsa_out = (n+sasamplingrate)/sasamplingrate;

	std::cerr << "[D] nsa_in" << nsa_in << " nsa_out=" << nsa_out << " sasamplingrate=" << sasamplingrate << std::endl;

	::libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO64(out,64);
	SGO64.put(primary);
	for ( uint64_t i = 1; i < L2.size(); ++i )
		SGO64.put(L2[i]);
	SGO64.put(sasamplingrate);
	SGO64.put(L2[4]);
	SGO64.flush();

	// copy rest of sampled suffix array
	::libmaus2::util::GetFileSize::copy(saCIS,out,(nsa_in-1)*sizeof(uint64_t));

	if ( nsa_out-nsa_in )
	{
		SGO64.put(0);
		SGO64.flush();
	}

	out.flush();
}

void libmaus2::fm::MausFmToBwaConversion::rewrite(
	std::string const & inbwt,
	std::string const & outbwt,
	std::string const & outsa
)
{
	std::set<std::string> S;
	S.insert(inbwt);
	S.insert(outbwt);
	S.insert(outsa);

	if ( S.size() != 3 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "error: conversion needs three different filenames, got "
			<< inbwt << ", " << outbwt << " and " << outsa << std::endl;
		se.finish();
		throw se;
	}

	::libmaus2::aio::OutputStreamInstance COSbwt(outbwt);
	rewriteBwt(inbwt,COSbwt);
	COSbwt.flush();

	::libmaus2::aio::OutputStreamInstance COSsa(outsa);
	rewriteSa(inbwt,COSsa);
	COSsa.flush();
}
