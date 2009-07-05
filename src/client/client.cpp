/**
 * \file client.cpp
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Jo�o Luis M. Assirati
 * \author Lars Gullik Bj�nnes
 *
 * Full author contact details are available in file CREDITS.
 */


#include <config.h>

#include "support/debug.h"
#include "support/FileName.h"
#include "support/FileNameList.h"
#include "support/lstrings.h"
#include "support/unicode.h"

#include <boost/scoped_ptr.hpp>

// getpid(), getppid()
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

// struct timeval
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

// select()
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

// socket(), connect()
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/un.h>

// fcntl()
#include <fcntl.h>

// strerror()
#include <string.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <iostream>

using namespace std;
using namespace lyx::support;

using ::boost::scoped_ptr;

namespace lyx {

namespace support {

string itoa(unsigned int i)
{
	char buf[20];
	sprintf(buf, "%d", i);
	return buf;
}


/// Returns the absolute pathnames of all lyx local sockets in
/// file system encoding.
/// Parts stolen from lyx::support::DirList().
FileNameList lyxSockets(string const & dir, string const & pid)
{
	FileNameList dirlist;

	FileName dirpath(dir + "/");

	if (!dirpath.exists() || !dirpath.isDirectory()) {
		lyxerr << dir << " does not exist or is not a directory."
		       << endl;
		return dirlist;
	}

	FileNameList dirs = dirpath.dirList("");
	FileNameList::const_iterator it = dirs.begin();
	FileNameList::const_iterator end = dirs.end();

	for (; it != end; ++it) {
		if (!it->isDirectory())
			continue;
		string const tmpdir = it->absFilename();
		if (!contains(tmpdir, "lyx_tmpdir" + pid))
			continue;

		FileName lyxsocket(tmpdir + "/lyxsocket");
		if (lyxsocket.exists())
			dirlist.push_back(lyxsocket);
	}

	return dirlist;
}


namespace socktools {


/// Connect to the socket \p name.
int connect(FileName const & name)
{
	int fd; // File descriptor for the socket
	sockaddr_un addr; // Structure that hold the socket address

	string const encoded = name.toFilesystemEncoding();
	// char sun_path[108]
	string::size_type len = encoded.size();
	if (len > 107) {
		cerr << "lyxclient: Socket address '" << name
		     << "' too long." << endl;
		return -1;
	}
	// Synonims for AF_UNIX are AF_LOCAL and AF_FILE
	addr.sun_family = AF_UNIX;
	encoded.copy(addr.sun_path, 107);
	addr.sun_path[len] = '\0';

	if ((fd = ::socket(PF_UNIX, SOCK_STREAM, 0))== -1) {
		cerr << "lyxclient: Could not create socket descriptor: "
		     << strerror(errno) << endl;
		return -1;
	}
	if (::connect(fd,
		      reinterpret_cast<struct sockaddr *>(&addr),
		      sizeof(addr)) == -1) {
		cerr << "lyxclient: Could not connect to socket " << name.absFilename()
		     << ": " << strerror(errno) << endl;
		::close(fd);
		return -1;
	}
	if (::fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		cerr << "lyxclient: Could not set O_NONBLOCK for socket: "
		     << strerror(errno) << endl;
		::close(fd);
		return -1;
	}
	return fd;
}


} // namespace socktools
} // namespace support



/////////////////////////////////////////////////////////////////////
//
// IOWatch
//
/////////////////////////////////////////////////////////////////////

class IOWatch {
public:
	IOWatch();
	void clear();
	void addfd(int);
	bool wait(double);
	bool wait();
	bool isset(int fd);
private:
	fd_set des;
	fd_set act;
};


IOWatch::IOWatch()
{
	clear();
}


void IOWatch::clear()
{
	FD_ZERO(&des);
}


void IOWatch::addfd(int fd)
{
	FD_SET(fd, &des);
}


bool IOWatch::wait(double timeout)
{
	timeval to;
	to.tv_sec = static_cast<long int>(timeout);
	to.tv_usec = static_cast<long int>((timeout - to.tv_sec)*1E6);
	act = des;
	return select(FD_SETSIZE, &act,
		      (fd_set *)0, (fd_set *)0, &to);
}


bool IOWatch::wait()
{
	act = des;
	return select(FD_SETSIZE, &act,
		      (fd_set *)0, (fd_set *)0, (timeval *)0);
}


bool IOWatch::isset(int fd)
{
	return FD_ISSET(fd, &act);
}



/////////////////////////////////////////////////////////////////////
//
// LyXDataSocket
//
/////////////////////////////////////////////////////////////////////

// Modified LyXDataSocket class for use with the client
class LyXDataSocket {
public:
	LyXDataSocket(FileName const &);
	~LyXDataSocket();
	// File descriptor of the connection
	int fd() const;
	// Connection status
	bool connected() const;
	// Line buffered input from the socket
	bool readln(string &);
	// Write the string + '\n' to the socket
	void writeln(string const &);
private:
	// File descriptor for the data socket
	int fd_;
	// True if the connection is up
	bool connected_;
	// buffer for input data
	string buffer;
};


LyXDataSocket::LyXDataSocket(FileName const & address)
{
	if ((fd_ = socktools::connect(address)) == -1) {
		connected_ = false;
	} else {
		connected_ = true;
	}
}


LyXDataSocket::~LyXDataSocket()
{
	::close(fd_);
}


int LyXDataSocket::fd() const
{
	return fd_;
}


bool LyXDataSocket::connected() const
{
	return connected_;
}


// Returns true if there was a complete line to input
// A line is of the form <key>:<value>
//   A line not of this form will not be passed
// The line read is splitted and stored in 'key' and 'value'
bool LyXDataSocket::readln(string & line)
{
	int const charbuf_size = 100;
	char charbuf[charbuf_size]; // buffer for the ::read() system call
	int count;
	string::size_type pos;

	// read and store characters in buffer
	while ((count = ::read(fd_, charbuf, charbuf_size - 1)) > 0) {
		charbuf[count] = '\0'; // turn it into a c string
		buffer += charbuf;
	}

	// Error conditions. The buffer must still be
	// processed for lines read
	if (count == 0) { // EOF -- connection closed
		connected_ = false;
	} else if ((count == -1) && (errno != EAGAIN)) { // IO error
		cerr << "lyxclient: IO error." << endl;
		connected_ = false;
	}

	// Cut a line from buffer
	if ((pos = buffer.find('\n')) == string::npos)
		return false; // No complete line stored
	line = buffer.substr(0, pos);
	buffer = buffer.substr(pos + 1);
	return true;
}


// Write a line of the form <key>:<value> to the socket
void LyXDataSocket::writeln(string const & line)
{
	string linen(line + '\n');
	int size = linen.size();
	int written = ::write(fd_, linen.c_str(), size);
	if (written < size) { // Allways mean end of connection.
		if ((written == -1) && (errno == EPIPE)) {
			// The program will also receive a SIGPIPE
			// that must be catched
			cerr << "lyxclient: connection closed while writing."
			     << endl;
		} else {
			// Anything else, including errno == EAGAIN, must be
			// considered IO error. EAGAIN should never happen
			// when line is small
			cerr << "lyxclient: IO error: " << strerror(errno);
		}
		connected_ = false;
	}
}


/////////////////////////////////////////////////////////////////////
//
// CmdLineParser
//
/////////////////////////////////////////////////////////////////////

class CmdLineParser {
public:
	typedef int (*optfunc)(vector<docstring> const & args);
	map<string, optfunc> helper;
	map<string, bool> isset;
	bool parse(int, char * []);
	vector<char *> nonopt;
};


bool CmdLineParser::parse(int argc, char * argv[])
{
	int opt = 1;
	while (opt < argc) {
		vector<docstring> args;
		if (helper[argv[opt]]) {
			isset[argv[opt]] = true;
			int arg = opt + 1;
			while ((arg < argc) && (!helper[argv[arg]])) {
				args.push_back(from_local8bit(argv[arg]));
				++arg;
			}
			int taken = helper[argv[opt]](args);
			if (taken == -1)
				return false;
			opt += 1 + taken;
		} else {
			if (argv[opt][0] == '-') {
				if ((argv[opt][1] == '-')
				   && (argv[opt][2]== '\0')) {
					++opt;
					while (opt < argc) {
						nonopt.push_back(argv[opt]);
						++opt;
					}
					return true;
				} else {
					cerr << "lyxclient: unknown option "
					     << argv[opt] << endl;
					return false;
				}
			}
			nonopt.push_back(argv[opt]);
			++opt;
		}
	}
	return true;
}
// ~Class CmdLineParser -------------------------------------------------------



namespace cmdline {

void usage()
{
	cerr <<
		"Usage: lyxclient [options]\n"
	  "Options are:\n"
	  "  -a address    set address of the lyx socket\n"
	  "  -t directory  set system temporary directory\n"
	  "  -p pid        select a running lyx by pidi\n"
	  "  -c command    send a single command and quit\n"
	  "  -g file row   send a command to go to file and row\n"
	  "  -n name       set client name\n"
	  "  -h name       display this help end exit\n"
	  "If -a is not used, lyxclient will use the arguments of -t and -p to look for\n"
	  "a running lyx. If -t is not set, 'directory' defaults to /tmp. If -p is set,\n"
	  "lyxclient will connect only to a lyx with the specified pid. Options -c and -g\n"
	  "cannot be set simultaneoulsly. If no -c or -g options are given, lyxclient\n"
	  "will read commands from standard input and disconnect when command read is BYE:"
	   << endl;
}


int h(vector<docstring> const &)
{
	usage();
	exit(0);
}


docstring clientName =
	from_ascii(itoa(::getppid()) + ">" + itoa(::getpid()));

int n(vector<docstring> const & arg)
{
	if (arg.size() < 1) {
		cerr << "lyxclient: The option -n requires 1 argument."
		     << endl;
		return -1;
	}
	clientName = arg[0];
	return 1;
}


docstring singleCommand;


int c(vector<docstring> const & arg)
{
	if (arg.size() < 1) {
		cerr << "lyxclient: The option -c requires 1 argument."
		     << endl;
		return -1;
	}
	singleCommand = arg[0];
	return 1;
}


int g(vector<docstring> const & arg)
{
	if (arg.size() < 2) {
		cerr << "lyxclient: The option -g requires 2 arguments."
		     << endl;
		return -1;
	}
	singleCommand = "LYXCMD:server-goto-file-row "
		+ arg[0] + ' '
		+ arg[1];
	return 2;
}


// empty if LYXSOCKET is not set in the environment
docstring serverAddress;


int a(vector<docstring> const & arg)
{
	if (arg.size() < 1) {
		cerr << "lyxclient: The option -a requires 1 argument."
		     << endl;
		return -1;
	}
	// -a supercedes LYXSOCKET environment variable
	serverAddress = arg[0];
	return 1;
}


docstring mainTmp(from_ascii("/tmp"));


int t(vector<docstring> const & arg)
{
	if (arg.size() < 1) {
		cerr << "lyxclient: The option -t requires 1 argument."
		     << endl;
		return -1;
	}
	mainTmp = arg[0];
	return 1;
}


string serverPid; // Init to empty string


int p(vector<docstring> const & arg)
{
	if (arg.size() < 1) {
		cerr << "lyxclient: The option -p requires 1 argument."
		     << endl;
		return -1;
	}
	serverPid = to_ascii(arg[0]);
	return 1;
}


} // namespace cmdline
} // namespace lyx


int main(int argc, char * argv[])
{
	using namespace lyx;
	lyxerr.setStream(cerr);

	char const * const lyxsocket = getenv("LYXSOCKET");
	if (lyxsocket)
		cmdline::serverAddress = from_local8bit(lyxsocket);

	CmdLineParser args;
	args.helper["-h"] = cmdline::h;
	args.helper["-c"] = cmdline::c;
	args.helper["-g"] = cmdline::g;
	args.helper["-n"] = cmdline::n;
	args.helper["-a"] = cmdline::a;
	args.helper["-t"] = cmdline::t;
	args.helper["-p"] = cmdline::p;

	// Command line failure conditions:
	if ((!args.parse(argc, argv))
	   || (args.isset["-c"] && args.isset["-g"])
	   || (args.isset["-a"] && args.isset["-p"])) {
		cmdline::usage();
		return 1;
	}

	scoped_ptr<LyXDataSocket> server;

	if (!cmdline::serverAddress.empty()) {
		server.reset(new LyXDataSocket(FileName(to_utf8(cmdline::serverAddress))));
		if (!server->connected()) {
			cerr << "lyxclient: " << "Could not connect to "
			     << to_utf8(cmdline::serverAddress) << endl;
			return EXIT_FAILURE;
		}
	} else {
		// We have to look for an address.
		// serverPid can be empty.
		FileNameList addrs = lyxSockets(to_filesystem8bit(cmdline::mainTmp), cmdline::serverPid);
		FileNameList::const_iterator addr = addrs.begin();
		FileNameList::const_iterator end = addrs.end();
		for (; addr != end; ++addr) {
			// Caution: addr->string() is in filesystem encoding
			server.reset(new LyXDataSocket(*addr));
			if (server->connected())
				break;
			lyxerr << "lyxclient: " << "Could not connect to "
			     << addr->absFilename() << endl;
		}
		if (addr == end) {
			lyxerr << "lyxclient: No suitable server found."
			       << endl;
			return EXIT_FAILURE;
		}
		cerr << "lyxclient: " << "Connected to " << addr->absFilename() << endl;
	}

	int const serverfd = server->fd();

	IOWatch iowatch;
	iowatch.addfd(serverfd);

	// Used to read from server
	string answer;

	// Send greeting
	server->writeln("HELLO:" + to_utf8(cmdline::clientName));
	// wait at most 2 seconds until server responds
	iowatch.wait(2.0);
	if (iowatch.isset(serverfd) && server->readln(answer)) {
		if (prefixIs(answer, "BYE:")) {
			cerr << "lyxclient: Server disconnected." << endl;
			cout << answer << endl;
			return EXIT_FAILURE;
		}
	} else {
		cerr << "lyxclient: No answer from server." << endl;
		return EXIT_FAILURE;
	}

	if (args.isset["-g"] || args.isset["-c"]) {
		server->writeln(to_utf8(cmdline::singleCommand));
		iowatch.wait(2.0);
		if (iowatch.isset(serverfd) && server->readln(answer)) {
			cout << answer;
			if (prefixIs(answer, "ERROR:"))
				return EXIT_FAILURE;
			return EXIT_SUCCESS;
		} else {
			cerr << "lyxclient: No answer from server." << endl;
			return EXIT_FAILURE;
		}
	}

	// Take commands from stdin
	iowatch.addfd(0); // stdin
	bool saidbye = false;
	while ((!saidbye) && server->connected()) {
		iowatch.wait();
		if (iowatch.isset(0)) {
			string command;
			getline(cin, command);
			if (command == "BYE:") {
				server->writeln("BYE:");
				saidbye = true;
			} else {
				server->writeln("LYXCMD:" + command);
			}
		}
		if (iowatch.isset(serverfd)) {
			while(server->readln(answer))
				cout << answer << endl;
		}
	}

	return EXIT_SUCCESS;
}


namespace boost {

void assertion_failed(char const* a, char const* b, char const* c, long d)
{
	lyx::lyxerr << "Assertion failed: " << a << ' ' << b << ' ' << c << ' '
		<< d << '\n';
}

} // namespace boost