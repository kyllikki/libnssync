#!/bin/bash

USERNAME=vince@kyllikki.org
PASSWORD=insecure

USERID=$(./sha1base32 ${USERNAME})

STORAGE_SERVER=$(curl -s "https://auth.services.mozilla.com/user/1.0/${USERID}/node/weave")

COLLECTIONS=$(curl "${STORAGE_SERVER}1.1/${USERID}/info/collections" --basic -u ${USERID}:${PASSWORD})

META_GLOBAL=$(curl "${STORAGE_SERVER}1.1/${USERID}/storage/meta/global" --basic -u ${USERID}:${PASSWORD})

BOOKMARKS_COLLECTION=$(curl "${STORAGE_SERVER}1.1/${USERID}/storage/bookmarks" --basic -u ${USERID}:${PASSWORD})

STORE_OBJ=$(curl "${STORAGE_SERVER}1.1/${USERID}/storage/bookmarks/-NuNnJomUB6a" --basic -u ${USERID}:${PASSWORD})

CRYPTO_KEYS=$(curl "${STORAGE_SERVER}1.1/${USERID}/storage/crypto/keys" --basic -u ${USERID}:${PASSWORD})

echo ${COLLECTIONS}
echo
echo ${META_GLOBAL}
echo
echo ${BOOKMARKS_COLLECTION}
echo
echo ${STORE_OBJ}
echo
echo ${CRYPTO_KEYS}

