%define frontend qt4
%define version_suffix ""

Summary: A WYSIWYM (What You See Is What You Mean) frontend to LaTeX
Name: lyx
Version: 1.5.4
Release: 1_%{frontend}
License: see COPYING file
Group: Applications/Publishing
URL: http://www.lyx.org/
Packager: Kayvan A. Sylvan <kayvan@sylvan.com>
Source: ftp://ftp.lyx.org/pub/lyx/stable/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Icon: %{name}.xpm
Prefix: %{_prefix}
Obsoletes: tetex-lyx

%description
LyX is a modern approach to writing documents which breaks with the
obsolete "typewriter paradigm" of most other document preparation
systems.

It is designed for people who want professional quality output
with a minimum of time and effort, without becoming specialists in
typesetting.

The major innovation in LyX is WYSIWYM (What You See Is What You Mean).
That is, the author focuses on content, not on the details of formatting.
This allows for greater productivity, and leaves the final typesetting
to the backends (like LaTeX) that are specifically designed for the task.

With LyX, the author can concentrate on the contents of his writing,
and let the computer take care of the rest.

This is LyX built with the %{frontend} frontend.

%prep
%setup

%build
unset LINGUAS || true
./configure --with-frontend=%{frontend} --prefix=%{_prefix} \
	--mandir=%{_mandir} --bindir=%{_bindir} --datadir=%{_datadir} \
 	--without-warnings --disable-debug --enable-optimization=-O2 \
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
# Don't use %{name}.xpm, this will not work with --version-suffix
cp -a lib/images/lyx.xpm ${RPM_BUILD_ROOT}%{_datadir}/%{name}/images/

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
%doc README UPGRADING ChangeLog NEWS
%doc lib/CREDITS
%{_bindir}/*
%{_mandir}/man?/*
%{_datadir}/locale/*/LC_MESSAGES/*
%{_datadir}/%{name}
%{_datadir}/texmf/tex/latex/%{name}
