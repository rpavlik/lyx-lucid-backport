/**
 * \file unicode.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Lars Gullik Bjønnes
 *
 * Full author contact details are available in file CREDITS.
 *
 * A collection of unicode conversion functions, using iconv.
 */

#include <config.h>

#include "unicode.h"

#include "debug.h"

#include <iconv.h>

#include <cerrno>
#include <iomanip>
#include <map>

using std::endl;
using std::map;
using std::make_pair;
using std::string;
using std::vector;

namespace {

#ifdef WORDS_BIGENDIAN
	char const * utf16_codeset = "UTF16-BE";
#else
	char const * utf16_codeset = "UTF16-LE";
#endif

}


namespace lyx {

#ifdef WORDS_BIGENDIAN
	char const * ucs4_codeset = "UCS-4BE";
#else
	char const * ucs4_codeset = "UCS-4LE";
#endif

static const iconv_t invalid_cd = (iconv_t)(-1);


struct IconvProcessor::Private {
	Private(): cd(invalid_cd) {}
	~Private()
	{
		if (cd != invalid_cd) {
			if (iconv_close(cd) == -1) {
				lyxerr << "Error returned from iconv_close("
				       << errno << ")" << endl;
			}
		}
	}
	iconv_t cd;
};


IconvProcessor::IconvProcessor(char const * tocode,
		char const * fromcode): tocode_(tocode), fromcode_(fromcode),
		pimpl_(new IconvProcessor::Private)
{
}


IconvProcessor::IconvProcessor(IconvProcessor const & other)
	: tocode_(other.tocode_), fromcode_(other.fromcode_),
	  pimpl_(new IconvProcessor::Private)
{
}


IconvProcessor & IconvProcessor::operator=(IconvProcessor const & other)
{
	if (&other == this)
		return *this;
	tocode_ = other.tocode_;
	fromcode_ = other.fromcode_;
	pimpl_.reset(new Private);
	return *this;
}


IconvProcessor::~IconvProcessor() {}


bool IconvProcessor::init()
{
	if (pimpl_->cd != invalid_cd)
		return true;

	pimpl_->cd = iconv_open(tocode_.c_str(), fromcode_.c_str());
	if (pimpl_->cd != invalid_cd)
		return true;

	lyxerr << "Error returned from iconv_open" << endl;
	switch (errno) {
		case EINVAL:
			lyxerr << "EINVAL The conversion from " << fromcode_
				<< " to " << tocode_
				<< " is not supported by the implementation."
				<< endl;
			break;
		default:
			lyxerr << "\tSome other error: " << errno << endl;
			break;
	}
	return false;
}


int IconvProcessor::convert(char const * buf, size_t buflen,
		char * outbuf, size_t maxoutsize)
{
	if (buflen == 0)
		return 0;

	if (pimpl_->cd == invalid_cd) {
		if (!init())
			return -1;
	}

	char ICONV_CONST * inbuf = const_cast<char ICONV_CONST *>(buf);
	size_t inbytesleft = buflen;
	size_t outbytesleft = maxoutsize;

	int res = iconv(pimpl_->cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

	//lyxerr << std::dec;
	//lyxerr << "Inbytesleft: " << inbytesleft << endl;
	//lyxerr << "Outbytesleft: " << outbytesleft << endl;

	if (res != -1)
		// Everything went well.
		return maxoutsize - outbytesleft;

	// There are some errors in the conversion
	lyxerr << "Error returned from iconv" << endl;
	switch (errno) {
		case E2BIG:
			lyxerr << "E2BIG  There is not sufficient room at *outbuf." << endl;
			break;
		case EILSEQ:
			lyxerr << "EILSEQ An invalid multibyte sequence"
				<< " has been encountered in the input.\n"
				<< "When converting from " << fromcode_
				<< " to " << tocode_ << ".\n";
			lyxerr << "Input:" << std::hex;
			for (size_t i = 0; i < buflen; ++i) {
				// char may be signed, avoid output of
				// something like 0xffffffc2
				boost::uint32_t const b =
					*reinterpret_cast<unsigned char const *>(buf + i);
				lyxerr << " 0x" << b;
			}
			lyxerr << std::dec << endl;
			break;
		case EINVAL:
			lyxerr << "EINVAL An incomplete multibyte sequence"
				<< " has been encountered in the input.\n"
				<< "When converting from " << fromcode_
				<< " to " << tocode_ << ".\n";
			lyxerr << "Input:" << std::hex;
			for (size_t i = 0; i < buflen; ++i) {
				// char may be signed, avoid output of
				// something like 0xffffffc2
				boost::uint32_t const b =
					*reinterpret_cast<unsigned char const *>(buf + i);
				lyxerr << " 0x" << b;
			}
			lyxerr << std::dec << endl;
			break;
		default:
			lyxerr << "\tSome other error: " << errno << endl;
			break;
	}
	// We got an error so we close down the conversion engine
	if (iconv_close(pimpl_->cd) == -1) {
		lyxerr << "Error returned from iconv_close("
			<< errno << ")" << endl;
	}
	pimpl_->cd = invalid_cd;
	return -1;
}


namespace {


template<typename RetType, typename InType>
vector<RetType>
iconv_convert(IconvProcessor & processor,
	      InType const * buf,
	      size_t buflen)
{
	if (buflen == 0)
		return vector<RetType>();

	char const * inbuf = reinterpret_cast<char const *>(buf);
	size_t inbytesleft = buflen * sizeof(InType);

	size_t const outsize = 32768;
	static char out[outsize];
	char * outbuf = out;

	int bytes = processor.convert(inbuf, inbytesleft, outbuf, outsize);
	if (bytes <= 0)
		// Conversion failed
		// FIXME Maybe throw an exception and handle that in the caller?
		return vector<RetType>();

	RetType const * tmp = reinterpret_cast<RetType const *>(out);
	return vector<RetType>(tmp, tmp + bytes / sizeof(RetType));
}

} // anon namespace


vector<char_type> utf8_to_ucs4(vector<char> const & utf8str)
{
	if (utf8str.empty())
		return vector<char_type>();

	return utf8_to_ucs4(&utf8str[0], utf8str.size());
}


vector<char_type>
utf8_to_ucs4(char const * utf8str, size_t ls)
{
	static IconvProcessor processor(ucs4_codeset, "UTF-8");
	return iconv_convert<char_type>(processor, utf8str, ls);
}


vector<char_type>
utf16_to_ucs4(unsigned short const * s, size_t ls)
{
	static IconvProcessor processor(ucs4_codeset, utf16_codeset);
	return iconv_convert<char_type>(processor, s, ls);
}


vector<unsigned short>
ucs4_to_utf16(char_type const * s, size_t ls)
{
	static IconvProcessor processor(utf16_codeset, ucs4_codeset);
	return iconv_convert<unsigned short>(processor, s, ls);
}


vector<char>
ucs4_to_utf8(char_type c)
{
	static IconvProcessor processor("UTF-8", ucs4_codeset);
	return iconv_convert<char>(processor, &c, 1);
}


vector<char>
ucs4_to_utf8(vector<char_type> const & ucs4str)
{
	if (ucs4str.empty())
		return vector<char>();

	return ucs4_to_utf8(&ucs4str[0], ucs4str.size());
}


vector<char>
ucs4_to_utf8(char_type const * ucs4str, size_t ls)
{
	static IconvProcessor processor("UTF-8", ucs4_codeset);
	return iconv_convert<char>(processor, ucs4str, ls);
}


vector<char_type>
eightbit_to_ucs4(char const * s, size_t ls, string const & encoding)
{
	static map<string, IconvProcessor> processors;
	if (processors.find(encoding) == processors.end()) {
		IconvProcessor processor(ucs4_codeset, encoding.c_str());
		processors.insert(make_pair(encoding, processor));
	}
	return iconv_convert<char_type>(processors[encoding], s, ls);
}


vector<char>
ucs4_to_eightbit(char_type const * ucs4str, size_t ls, string const & encoding)
{
	static map<string, IconvProcessor> processors;
	if (processors.find(encoding) == processors.end()) {
		IconvProcessor processor(encoding.c_str(), ucs4_codeset);
		processors.insert(make_pair(encoding, processor));
	}
	return iconv_convert<char>(processors[encoding], ucs4str, ls);
}


char ucs4_to_eightbit(char_type ucs4, string const & encoding)
{
	static map<string, IconvProcessor> processors;
	map<string, IconvProcessor>::iterator it = processors.find(encoding);
	if (it == processors.end()) {
		IconvProcessor processor(encoding.c_str(), ucs4_codeset);
		it = processors.insert(make_pair(encoding, processor)).first;
	}

	char out;
	int const bytes = it->second.convert((char *)(&ucs4), 4, &out, 1);
	if (bytes > 0)
		return out;
	return 0;
}


void ucs4_to_multibytes(char_type ucs4, vector<char> & out,
	string const & encoding)
{
	static map<string, IconvProcessor> processors;
	map<string, IconvProcessor>::iterator it = processors.find(encoding);
	if (it == processors.end()) {
		IconvProcessor processor(encoding.c_str(), ucs4_codeset);
		it = processors.insert(make_pair(encoding, processor)).first;
	}

	out.resize(4);
	int bytes = it->second.convert((char *)(&ucs4), 4, &out[0], 4);
	if (bytes > 0)
		out.resize(bytes);
	else
		out.clear();
}

} // namespace lyx