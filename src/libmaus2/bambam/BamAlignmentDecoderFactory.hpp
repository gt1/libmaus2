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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTDECODERFACTORY_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTDECODERFACTORY_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/bambam/CramRange.hpp>
#include <libmaus2/bambam/SamDecoderWrapper.hpp>

#if defined(LIBMAUS2_HAVE_IO_LIB)
#include <libmaus2/bambam/ScramDecoder.hpp>
#include <libmaus2/bambam/ScramInputContainer.hpp>
#endif

#include <libmaus2/bambam/BamAlignmentDecoderInfo.hpp>
#include <libmaus2/bambam/BamRangeDecoder.hpp>

#include <libmaus2/aio/InputStreamFactory.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamAlignmentDecoderFactory
		{
			typedef BamAlignmentDecoderFactory this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			BamAlignmentDecoderInfo const BADI;
			
			BamAlignmentDecoderFactory(BamAlignmentDecoderInfo const & rBADI) : BADI(rBADI) {}
			virtual ~BamAlignmentDecoderFactory() {}

			static std::set<std::string> getValidInputFormatsSet()
			{
				std::set<std::string> S;
				S.insert("bam");
				S.insert("sam");
				S.insert("maussam");

				#if defined(LIBMAUS2_HAVE_IO_LIB)
				S.insert("sbam");
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
			
			
			libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type operator()() const
			{
				libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(construct(BADI));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			static libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				BamAlignmentDecoderInfo const & BADI,
				std::istream & istdin = std::cin
			)
			{
				libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
					construct(istdin,BADI.inputfilename,BADI.inputformat,BADI.inputthreads,BADI.reference,BADI.putrank,BADI.copystr,BADI.range)
				);
				
				return UNIQUE_PTR_MOVE(tptr);
			}

			static libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				libmaus2::bambam::BamAlignmentDecoderInfo const & BADI, bool const putrank, std::istream & istdin
			)
			{
				libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
					libmaus2::bambam::BamAlignmentDecoderFactory::construct(
						istdin,
						BADI.inputfilename,
						BADI.inputformat,
						BADI.inputthreads,
						BADI.reference,
						putrank,
						BADI.copystr,
						BADI.range)
				);

				return UNIQUE_PTR_MOVE(tptr);
			}

			static libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				std::istream & istdin = std::cin,
				std::string const & inputfilename = BamAlignmentDecoderInfo::getDefaultInputFileName(),
				std::string const & inputformat = BamAlignmentDecoderInfo::getDefaultInputFormat(),
				uint64_t const inputthreads = BamAlignmentDecoderInfo::getDefaultThreads(),
				std::string const & 
					#if defined(LIBMAUS2_HAVE_IO_LIB)
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
								libmaus2::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for input via stdin" << std::endl;
								ex.finish();
								throw ex;								
							}
							else if ( copystr )
							{
								libmaus2::aio::InputStream::unique_ptr_type iptr(
									new libmaus2::aio::InputStream(istdin)
								);								
								libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamDecoderWrapper(iptr,*copystr,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
							else
							{
								libmaus2::aio::InputStream::unique_ptr_type iptr(
									new libmaus2::aio::InputStream(istdin)
								);
								libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamDecoderWrapper(iptr,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
						}
						else
						{
							if ( copystr )
							{
								libmaus2::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for file based input" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								if ( range.size() )
								{
									libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
										new libmaus2::bambam::BamRangeDecoderWrapper(inputfilename,range,putrank)
									);
									return UNIQUE_PTR_MOVE(tptr);
								}
								else
								{
									libmaus2::aio::InputStream::unique_ptr_type iptr(
										libmaus2::aio::InputStreamFactoryContainer::constructUnique(inputfilename)
									);
									libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
										new BamDecoderWrapper(iptr,putrank)
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
							libmaus2::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for parallel input" << std::endl;
							ex.finish();
							throw ex;								
						}
						else if ( inputisstdin )
						{
							if ( copystr )
							{
								libmaus2::aio::InputStream::unique_ptr_type iptr(new libmaus2::aio::InputStream(istdin));
								libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(iptr,*copystr,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
							else
							{
								libmaus2::aio::InputStream::unique_ptr_type iptr(new libmaus2::aio::InputStream(istdin));
								libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(iptr,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
						}
						else
						{
							if ( copystr )
							{
								libmaus2::exception::LibMausException ex;
								ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for file based input" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								libmaus2::aio::InputStream::unique_ptr_type iptr(
									libmaus2::aio::InputStreamFactoryContainer::constructUnique(inputfilename)
								);
								libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(iptr,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);					
							}
						}
					}
				}
				else if ( 
					inputformat == "maussam" 
					#if ! defined(LIBMAUS2_HAVE_IO_LIB)
					||
					inputformat == "sam"
					#endif
				)
				{
					if ( copystr )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for SAM based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for the sam input format" << std::endl;
						ex.finish();
						throw ex;		
					}
					
					if ( inputisstdin )
					{
						libmaus2::aio::InputStream::unique_ptr_type iptr(new libmaus2::aio::InputStream(istdin));
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::SamDecoderWrapper(iptr,putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						libmaus2::aio::InputStream::unique_ptr_type iptr(
							libmaus2::aio::InputStreamFactoryContainer::constructUnique(inputfilename)
						);
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::SamDecoderWrapper(iptr,putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);					
					}
				}
				#if defined(LIBMAUS2_HAVE_IO_LIB)
				else if ( inputformat == "sam" )
				{
					if ( copystr )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for io_lib based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for the sam input format" << std::endl;
						ex.finish();
						throw ex;		
					}
					
					if ( inputisstdin )
					{
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::ScramDecoderWrapper("-","rs","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::ScramDecoderWrapper(inputfilename,"rs","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);					
					}
				}
				else if ( inputformat == "sbam" )
				{
					if ( copystr )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for io_lib based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for the sbam input format" << std::endl;
						ex.finish();
						throw ex;		
					}
					
					if ( inputisstdin )
					{
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::ScramDecoderWrapper("-","rb","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::ScramDecoderWrapper(inputfilename,"rb","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);					
					}
				}
				else if ( inputformat == "cram" )
				{
					CramRange cramrange;
										
					if ( copystr )
					{
						libmaus2::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for io_lib based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						if ( inputisstdin )
						{
							libmaus2::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for input via stdin" << std::endl;
							ex.finish();
							throw ex;		
						}

						cramrange = CramRange(range);
					}

					if ( inputisstdin )
					{
						#if defined(LIBMAUS2_HAVE_IO_LIB_INPUT_CALLBACKS)
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::ScramDecoderWrapper(
								std::string("-"),
								libmaus2::bambam::ScramInputContainer::allocate,
								libmaus2::bambam::ScramInputContainer::deallocate,
								4096,
								reference,
								putrank
							)
						);
						#else
						libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus2::bambam::ScramDecoderWrapper("-","rc",reference,putrank)
						);
						#endif
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						if ( cramrange.rangeref.size() )
						{
							#if defined(LIBMAUS2_HAVE_IO_LIB_INPUT_CALLBACKS)
							libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus2::bambam::ScramDecoderWrapper(
									inputfilename,
									libmaus2::bambam::ScramInputContainer::allocate,
									libmaus2::bambam::ScramInputContainer::deallocate,
									4096,
									reference,
									cramrange.rangeref,cramrange.rangestart,cramrange.rangeend,
									putrank
								)
							);
							#else
							libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus2::bambam::ScramDecoderWrapper(inputfilename,"rc",reference,
									cramrange.rangeref,cramrange.rangestart,cramrange.rangeend,putrank)
							);
							#endif
							return UNIQUE_PTR_MOVE(tptr);											
						}
						else
						{
							#if defined(LIBMAUS2_HAVE_IO_LIB_INPUT_CALLBACKS)
							libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus2::bambam::ScramDecoderWrapper(
									inputfilename,
									libmaus2::bambam::ScramInputContainer::allocate,
									libmaus2::bambam::ScramInputContainer::deallocate,
									4096,
									reference,
									putrank
								)
							);
							#else			
							libmaus2::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus2::bambam::ScramDecoderWrapper(inputfilename,"rc",reference,putrank)
							);
							#endif
							return UNIQUE_PTR_MOVE(tptr);					
						}
					}
				}
				#endif
				else
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "Invalid input format " << inputformat << std::endl;
					ex.finish();
					throw ex;
				}
			}
		};
	}
}
#endif
