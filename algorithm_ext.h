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

template<typename FIterator1, typename FIterator2>
bool equal(FIterator1 first1, FIterator1 last1, FIterator2 first2, FIterator2 last2)
{
	if (std::distance(first1, last1) != std::distance(first2, last2)) return false;
	return std::equal(first1, last1, first2);
}

template<typename FIterator, typename BinaryPredicate>
bool adjacent_range(FIterator &first, FIterator &second, FIterator last, BinaryPredicate p)
{
	if (second == last) return true;
	do first = second; while (++second != last && !p(*first, *second));
	return false;
}

template<typename FIterator, typename BP, typename A1, typename A2>
void process_adjacent(FIterator first, FIterator last, BP p, A1 not_p, A2 are_p)
{
	typedef typename std::iterator_traits<FIterator>::value_type T;
	auto ip = [p] (const T &a, const T &b) { return !p(a, b); };
	
	while (true) {
		FIterator a, b = first;
		if (alg::adjacent_range(a, first, last,  p)) break; not_p(b, a);
		if (alg::adjacent_range(b, first, last, ip)) break; are_p(a, b);
	}
}

};

#endif // ALGORITHM_EXT_H
