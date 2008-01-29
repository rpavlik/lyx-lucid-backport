/**
 * \file MathStream.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Andr� P�nitz
 *
 * Full author contact details are available in file CREDITS.
 */

#include <config.h>

#include "InsetMath.h"
#include "MathData.h"
#include "MathExtern.h"
#include "MathStream.h"

#include "support/lyxalgo.h"
#include "support/textutils.h"


namespace lyx {

using std::strlen;




//////////////////////////////////////////////////////////////////////


NormalStream & operator<<(NormalStream & ns, MathAtom const & at)
{
	at->normalize(ns);
	return ns;
}


NormalStream & operator<<(NormalStream & ns, MathData const & ar)
{
	normalize(ar, ns);
	return ns;
}


NormalStream & operator<<(NormalStream & ns, docstring const & s)
{
	ns.os() << s;
	return ns;
}


NormalStream & operator<<(NormalStream & ns, const std::string & s)
{
	ns.os() << from_utf8(s);
	return ns;
}


NormalStream & operator<<(NormalStream & ns, char const * s)
{
	ns.os() << s;
	return ns;
}


NormalStream & operator<<(NormalStream & ns, char c)
{
	ns.os() << c;
	return ns;
}


NormalStream & operator<<(NormalStream & ns, int i)
{
	ns.os() << i;
	return ns;
}



/////////////////////////////////////////////////////////////////


WriteStream & operator<<(WriteStream & ws, docstring const & s)
{
	if (ws.pendingSpace() && s.length() > 0) {
		if (isAlphaASCII(s[0]))
			ws.os() << ' ';
		ws.pendingSpace(false);
	}
	ws.os() << s;
	int lf = 0;
	docstring::const_iterator dit = s.begin();
	docstring::const_iterator end = s.end();
	for (; dit != end; ++dit)
		if ((*dit) == '\n')
			++lf;
	ws.addlines(lf);
	return ws;
}


WriteStream::WriteStream(odocstream & os, bool fragile, bool latex)
	: os_(os), fragile_(fragile), firstitem_(false), latex_(latex),
	  pendingspace_(false), line_(0)
{}


WriteStream::WriteStream(odocstream & os)
	: os_(os), fragile_(false), firstitem_(false), latex_(false),
	  pendingspace_(false), line_(0)
{}


WriteStream::~WriteStream()
{
	if (pendingspace_)
		os_ << ' ';
}


void WriteStream::addlines(unsigned int n)
{
	line_ += n;
}


void WriteStream::pendingSpace(bool how)
{
	pendingspace_ = how;
}


WriteStream & operator<<(WriteStream & ws, MathAtom const & at)
{
	at->write(ws);
	return ws;
}


WriteStream & operator<<(WriteStream & ws, MathData const & ar)
{
	write(ar, ws);
	return ws;
}


WriteStream & operator<<(WriteStream & ws, char const * s)
{
	if (ws.pendingSpace() && strlen(s) > 0) {
		if (isAlphaASCII(s[0]))
			ws.os() << ' ';
		ws.pendingSpace(false);
	}
	ws.os() << s;
	ws.addlines(int(count(s, s + strlen(s), '\n')));
	return ws;
}


WriteStream & operator<<(WriteStream & ws, char c)
{
	if (ws.pendingSpace()) {
		if (isAlphaASCII(c))
			ws.os() << ' ';
		ws.pendingSpace(false);
	}
	ws.os() << c;
	if (c == '\n')
		ws.addlines(1);
	return ws;
}


WriteStream & operator<<(WriteStream & ws, int i)
{
	ws.os() << i;
	return ws;
}


WriteStream & operator<<(WriteStream & ws, unsigned int i)
{
	ws.os() << i;
	return ws;
}


//////////////////////////////////////////////////////////////////////


MathStream::MathStream(odocstream & os)
	: os_(os), tab_(0), line_(0), lastchar_(0)
{}


MathStream & operator<<(MathStream & ms, MathAtom const & at)
{
	at->mathmlize(ms);
	return ms;
}


MathStream & operator<<(MathStream & ms, MathData const & ar)
{
	mathmlize(ar, ms);
	return ms;
}


MathStream & operator<<(MathStream & ms, char const * s)
{
	ms.os() << s;
	return ms;
}


MathStream & operator<<(MathStream & ms, char c)
{
	ms.os() << c;
	return ms;
}


MathStream & operator<<(MathStream & ms, MTag const & t)
{
	++ms.tab();
	ms.cr();
	ms.os() << '<' << t.tag_ << '>';
	return ms;
}


MathStream & operator<<(MathStream & ms, ETag const & t)
{
	ms.cr();
	if (ms.tab() > 0)
		--ms.tab();
	ms.os() << "</" << t.tag_ << '>';
	return ms;
}


void MathStream::cr()
{
	os() << '\n';
	for (int i = 0; i < tab(); ++i)
		os() << ' ';
}


MathStream & operator<<(MathStream & ms, docstring const & s)
{
	ms.os() << s;
	return ms;
}

//////////////////////////////////////////////////////////////////////


MapleStream & operator<<(MapleStream & ms, MathAtom const & at)
{
	at->maple(ms);
	return ms;
}


MapleStream & operator<<(MapleStream & ms, MathData const & ar)
{
	maple(ar, ms);
	return ms;
}


MapleStream & operator<<(MapleStream & ms, char const * s)
{
	ms.os() << s;
	return ms;
}


MapleStream & operator<<(MapleStream & ms, char c)
{
	ms.os() << c;
	return ms;
}


MapleStream & operator<<(MapleStream & ms, int i)
{
	ms.os() << i;
	return ms;
}


MapleStream & operator<<(MapleStream & ms, char_type c)
{
	ms.os().put(c);
	return ms;
}


MapleStream & operator<<(MapleStream & ms, docstring const & s)
{
	ms.os() << s;
	return ms;
}


//////////////////////////////////////////////////////////////////////


MaximaStream & operator<<(MaximaStream & ms, MathAtom const & at)
{
	at->maxima(ms);
	return ms;
}


MaximaStream & operator<<(MaximaStream & ms, MathData const & ar)
{
	maxima(ar, ms);
	return ms;
}


MaximaStream & operator<<(MaximaStream & ms, char const * s)
{
	ms.os() << s;
	return ms;
}


MaximaStream & operator<<(MaximaStream & ms, char c)
{
	ms.os() << c;
	return ms;
}


MaximaStream & operator<<(MaximaStream & ms, int i)
{
	ms.os() << i;
	return ms;
}


MaximaStream & operator<<(MaximaStream & ms, docstring const & s)
{
	ms.os() << s;
	return ms;
}


MaximaStream & operator<<(MaximaStream & ms, char_type c)
{
	ms.os().put(c);
	return ms;
}


//////////////////////////////////////////////////////////////////////


MathematicaStream & operator<<(MathematicaStream & ms, MathAtom const & at)
{
	at->mathematica(ms);
	return ms;
}


MathematicaStream & operator<<(MathematicaStream & ms, MathData const & ar)
{
	mathematica(ar, ms);
	return ms;
}


MathematicaStream & operator<<(MathematicaStream & ms, char const * s)
{
	ms.os() << s;
	return ms;
}


MathematicaStream & operator<<(MathematicaStream & ms, char c)
{
	ms.os() << c;
	return ms;
}


MathematicaStream & operator<<(MathematicaStream & ms, int i)
{
	ms.os() << i;
	return ms;
}


MathematicaStream & operator<<(MathematicaStream & ms, docstring const & s)
{
	ms.os() << s;
	return ms;
}


MathematicaStream & operator<<(MathematicaStream & ms, char_type c)
{
	ms.os().put(c);
	return ms;
}


//////////////////////////////////////////////////////////////////////


OctaveStream & operator<<(OctaveStream & ns, MathAtom const & at)
{
	at->octave(ns);
	return ns;
}


OctaveStream & operator<<(OctaveStream & ns, MathData const & ar)
{
	octave(ar, ns);
	return ns;
}


OctaveStream & operator<<(OctaveStream & ns, char const * s)
{
	ns.os() << s;
	return ns;
}


OctaveStream & operator<<(OctaveStream & ns, char c)
{
	ns.os() << c;
	return ns;
}


OctaveStream & operator<<(OctaveStream & ns, int i)
{
	ns.os() << i;
	return ns;
}


OctaveStream & operator<<(OctaveStream & ns, docstring const & s)
{
	ns.os() << s;
	return ns;
}


OctaveStream & operator<<(OctaveStream & ns, char_type c)
{
	ns.os().put(c);
	return ns;
}


OctaveStream & operator<<(OctaveStream & os, std::string const & s)
{
	os.os() << from_utf8(s);
	return os;
}


} // namespace lyx