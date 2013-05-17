
# NOTE. THIS IS NOT YET TESTED AND FOR SURE IT MUST BE FIXED. TODO

# norootforbuild

Name:           nproxyd
Version:        0.1
Release:        1
Summary:        HTTP Programmable Proxy Daemon
Group:          Network
License:        Boh?
Url:            https://github.com/ctrix/nproxy
BuildRequires:  cmake
PreReq:         lua-devel
Source:         %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}--%{release}-root
AutoReqProv:    on

%define nproxydsrcdirname %{name}-%{version}-src

%description
HTTP Programmable Proxy Daemon

%prep
%setup -q -n %{nproxydsrcdirname}

%build
cd ..
rm -fr build_tree
mkdir build_tree
cd build_tree
cmake -DCMAKE_INSTALL_PREFIX="%{_prefix}" \
   -DLIB_INSTALL_DIR="%{_libdir}" \
   -DNPROXY_CONFIG_DIR="%{_sysconfdir}/nproxy/" \
   -DNPROXY_STATE_DIR:STRING="%{_var}/run/nproxy" \
   -DNPROXY_LOG_DIR:STRING="%{_var}/log/nproxy" \
   -DNPROXY_USER:STRING="nproxy" \
   -DNPROXY_GROUP:STRING="nproxy" ../%{nproxydsrcdirname}
make %{?_smp_mflags}

%install
cd ../build_tree
make install DESTDIR=%{_tmppath}/%{name}-%{version}--%{release}-root
mkdir -p %{_tmppath}/%{name}-%{version}--%{release}-root/var/log/nproxy

%clean
rm -rf ${nproxydsrcdirname}
rm -rf build_tree

%files
%defattr(-,root,root)
%attr(770,nproxy,nproxy) %{_sbindir}/nproxyd
%dir %attr(770,nproxy,nproxy) %{_sysconfdir}/nproxy
%dir %attr(770,nproxy,nproxy) %{_sysconfdir}/nproxy/scripts
%attr(770,nproxy,nproxy) %config(noreplace) %{_sysconfdir}/nproxy/*
%attr(770,nproxy,nproxy) %config(noreplace) %{_sysconfdir}/nproxy/scripts/*
%dir %attr(770,nproxy,nproxy) %{_var}/log/nproxy
%dir %attr(770,nproxy,nproxy) %{_datadir}/nproxy
%dir %attr(770,nproxy,nproxy) %{_datadir}/nproxy/templates
%attr(770,nproxy,nproxy) %{_datadir}/nproxy/templates/*

#%config(noreplace) %{_sysconfdir}/init.d/nproxy
#%config(noreplace) %{_sysconfdir}/default/nproxy
#%config(noreplace) %{_sysconfdir}/logrotate.d/nproxy
#%dir %attr(770,nproxy,nproxy) %{_sysconfdir}/monit.d
#%attr(770,nproxy,nproxy) %config(noreplace) %{_sysconfdir}/monit.d/*
#%doc %{_docdir}/nproxy-%{version}

%changelog
* Wed Mar 23 2010 foo@bar
   Initial release

