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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPPROPERCHECK_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPPROPERCHECK_HPP

#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/Overlap.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapProperCheckReadLengthAccessor
			{
				typedef OverlapProperCheckReadLengthAccessor this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				virtual ~OverlapProperCheckReadLengthAccessor() {}
				virtual uint64_t operator[](uint64_t const id) const = 0;
			};

			struct OverlapProperCheckReadLengthVectorAccessor : public OverlapProperCheckReadLengthAccessor
			{
				std::vector<uint64_t> const & R;

				OverlapProperCheckReadLengthVectorAccessor(std::vector<uint64_t> const & rR) : R(rR) {}

				uint64_t operator[](uint64_t const id) const
				{
					return R.at(id);
				}
			};

			struct OverlapProperCheckReadLengthAccessorDatabaseFile  : public OverlapProperCheckReadLengthAccessor
			{
				libmaus2::dazzler::db::DatabaseFile const & DB;

				OverlapProperCheckReadLengthAccessorDatabaseFile(libmaus2::dazzler::db::DatabaseFile const & rDB) : DB(rDB) {}

				uint64_t operator[](uint64_t const id) const
				{
					return DB.getRead(id).rlen;
				}
			};

			struct OverlapProperCheck
			{
				typedef OverlapProperCheck this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				OverlapProperCheckReadLengthAccessor::unique_ptr_type RL;
				libmaus2::dazzler::db::Track const & inqual;
				libmaus2::dazzler::db::TrackAnnoInterface const & anno;

				static unsigned int const defaulterr = 255;
				double const escale;
				double const termval;


				static double getDefaultEscale()
				{
					return defaulterr;
				}

				OverlapProperCheck(
					libmaus2::dazzler::db::DatabaseFile const & DB,
					libmaus2::dazzler::db::Track const & rinqual,
					double const rtermval
				)
				: RL(new OverlapProperCheckReadLengthAccessorDatabaseFile(DB)),
				  inqual(rinqual), anno(inqual.getAnno()), escale(getDefaultEscale()), termval(rtermval)
				{

				}


				OverlapProperCheck(
					std::vector<uint64_t> const & VRL,
					libmaus2::dazzler::db::Track const & rinqual,
					double const rtermval
				)
				: RL(new OverlapProperCheckReadLengthVectorAccessor(VRL)),
				  inqual(rinqual), anno(inqual.getAnno()), escale(getDefaultEscale()), termval(rtermval)
				{

				}

				struct OverlapProperCheckInfo
				{
					bool proper;
					bool termleft;
					bool termright;

					OverlapProperCheckInfo() {}
					OverlapProperCheckInfo(
						bool const rproper,
						bool const rtermleft,
						bool const rtermright
					) : proper(rproper), termleft(rtermleft), termright(rtermright) {}
				};

				OverlapProperCheckInfo operator()(
					libmaus2::dazzler::align::Overlap const & OVL,
					int64_t const tspace,
					int const verbose = 0,
					std::ostream * out = 0
				) const
				{
					unsigned char const * aa = inqual.Adata->begin() + anno[OVL.aread];
					unsigned char const * ae = inqual.Adata->begin() + anno[OVL.aread+1];
					unsigned char const * ba = inqual.Adata->begin() + anno[OVL.bread];
					unsigned char const * be = inqual.Adata->begin() + anno[OVL.bread+1];

					if ( OVL.path.aepos > OVL.path.abpos && OVL.path.bepos > OVL.path.bbpos )
					{
						uint64_t abpos = OVL.path.abpos;
						uint64_t aepos = OVL.path.aepos;

						uint64_t bbpos = OVL.path.bbpos;
						uint64_t bepos = OVL.path.bepos;

						// remap coordinates on B, but begin remains begin and end remains end
						// so do not swap bbpos and bepos
						if ( OVL.isInverse() )
						{
							uint64_t const blen = (*RL)[OVL.bread];
							// bbpos needs to be included
							bbpos = blen - bbpos - 1;
							// bepos needs to be excluded
							bepos = blen - bepos + 1;

							assert ( bbpos < blen );
							assert ( bepos <= blen );
						}

						uint64_t const aleft = abpos / tspace;
						uint64_t const bleft = bbpos / tspace;
						uint64_t const aright = (aepos-1) / tspace;
						uint64_t const bright = (bepos-1) / tspace;

						assert ( aa + aleft < ae );
						assert ( ba + bleft < be );
						assert ( aa + aright < ae );
						assert ( ba + bright < be );

						unsigned char const qa0 = aa[aleft];
						unsigned char const qa1 = aleft ? aa[aleft-1] : defaulterr;
						unsigned char const ra0 = aa[aright];
						unsigned char const ra1 = ((aa + aright + 1) < ae) ? aa[aright+1] : defaulterr;

						unsigned char const qb0 = ba[bleft];
						unsigned char const qb1 =
							OVL.isInverse()
							?
							((ba + bleft + 1 < be) ? ba[bleft+1] : defaulterr)
							:
							(bleft ? ba[bleft-1] : defaulterr);
						unsigned char const rb0 = ba[bright];
						unsigned char const rb1 =
							OVL.isInverse()
							?
							(bright ? ba[bright-1] : defaulterr)
							:
							(((ba + bright + 1) < be) ? ba[bright+1] : defaulterr);

						double const err_a_left_0 = qa0 / escale;
						double const err_a_left_1 = qa1 / escale;
						double const err_b_left_0 = qb0 / escale;
						double const err_b_left_1 = qb1 / escale;

						double const err_a_right_0 = ra0 / escale;
						double const err_a_right_1 = ra1 / escale;
						double const err_b_right_0 = rb0 / escale;
						double const err_b_right_1 = rb1 / escale;

						if ( verbose && out )
						{
							(*out)
								<< "abpos=" << abpos
								<< ",aepos=" << aepos
								<< ",alen=" << (*RL)[OVL.aread]
								<< ",bbpos=" << bbpos
								<< ",bepos=" << bepos
								<< ",blen=" << (*RL)[OVL.bread]
								<< ",OVL.path.bbpos=" << OVL.path.bbpos
								<< ",OVL.path.bepos=" << OVL.path.bepos
								<< ",err_a("
								<< err_a_left_1 << ","
								<< err_a_left_0 << ","
								<< err_a_right_0 << ","
								<< err_a_right_1 << "),err_b("
								<< err_b_left_1 << ","
								<< err_b_left_0 << ","
								<< err_b_right_0 << ","
								<< err_b_right_1 << ")\n";
						}

						bool const term_a_left_0 = err_a_left_0 >= termval;
						bool const term_a_left_1 = err_a_left_1 >= termval;
						bool const term_b_left_0 = err_b_left_0 >= termval;
						bool const term_b_left_1 = err_b_left_1 >= termval;

						bool const term_a_right_0 = err_a_right_0 >= termval;
						bool const term_a_right_1 = err_a_right_1 >= termval;
						bool const term_b_right_0 = err_b_right_0 >= termval;
						bool const term_b_right_1 = err_b_right_1 >= termval;

						bool const term_a_left = term_a_left_0 || term_a_left_1;
						bool const term_b_left = term_b_left_0 || term_b_left_1;
						bool const term_a_right = term_a_right_0 || term_a_right_1;
						bool const term_b_right = term_b_right_0 || term_b_right_1;

						bool const term_left = term_a_left || term_b_left;
						bool const term_right = term_a_right || term_b_right;

						bool const proper = term_left && term_right;

						return OverlapProperCheckInfo(proper,term_left,term_right);
					}
					else
					{
						return OverlapProperCheckInfo(false,false,false);
					}
				}
			};
		}
	}
}
#endif
