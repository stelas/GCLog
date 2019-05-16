Name:           gclog
Version:        0.2.5
Release:        1%{?dist}
Summary:        Geiger Counter Logger
License:        GPL-3.0-or-later
Group:          Applications/System
Url:            http://www.dateihal.de/cms/gclog
Source0:        http://www.dateihal.de/%{name}/%{name}-%{version}.tar.gz

%if 0%{?suse_version}
PreReq: %insserv_prereq
%endif

%description
GCLOG is a lightweight daemon running in the background and
constantly reporting your Geiger counter readings to various
radiation monitoring websites.

%global debug_package %{nil}

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%clean
make clean

%preun
/sbin/service gclogd stop || :
if [ "$1" = "0" ]; then
/sbin/chkconfig --del gclogd
fi

%post
if [ "$1" = "1" ]; then
/sbin/chkconfig --add gclogd
fi

%files
%defattr(-, root, root)
%{_bindir}/gclog
%{_sysconfdir}/init.d/gclogd
%config(noreplace) %{_sysconfdir}/gclog.conf
%{_mandir}/man1/gclog.1*
%doc README* CHANGELOG TODO COPYING

%changelog
* Mon Apr 22 2019 Steffen Lange <gclog@stelas.de> 0.2.5-1
- More minor changes
* Sat Apr 14 2018 Steffen Lange <gclog@stelas.de> 0.2.4-1
- Minor enhancements
* Sun Feb 28 2016 Steffen Lange <gclog@stelas.de> 0.2.3-1
- Minor fixes
* Sat Apr 25 2015 Steffen Lange <gclog@stelas.de> 0.2.2-1
- New upstream release
* Wed Mar 18 2015 Steffen Lange <gclog@stelas.de> 0.2.1-1
- Initial package of version 0.2.1
