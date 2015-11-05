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
#if ! defined(LIBMAUS2_RANDOM_DNABASENOISESPIKER_HPP)
#define LIBMAUS2_RANDOM_DNABASENOISESPIKER_HPP

#include <libmaus2/random/GaussianRandom.hpp>
#include <libmaus2/fastx/StreamFastAReader.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <map>
#include <vector>

namespace libmaus2
{
	namespace random
	{
		/**
		 * two error level noise spiker for DNA sequences
		 **/
		struct DNABaseNoiseSpiker
		{
			enum err_type_enum
			{
			    err_subst,
			    err_ins,
			    err_del
			};

			enum state_enum
			{
			    state_error_low = 0,
			    state_error_high = 1
			};

			std::map<int,double> err_prob_cumul;

			std::map<int,double> state_erate;
			std::map<int,double> state_erate_sq;

			std::map< int, std::map<int,double> > state_change_map;
			std::map< int, double > state_start_map;

			static bool contains(std::vector<err_type_enum> const & V, err_type_enum type)
			{
			    for ( uint64_t i = 0; i < V.size(); ++i )
				if ( V[i] == type )
				    return true;
			    return false;
			}

			DNABaseNoiseSpiker(
				double substrate,
				double delrate,
				double insrate,
				double eratelow,
				double eratehigh,
				double eratelowstddev,
				double eratehighstddev,
				double keeplowstate,
				double keephighstate,
				double startlowprob
			)
			{
				double ratesum = substrate + delrate + insrate;
				substrate /= ratesum;
				delrate /= ratesum;
				insrate /= ratesum;

				err_prob_cumul[err_subst] = substrate;
				err_prob_cumul[err_del] = substrate + delrate;
				err_prob_cumul[err_ins] = substrate + delrate + insrate;

				if ( std::abs ( err_prob_cumul.find(err_ins)->second - 1.0 ) > 1e-3 )
				{
				    libmaus2::exception::LibMausException lme;
				    lme.getStream() << "subst, del and insrate cannot be normalised to sum 1" << std::endl;
				    lme.finish();
				    throw lme;
				}

				state_erate[state_error_low] = eratelow;
				state_erate[state_error_high] = eratehigh;
				state_erate_sq[state_error_low] = eratelowstddev;
				state_erate_sq[state_error_high] = eratehighstddev;

				state_change_map[state_error_low][state_error_low ] = keeplowstate;
				state_change_map[state_error_low][state_error_high] = 1-state_change_map[state_error_low][state_error_low ];
				state_change_map[state_error_high][state_error_high] = keephighstate;
				state_change_map[state_error_high][state_error_low] = 1-state_change_map[state_error_high][state_error_high ];
				state_start_map[state_error_low] = startlowprob;
				state_start_map[state_error_high] = 1 - state_start_map[state_error_low];
			}

			struct ErrorStats
			{
				uint64_t numins;
				uint64_t numdel;
				uint64_t numsubst;

				ErrorStats()
				{

				}

				ErrorStats(
					uint64_t rnumins,
					uint64_t rnumdel,
					uint64_t rnumsubst
				) : numins(rnumins), numdel(rnumdel), numsubst(rnumsubst)
				{

				}
			};

			std::string modify(std::string const & sub, ErrorStats * const estats = 0) const
			{
				std::ostringstream ostr;
				ErrorStats const E = modify(ostr,sub,0,0,0,0);
				if ( estats )
					*estats = E;
				std::istringstream istr(ostr.str());
			        libmaus2::fastx::StreamFastAReaderWrapper SFQR(istr);
				libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;

				if ( SFQR.getNextPatternUnlocked(pattern) )
					return pattern.spattern;
				else
				{
				    libmaus2::exception::LibMausException lme;
				    lme.getStream() << "DNABaseNoiseSpiker: failed to produce sequence" << std::endl;
				    lme.finish();
				    throw lme;
				}
			}

			std::pair<std::string,std::string> modifyAndComment(std::string const & sub, ErrorStats * const estats = 0) const
			{
				std::ostringstream ostr;
				ErrorStats const E = modify(ostr,sub,0,0,0,0);
				if ( estats )
					*estats = E;
				std::istringstream istr(ostr.str());
			        libmaus2::fastx::StreamFastAReaderWrapper SFQR(istr);
				libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;

				if ( SFQR.getNextPatternUnlocked(pattern) )
					return std::pair<std::string,std::string>(pattern.spattern,pattern.sid);
				else
				{
				    libmaus2::exception::LibMausException lme;
				    lme.getStream() << "DNABaseNoiseSpiker: failed to produce sequence" << std::endl;
				    lme.finish();
				    throw lme;
				}
			}

			static std::string modify(
				std::string const & sub,
				double const substrate, double const delrate, double const insrate,
				double const erateavg,
				double const eratestddev,
				ErrorStats * const estats = 0
			)
			{
				DNABaseNoiseSpiker spiker(
					substrate,delrate,insrate,erateavg,erateavg,eratestddev,eratestddev,1,0,1
				);
				return spiker.modify(sub,estats);
			}

			static std::pair<std::string,std::string> modifyAndComment(
				std::string const & sub,
				double const substrate, double const delrate, double const insrate,
				double const erateavg,
				double const eratestddev,
				ErrorStats * const estats = 0
			)
			{
				DNABaseNoiseSpiker spiker(
					substrate,delrate,insrate,erateavg,erateavg,eratestddev,eratestddev,1,0,1
				);
				return spiker.modifyAndComment(sub,estats);
			}

			ErrorStats modify(
				std::ostream & out,
				std::string sub,
				uint64_t const pos,
				bool const strand,
				uint64_t const runid,
				uint64_t const readid
			) const
			{
				std::ostringstream errostr;
				std::ostringstream baseostr;
				std::ostringstream cigopstr;

				std::vector < state_enum > states;
				state_enum state = (libmaus2::random::UniformUnitRandom::uniformUnitRandom() < state_start_map.find(state_error_low)->second) ? state_error_low : state_error_high;
				for ( uint64_t i = 0; i < sub.size(); ++i )
				{
				    states.push_back(state);

				    if ( libmaus2::random::UniformUnitRandom::uniformUnitRandom() < state_change_map.find(state)->second.find(state_error_low)->second )
					state = state_error_low;
				    else
					state = state_error_high;
				}

				std::map<uint64_t,std::vector<err_type_enum> > errM;
				for ( uint64_t i = 0; i < sub.size(); ++i )
				    errM[i] = std::vector<err_type_enum>(0);

				uint64_t low = 0;
				while ( low < sub.size() )
				{
				    uint64_t high = low;
				    while ( high < sub.size() && states[high] == states[low] )
					++high;

				    double erate = -1;
				    while ( (erate = libmaus2::random::GaussianRandom::random(state_erate_sq.find(states[low])->second,state_erate.find(states[low])->second)) < 0 )
				    {

				    }

				    uint64_t const numerr = std::floor(erate * (high-low) + 0.5);

				    errostr << "einterval(" << "[" << low << "," << high << ")" << ",erate=" << erate << ",numerr=" << numerr << ')' << ';';

				    uint64_t errplaced = 0;

				    while ( errplaced < numerr )
				    {
					double const p = libmaus2::random::UniformUnitRandom::uniformUnitRandom();
					uint64_t const errpos = std::floor(libmaus2::random::UniformUnitRandom::uniformUnitRandom() * (high-low-1) + 0.5) + low;

					if ( p < err_prob_cumul.find(err_subst)->second )
					{
					    // no error yet?
					    if ( errM.find(errpos) == errM.end() || errM.find(errpos)->second.size() == 0 )
					    {
						errM[errpos].push_back(err_subst);
						errplaced++;
					    }
					    // no error so far a deletion or substitution?
					    else if (
						!contains(errM.find(errpos)->second,err_del)
						&&
						!contains(errM.find(errpos)->second,err_subst)
					    )
					    {
						errM[errpos].push_back(err_subst);
						errplaced++;
					    }
					}
					else if ( p < err_prob_cumul.find(err_del)->second )
					{
					    // no error yet?
					    if ( errM.find(errpos) == errM.end() || errM.find(errpos)->second.size() == 0 )
					    {
						errM[errpos].push_back(err_del);
						errplaced++;
					    }
					    // no error so far a deletion or substitution?
					    else if (
						!contains(errM.find(errpos)->second,err_del)
						&&
						!contains(errM.find(errpos)->second,err_subst)
					    )
					    {
						errM[errpos].push_back(err_subst);
						errplaced++;
					    }
					}
					else
					{
					    errM[errpos].push_back(err_ins);
					    errplaced++;
					}
				    }

				    low = high;
				}

				errostr << "pos=" << pos << ';';
				errostr << "strand=" << strand << ';';

				uint64_t numins = 0, numdel = 0, numsubst = 0;

				for ( std::map<uint64_t,std::vector<err_type_enum> >::const_iterator ita = errM.begin(); ita != errM.end(); ++ita )
				{
				    uint64_t const pos = ita->first;
				    std::vector<err_type_enum> const & errV = ita->second;

				    // insertions
				    for ( uint64_t i = 0; i < errV.size(); ++i )
					if ( errV[i] == err_ins )
					{
					    int const v = libmaus2::random::Random::rand8() & 3;
					    int insbase = -1;
					    switch ( v )
					    {
						case 0: insbase = 'A'; break;
						case 1: insbase = 'C'; break;
						case 2: insbase = 'G'; break;
						case 3: insbase = 'T'; break;
					    }
					    baseostr.put(insbase);
					    errostr << 'i' << static_cast<char>(insbase);
					    cigopstr.put('I');
					    numins += 1;
					}

				    // base not deleted?
				    if ( !contains(errV,err_del) )
				    {
					if ( contains(errV,err_subst) )
					{
					    int const v = libmaus2::random::Random::rand8() & 3;
					    int insbase = -1;
					    switch ( v )
					    {
						case 0: insbase = 'A'; break;
						case 1: insbase = 'C'; break;
						case 2: insbase = 'G'; break;
						case 3: insbase = 'T'; break;
					    }
					    baseostr.put(insbase);
					    errostr << 's' << static_cast<char>(insbase);
					    cigopstr.put('X');
					    numsubst += 1;
					}
					else
					{
					   baseostr.put(sub[pos]);
					   errostr << 'o';
					   cigopstr.put('=');
					}
				    }
				    else
				    {
					errostr.put('d');
					numdel += 1;
					cigopstr.put('D');
				    }
				}

				std::string const cigprestr = cigopstr.str();
				uint64_t z = 0;
				std::ostringstream cigfinalstr;
				while ( z < cigprestr.size() )
				{
					uint64_t h = z;
					while ( h < cigprestr.size() && cigprestr[z] == cigprestr[h] )
						++h;
					cigfinalstr << h-z << cigprestr[z];
					z = h;
				}
				std::string const cigstr = cigfinalstr.str();

				out << '>' << 'L' << runid << '/' << (readid) << '/' << 0 << '_' << baseostr.str().size() << " RQ=0.851 " << errostr.str() << " CIGAR=[" << cigstr << "]\n";

				uint64_t b_low = 0;
				std::string const bases = baseostr.str();

				while ( b_low < bases.size() )
				{
				    uint64_t const high = std::min(b_low + 80, static_cast<uint64_t>(bases.size()));

				    out.write(bases.c_str() + b_low, high-b_low);
				    out.put('\n');

				    b_low = high;
				}

				return ErrorStats(numins,numdel,numsubst);
			}
		};
	}
}
#endif
