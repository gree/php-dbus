%define git_repo php-dbus
%define git_head xrg


%define build_test 1
%{?_with_test: %{expand: %%global build_test 1}}
%{?_without_test: %{expand: %%global build_test 0}}
%define epoch 3
%define major 5
%define libname %mklibname php5_common %{major}

Summary:	DBus binding for PHP
Name:		php-dbus
# Version:	%(php-config --version)
Version:	%git_get_ver
Release:	%mkrel %git_get_rel
Group:		Development/PHP
License:	PHP License 3.0
URL:		http://labs.gree.jp/Top/OpenSource/DBus-en.html
Source0:	%git_bs_source %{name}-%{version}.tar.gz
BuildRequires:  php-devel
BuildRequires:  dbus-devel
BuildRequires:	autoconf2.5
BuildRequires:	automake1.7
BuildRequires:	libtool
BuildRequires:	multiarch-utils >= 1.0.3
Requires:	%{libname} >= %{epoch}:%{version}
Epoch:		0
#BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
PHP Dbus is a PHP extension to handle D-Bus functions. This module 
provides D-Bus interfaces via PHP classes and enables Inter Porcess 
Communication in D-Bus (currently, this module is mainly tested 
w/ Skype API communication on Linux).

%prep
%git_get_source
%setup -q
%{_bindir}/phpize

%build
%configure
%make

%install
rm -rf %{buildroot}

install -d %{buildroot}%{_libdir}/php/extensions
install -d %{buildroot}%{_sysconfdir}/php.d
echo "extension = dbus.so"	> %{buildroot}%{_sysconfdir}/php.d/50_dbus.ini
%makeinstall INSTALL_ROOT=%{buildroot}

%post
if [ -f /var/lock/subsys/httpd ]; then
    %{_initrddir}/httpd restart >/dev/null || :
fi

%postun
if [ "$1" = "0" ]; then
    if [ -f /var/lock/subsys/httpd ]; then
        %{_initrddir}/httpd restart >/dev/null || :
    fi
fi

%files
%defattr(-,root,root)
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/php.d/50_dbus.ini
%attr(0755,root,root) %{_libdir}/php/extensions/dbus.so
