#! /bin/bash

#Copyright (c) 2017 Sprint
#
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under
# the terms found in the LICENSE file in the root of this source tree.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

OS_DISTRO=$(grep "^ID=" /etc/os-release | sed "s/ID=//" | sed "s/\"//g")
case "$OS_DISTRO" in
  fedora) OS_BASEDISTRO="fedora";;
  rhel)   OS_BASEDISTRO="fedora";;
  centos) OS_BASEDISTRO="fedora";;
  debian) OS_BASEDISTRO="debian";;
  ubuntu) OS_BASEDISTRO="debian";;
esac

rm -rf demoCA
mkdir demoCA
echo 01 > demoCA/serial
touch demoCA/index.txt
if [[ "$OS_BASEDISTRO" == "debian" ]]
then
  echo 'unique_subject = yes' > demoCA/index.txt.attr
fi

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
openssl req  -new -batch -x509 -days 3650 -nodes -newkey rsa:1024 -out cacert.pem -keyout cakey.pem -subj /CN=$HOST.$DOMAIN/C=FR/ST=BdR/L=Aix/O=fD/OU=Tests
#
openssl genrsa -out $HOST.key.pem 1024
openssl req -new -batch -out $HOST.csr.pem -key $HOST.key.pem -subj /CN=$HOST.$DOMAIN/C=FR/ST=BdR/L=Aix/O=fD/OU=Tests
if [[ "$OS_BASEDISTRO" == "fedora" ]]; then
  cp /etc/pki/tls/openssl.cnf .
  chmod 664 openssl.cnf
  sed -i -e "s#/etc/pki/CA#./demoCA#" openssl.cnf
  openssl ca -cert cacert.pem -keyfile cakey.pem -in $HOST.csr.pem -out $HOST.cert.pem -outdir . -batch -config openssl.cnf
else
  openssl ca -cert cacert.pem -keyfile cakey.pem -in $HOST.csr.pem -out $HOST.cert.pem -outdir . -batch
fi

IS_CONTAINER=`egrep -c "docker|podman|kubepods|libpod|buildah" /proc/self/cgroup`

if [ $IS_CONTAINER -eq 0 ]
then
  SUDO='sudo -S -E'
else
  SUDO=''
fi

$SUDO cp -upv $HOST.cert.pem cacert.pem $HOST.key.pem  $PREFIX/freeDiameter

