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
#include <libmaus/bambam/CramRange.hpp>
#include <libmaus/bambam/SamDecoderWrapper.hpp>

#if defined(LIBMAUS_HAVE_IO_LIB)
#include <libmaus/bambam/ScramDecoder.hpp>
#include <libmaus/bambam/ScramInputContainer.hpp>
#endif

#include <libmaus/bambam/BamAlignmentDecoderInfo.hpp>
#include <libmaus/bambam/BamRangeDecoder.hpp>

#include <libmaus/aio/InputStreamFactory.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>

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
				S.insert("sam");
				S.insert("maussam");

				#if defined(LIBMAUS_HAVE_IO_LIB)
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
			
			
			libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type operator()() const
			{
				libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(construct(BADI));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			static libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				BamAlignmentDecoderInfo const & BADI,
				std::istream & stdin = std::cin
			)
			{
				libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
					construct(stdin,BADI.inputfilename,BADI.inputformat,BADI.inputthreads,BADI.reference,BADI.putrank,BADI.copystr,BADI.range)
				);
				
				return UNIQUE_PTR_MOVE(tptr);
			}

			static libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				libmaus::bambam::BamAlignmentDecoderInfo const & BADI, bool const putrank, std::istream & stdin
			)
			{
				libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
					libmaus::bambam::BamAlignmentDecoderFactory::construct(
						stdin,
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

			static libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type construct(
				std::istream & stdin = std::cin,
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
								libmaus::aio::InputStream::unique_ptr_type iptr(
									new libmaus::aio::InputStream(stdin)
								);								
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamDecoderWrapper(iptr,*copystr,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
							else
							{
								libmaus::aio::InputStream::unique_ptr_type iptr(
									new libmaus::aio::InputStream(stdin)
								);
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamDecoderWrapper(iptr,putrank)
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
									libmaus::aio::InputStream::unique_ptr_type iptr(
										libmaus::aio::InputStreamFactoryContainer::constructUnique(inputfilename)
									);
									libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
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
							libmaus::exception::LibMausException ex;
							ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for parallel input" << std::endl;
							ex.finish();
							throw ex;								
						}
						else if ( inputisstdin )
						{
							if ( copystr )
							{
								libmaus::aio::InputStream::unique_ptr_type iptr(new libmaus::aio::InputStream(stdin));
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(iptr,*copystr,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);
							}
							else
							{
								libmaus::aio::InputStream::unique_ptr_type iptr(new libmaus::aio::InputStream(stdin));
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(iptr,inputthreads,putrank)
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
								libmaus::aio::InputStream::unique_ptr_type iptr(
									libmaus::aio::InputStreamFactoryContainer::constructUnique(inputfilename)
								);
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
									new BamParallelDecoderWrapper(iptr,inputthreads,putrank)
								);
								return UNIQUE_PTR_MOVE(tptr);					
							}
						}
					}
				}
				else if ( 
					inputformat == "maussam" 
					#if ! defined(LIBMAUS_HAVE_IO_LIB)
					||
					inputformat == "sam"
					#endif
				)
				{
					if ( copystr )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): Stream copy option is not valid for SAM based input" << std::endl;
						ex.finish();
						throw ex;		
					}
					if ( range.size() )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for the sam input format" << std::endl;
						ex.finish();
						throw ex;		
					}
					
					if ( inputisstdin )
					{
						libmaus::aio::InputStream::unique_ptr_type iptr(new libmaus::aio::InputStream(stdin));
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::SamDecoderWrapper(iptr,putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						libmaus::aio::InputStream::unique_ptr_type iptr(
							libmaus::aio::InputStreamFactoryContainer::constructUnique(inputfilename)
						);
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::SamDecoderWrapper(iptr,putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);					
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
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for the sam input format" << std::endl;
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
				else if ( inputformat == "sbam" )
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
						ex.getStream() << "BamAlignmentDecoderFactory::construct(): ranges are not supported for the sbam input format" << std::endl;
						ex.finish();
						throw ex;		
					}
					
					if ( inputisstdin )
					{
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper("-","rb","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper(inputfilename,"rb","",putrank)
						);
						return UNIQUE_PTR_MOVE(tptr);					
					}
				}
				else if ( inputformat == "cram" )
				{
					CramRange cramrange;
										
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

						cramrange = CramRange(range);
					}

					if ( inputisstdin )
					{
						#if defined(LIBMAUS_HAVE_IO_LIB_INPUT_CALLBACKS)
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper(
								std::string("-"),
								libmaus::bambam::ScramInputContainer::allocate,
								libmaus::bambam::ScramInputContainer::deallocate,
								4096,
								reference,
								putrank
							)
						);
						#else
						libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
							new libmaus::bambam::ScramDecoderWrapper("-","rc",reference,putrank)
						);
						#endif
						return UNIQUE_PTR_MOVE(tptr);						
					}
					else
					{
						if ( cramrange.rangeref.size() )
						{
							#if defined(LIBMAUS_HAVE_IO_LIB_INPUT_CALLBACKS)
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus::bambam::ScramDecoderWrapper(
									inputfilename,
									libmaus::bambam::ScramInputContainer::allocate,
									libmaus::bambam::ScramInputContainer::deallocate,
									4096,
									reference,
									cramrange.rangeref,cramrange.rangestart,cramrange.rangeend,
									putrank
								)
							);
							#else
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus::bambam::ScramDecoderWrapper(inputfilename,"rc",reference,
									cramrange.rangeref,cramrange.rangestart,cramrange.rangeend,putrank)
							);
							#endif
							return UNIQUE_PTR_MOVE(tptr);											
						}
						else
						{
							#if defined(LIBMAUS_HAVE_IO_LIB_INPUT_CALLBACKS)
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus::bambam::ScramDecoderWrapper(
									inputfilename,
									libmaus::bambam::ScramInputContainer::allocate,
									libmaus::bambam::ScramInputContainer::deallocate,
									4096,
									reference,
									putrank
								)
							);
							#else			
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr(
								new libmaus::bambam::ScramDecoderWrapper(inputfilename,"rc",reference,putrank)
							);
							#endif
							return UNIQUE_PTR_MOVE(tptr);					
						}
					}
				}
				#endif
				else
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "Invalid input format " << inputformat << std::endl;
					ex.finish();
					throw ex;
				}
			}
		};
	}
}
#endif
