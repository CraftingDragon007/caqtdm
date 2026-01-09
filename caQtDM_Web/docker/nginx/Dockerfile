FROM nginx:mainline-alpine

RUN apk add --no-cache openssl

ENV INSTALL_HOSTNAME=localhost
ENV INSTALL_CERT_BASENAME=caqtdm_web
ENV PROXY_BACKEND=http://127.0.0.1

RUN mkdir -p /etc/nginx/ssl

COPY user /var/www/caqtdm_web/user
COPY admin /var/www/caqtdm_web/admin
COPY caqtdm_web.conf.template /etc/nginx/templates/caqtdm_web.conf.template

RUN sed -i 's|_INSTALL_HOSTNAME_|${INSTALL_HOSTNAME}|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|_INSTALL_CERT_BASENAME_|${INSTALL_CERT_BASENAME}|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|http://127\.0\.0\.1|${PROXY_BACKEND}|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|/etc/pki/tls/certs|/etc/nginx/ssl|g' /etc/nginx/templates/caqtdm_web.conf.template && \
    sed -i 's|/etc/pki/tls/private|/etc/nginx/ssl|g' /etc/nginx/templates/caqtdm_web.conf.template

RUN echo 'resolver ${DNS_SERVER} ipv6=off;' > /etc/nginx/templates/resolvers.conf.template

RUN openssl genrsa -out /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.key 2048 && \
    chmod 600 /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.key && \
    openssl req -x509 -new -nodes \
        -key /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.key \
        -sha256 -days 365 \
        -subj "/C=CH/ST=Aargau/L=Villigen PSI/O=Default Org/OU=caqtdm_web/CN=${INSTALL_HOSTNAME}" \
        -out /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.crt && \
    chmod 644 /etc/nginx/ssl/${INSTALL_CERT_BASENAME}.crt

RUN chown -R nginx:nginx /var/www/caqtdm_web && \
    chmod -R 755 /var/www/caqtdm_web && \
    find /var/www/caqtdm_web -type f -exec chmod 644 {} +

COPY docker/nginx/15-local-resolvers.envsh /docker-entrypoint.d/15-local-resolvers.envsh
RUN chmod +x /docker-entrypoint.d/15-local-resolvers.envsh