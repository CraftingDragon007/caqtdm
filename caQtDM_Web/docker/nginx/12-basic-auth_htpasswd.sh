#!/bin/sh
if [ -n "$BASIC_AUTH_USER" ] && [ -n "$BASIC_AUTH_PASSWORD" ]; then
    echo "Creating .htpasswd file for user: $BASIC_AUTH_USER"

    htpasswd -cb -B /etc/nginx/.htpasswd "$BASIC_AUTH_USER" "$BASIC_AUTH_PASSWORD"

    echo "Basic authentication has been configured."
else
    echo "BASIC_AUTH_USER or BASIC_AUTH_PASSWORD not set. Skipping Basic Auth setup."
fi
