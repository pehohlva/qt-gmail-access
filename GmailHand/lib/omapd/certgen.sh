#!/bin/bash

cn=omapd

if [ $1"X" != "X" ]; then
    cn=$1
fi

openssl req -x509 -new -newkey rsa:1024 -keyout server.key -out server.pem -nodes -subj /C=US/O=omapdServer/OU=OpenSource/CN=$cn/emailAddress=omapd@omapd.org -days 365

openssl x509 -in server.pem -subject -noout
