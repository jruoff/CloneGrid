/*
 * Copyright (c) 2013, Jasper Ruoff <jruoff@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef ALGORITHM_EXT_H
#define ALGORITHM_EXT_H

#include <algorithm>

namespace alg {

template<typename RAIterator1, typename RAIterator2>
bool equal(RAIterator1 first1, RAIterator1 last1, RAIterator2 first2, RAIterator2 last2)
{
	if (last1 - first1 != last2 - first2) return false;
	return std::equal(first1, last1, first2);
}

template<typename FIterator, typename BinaryPredicate>
std::pair<FIterator, FIterator> adjacent_find_range(FIterator first, FIterator last, BinaryPredicate p) {
	typedef typename std::iterator_traits<FIterator>::value_type value_type;
	
	std::pair<FIterator, FIterator> result(last, last);
	
	first = std::adjacent_find(first, last, p);
	if (first == last) return result;
	
	result.first  = first;
	result.second = std::adjacent_find(++first, last,
		[p] (const value_type &a, const value_type &b) { return !p(a, b); }
	);
	
	return result;
}

};

#endif // ALGORITHM_EXT_H
