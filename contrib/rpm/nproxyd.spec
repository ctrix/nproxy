
# NOTE:
# This should work but it's missing the init script and the log dir in the config file is wrong.

# norootforbuild

Name:           nproxyd
Version:        2013.05
Release:        1
Summary:        HTTP Programmable Proxy Daemon
Group:          Network
License:        MPL 1.1
Url:            https://github.com/ctrix/nproxy
BuildRequires:  cmake
PreReq:         lua-devel apr-devel 
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
   -DNPROXY_CONFIG_DIR="/etc/nproxy" \
   -DNPROXY_STATE_DIR:STRING="/var/run" \
   -DNPROXY_LOG_DIR:STRING="/var/log/nproxy" \
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

%pre
getent group  nproxy >/dev/null || groupadd -f -r nproxy
getent passwd nproxy >/dev/null || useradd -r -M -d / -s /sbin/nologin -c "NProxy system user" -g nproxy nproxy

%changelog
* Wed May 13 2013 mcetra@gmail.com
   Initial release

