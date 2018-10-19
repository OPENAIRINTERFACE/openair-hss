#! /bin/bash

#Copyright (c) 2017 Sprint
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

rm -rf demoCA
mkdir demoCA
echo 01 > demoCA/serial
touch demoCA/index.txt

if [ "$#" -lt 2 ]; then
  echo "error provide arguments: host domain [prefix]"
  return
fi
if [ "$#" -gt 3 ]; then
  echo "error provide arguments: host domain [prefix]"
  return
fi

HOST=$1
DOMAIN=$2
# usually /usr/local/etc/oai
if [ "$#" -eq 3 ]; then
  PREFIX=$3
else
  PREFIX="/usr/local/etc/oai"
fi

# CA self certificate
openssl req  -new -batch -x509 -days 3650 -nodes -newkey rsa:1024 -out cacert.pem -keyout cakey.pem -subj /CN=ca.localdomain/C=FR/ST=BdR/L=Aix/O=fD/OU=Tests
#
openssl genrsa -out $HOST.key.pem 1024
openssl req -new -batch -out $HOST.csr.pem -key $HOST.key.pem -subj /CN=$HOST.$DOMAIN/C=FR/ST=BdR/L=Aix/O=fD/OU=Tests
openssl ca -cert cacert.pem -keyfile cakey.pem -in $HOST.csr.pem -out $HOST.cert.pem -outdir . -batch

sudo cp -upv $HOST.cert.pem cacert.pem $HOST.key.pem  $PREFIX/freeDiameter


