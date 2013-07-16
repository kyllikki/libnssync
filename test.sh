#!/bin/bash

USERNAME=vince@kyllikki.org
PASSWORD=insecure

USERID=$(./sha1base32 ${USERNAME})

STORAGE_SERVER=$(curl -s "https://auth.services.mozilla.com/user/1.0/${USERID}/node/weave")

curl "${STORAGE_SERVER}1.1/${USERID}/storage/meta/global?full=1" --basic -u ${USERID}:${PASSWORD}
