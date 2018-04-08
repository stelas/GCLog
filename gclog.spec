Name:           gclog
Version:        0.2.3
Release:        1%{?dist}
Summary:        Geiger Counter Logger
License:        GPLv3
Group:          Applications/System
Url:            http://www.dateihal.de/cms/gclog
Source0:        http://www.dateihal.de/%{name}/%{name}-%{version}.tar.gz

%description
GCLOG is a lightweight daemon running in the background and
constantly reporting your Geiger counter readings to various
radiation monitoring websites.

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
* Sun Feb 28 2016 Steffen Lange <gclog@stelas.de> 0.2.3-1
- Minor fixes
* Sat Apr 25 2015 Steffen Lange <gclog@stelas.de> 0.2.2-1
- New upstream release
* Wed Mar 18 2015 Steffen Lange <gclog@stelas.de> 0.2.1-1
- Initial package of version 0.2.1
