FROM nginx:mainline-alpine

RUN apk update
RUN apk upgrade
RUN apk add --no-cache openssl apache2-utils

ENV INSTALL_HOSTNAME=localhost
ENV INSTALL_CERT_BASENAME=caqtdm_web

RUN mkdir -p /etc/nginx/ssl

RUN rm -f /etc/nginx/conf.d/default.conf

COPY user /var/www/caqtdm_web/user
COPY admin /var/www/caqtdm_web/admin
COPY caqtdm_web.conf.template /etc/nginx/templates/caqtdm_web.conf.template

RUN sed -i 's|_INSTALL_HOSTNAME_|${INSTALL_HOSTNAME}|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|_INSTALL_CERT_BASENAME_|${INSTALL_CERT_BASENAME}|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|http://127\.0\.0\.1|\${PROXY_BACKEND}|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|/etc/pki/tls/certs|/etc/nginx/ssl|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|/etc/pki/tls/private|/etc/nginx/ssl|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed '/server {/a \    ${AUTH_CONFIG}' /etc/nginx/templates/caqtdm_web.conf.template > /etc/nginx/templates/caqtdm_web.conf.tmp && \
    mv /etc/nginx/templates/caqtdm_web.conf.tmp /etc/nginx/templates/caqtdm_web.conf.template

RUN echo 'resolver ${DNS_SERVER} ipv6=off;' > /etc/nginx/templates/resolvers.conf.template

RUN openssl genrsa -out /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.key 2048 && \
    chmod 600 /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.key && \
    openssl req -x509 -new -nodes \
        -key /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.key \
        -sha256 -days 365 \
        -subj "/C=CH/ST=Aargau/L=Villigen PSI/O=Paul Scherrer Institut/OU=caqtdm_web/CN=${INSTALL_HOSTNAME}" \
        -out /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.crt && \
    chmod 644 /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.crt

RUN chown -R nginx:nginx /var/www/caqtdm_web && \
    chmod -R 755 /var/www/caqtdm_web && \
    find /var/www/caqtdm_web -type f -exec chmod 644 {} +

COPY docker/nginx/12-basic-auth_htpasswd.sh /docker-entrypoint.d/12-basic-auth-htpasswd.sh
RUN chmod +x /docker-entrypoint.d/12-basic-auth-htpasswd.sh
COPY docker/nginx/13-basic-auth_config.envsh /docker-entrypoint.d/13-basic-auth_config.envsh
RUN chmod +x /docker-entrypoint.d/13-basic-auth_config.envsh
COPY docker/nginx/15-local-resolvers.envsh /docker-entrypoint.d/15-local-resolvers.envsh
RUN chmod +x /docker-entrypoint.d/15-local-resolvers.envsh

ENV PROXY_BACKEND=http://caqtdm
