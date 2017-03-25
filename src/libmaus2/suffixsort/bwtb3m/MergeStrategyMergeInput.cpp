/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInput.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeInternalSmallBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyMergeExternalBlock.hpp>
#include <libmaus2/suffixsort/bwtb3m/MergeStrategyBaseBlock.hpp>

libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInput::loadBlock(
	std::istream & in
)
{
	MergeStrategyBlock::merge_block_type const type =
		static_cast<MergeStrategyBlock::merge_block_type>(libmaus2::util::NumberSerialisation::deserialiseNumber(in));

	switch ( type )
	{
		case MergeStrategyBlock::merge_block_type_internal:
		{
			libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type tptr(
				new libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalBlock(in)
			);
			return tptr;
		}
		case MergeStrategyBlock::merge_block_type_internal_small:
		{
			libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type tptr(
				new libmaus2::suffixsort::bwtb3m::MergeStrategyMergeInternalSmallBlock(in)
			);
			return tptr;
		}
		case MergeStrategyBlock::merge_block_type_external:
		{
			libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type tptr(
				new libmaus2::suffixsort::bwtb3m::MergeStrategyMergeExternalBlock(in)
			);
			return tptr;
		}
		case MergeStrategyBlock::merge_block_type_base:
		{
			libmaus2::suffixsort::bwtb3m::MergeStrategyBlock::shared_ptr_type tptr(
				new libmaus2::suffixsort::bwtb3m::MergeStrategyBaseBlock(in)
			);
			return tptr;
		}
		default:
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "MergeStrategyMergeInput::loadBlock(): unknown node type " << static_cast<int>(type) << std::endl;
			lme.finish();
			throw lme;
		}
	}
}
