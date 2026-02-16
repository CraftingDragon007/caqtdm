#!/usr/bin/env bash
openssl req -new -x509  -config certificate.config -newkey rsa:4096 -keyout caQtDM.pem -nodes -outform der -out caQtDM.der
chmod 600 caQtDM.pem
echo You can now move the key to the pki folder of caQtDM, which is in your local app data location for Paul Scherrer Institute