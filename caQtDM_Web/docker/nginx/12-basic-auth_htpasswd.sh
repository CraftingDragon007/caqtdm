#!/bin/sh

set -eu

has_any_auth=0
default_file="/etc/nginx/.htpasswd"
per_route_map="${BASIC_AUTH_PATH_MAP:-}"
default_initialized=0

# Legacy/global credentials: keep existing behavior for single-instance deployments.
if [ -n "${BASIC_AUTH_USER:-}" ] && [ -n "${BASIC_AUTH_PASSWORD:-}" ]; then
    echo "Creating global .htpasswd file for user: ${BASIC_AUTH_USER}"
    htpasswd -cb -B "$default_file" "$BASIC_AUTH_USER" "$BASIC_AUTH_PASSWORD"
    has_any_auth=1
    default_initialized=1
fi

# Optional per-route credentials. BASIC_AUTH_PATH_MAP line format:
#   ~^/sls/ SLS;
# Variables per key:
#   BASIC_AUTH_USER_SLS / BASIC_AUTH_PASSWORD_SLS
if [ -n "$per_route_map" ]; then
    echo "Configuring per-route htpasswd files from BASIC_AUTH_PATH_MAP"

    while IFS= read -r line; do
        trimmed=$(printf '%s' "$line" | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')
        [ -z "$trimmed" ] && continue
        case "$trimmed" in
            \#*) continue ;;
        esac

        key=$(printf '%s' "$trimmed" | awk '{print $2}' | sed 's/;$//')
        [ -z "$key" ] && continue

        user_var="BASIC_AUTH_USER_${key}"
        pass_var="BASIC_AUTH_PASSWORD_${key}"

        eval "route_user=\${$user_var:-}"
        eval "route_pass=\${$pass_var:-}"

        [ -z "$route_user" ] && continue
        [ -z "$route_pass" ] && continue

        route_file="/etc/nginx/.htpasswd-${key}"
        echo "Creating per-route .htpasswd file: ${route_file}"
        htpasswd -cb -B "$route_file" "$route_user" "$route_pass"

        # Keep a fallback file for non-subroute paths; append per-route users.
        if [ "$default_initialized" -eq 0 ]; then
            htpasswd -cb -B "$default_file" "$route_user" "$route_pass"
            default_initialized=1
        else
            htpasswd -b -B "$default_file" "$route_user" "$route_pass"
        fi

        has_any_auth=1
    done <<EOF
$per_route_map
EOF
fi

if [ "$has_any_auth" -eq 1 ]; then
    echo "Basic authentication has been configured."
else
    echo "No auth credentials configured. Skipping Basic Auth setup."
fi
