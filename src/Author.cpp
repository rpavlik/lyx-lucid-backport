/**
 * \file Author.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author John Levon
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "Author.h"

#include "support/lassert.h"
#include "support/lstrings.h"

#include <algorithm>
#include <istream>

using namespace std;
using namespace lyx::support;

namespace lyx {

static int computeHash(docstring const & name,
	docstring const & email)
{
	string const full_author_string = to_utf8(name + email);
	// Bernstein's hash function
	unsigned int hash = 5381;
	for (unsigned int i = 0; i < full_author_string.length(); ++i)
		hash = ((hash << 5) + hash) + (unsigned int)(full_author_string[i]);
	return int(hash);
}


Author::Author(docstring const & name, docstring const & email)
	: name_(name), email_(email), used_(true)
{
	buffer_id_ = computeHash(name_, email_);
}


bool operator==(Author const & l, Author const & r)
{
	return l.name() == r.name() && l.email() == r.email();
}


ostream & operator<<(ostream & os, Author const & a)
{
	// FIXME UNICODE
	os << a.buffer_id_ << " \"" << to_utf8(a.name_)
			<< "\" " << to_utf8(a.email_);
		
	return os;
}

istream & operator>>(istream & is, Author & a)
{
	string s;
	is >> a.buffer_id_;
	getline(is, s);
	// FIXME UNICODE
	a.name_ = from_utf8(trim(token(s, '\"', 1)));
	a.email_ = from_utf8(trim(token(s, '\"', 2)));
	return is;
}


bool author_smaller(Author const & lhs, Author const & rhs) {
	return lhs.bufferId() < rhs.bufferId();
}


AuthorList::AuthorList()
	: last_id_(0)
{
}


int AuthorList::record(Author const & a)
{
	// If we record an author which equals the current
	// author, we copy the buffer_id, so that it will
	// keep the same id in the file.
	if (authors_.size() > 0 && a == authors_[0])
		authors_[0].setBufferId(a.bufferId());

	Authors::const_iterator it(authors_.begin());
	Authors::const_iterator itend(authors_.end());
	for (int i = 0;  it != itend; ++it, ++i) {
		if (*it == a)
			return i;
	}
	authors_.push_back(a);
	return last_id_++;
}


void AuthorList::record(int id, Author const & a)
{
	LASSERT(unsigned(id) < authors_.size(), /**/);

	authors_[id] = a;
}


void AuthorList::recordCurrentAuthor(Author const & a)
{
	// current author has id 0
	record(0, a);
}


Author const & AuthorList::get(int id) const
{
	LASSERT(id < (int)authors_.size() , /**/);
	return authors_[id];
}


AuthorList::Authors::const_iterator AuthorList::begin() const
{
	return authors_.begin();
}


AuthorList::Authors::const_iterator AuthorList::end() const
{
	return authors_.end();
}


void AuthorList::sort() {
	std::sort(authors_.begin(), authors_.end(), author_smaller);
}


ostream & operator<<(ostream & os, AuthorList const & a) {
	// Copy the authorlist, because we don't want to sort the original
	AuthorList sorted = a;
	sorted.sort();

	AuthorList::Authors::const_iterator a_it = sorted.begin();
	AuthorList::Authors::const_iterator a_end = sorted.end();
	
	for (a_it = sorted.begin(); a_it != a_end; ++a_it) {
		if (a_it->used())
			os << "\\author " << *a_it << "\n";	
	}
	return os;
}


} // namespace lyx
