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

#include <libmaus2/trie/TrieState.hpp>
#include <libmaus2/types/types.hpp>
#include <string>
#include <iostream>

struct PrintCallback
{
	std::vector<std::string> const & dict;

	PrintCallback(std::vector<std::string> const & rdict)
	: dict(rdict)
	{

	}

	void operator()(uint64_t const offset, uint64_t id) const
	{
		std::cout << "offset " << offset << " id " << id << " string " << dict[id] << std::endl;
	}
};

int main()
{
	::libmaus2::trie::Trie<char> trie;
	std::vector < std::string > dict;
	dict.push_back("aa");
	dict.push_back("abaaa");
	dict.push_back("abab");
	trie.insertContainer(dict);
	trie.fillFailureFunction();

	::libmaus2::trie::LinearTrie<char> linear = trie.toLinear();
	std::cout << linear;

	::libmaus2::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHT(trie.toLinearHashTrie<uint32_t>());
	std::cout << *LHT;

	std::string const text = "ccaabab_abaaa";
	std::cout << "Scanning text " << text << std::endl;
	PrintCallback PCB(dict);
	LHT->search(text,PCB);

	::libmaus2::trie::Trie<char> trienofailure;
	// std::vector<std::string> dict2;
	trienofailure.insertContainer(dict);
	::libmaus2::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure(trienofailure.toLinearHashTrie<uint32_t>());
	std::cout << LHTnofailure->searchCompleteNoFailure(dict[0]) << std::endl;
	std::cout << LHTnofailure->searchCompleteNoFailure(dict[1]) << std::endl;
	std::cout << LHTnofailure->searchCompleteNoFailure(dict[2]) << std::endl;
}
