%define version_suffix ""

Summary: A WYSIWYM (What You See Is What You Mean) document processor
Name: lyx
Version: 2.0.1
Release: 2
License: GPL
Group: Applications/Publishing
URL: http://www.lyx.org/
Packager: The LyX Team <lyx-devel@lists.lyx.org>
Source: ftp://ftp.lyx.org/pub/lyx/stable/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Prefix: %{_prefix}
Obsoletes: tetex-lyx

%description
LyX is a document processor that encourages an approach to writing
based on the structure of your documents, not their appearance. It
is released under a Free Software/Open Source license (GPL v.2).

LyX is for people that write and want their writing to look great,
right out of the box. No more endless tinkering with formatting
details, 'finger painting' font attributes or futzing around with
page boundaries. You just write. In the background, Prof. Knuth's
legendary TeX typesetting engine makes you look good.

On screen, LyX looks like any word processor; its printed output --
or richly cross-referenced PDF, just as readily produced -- looks
like nothing else. Gone are the days of industrially bland .docs,
all looking similarly not-quite-right, yet coming out unpredictably
different on different printer drivers. Gone are the crashes
'eating' your dissertation the evening before going to press.

LyX is stable and fully featured. It is a multi-platform, fully
internationalized application running natively on Unix/Linux, the
Macintosh and modern Windows platforms.
%prep
%setup

%build
unset LINGUAS || true
./configure --prefix=%{_prefix} \
	--mandir=%{_mandir} --bindir=%{_bindir} --datadir=%{_datadir} \
	--enable-build-type=rel \
	%{version_suffix}
make

%install
unset LINGUAS || true
rm -rf ${RPM_BUILD_ROOT}
install -d -m 755 ${RPM_BUILD_ROOT}
make DESTDIR=${RPM_BUILD_ROOT} install
gzip -f9 ${RPM_BUILD_ROOT}%{_mandir}/man?/*

#
# Set up the lyx-specific class files where TeX can see them
#
TEXMF=%{_datadir}/texmf
mkdir -p ${RPM_BUILD_ROOT}${TEXMF}/tex/latex
mv ${RPM_BUILD_ROOT}%{_datadir}/%{name}/tex \
      ${RPM_BUILD_ROOT}/${TEXMF}/tex/latex/%{name}

#
# Miscellaneous files
#
# Don't use %{name}.png, this will not work with --version-suffix
cp -a lib/images/lyx.png ${RPM_BUILD_ROOT}%{_datadir}/%{name}/images/

%clean
rm -rf ${RPM_BUILD_ROOT}

%post
# Make TeX understand where LyX-specific packages are
texhash

%postun
# Fix the TeX file hash
#
texhash

%files
%defattr(-,root,root)
%doc ABOUT-NLS ANNOUNCE COPYING
%doc README UPGRADING NEWS
%doc lib/CREDITS
%{_bindir}/*
%{_mandir}/man?/*
%{_datadir}/locale/*/LC_MESSAGES/*
%{_datadir}/%{name}
%{_datadir}/texmf/tex/latex/%{name}
