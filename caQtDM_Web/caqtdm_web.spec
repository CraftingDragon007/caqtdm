%if 0%{?fedora}
%global qt_major 6
%global qt_pkg_prefix qt6
%elif 0%{?rhel} && 0%{?rhel} < 10
%global qt_major 5
%global qt_pkg_prefix qt5
%else
%global qt_major 6
%global qt_pkg_prefix qt6
%endif

Name:           caqtdm_web
Version:        1.0.0
Release:        1%{?dist}
Summary:        Nginx configuration and static files for an application, with dynamic hostname and self-signed certificates.

License:        GPLv3+
URL:            https://caqtdm.github.io/
Source0:        caqtdm_web.tar.gz

BuildArch:      noarch
# This package contains only configuration and static content.

Requires:       nginx
Requires:       openssl
Requires:       caqtdm-bin-qt%{qt_major}
Requires:       qt-novnc-platform-plugin

%description
This RPM package provides the Nginx configuration file for the application.
It dynamically sets the Nginx `server_name` to the machine's Fully Qualified Domain Name (FQDN)
at install time. It also generates a 1-year self-signed TLS certificate and private key
for HTTPS, placing them in /etc/pki/tls/.
Static web content from the admin and user directories is served from /var/www/%{name}/
and dynamic WebSocket traffic is proxied.

# Define constants for clarity and maintainability
%define APP_NAME caqtdm_web
%define WEB_ROOT /var/www/%{APP_NAME}
%define NGINX_CONF_DIR /etc/nginx/conf.d
%define NGINX_CONF_FILE %{NGINX_CONF_DIR}/%{APP_NAME}.conf
%define CERT_DIR /etc/pki/tls/certs
%define KEY_DIR /etc/pki/tls/private
%define CERT_BASENAME %{APP_NAME}
%define CERT_FILE %{CERT_DIR}/%{CERT_BASENAME}.crt
%define KEY_FILE %{KEY_DIR}/%{CERT_BASENAME}.key

%prep
%setup -q -c %{name}-%{version}
tar -xzf %{SOURCE0} -C .


%build
# Nothing to compile for static content and configuration.


%install
install -d %{buildroot}%{NGINX_CONF_DIR}
install -d %{buildroot}%{WEB_ROOT}/admin
install -d %{buildroot}%{WEB_ROOT}/user
install -d %{buildroot}%{CERT_DIR}
install -d %{buildroot}%{KEY_DIR}

install -m 0644 %{APP_NAME}.conf.template  %{buildroot}%{NGINX_CONF_FILE}.template

cp -r admin/* %{buildroot}%{WEB_ROOT}/admin/
cp -r user/* %{buildroot}%{WEB_ROOT}/user/

chmod -R 0755 %{buildroot}%{WEB_ROOT}


%post
# $1 is 1 for initial install, 2 for upgrade
if [ "$1" -ge 1 ]; then
    echo "--- %{APP_NAME} post-installation script ---"

    FQDN=$(hostname -f)
    if [ -z "$FQDN" ]; then
        echo "WARNING: Could not determine FQDN. Using 'localhost' as fallback." >&2
        FQDN="localhost"
    fi
    echo "Determined FQDN for Nginx: ${FQDN}"

    echo "Generating Nginx configuration file: %{NGINX_CONF_FILE}"
    sed -e "s/_INSTALL_HOSTNAME_/${FQDN}/g" \
        -e "s/_INSTALL_CERT_BASENAME_/%{CERT_BASENAME}/g" \
        %{NGINX_CONF_FILE}.template > %{NGINX_CONF_FILE}

    chmod 0644 %{NGINX_CONF_FILE}
    chown root:root %{NGINX_CONF_FILE}
    echo "Nginx configuration updated with ${FQDN}."

    echo "Generating self-signed TLS certificate and private key..."

    if [ ! -f "%{KEY_FILE}" ] || [ ! -f "%{CERT_FILE}" ]; then
        echo "Creating new self-signed certificate and key for ${FQDN}..."
        openssl genrsa -out %{KEY_FILE} 2048
        chmod 0600 %{KEY_FILE}
        chown root:root %{KEY_FILE}

        openssl req -x509 -new -nodes -key %{KEY_FILE} -sha256 -days 365 \
            -subj "/C=CH/ST=Aargau/L=Villigen PSI/O=Paul Scherrer Institut/OU=%{APP_NAME}/CN=${FQDN}" \
            -out %{CERT_FILE}

        chmod 0644 %{CERT_FILE}
        chown root:root %{CERT_FILE}
        echo "Self-signed certificate and key generated: %{CERT_FILE} and %{KEY_FILE}."
    else
        echo "Existing certificate (%{CERT_FILE}) and key (%{KEY_FILE}) found. Skipping generation."
    fi

    echo "Setting ownership and permissions for web root %{WEB_ROOT}..."
    chown -R nginx:nginx %{WEB_ROOT}
    chmod -R 0755 %{WEB_ROOT}
    find %{WEB_ROOT} -type f -exec chmod 0644 {} +

    # Reload Nginx to apply changes
    echo "Attempting to reload Nginx service..."
    systemctl try-reload-or-restart nginx >/dev/null 2>&1 || {
        echo "WARNING: Nginx service reload failed. Please check Nginx configuration and logs." >&2
    }
fi


%preun
# $1 is 0 for uninstall, 1 for upgrade
if [ "$1" -eq 0 ]; then # This block runs only on uninstallation (not upgrade)
    echo "--- %{APP_NAME} pre-uninstallation script ---"

    # Check Nginx config validity before allowing package removal
    # This prevents leaving Nginx in a broken state if the config is invalid
    systemctl --no-pager is-active nginx >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "Checking Nginx configuration validity before uninstallation..."
        if nginx -t >/dev/null 2>&1; then
            echo "Nginx configuration OK. Proceeding with removal."
        else
            echo "Nginx configuration error detected. Aborting removal to prevent breaking Nginx."
            exit 1
        fi
    fi

    echo "Removing generated certificates and Nginx configuration file..."
    rm -f %{KEY_FILE} %{CERT_FILE}
    rm -f %{NGINX_CONF_FILE} %{NGINX_CONF_FILE}.template


    systemctl try-reload-or-restart nginx >/dev/null 2>&1 || :
fi


%files
%attr(0644, root, root) %{NGINX_CONF_FILE}.template
%ghost %attr(0644, root, root) %config(noreplace) %{NGINX_CONF_FILE}
# The template itself is not part of the installed package.

%ghost %attr(0644, root, root) %{CERT_FILE}
%ghost %attr(0600, root, root) %{KEY_FILE}

# Web root directory and its content
%dir %attr(0755, nginx, nginx) %{WEB_ROOT}
%attr(0755, nginx, nginx) %{WEB_ROOT}/admin
%attr(0755, nginx, nginx) %{WEB_ROOT}/user
# Recursively include all files within the static content directories
%{WEB_ROOT}/admin/
%{WEB_ROOT}/user/

%changelog
* Thu Nov 27 2025 Julian Houba <info@craftingdragon.ch> - 1.0.0-1
- Initial RPM release with dynamic hostname and self-signed certificate generation.
