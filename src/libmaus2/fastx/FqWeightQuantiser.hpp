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
#if ! defined(LIBMAUS_FASTX_FQWEIGHTQUANTISER_HPP)
#define LIBMAUS_FASTX_FQWEIGHTQUANTISER_HPP

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS_HAVE_KMLOCAL)
#include <libmaus2/quantisation/ClusterComputation.hpp>
#include <libmaus2/fastx/FastQReader.hpp>
#include <libmaus2/fastx/Phred.hpp>
#include <libmaus2/util/ArgInfo.hpp>
#include <libmaus2/lz/BufferedGzipStream.hpp>
#include <libmaus2/fastx/StreamFastQReader.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FqWeightQuantiser;
		std::ostream & operator<<(std::ostream & out, FqWeightQuantiser const & fqwq);

		struct FqWeightQuantiser
		{
			::libmaus2::util::ArgInfo const nullarginfo;
			::libmaus2::quantisation::Quantiser::unique_ptr_type const uquant;
			::libmaus2::quantisation::Quantiser const & quant;
			double psnr;
			::libmaus2::autoarray::AutoArray<uint8_t> const rephredTable;
			::libmaus2::autoarray::AutoArray<uint8_t> const phredForQuant;
			::libmaus2::autoarray::AutoArray<uint8_t> const quantForPhred;
			
			uint64_t byteSize() const
			{
				return
					sizeof(FqWeightQuantiser)+
					quant.byteSize()+
					rephredTable.byteSize()+
					phredForQuant.byteSize()+
					quantForPhred.byteSize();
			}
			
			template<typename iterator>
			void rephred(iterator ita, iterator const ite, int const offset = 0) const
			{
				for ( ; ita != ite; ++ita )
					*ita = rephredTable[*ita] + offset;
			}
			
			std::string rephred(std::string const & s, int const offset = 0) const
			{
				std::string r = s;
				rephred(r.begin(),r.end(),offset);
				return r;
			}
			
			static int getQualityOffset(
				::libmaus2::util::ArgInfo const & arginfo,
				std::vector<std::string> const & filenames,
				std::ostream * logstr = 0
			)
			{
				// input is compressed
				bool const gzinput = arginfo.getValue<unsigned int>("bgzf",0);

				// fastq quality offset
				int fqoffset = arginfo.getValue<int>("fqoffset",0);
				
				if ( gzinput )
				{
					for ( uint64_t f = 0; (!fqoffset) && f < filenames.size(); ++f )
					{
						libmaus2::aio::CheckedInputStream CIS(filenames[f]);
						libmaus2::lz::BufferedGzipStream BGS(CIS);
						libmaus2::fastx::StreamFastQReaderWrapper fqin(BGS);
						fqoffset = fqin.getOffset();
					}
					
					if ( logstr )
					{
						if ( fqoffset )
							(*logstr) << "Got quality offset " << fqoffset << " from compressed input." << std::endl;
						else
							(*logstr) << "Failed to get quality offset from compressed input." << std::endl;
					}
				}
				else
				{
					fqoffset = fqoffset ? fqoffset : ::libmaus2::fastx::FastQReader::getOffset(filenames);
						
					if ( logstr )
					{
						if ( fqoffset )
							(*logstr) << "Got quality offset " << fqoffset << " from uncompressed input." << std::endl;
						else
							(*logstr) << "Failed to get quality offset from uncompressed input." << std::endl;
					}
				}

				if ( ! fqoffset )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Unable to guess fastq quality format offset." << std::endl;
					se.finish();
					throw se;
				}
			
				return fqoffset;
			}

			static std::map<uint64_t,uint64_t> getFastQWeightHistogram(
				::libmaus2::util::ArgInfo const & arginfo,
				std::vector<std::string> const & filenames,
				std::ostream * logstr = 0
			)
			{
				// input is compressed
				bool const gzinput = arginfo.getValue<unsigned int>("bgzf",0);
				// maximum number of reads considered
				uint64_t const fqmax = arginfo.getValue<uint64_t>("fqmax",std::numeric_limits<uint64_t>::max());
				// get quality offset				
				int const fqoffset = getQualityOffset(arginfo,filenames,logstr);

				if ( logstr )
					(*logstr) << "Computing weight histogram from " 
						<< ((fqmax == std::numeric_limits<uint64_t>::max()) ? std::string("all") :
							::libmaus2::util::NumberSerialisation::formatNumber(fqmax,0))
						<< " reads, offset=" << fqoffset << "...";
				
				::libmaus2::autoarray::AutoArray<unsigned int> qtab(256);
				for ( uint64_t i = 0; i < 40; ++i )
					qtab[i] = i;
				for ( uint64_t i = 40; i < qtab.size(); ++i )
					qtab[i] = 40;
				
				::libmaus2::util::Histogram hist;

				if ( gzinput )
				{
					uint64_t decoded = 0;
					
					for ( uint64_t f = 0; decoded < fqmax && f < filenames.size(); ++f )
					{
						libmaus2::aio::CheckedInputStream CIS(filenames[f]);
						libmaus2::lz::BufferedGzipStream BGS(CIS);
						libmaus2::fastx::StreamFastQReaderWrapper fqin(BGS,fqoffset);
						::libmaus2::fastx::StreamFastQReaderWrapper::pattern_type pattern;

						while ( fqin.getNextPatternUnlocked(pattern) && decoded++ < fqmax )
						{
							uint8_t const * q = reinterpret_cast<uint8_t const *>(pattern.quality.c_str());
							for ( uint64_t i = 0; i < pattern.getPatternLength(); ++i )
								hist ( qtab[q[i]] );
							if ( logstr && (decoded & (1024*1024-1)) == 0 )
								(*logstr) << "(" << (decoded/(1024*1024)) << ")";
						}
					}
					
					if ( logstr )
						(*logstr) << "Decoded " << decoded << " reads from compressed files." << std::endl;
				}
				else
				{
					::libmaus2::fastx::FastQReader reader(filenames,fqoffset);
					::libmaus2::fastx::FastQReader::pattern_type pattern;
					while ( reader.getNextPatternUnlocked(pattern) && pattern.patid < fqmax )
					{
						uint8_t const * q = reinterpret_cast<uint8_t const *>(pattern.quality.c_str());
						for ( uint64_t i = 0; i < pattern.getPatternLength(); ++i )
							hist ( qtab[q[i]] );
						if ( logstr && (pattern.patid & (1024*1024-1)) == 0 )
							(*logstr) << "(" << (pattern.patid/(1024*1024)) << ")";
					}
				}
				
				std::map<uint64_t,uint64_t> const M = hist.get();
				
				if ( logstr )
					*logstr << "done." << std::endl;

				return M;
			}

			static std::map < uint64_t, uint64_t > histogramToWeights(
				std::map < uint64_t, uint64_t > const weighthistogram,
				uint64_t const samples
				)
			{
				uint64_t total = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = weighthistogram.begin(); ita != weighthistogram.end(); ++ita )
					total += ita->second;
				std::map < uint64_t, uint64_t > Qfreq;
				uint64_t totalfreq = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = weighthistogram.begin(); ita != weighthistogram.end(); ++ita )
				{
					double const frac = ita->second / static_cast<double>(total);
					uint64_t const freq = 
						ita->second ?
							std::max(
								static_cast<uint64_t>(std::floor(frac * samples + 0.5)),
								static_cast<uint64_t>(1ull)
							)
							: 0ull;
					totalfreq += freq;
					Qfreq[ita->first] = freq;
					#if 0
					std::cerr << "q=" << ita->first << " frac=" << frac << " freq=" << freq << std::endl;
					#endif
				}

				return Qfreq;
			}

			template<typename container_type>
			static uint64_t sumSecond(container_type const & C)
			{
				typedef typename container_type::value_type::second_type value_type;
				value_type s = value_type();
				
				for ( typename container_type::const_iterator ita = C.begin(); ita != C.end(); ++ita )
					s += ita->second;

				return s;	
			}

			static void constructSampleVector(std::map < uint64_t, uint64_t > const & Qfreq, std::vector<double> & VQ)
			{
				VQ = std::vector<double>(sumSecond(Qfreq));
				std::vector<double>::iterator p = VQ.begin();
				for ( std::map < uint64_t, uint64_t >::const_iterator ita = Qfreq.begin(); ita != Qfreq.end(); ++ita )
				{
					double const err = ::libmaus2::fastx::Phred::phred_error[ita->first];
					for ( uint64_t i = 0; i < ita->second; ++i )
						*(p++) = err;
				}
			}

			double computePsnr(
				std::map<uint64_t,uint64_t> const & weighthistogram
			) const
			{
				return computePsnr(quant,weighthistogram);
			}

			static double computePsnr(
				::libmaus2::quantisation::Quantiser const & quant,
				std::map<uint64_t,uint64_t> const & weighthistogram
			)
			{
				uint64_t const total = sumSecond(weighthistogram);
				
				// compute mean squared error
				double mse = 0;
				double maxphred = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = weighthistogram.begin(); ita != weighthistogram.end(); ++ita )
				{
					uint64_t const v = ita->first;
					uint64_t const c = ita->second;

					maxphred = std::max(maxphred,::libmaus2::fastx::Phred::phred_error[v]);				
					mse += c * squaredError(quant,v);
				}
				mse /= total;
				
				// compute peak signal to noise ratio
				double const psnr = 10.0 * ::std::log( (maxphred * maxphred)/ mse ) / ::std::log(10.0);

				return psnr;			
			}
			
			::libmaus2::quantisation::Quantiser::unique_ptr_type constructQuantiser(
				::libmaus2::util::ArgInfo const & arginfo,
				uint64_t const numsteps,
				std::vector<double> const & VQ
				)
			{
				::libmaus2::quantisation::Quantiser::unique_ptr_type uquant = 
					UNIQUE_PTR_MOVE(
						::libmaus2::quantisation::ClusterComputation::constructQuantiser(
							VQ,numsteps,arginfo.getValue<uint64_t>("fqquantruns",5000)
						)
					);
				return UNIQUE_PTR_MOVE(uquant);	
			}

			::libmaus2::quantisation::Quantiser::unique_ptr_type constructQuantiser(
				::libmaus2::util::ArgInfo const & arginfo,
				uint64_t const numsteps,
				std::map < uint64_t, uint64_t > const & Qfreq
				)
			{
				std::vector<double> VQ; constructSampleVector(Qfreq,VQ);
				return UNIQUE_PTR_MOVE(constructQuantiser(arginfo,numsteps,VQ));
			}

			::libmaus2::quantisation::Quantiser::unique_ptr_type constructQuantiser(
				::libmaus2::util::ArgInfo const & arginfo,
				uint64_t const numsteps,
				std::vector<std::string> const & filenames,
				std::ostream * logstr = 0
				)
			{
				std::map<uint64_t,uint64_t> weighthistogram = getFastQWeightHistogram(arginfo,filenames,logstr);
				
				// std::cerr << "weighthistogram.size()=" << weighthistogram.size() << " numsteps=" << numsteps << std::endl;

				if ( numsteps >= weighthistogram.size() )
				{
					std::vector<double> VQ;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = weighthistogram.begin(); ita != weighthistogram.end(); ++ita )
					{
						uint64_t const weight = ita->first;
						double const phred = ::libmaus2::fastx::Phred::phred_error[weight];
						VQ.push_back(phred);
					}
					
					//std::cerr << "Constructing direct quantiser." << std::endl;

					if ( logstr )
					{
						*logstr << "Constructing quantiser for " << VQ.size() << " steps...";
					}
					::libmaus2::quantisation::Quantiser::unique_ptr_type ptr = 
						UNIQUE_PTR_MOVE(constructQuantiser(arginfo,VQ.size(),VQ));
						
					psnr = computePsnr(*ptr,weighthistogram);
					
					if ( logstr )
					{
						*logstr << "done, PSNR is " << psnr << std::endl;
						*logstr << "Quantiser levels:\n" << *ptr;
					}

					return UNIQUE_PTR_MOVE(ptr);
				}
				else
				{
					std::map < uint64_t, uint64_t > const Qfreq = histogramToWeights(weighthistogram,
						arginfo.getValue<uint64_t>("fqweightsamples",64ull*1024ull));
				
					if ( logstr )
					{
						*logstr << "Constructing quantiser for " << numsteps << " steps...";
					}
					::libmaus2::quantisation::Quantiser::unique_ptr_type ptr = 
						UNIQUE_PTR_MOVE(constructQuantiser(arginfo,numsteps,Qfreq));
						
					psnr = computePsnr(*ptr,weighthistogram);
					
					if ( logstr )
					{
						*logstr << "done, PSNR is " << psnr << std::endl;
						*logstr << "Quantiser levels:\n" << *ptr;
					}
					return UNIQUE_PTR_MOVE(ptr);
				}					
			}
			
			FqWeightQuantiser() 
			  : nullarginfo(std::vector<std::string>(1,"<prog>")),
			    uquant(UNIQUE_PTR_MOVE(new ::libmaus2::quantisation::Quantiser(std::vector<double>(0)))),
			    quant(*uquant),
			    rephredTable(0),
			    phredForQuant(0),
			    quantForPhred(0)
			{
			}
			FqWeightQuantiser(
				std::vector<std::string> const & filenames,
				uint64_t const numsteps,
				::libmaus2::util::ArgInfo const * rarginfo = 0,
				std::ostream * logstr = 0
			) : nullarginfo(std::vector<std::string>(1,"<prog>")),
			    uquant(UNIQUE_PTR_MOVE(constructQuantiser(rarginfo ? *rarginfo : nullarginfo, numsteps, filenames, logstr))),
			    quant(*uquant),
			    rephredTable(getPhredMappingTable()),
			    phredForQuant(getPhredForQuantTable()),
			    quantForPhred(getQuantForPhredTable())
			{

				if ( logstr )
					printClosestPhred(*logstr);
			}
			FqWeightQuantiser(
				std::vector<double> const & VQ,
				uint64_t const numsteps,
				::libmaus2::util::ArgInfo const * rarginfo = 0
			) : nullarginfo(std::vector<std::string>(1,"<prog>")),
			    uquant(UNIQUE_PTR_MOVE(constructQuantiser(rarginfo ? *rarginfo : nullarginfo, numsteps, VQ))),
			    quant(*uquant),
			    psnr(std::numeric_limits<double>::max()),
			    rephredTable(getPhredMappingTable()),
			    phredForQuant(getPhredForQuantTable()),
			    quantForPhred(getQuantForPhredTable())
			{
				#if 0
				std::ostringstream ostr;
				quant.serialise(ostr);
				std::istringstream istr(ostr.str());
				
				::libmaus2::quantisation::Quantiser requant(istr);
				
				std::cerr << "----AAA----\n";
				std::cerr << quant;
				std::cerr << "----BBB----\n";
				std::cerr << requant;
				#endif
			}
			
			template<typename stream_type>
			FqWeightQuantiser(stream_type & in)
			: nullarginfo(std::vector<std::string>(1,"<prog>")),
			  uquant(new ::libmaus2::quantisation::Quantiser(in)),
			  quant(*uquant),
			  psnr(::libmaus2::util::NumberSerialisation::deserialiseDouble(in)),
			  rephredTable(quant.size() ? getPhredMappingTable() : ::libmaus2::autoarray::AutoArray<uint8_t>() ),
			  phredForQuant(quant.size() ? getPhredForQuantTable() : ::libmaus2::autoarray::AutoArray<uint8_t>() ),
			  quantForPhred(quant.size() ? getQuantForPhredTable() : ::libmaus2::autoarray::AutoArray<uint8_t>() )
			{
			
			}
			
			void serialise(std::ostream & out) const
			{
				quant.serialise(out);
				::libmaus2::util::NumberSerialisation::serialiseDouble(out,psnr);
			}
			
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
			
			// quantise probability
			uint64_t operator()(double const v) const
			{
				return quant(v);
			}
			
			// quantise phred
			uint64_t operator()(uint64_t const v) const
			{
				return quant(::libmaus2::fastx::Phred::phred_error[v]);
			}
			
			// decode
			double operator[](uint64_t const v) const
			{
				// std::cerr << "Here, v=" << v << " quant[v]=" << quant[v] << std::endl;
				return quant[v];
			}
			
			static double squaredError(::libmaus2::quantisation::Quantiser const & quantiser, uint64_t const v)
			{
				double const phred = ::libmaus2::fastx::Phred::phred_error[v];
				uint64_t const quant = quantiser(phred);
				double const decoded = quantiser[quant];
				double const diff = (decoded-phred);
				return diff*diff;
			}
			
			double squaredError(uint64_t const v) const
			{
				return squaredError(quant,v);
			}
			
			static uint64_t getClosestPhredForProb(double const prob)
			{
				double minerr = std::numeric_limits<double>::max();
				uint64_t minv = 0;
				
				for ( 
					uint64_t i = 0; 
					i < sizeof(::libmaus2::fastx::Phred::phred_error)
					   /sizeof(::libmaus2::fastx::Phred::phred_error[0]); 
					++i 
				)
				{
					double const lerr = std::abs(prob-::libmaus2::fastx::Phred::phred_error[i]);
					if ( lerr < minerr )
					{
						minerr = lerr;
						minv = i;
					}
				}
				
				return minv;
			}
			
			uint64_t getClosestPhredForQuant(uint64_t const q) const
			{
				return getClosestPhredForProb(quant[q]);
			}
			
			::libmaus2::autoarray::AutoArray<uint8_t> getPhredForQuantTable() const
			{
				::libmaus2::autoarray::AutoArray<uint8_t> T(quant.size(),false);
				for ( uint64_t i = 0; i < quant.size(); ++i )
					T[i] = getClosestPhredForQuant(i);
				return T;
			}
			
			::libmaus2::autoarray::AutoArray<uint8_t> getQuantForPhredTable() const
			{
				::libmaus2::autoarray::AutoArray<uint8_t> T(256,false);
				for ( uint64_t i = 0; i < T.size(); ++i )
					T[i] = (*this)(i);
				return T;
			}

			::libmaus2::autoarray::AutoArray<uint8_t> getPhredMappingTable() const
			{
				::libmaus2::autoarray::AutoArray<uint8_t> T(256,false);
				
				for ( uint64_t i = 0; i < T.size(); ++i )
				{
					uint64_t const q = (*this)(i);
					uint64_t const r = getClosestPhredForQuant(q);
					// std::cerr << "pthred " << i << " -> " << r << std::endl;
					T [ i ] = r;
				}
				
				return T;
			}
						
			void printClosestPhred(std::ostream & logstr) const
			{
				for ( uint64_t q = 0; q < quant.size(); ++q )
				{
					logstr
						<< "q=" << q << " quant[q]=" << quant[q]
						<< " closest phred=" << getClosestPhredForQuant(q)
						<< " phred val=" << ::libmaus2::fastx::Phred::phred_error[getClosestPhredForQuant(q)] << std::endl;
				}
			}
			
			static void rephredFastq(
				std::vector<std::string> const & filenames,
				::libmaus2::util::ArgInfo const & arginfo,
				std::ostream * logstr = 0
			)
			{
				rephredFastq(filenames,arginfo,std::cout,logstr);
			}

			static void rephredFastq(
				std::vector<std::string> const & filenames,
				::libmaus2::util::ArgInfo const & arginfo,
				std::ostream & out,
				std::ostream * logstr = 0
			)
			{
				// input is compressed
				bool const gzinput = arginfo.getValue<unsigned int>("bgzf",0);
				uint64_t const numsteps = arginfo.getValue<uint64_t>("fqquantsteps",8);
				FqWeightQuantiser fqwq(filenames,numsteps,&arginfo,logstr);

				// get quality offset				
				int const fqoffset = getQualityOffset(arginfo,filenames,logstr);
				
				if ( gzinput )
				{
					for ( uint64_t f = 0; f < filenames.size(); ++f )
					{
						libmaus2::aio::CheckedInputStream CIS(filenames[f]);
						libmaus2::lz::BufferedGzipStream BGS(CIS);
						libmaus2::fastx::StreamFastQReaderWrapper fqin(BGS,fqoffset);
						::libmaus2::fastx::StreamFastQReader::pattern_type pattern;

						while ( fqin.getNextPatternUnlocked(pattern) )
						{
							pattern.quality = fqwq.rephred(pattern.quality,fqoffset);
							out << pattern;
						}
					}
				}
				else
				{				
					::libmaus2::fastx::FastQReader reader(filenames,fqoffset);
					::libmaus2::fastx::FastQReader::pattern_type pattern;
				
					while ( reader.getNextPatternUnlocked(pattern) )
					{
						pattern.quality = fqwq.rephred(pattern.quality,fqoffset);
						out << pattern;
					}
				}
				
				out.flush();
			}
			
			
			static void statsRun(
				::libmaus2::util::ArgInfo const & arginfo, std::vector<std::string> const & filenames,
				std::ostream & logstr
			)
			{
				std::map<uint64_t,uint64_t> weighthistogram = getFastQWeightHistogram(arginfo,filenames,(&logstr));

				std::map < uint64_t, uint64_t > const Qfreq = histogramToWeights(weighthistogram,
					arginfo.getValue<uint64_t>("fqweightsamples",64ull*1024ull));

				std::vector<double> VQ; constructSampleVector(Qfreq,VQ);

				for ( unsigned int numsteps = 1 ; numsteps <= 32; ++numsteps )
				{
					logstr << "---------------------------------\n\n";
			
					FqWeightQuantiser fqwq(VQ,numsteps);
					logstr << fqwq;

					uint64_t const total = sumSecond(weighthistogram);
					
					double const psnr = fqwq.computePsnr(weighthistogram);
					
					double err = 0;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = weighthistogram.begin(); ita != weighthistogram.end(); ++ita )
					{
						uint64_t const v = ita->first;
						uint64_t const c = ita->second;
						
						double const phred = ::libmaus2::fastx::Phred::phred_error[v]; // computed phred
						double const dec = fqwq.quant[fqwq.quant(phred)]; // quantise and decode
						double const lerr = std::sqrt(((dec-phred)*(dec-phred)));
						err += lerr * c;
						
						logstr 
							<< "q[" << v << "] phred=" << phred << " "
							<< "dec=" << dec << " "
							<< "error per sample " 
							<< lerr << std::endl;
					}

					logstr << "numsteps " << numsteps << " averge error " << err/total << " psnr " << psnr << std::endl;

					fqwq.printClosestPhred(logstr);
					::libmaus2::autoarray::AutoArray<uint8_t> T = fqwq.getPhredMappingTable();
					
					logstr << "\n";					
				}
			
			}
		};

		inline std::ostream & operator<<(std::ostream & out, FqWeightQuantiser const & fqwq)
		{
			out << "FqWeightQuantiser(" << fqwq.quant << ")";
			return out;
		}
	}
}
#endif
#endif
