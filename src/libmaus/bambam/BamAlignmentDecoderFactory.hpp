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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTDECODERFACTORY_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTDECODERFACTORY_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/bambam/BamDecoder.hpp>

#if defined(LIBMAUS_HAVE_IO_LIB)
#include <libmaus/bambam/ScramDecoder.hpp>
#endif

#include <libmaus/bambam/BamAlignmentDecoderInfo.hpp>
#include <libmaus/bambam/BamRangeDecoder.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentDecoderFactory
		{
			typedef BamAlignmentDecoderFactory this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			BamAlignmentDecoderInfo const BADI;
			
			BamAlignmentDecoderFactory(BamAlignmentDecoderInfo const & rBADI) : BADI(rBADI) {}
			virtual ~BamAlignmentDecoderFactory() {}

			static std::set<std::string> getValidInputFormatsSet()
			{
				std::set<std::string> S;
				S.insert("bam");

				#if defined(LIBMAUS_HAVE_IO_LIB)
				S.insert("sam");
				S.insert("cram");
				#endif
				
				return S;
			}
			
			static std::string getValidInputFormats()
			{
				std::set<std::string> const S = getValidInputFormatsSet();

				std::ostringstream ostr;
				for ( std::set<std::string>::const_iterator ita = S.begin();
					ita != S.end(); ++ita )
					ostr << ((ita!=S.begin())?",":"") << (*ita);
				
				return ostr.str();
			}
			
			
			libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type operator()() const
			{
				libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(construct(BADI));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			static libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				BamAlignmentDecoderInfo const & BADI
			)
			{
				libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
					construct(BADI.inputfilename,BADI.inputformat,BADI.inputthreads,BADI.reference,BADI.putrank,BADI.copystr,BADI.range)
				);
				
				return UNIQUE_PTR_MOVE(tptr);
			}

			static libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				std::string const & inputfilename = BamAlignmentDecoderInfo::getDefaultInputFileName(),
				std::string const & inputformat = BamAlignmentDecoderInfo::getDefaultInputFormat(),
				uint64_t const inputthreads = BamAlignmentDecoderInfo::getDefaultThreads(),
				std::string const & 
					#if defined(LIBMAUS_HAVE_IO_LIB)
					reference
					#endif
					= BamAlignmentDecoderInfo::getDefaultReference(),
				bool const putrank = BamAlignmentDecoderInfo::getDefaultPutRank(),
				std::ostream * copystr = BamAlignmentDecoderInfo::getDefaultCopyStr(),
				std::string const & range = BamAlignmentDecoderInfo::getDefaultRange()
			)
			{
				bool const inputisstdin = (!inputfilename.size()) || (inputfilename == "-");
				
				if ( inputformat == "bam" )
				{
					if ( inputthreads <= 1 )
					{
						if ( inputisstdin )
						{
							if ( range.size() )
							{
								libmaus::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for input via stdin" << std::endl;
								ex.finish();
								throw ex;								
							}
							else if ( copystr )
							{							
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamDecoderWrapper(std::cin,*copystr,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
							else
							{
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamDecoderWrapper(std::cin,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
						}
						else
						{
							if ( copystr )
							{
								libmaus::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for file based input" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								if ( range.size() )
								{
									libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
										new libmaus::bambam::BamRangeDecoderWrapper(inputfilename,range,putrank)
									);
									return UNIQUE_PTR_MOVE(tptr);
								}
								else
								{
									libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
										new BamDecoderWrapper(inputfilename,putrank)
									);
									return UNIQUE_PTR_MOVE(tptr);
								}
							}
						}
					}
					else
					{					
						if ( range.size() )
						{
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for parallel input" << std::endl;
							ex.finish();
							throw ex;								
						}
						else if ( inputisstdin )
						{
							if ( copystr )
							{							
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(std::cin,*copystr,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
							else
							{
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(std::cin,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
						}
						else
						{
							if ( copystr )
							{
								libmaus::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for file based input" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(inputfilename,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);					
							}
						}
					}
				}
				#if defined(LIBMAUS_HAVE_IO_LIB)
				else if ( inputformat == "sam" )
				{
					if ( copystr )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for io_lib based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are only supported for BAM input format" << std::endl;
						ex.finish();
						throw ex;		
					}
					
					if ( inputisstdin )
					{
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper("-","rs","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper(inputfilename,"rs","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);					
					}
				}
				else if ( inputformat == "cram" )
				{
					std::string rangeref;
					int64_t rangestart = -1;
					int64_t rangeend = -1;
					
					if ( copystr )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for io_lib based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						if ( inputisstdin )
						{
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for input via stdin" << std::endl;
							ex.finish();
							throw ex;		
						}

						std::size_t const colpos = range.find(":");
						if ( colpos == std::string::npos )
						{
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): cannot parse CRAM input range " << range << " (does not contain :)" << std::endl;
							ex.finish();
							throw ex;
						}

						rangeref = range.substr(0,colpos);

						std::string rangerest = range.substr(colpos+1);
						std::size_t const minpos = rangerest.find("-");

						if ( minpos == std::string::npos )
						{
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): cannot parse CRAM input range " << range << " (range does not contain -)" << std::endl;
							ex.finish();
							throw ex;
						}
						
						std::string const sstart = rangerest.substr(0,minpos);
						std::string const send = rangerest.substr(minpos+1);
						
						rangestart = 0;
						for ( uint64_t i = 0; i < sstart.size(); ++i )
							if ( ! isdigit(static_cast<uint8_t>(sstart[i])) )
							{
								libmaus::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): cannot parse CRAM input range " << range << " (start point is not a number)" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								rangestart *= 10;
								rangestart += (sstart[i]-'0');
							}
						rangeend = 0;
						for ( uint64_t i = 0; i < send.size(); ++i )
							if ( ! isdigit(static_cast<uint8_t>(send[i])) )
							{
								libmaus::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): cannot parse CRAM input range " << range << " (end point is not a number)" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								rangeend *= 10;
								rangeend += (send[i]-'0');
							}
					}

					if ( inputisstdin )
					{
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper("-","rc",reference,putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						if ( rangeref.size() )
						{
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus::bambam::ScramDecoderWrapper(inputfilename,"rc",reference,rangeref,rangestart,rangeend,putrank)
							);
							return UNIQUE_PTR_MOVE(tptr);											
						}
						else
						{
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus::bambam::ScramDecoderWrapper(inputfilename,"rc",reference,putrank)
							);
							return UNIQUE_PTR_MOVE(tptr);					
						}
					}
				}
				#endif
				else
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "Invalid input input format " << inputformat << std::endl;
					ex.finish();
					throw ex;
				}
			}
		};
	}
}
#endif
