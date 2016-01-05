/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_GAMMA_GAMMADIFFERENCEMERGE_HPP)
#define LIBMAUS2_GAMMA_GAMMADIFFERENCEMERGE_HPP

#include <libmaus2/gamma/GammaDifferenceEncoder.hpp>
#include <libmaus2/gamma/GammaDifferenceDecoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type, int mindif = 1>
		struct GammaDifferenceMerge
		{
			typedef _data_type data_type;
			typedef GammaDifferenceMerge<data_type,mindif> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string const prefix;
			uint64_t nextid;

			std::map< uint64_t,std::vector<std::string> > F;

			std::string getNextFileName()
			{
				std::ostringstream ostr;
				ostr << prefix << "_" << nextid++;
				std::string const fn = ostr.str();
				libmaus2::util::TempFileRemovalContainer::addTempFile(fn);
				return fn;
			}

			GammaDifferenceMerge(std::string const & rprefix)
			: prefix(rprefix), nextid(0)
			{

			}

			template<typename iterator>
			void add(iterator ita, iterator ite)
			{
				if ( ite != ita )
				{
					std::sort(ita,ite);
					std::string const fn = getNextFileName();
					typename libmaus2::gamma::GammaDifferenceEncoder<data_type,mindif>::unique_ptr_type GE(new libmaus2::gamma::GammaDifferenceEncoder<data_type,mindif>(fn));
					while ( ita != ite )
						GE->encode(*(ita++));
					GE.reset();

					uint64_t l = 0;
					F[l].push_back(fn);

					while ( F.find(l)->second.size() > 1 )
					{
						assert ( F.find(l)->second.size() == 2 );
						std::string const nextfn = getNextFileName();
						merge(nextfn,F.find(l)->second.at(0),F.find(l)->second.at(1));
						F.erase(F.find(l));
						F[++l].push_back(nextfn);
					}
				}
			}

			void merge(std::string const & ofn)
			{
				std::vector<std::string> Vfn;
				for ( std::map< uint64_t,std::vector<std::string> >::const_iterator ita = F.begin(); ita != F.end(); ++ita )
				{
					std::vector<std::string> const & Ifn = ita->second;
					for ( uint64_t i = 0; i < Ifn.size(); ++i )
						Vfn.push_back(Ifn[i]);
				}
				while ( Vfn.size() > 1 )
				{
					uint64_t const numo = (Vfn.size()+1)/2;

					for ( uint64_t i = 0; i < numo; ++i )
					{
						if ( 2*i+1 < Vfn.size() )
						{
							std::string const fn0 = Vfn[2*i+0];
							std::string const fn1 = Vfn[2*i+1];
							std::string const ofn = getNextFileName();
							merge(ofn,fn0,fn1);
							Vfn[i] = ofn;
							libmaus2::aio::FileRemoval::removeFile(fn0);
							libmaus2::aio::FileRemoval::removeFile(fn1);
						}
						else
						{
							Vfn[i] = Vfn[2*i+0];
						}
					}

					Vfn.resize(numo);
				}

				if ( Vfn.size() )
				{
					assert ( Vfn.size() == 1 );
					libmaus2::aio::OutputStreamFactoryContainer::rename(Vfn[0],ofn);
				}
				else
				{
					libmaus2::gamma::GammaDifferenceEncoder<data_type,mindif> GE(ofn);
				}
			}

			static void merge(std::ostream & out, std::istream & in0, std::istream & in1, int64_t const rprev = -1)
			{
				libmaus2::gamma::GammaDifferenceDecoder<data_type,mindif> GD0(in0);
				libmaus2::gamma::GammaDifferenceDecoder<data_type,mindif> GD1(in1);
				libmaus2::gamma::GammaDifferenceEncoder<data_type,mindif> GE(out,rprev);

				data_type d0;
				data_type d1;
				bool haved0 = GD0.decode(d0);
				bool haved1 = GD1.decode(d1);

				while ( haved0 && haved1 )
				{
					assert ( d0 != d1 );

					if ( d0 < d1 )
					{
						GE.encode(d0);
						haved0 = GD0.decode(d0);
					}
					else
					{
						GE.encode(d1);
						haved1 = GD1.decode(d1);
					}
				}

				while ( haved0 )
				{
					GE.encode(d0);
					haved0 = GD0.decode(d0);
				}

				while ( haved1 )
				{
					GE.encode(d1);
					haved1 = GD1.decode(d1);
				}
			}

			static void filterOut(std::ostream & out, std::istream & in0, std::istream & in1, int64_t const rprev = -1)
			{
				libmaus2::gamma::GammaDifferenceDecoder<data_type,mindif> GD0(in0);
				libmaus2::gamma::GammaDifferenceDecoder<data_type,mindif> GD1(in1);
				libmaus2::gamma::GammaDifferenceEncoder<data_type,mindif> GE(out,rprev);

				data_type d0;
				data_type d1;
				bool haved0 = GD0.decode(d0);
				bool haved1 = GD1.decode(d1);

				while ( haved0 && haved1 )
				{
					// same, filter out
					if ( d0 == d1 )
					{
						haved0 = GD0.decode(d0);
						haved1 = GD1.decode(d1);
					}
					// stream value < filter value, keep
					else if ( d0 < d1 )
					{
						GE.encode(d0);
						haved0 = GD0.decode(d0);
					}
					// read next filter value
					else
					{
						haved1 = GD1.decode(d1);
					}
				}

				// copy rest of stream values
				while ( haved0 )
				{
					GE.encode(d0);
					haved0 = GD0.decode(d0);
				}

				// read rest of filter values
				while ( haved1 )
				{
					haved1 = GD1.decode(d1);
				}
			}

			static void merge(std::string const & out, std::string const & in0, std::string const & in1, int64_t const prev = -1)
			{
				libmaus2::aio::InputStreamInstance ISI0(in0);
				libmaus2::aio::InputStreamInstance ISI1(in1);
				libmaus2::aio::OutputStreamInstance OSI(out);
				merge(OSI,ISI0,ISI1,prev);
			}

			static void filterOut(std::string const & out, std::string const & in0, std::string const & in1, int64_t const prev = -1)
			{
				libmaus2::aio::InputStreamInstance ISI0(in0);
				libmaus2::aio::InputStreamInstance ISI1(in1);
				libmaus2::aio::OutputStreamInstance OSI(out);
				filterOut(OSI,ISI0,ISI1,prev);
			}
		};
	}
}
#endif
