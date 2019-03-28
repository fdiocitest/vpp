ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
PKG=deb
else ifeq ($(filter rhel centos fedora opensuse opensuse-leap opensuse-tumbleweed,$(OS_ID)),$(OS_ID))
PKG=rpm
endif

# +libganglia1-dev if building the gmond plugin

DEB_DEPENDS  = curl build-essential autoconf automake ccache
DEB_DEPENDS += debhelper dkms git libtool libapr1-dev dh-systemd
DEB_DEPENDS += libconfuse-dev git-review exuberant-ctags cscope pkg-config
DEB_DEPENDS += lcov chrpath autoconf indent clang-format libnuma-dev
DEB_DEPENDS += python-all python3-all python3-setuptools python-dev
DEB_DEPENDS += python-virtualenv python-pip libffi6 check
DEB_DEPENDS += libboost-all-dev libffi-dev python3-ply libmbedtls-dev
DEB_DEPENDS += cmake ninja-build uuid-dev
ifeq ($(OS_VERSION_ID),14.04)
	DEB_DEPENDS += libssl-dev
else ifeq ($(OS_ID)-$(OS_VERSION_ID),debian-8)
	DEB_DEPENDS += libssl-dev
	APT_ARGS = -t jessie-backports
else ifeq ($(OS_ID)-$(OS_VERSION_ID),debian-9)
	DEB_DEPENDS += libssl1.0-dev
else
	DEB_DEPENDS += libssl-dev
endif

RPM_DEPENDS  = redhat-lsb glibc-static
RPM_DEPENDS += apr-devel
RPM_DEPENDS += numactl-devel
RPM_DEPENDS += check check-devel
RPM_DEPENDS += boost boost-devel
RPM_DEPENDS += selinux-policy selinux-policy-devel
RPM_DEPENDS += ninja-build
RPM_DEPENDS += libuuid-devel
RPM_DEPENDS += mbedtls-devel

ifeq ($(OS_ID),fedora)
	RPM_DEPENDS += dnf-utils
	RPM_DEPENDS += subunit subunit-devel
	RPM_DEPENDS += compat-openssl10-devel
	RPM_DEPENDS += python2-devel python34-ply
	RPM_DEPENDS += python2-virtualenv
	RPM_DEPENDS += cmake
	RPM_DEPENDS_GROUPS = 'C Development Tools and Libraries'
else
	RPM_DEPENDS += yum-utils
	RPM_DEPENDS += openssl-devel
	RPM_DEPENDS += python-devel python34-ply
	RPM_DEPENDS += python34-devel python34-pip
	RPM_DEPENDS += python-virtualenv
	RPM_DEPENDS += devtoolset-7
	RPM_DEPENDS += cmake3
	RPM_DEPENDS_GROUPS = 'Development Tools'
endif

# +ganglia-devel if building the ganglia plugin

RPM_DEPENDS += chrpath libffi-devel rpm-build

SUSE_NAME= $(shell grep '^NAME=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g' | cut -d' ' -f2)
SUSE_ID= $(shell grep '^VERSION_ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g' | cut -d' ' -f2)
RPM_SUSE_BUILDTOOLS_DEPS = autoconf automake ccache check-devel chrpath
RPM_SUSE_BUILDTOOLS_DEPS += clang cmake indent libtool make ninja python3-ply

RPM_SUSE_DEVEL_DEPS = glibc-devel-static libnuma-devel
RPM_SUSE_DEVEL_DEPS += libopenssl-devel openssl-devel mbedtls-devel libuuid-devel

RPM_SUSE_PYTHON_DEPS = python-devel python3-devel python-pip python3-pip
RPM_SUSE_PYTHON_DEPS += python-rpm-macros python3-rpm-macros

RPM_SUSE_PLATFORM_DEPS = distribution-release shadow rpm-build

ifeq ($(OS_ID),opensuse)
ifeq ($(SUSE_NAME),Tumbleweed)
	RPM_SUSE_DEVEL_DEPS = libboost_headers1_68_0-devel-1.68.0  libboost_thread1_68_0-devel-1.68.0 gcc
	RPM_SUSE_PYTHON_DEPS += python3-ply python2-virtualenv
endif
ifeq ($(SUSE_ID),15.0)
	RPM_SUSE_DEVEL_DEPS += libboost_headers-devel libboost_thread-devel gcc
	RPM_SUSE_PYTHON_DEPS += python3-ply python2-virtualenv
else
	RPM_SUSE_DEVEL_DEPS += libboost_headers1_68_0-devel-1.68.0 gcc6
	RPM_SUSE_PYTHON_DEPS += python-virtualenv
endif
endif

ifeq ($(OS_ID),opensuse-leap)
ifeq ($(SUSE_ID),15.0)
	RPM_SUSE_DEVEL_DEPS += libboost_headers-devel libboost_thread-devel gcc git curl
	RPM_SUSE_PYTHON_DEPS += python3-ply python2-virtualenv
endif
endif

RPM_SUSE_DEPENDS += $(RPM_SUSE_BUILDTOOLS_DEPS) $(RPM_SUSE_DEVEL_DEPS) $(RPM_SUSE_PYTHON_DEPS) $(RPM_SUSE_PLATFORM_DEPS)