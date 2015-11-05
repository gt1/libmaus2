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
#if ! defined(LIBMAUS2_LCS_CHECKOVERLAPRESULT_HPP)
#define LIBMAUS2_LCS_CHECKOVERLAPRESULT_HPP

#include <libmaus2/lcs/OrientationWeightEncoding.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/StringSerialisation.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct CheckOverlapResult : public OrientationWeightEncoding
		{
			typedef CheckOverlapResult this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			bool valid;
			uint64_t ia;
			uint64_t ib;
			int64_t score;
			int64_t windowminscore;
			uint64_t clipa;
			uint64_t clipb;
			uint64_t nummat;
			uint64_t nummis;
			uint64_t numins;
			uint64_t numdel;
			overlap_orientation orientation;
			std::string trace;

			template<typename iterator>
			static void rlEncodeTrace(iterator ita, iterator ite, std::ostream & out)
			{
				while ( ita != ite )
				{
					iterator itc = ita;

					while ( itc != ite && *itc == *ita )
						++itc;

					uint64_t const rl = itc-ita;
					assert ( rl != 0 );
					::libmaus2::util::UTF8::encodeUTF8(rl,out);
					out.put(*ita);

					ita = itc;
				}

				out.put(0);
			}

			static void rlEncodeTrace(std::string const & trace, std::ostream & out)
			{
				rlEncodeTrace(trace.begin(),trace.end(),out);
			}

			static std::string rlEncodeTrace(std::string const & trace)
			{
				std::ostringstream ostr;
				rlEncodeTrace(trace,ostr);
				return ostr.str();
			}

			static std::string rlDecodeTrace(std::string const & rl)
			{
				std::istringstream in(rl);
				std::ostringstream out;

				bool decoding = true;

				while ( decoding )
				{
					uint32_t const runlen = ::libmaus2::util::UTF8::decodeUTF8(in);

					if ( runlen )
					{
						uint8_t const sym = in.get();
						for ( uint64_t i = 0; i < runlen; ++i )
						{
							out.put(sym);
						}
					}
					else
					{
						decoding = false;
					}
				}

				return out.str();
			}

			static bool checkRlEncoding(std::string const & s)
			{
				std::string const rlencoding = rlEncodeTrace(s);
				#if 0
				std::cerr << "Uncompressed: " << s.size() << std::endl;
				std::cerr << "Compressed: " << rlencoding.size() << std::endl;
				#endif
				return s == rlDecodeTrace(rlencoding);
			}

			bool checkRlEncoding()
			{
				return checkRlEncoding(trace);
			}

			static shared_ptr_type load(std::istream & in)
			{
				if ( in.peek() < 0 )
					return shared_ptr_type();
				else
				{
					this_type * ptr = new this_type(in);
					return shared_ptr_type(ptr);
				}
			}

			CheckOverlapResult(std::istream & in)
			:
				#if 0
				valid(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				#else
				valid(in.get()),
				#endif
				ia(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				ib(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				score(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				windowminscore(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				#if 0
				clipa(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				clipb(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				nummat(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				nummis(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				numins(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				numdel(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				orientation(static_cast<overlap_orientation>(::libmaus2::util::NumberSerialisation::deserialiseNumber(in))),
				#else
				clipa(::libmaus2::util::UTF8::decodeUTF8(in)),
				clipb(::libmaus2::util::UTF8::decodeUTF8(in)),
				nummat(::libmaus2::util::UTF8::decodeUTF8(in)),
				nummis(::libmaus2::util::UTF8::decodeUTF8(in)),
				numins(::libmaus2::util::UTF8::decodeUTF8(in)),
				numdel(::libmaus2::util::UTF8::decodeUTF8(in)),
				orientation(static_cast<overlap_orientation>(::libmaus2::util::UTF8::decodeUTF8(in))),
				#endif
				#if 0
				trace(::libmaus2::util::StringSerialisation::deserialiseString(in))
				#else
				trace(rlDecodeTrace(::libmaus2::util::StringSerialisation::deserialiseString(in)))
				#endif
			{
			}

			void serialise(std::ostream & out) const
			{
				#if 0
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,valid);
				#else
				out.put(valid ? 1 : 0);
				#endif

				::libmaus2::util::NumberSerialisation::serialiseNumber(out,ia);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,ib);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,score);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,windowminscore);

				#if 0
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,clipa);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,clipb);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,nummat);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,nummis);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,numins);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,numdel);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,static_cast<unsigned int>(orientation));
				#else
				::libmaus2::util::UTF8::encodeUTF8(clipa,out);
				::libmaus2::util::UTF8::encodeUTF8(clipb,out);
				::libmaus2::util::UTF8::encodeUTF8(nummat,out);
				::libmaus2::util::UTF8::encodeUTF8(nummis,out);
				::libmaus2::util::UTF8::encodeUTF8(numins,out);
				::libmaus2::util::UTF8::encodeUTF8(numdel,out);
				::libmaus2::util::UTF8::encodeUTF8(static_cast<unsigned int>(orientation),out);
				#endif

				#if 0
				::libmaus2::util::StringSerialisation::serialiseString(out,trace);
				#else
				::libmaus2::util::StringSerialisation::serialiseString(out,rlEncodeTrace(trace));
				#endif
			}

			static void serialiseVector(std::ostream & out, std::vector<CheckOverlapResult> const & V)
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,V.size());
				for ( uint64_t i = 0; i < V.size(); ++i )
					V[i].serialise(out);
			}

			static std::string serialiseVector(std::vector<CheckOverlapResult> const & V)
			{
				std::ostringstream ostr;
				serialiseVector(ostr,V);
				return ostr.str();
			}

			static std::vector<CheckOverlapResult::shared_ptr_type> deserialiseVectorShared(std::istream & in)
			{
				uint64_t const n = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				std::vector<CheckOverlapResult::shared_ptr_type> V(n);
				for ( uint64_t i = 0; i < n; ++i )
					V[i] = CheckOverlapResult::shared_ptr_type(new CheckOverlapResult(in));
				return V;
			}

			static std::vector<CheckOverlapResult> deserialiseVector(std::istream & in)
			{
				uint64_t const n = ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				std::vector<CheckOverlapResult> V(n);
				for ( uint64_t i = 0; i < n; ++i )
					V[i] = CheckOverlapResult(in);
				return V;
			}

			static std::vector<CheckOverlapResult> deserialiseVector(std::string const & in)
			{
				std::istringstream istr(in);
				return deserialiseVector(istr);
			}

			static std::vector<CheckOverlapResult::shared_ptr_type> deserialiseVectorShared(std::string const & in)
			{
				std::istringstream istr(in);
				return deserialiseVectorShared(istr);
			}

			static void serialiseVector(std::ostream & out, std::vector<CheckOverlapResult::shared_ptr_type> const & V)
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,V.size());
				for ( uint64_t i = 0; i < V.size(); ++i )
					V[i]->serialise(out);
			}
			static std::string serialiseVector(std::vector<CheckOverlapResult::shared_ptr_type> const & V)
			{
				std::ostringstream ostr;
				serialiseVector(ostr,V);
				return ostr.str();
			}

			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}

			CheckOverlapResult() : valid(false), ia(0), ib(0), score(0), windowminscore(0), clipa(0), clipb(0),
				nummat(0), nummis(0), numins(0), numdel(0), orientation(overlap_cover_complete),
				trace() {}
			CheckOverlapResult(
				bool const rvalid,
				uint64_t const ria,
				uint64_t const rib,
				int64_t const rscore,
				int64_t const rwindowminscore,
				uint64_t const rclipa,
				uint64_t const rclipb,
				uint64_t const rnummat,
				uint64_t const rnummis,
				uint64_t const rnumins,
				uint64_t const rnumdel,
				overlap_orientation const rorientation,
				std::string const & rtrace
				)
			: valid(rvalid), ia(ria), ib(rib), score(rscore), windowminscore(rwindowminscore), clipa(rclipa), clipb(rclipb),
			  nummat(rnummat), nummis(rnummis), numins(rnumins), numdel(rnumdel),
			  orientation(rorientation),trace(rtrace) {}

			CheckOverlapResult(CheckOverlapResult const & o)
			: valid(o.valid),
			  ia(o.ia),
			  ib(o.ib),
			  score(o.score),
			  windowminscore(o.windowminscore),
			  clipa(o.clipa),
			  clipb(o.clipb),
			  nummat(o.nummat),
			  nummis(o.nummis),
			  numins(o.numins),
			  numdel(o.numdel),
			  orientation(o.orientation),
			  trace(o.trace)
			{

			}

			uint64_t getUsedA() const
			{
				return nummat + nummis + numins;
			}

			uint64_t getUsedB() const
			{
				return nummat + nummis + numdel;
			}

			bool operator<(CheckOverlapResult const & o) const
			{
				return this->score < o.score;
			}

			CheckOverlapResult invert() const
			{
				CheckOverlapResult o = *this;

				if (
					isLeftEdge(o.orientation)
					||
					isRightEdge(o.orientation)
				)
					std::swap(o.clipa,o.clipb);

				std::swap(o.numins,o.numdel);
				std::swap(o.ia,o.ib);

				std::reverse(o.trace.begin(),o.trace.end());
				for ( uint64_t i = 0; i < o.trace.size(); ++i )
					switch ( o.trace[i] )
					{
						case 'I': o.trace[i] = 'D'; break;
						case 'D': o.trace[i] = 'I'; break;
						default: break;
					}

				return o;
			}

			CheckOverlapResult invertIncludingOrientation() const
			{
				CheckOverlapResult o = *this;

				if (
					isLeftEdge(o.orientation)
					||
					isRightEdge(o.orientation)
				)
					std::swap(o.clipa,o.clipb);

				std::swap(o.numins,o.numdel);
				std::swap(o.ia,o.ib);

				o.orientation = getInverse(o.orientation);

				std::reverse(o.trace.begin(),o.trace.end());
				for ( uint64_t i = 0; i < o.trace.size(); ++i )
					switch ( o.trace[i] )
					{
						case 'I': o.trace[i] = 'D'; break;
						case 'D': o.trace[i] = 'I'; break;
						default: break;
					}

				return o;
			}

			static CheckOverlapResult const & max(
				CheckOverlapResult const & A, CheckOverlapResult const & B
				)
			{
				if ( (!A.valid) )
					return B;
				else if ( (!B.valid) )
					return A;
				else if ( A.score >= B.score )
					return A;
				else
					return B;
			}

			edge_type toEdge() const
			{
				return edge_type(
					ia,ib,
					addOrientation(clipb,orientation)
				);

			}
		};

		inline std::ostream & operator<<(std::ostream & out, CheckOverlapResult const & C)
		{
			out
				<< "CheckOverlapResult("
				<< "valid=" << (C.valid?"true":"false")
				<< ",ia=" << C.ia
				<< ",ib=" << C.ib
				<< ",score=" << C.score
				<< ",windowminscore=" << C.windowminscore
				<< ",clipa=" << C.clipa
				<< ",clipb=" << C.clipb
				<< ",nummat=" << C.nummat
				<< ",nummis=" << C.nummis
				<< ",numins=" << C.numins
				<< ",numdel=" << C.numdel
				<< ",orientation=" << C.orientation
				<< ",trace=" << C.trace
				<< ")";
			return out;
		}

	}
}
#endif
