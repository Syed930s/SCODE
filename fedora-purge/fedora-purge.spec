Name:           fedora-purge
Version:        1.0.0
Release:        1%{?dist}
Summary:        Safe purge utilities for Fedora (RPM + Flatpak)

License:        GPL-3.0-or-later
URL:            https://github.com/yourname/fedora-purge
Source0:        %{name}-%{version}.tar.gz

BuildArch:      noarch
Requires:       flatpak
Requires:       dnf

%description
fedora-purge provides two safe, technician-grade purge utilities:

- dnf-purge: safely removes RPM packages with optional purge mode
- flatpurge: safely removes Flatpak apps and leftover sandbox data

Both tools include safety checks, user confirmation prompts, and
protection against removing critical system components.

%prep
%setup -q

%install
install -D -m 0755 dnf-purge %{buildroot}/usr/bin/dnf-purge
install -D -m 0755 flatpurge %{buildroot}/usr/bin/flatpurge

%files
/usr/bin/dnf-purge
/usr/bin/flatpurge
%license LICENSE
%doc README.md

%changelog
* Sat Apr 18 2026 Syed <195543322+Syed930s@users.noreply.github.com> - 1.0.0-1
- Initial public release of fedora-purge
