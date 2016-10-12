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


#if ! defined(LIBMAUS2_FASTX_PATTERN_HPP)
#define LIBMAUS2_FASTX_PATTERN_HPP

#include <string>
#include <sstream>
#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/util/ToUpperTable.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace fastx
	{
                struct PatternBase
                {
                        unsigned int patlen;
                        size_t patid;

                        char const * mapped;
                        char const * transposed;

                        PatternBase() : patlen(0), patid(0), mapped(0), transposed(0) {}

                        virtual ~PatternBase() {}

                        private:
                        PatternBase(PatternBase const &);
                        PatternBase & operator=(PatternBase const &);

                        public:
                        unsigned int getQuality(unsigned int const /* i */) const
                        {
                                return 30;
                        }

                        bool isDontCareFree() const
                        {
                                for ( unsigned int i = 0; i < patlen; ++i )
                                        if ( mapped[i] > 3 )
                                                return false;
                                return true;
                        }

                        unsigned int getPatternLength() const
                        {
                                return patlen;
                        }

                        size_t getPatID() const
                        {
                                return patid;
                        }
                        inline void computeMapped()
                        {}
                        std::string getStringId() const
                        {
                                std::ostringstream ostr;
                                ostr << patid;
                                ostr.flush();
                                return ostr.str();
                        }
                };

                struct PatternQualityBase : public PatternBase
                {
                        char const * quality;

                        PatternQualityBase() : PatternBase(), quality(0) {}

                        unsigned int getQuality(unsigned int const i) const
                        {
                                return quality[i];
                        }

                        private:
                        PatternQualityBase(PatternQualityBase const &);
                        PatternQualityBase & operator=(PatternQualityBase const &);
                };

                struct Pattern : public PatternBase
                {
                        typedef Pattern this_type;
                        typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

                        public:
                        std::string sid;

                        // private:
                        std::string spattern;
                        char const * pattern;

                        public:
                        std::string smapped;
                        std::string stransposed;

                        void toupper(libmaus2::util::ToUpperTable const & U)
                        {
                        	for ( uint64_t i = 0; i < spattern.size(); ++i )
                        		spattern[i] = U(static_cast<uint8_t>(spattern[i]));
                        }

                        Pattern & operator=(Pattern const & o)
                        {
                        	if ( this != &o )
                        	{
                        		sid = o.sid;
                        		patlen = o.patlen;
                        		patid = o.patid;
                        		smapped = o.smapped;
                        		stransposed = o.stransposed;
                        		spattern = o.spattern;

                        		if ( !o.spattern.size() )
                        		{
                        			//std::cerr << "spattern empty, copying pointer." << '\n';
	                        		pattern = o.pattern;
					}
					else
					{
						//std::cerr << "spattern not empty, copying string." << '\n';
						pattern = spattern.c_str();
					}
                        	}
                        	else
                        	{
                        		//std::cerr << "self copy." << '\n';
                        	}
                        	return *this;
                        }

                        std::string getNamePrefix() const
                        {
                        	uint64_t j = sid.size();
                        	for ( uint64_t i = 0; i+1 < sid.size(); ++i )
                        		if (
                        			sid[i] == '/' &&
                        			(sid[i+1] == '1' || sid[i+1] == '2')
					)
					j = i;

				if ( j == sid.size() )
					return sid;
				else
					return sid.substr(0,j);
                        }

                        Pattern() : PatternBase(), sid(), spattern(), pattern(0), smapped(), stransposed() {}
                        Pattern(Pattern const & o) : PatternBase(), sid(), spattern(), pattern(0), smapped(), stransposed()
                        {
                        	//std::cerr << "copy construct." << '\n';
                        	*this = o;
                        }

                        void copyPattern(char * p) const
                        {
                                ::std::copy(
                                        spattern.begin(),
                                        spattern.end(),
                                        p);
                                p[spattern.size()] = 0;
                        }
                        void reverse()
                        {
                                std::reverse(spattern.begin(),spattern.end());
                                pattern = spattern.c_str();
                        }

                        void computeMapped(int const shift = 0, bool pad = false, char const padsym = 0)
                        {
                                smapped.resize(patlen+(pad?2:0));
                                stransposed.resize(patlen+(pad?2:0));

                                if ( pad )
                                {
                                	smapped[0] = smapped[smapped.size()-1] = padsym;
                                	stransposed[0] = stransposed[stransposed.size()-1] = padsym;
                                }

                                std::string::iterator       smapped_a = smapped.begin() + (pad?1:0);
                                std::string::iterator const smapped_e = smapped.end() - (pad?1:0);
                                std::string::reverse_iterator stransposed_a = stransposed.rbegin() + (pad?1:0);

                                char const * tpattern = pattern;
                                while ( smapped_a != smapped_e )
                                {
                                        switch ( *(tpattern++) )
                                        {
                                                case 'a': case 'A': *(smapped_a++) = ::libmaus2::fastx::mapChar('A') + shift;
                                                          *(stransposed_a++) = ::libmaus2::fastx::mapChar(::libmaus2::fastx::invertUnmapped('A')) + shift;
                                                          break;
                                                case 'c': case 'C': *(smapped_a++) = ::libmaus2::fastx::mapChar('C') + shift;
                                                          *(stransposed_a++) = ::libmaus2::fastx::mapChar(::libmaus2::fastx::invertUnmapped('C')) + shift;
                                                          break;
                                                case 'g': case 'G': *(smapped_a++) = ::libmaus2::fastx::mapChar('G') + shift;
                                                          *(stransposed_a++) = ::libmaus2::fastx::mapChar(::libmaus2::fastx::invertUnmapped('G')) + shift;
                                                          break;
                                                case 't': case 'T': *(smapped_a++) = ::libmaus2::fastx::mapChar('T') + shift;
                                                          *(stransposed_a++) = ::libmaus2::fastx::mapChar(::libmaus2::fastx::invertUnmapped('T')) + shift;
                                                          break;
                                                default:  *(smapped_a++) = ::libmaus2::fastx::mapChar('N') + shift;
                                                          *(stransposed_a++) = ::libmaus2::fastx::mapChar(::libmaus2::fastx::invertUnmapped('N')) + shift;
                                                          break;
                                        }
                                }

                                mapped = smapped.c_str() + (pad ? 1:0);
                                transposed = stransposed.c_str() + (pad ? 1 : 0);
                        }
                        std::string const & getStringId() const
                        {
                                return sid;
                        }

			std::string const getShortStringId() const
			{
				std::string::size_type i = 0;
				while ( i < sid.size() && !::isspace(sid[i]) )
					++i;
				return sid.substr(0,i);
			}

                        void unmapSource()
                        {
                                for ( uint64_t i = 0; i < spattern.size(); ++i )
                                        spattern[i] = ::libmaus2::fastx::remapChar(spattern[i]);
                        }

                        std::ostream & printMultiLine(std::ostream & out, unsigned int const cols) const
                        {
                        	std::string const id = getStringId();

                        	out.put('>');
                        	out.write(id.c_str(),id.size());
                        	out.put('\n');
				if ( pattern )
				{
					char const * ita = pattern;
					uint64_t rem = patlen;

					while ( rem )
					{
						uint64_t const toprint = std::min(static_cast<uint64_t>(cols),rem);
						out.write(ita,toprint);
						out.put('\n');
						rem -= toprint;
						ita += toprint;
					}
				}
				return out;
                        }
                        std::ostream & printMultiLine(std::ostream & out, unsigned int const cols, uint64_t & offset) const
                        {
                        	std::string const id = getStringId();

				out << '>' << id << '\n';

				offset += 2 + id.size();

				if ( pattern )
				{
					char const * ita = pattern;
					uint64_t rem = patlen;

					while ( rem )
					{
						uint64_t const toprint = std::min(static_cast<uint64_t>(cols),rem);
						out.write(ita,toprint);
						out.put('\n');
						rem -= toprint;
						ita += toprint;

						offset += toprint + 1;
					}
				}
				return out;
                        }
                };

		inline std::ostream & operator<< ( std::ostream & out, Pattern const & p)
		{
			out << ">" << p.getStringId() << '\n';
			if ( p.pattern )
				out << p.pattern << '\n';
			else
				out << "null" << '\n';
			return out;
		}

		inline std::ostream & oneline ( std::ostream & out, Pattern const & p)
		{
			out << ">" << p.getStringId() << '\t';
			if ( p.pattern ) out << p.pattern;
                        out << '\n';
			return out;
		}
		struct PatternCount
		{
			uint64_t numpat;
			uint64_t numsyms;
			uint64_t minlen;
			uint64_t maxlen;

			PatternCount() : numpat(0), numsyms(0), minlen(0), maxlen(0) {}
			PatternCount(
				uint64_t const rnumpat,
				uint64_t const rnumsyms,
				uint64_t const rminlen,
				uint64_t const rmaxlen
				)
			: numpat(rnumpat), numsyms(rnumsyms), minlen(rminlen), maxlen(rmaxlen)
			{

			}
		};
	}
}
#endif
