#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# *	the terms found in the LICENSE file in the root of this source tree.
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *	  contact@openairinterface.org
# */
#---------------------------------------------------------------------

import os
import re
import sys
import ipaddress

class hssConfigGen():
	def __init__(self):
		self.kind = ''
		self.cassandra_IP = ipaddress.ip_address('0.0.0.0')
		self.hss_s6a_IP = ipaddress.ip_address('0.0.0.0')
		self.fromDockerFile = False
		self.envForEntrypoint = False
		self.apn1 = 'apn.oai.svc.cluster.local'
		self.apn2 = 'internet'
		self.ltek = '8baf473f2f8fd09487cccbd7097c6862'
		self.op = '11111111111111111111111111111111'
		self.imsi = '311480100001101'
		self.realm = 'openairinterface.org'
		self.users = '10'
		self.nb_mmes = 1

	def GenerateHssConfigurer(self):
		hssFile = open('./hss-cfg.sh', 'w')
		hssFile.write('#!/bin/bash\n')
		hssFile.write('\n')
		if self.fromDockerFile:
			hssFile.write('cd /openair-hss/scripts\n')
		else:
			hssFile.write('cd /home/scripts\n')
		hssFile.write('\n')
		# CASSANDRA ADDRESS SHOULD BE THE CASSANDRA CONTAINER default address
		hssFile.write('Cassandra_Server_IP=\'' + str(self.cassandra_IP) + '\'\n')
		if self.fromDockerFile:
			hssFile.write('PREFIX=\'/openair-hss/etc\'\n')
		else:
			hssFile.write('PREFIX=\'/usr/local/etc/oai\'\n')
		# The following variables could be later be passed as parameters
		hssFile.write('MY_REALM=\'' + self.realm + '\'\n')
		hssFile.write('MY_APN1=\'' + self.apn1 + '\'\n')
		hssFile.write('MY_APN2=\'' + self.apn2 + '\'\n')
		hssFile.write('MY_LTE_K=\'' + self.ltek + '\'\n')
		hssFile.write('MY_OP_K=\'' + self.op + '\'\n')
		hssFile.write('MY_IMSI=\'' + self.imsi + '\'\n')
		hssFile.write('MY_USERS=\'' + self.users + '\'\n')
		hssFile.write('\n')
		if self.fromDockerFile:
			hssFile.write('mkdir -p /openair-hss/logs\n')
		else:
			hssFile.write('rm -Rf $PREFIX\n')
			hssFile.write('\n')
			hssFile.write('mkdir -p $PREFIX\n')
			hssFile.write('mkdir $PREFIX/freeDiameter\n')
			hssFile.write('mkdir $PREFIX/logs\n')
			hssFile.write('mkdir -p logs\n')
		hssFile.write('\n')
		hssFile.write('# provision users\n')
		hssFile.write('./data_provisioning_users --apn $MY_APN1 --apn2 $MY_APN2 --key $MY_LTE_K --imsi-first $MY_IMSI --msisdn-first 00000001 --mme-identity mme.$MY_REALM --no-of-users $MY_USERS --realm $MY_REALM --truncate True --verbose True --cassandra-cluster $Cassandra_Server_IP\n')
		hssFile.write('# provision mme\n')
		hssFile.write('./data_provisioning_mme --id 3 --mme-identity mme.$MY_REALM --realm $MY_REALM --ue-reachability 1 --truncate True  --verbose True -C $Cassandra_Server_IP\n')
		if self.nb_mmes > 1:
			i = 1
			while i < self.nb_mmes:
				hssFile.write('./data_provisioning_mme --id ' + str(i+3) + ' --mme-identity mme' + str(i) + '.$MY_REALM --realm $MY_REALM --ue-reachability 1 --truncate False  --verbose True -C $Cassandra_Server_IP\n')
				i += 1
		hssFile.write('\n')
		if not self.fromDockerFile:
			hssFile.write('cp ../etc/acl.conf ../etc/hss_rel14_fd.conf $PREFIX/freeDiameter\n')
			hssFile.write('cp ../etc/hss_rel14.conf ../etc/hss_rel14.json $PREFIX\n')
			hssFile.write('cp ../etc/oss.json $PREFIX\n')
		hssFile.write('\n')
		hssFile.write('declare -A HSS_CONF\n')
		hssFile.write('HSS_CONF[@PREFIX@]=$PREFIX\n')
		hssFile.write('HSS_CONF[@REALM@]=$MY_REALM\n')
		hssFile.write('HSS_CONF[@HSS_FQDN@]="hss.${HSS_CONF[@REALM@]}"\n')
		hssFile.write('HSS_CONF[@HSS_HOSTNAME@]="hss"\n')
		hssFile.write('HSS_CONF[@cassandra_Server_IP@]=$Cassandra_Server_IP\n')
		hssFile.write('HSS_CONF[@OP_KEY@]=$MY_OP_K\n')
		hssFile.write('HSS_CONF[@ROAMING_ALLOWED@]=\'true\'\n')
		hssFile.write('for K in "${!HSS_CONF[@]}"; do	egrep -lRZ "$K" $PREFIX | xargs -0 -l sed -i -e "s|$K|${HSS_CONF[$K]}|g"; done\n')
		# HSS S6A is for the moment the default OAI-HSS Container
		# We could later dedicate a container new interface
		if self.fromDockerFile:
			hssFile.write('sed -i -e \'s/#ListenOn.*$/ListenOn = "' + str(self.hss_s6a_IP) + '";/g\' $PREFIX/hss_rel14_fd.conf\n')
		else:
			hssFile.write('sed -i -e \'s/#ListenOn.*$/ListenOn = "' + str(self.hss_s6a_IP) + '";/g\' $PREFIX/freeDiameter/hss_rel14_fd.conf\n')
		hssFile.write('./make_certs.sh hss ${HSS_CONF[@REALM@]} $PREFIX\n')
		hssFile.close()

	def GenerateHssEnvList(self):
		hssFile = open('./hss-env.list', 'w')
		hssFile.write('# Environment Variables used by the OAI-HSS Entrypoint Script\n')
		hssFile.write('REALM=' + self.realm + '\n')
		hssFile.write('HSS_FQDN=hss.' + self.realm + '\n')
		hssFile.write('PREFIX=/openair-hss/etc\n')
		hssFile.write('cassandra_Server_IP=' + str(self.cassandra_IP) + '\n')
		hssFile.write('OP_KEY=' + self.op + '\n')
		hssFile.write('LTE_K=' + self.ltek + '\n')
		hssFile.write('APN1=' + self.apn1 + '\n')
		hssFile.write('APN2=' + self.imsi + '\n')
		hssFile.write('FIRST_IMSI=' + self.imsi + '\n')
		hssFile.write('NB_USERS=' + self.users + '\n')
		hssFile.close()

#-----------------------------------------------------------
# Usage()
#-----------------------------------------------------------
def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('generateConfigFiles.py')
	print('   Prepare a bash script to be run in the workspace where HSS is being built.')
	print('   That bash script will copy configuration template files and adapt to your configuration.')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python3 generateConfigFiles.py [options]')
	print('  --help  Show this help.')
	print('---------------------------------------------------------------------------------------------------- HSS Options -----')
	print('  --kind=HSS')
	print('  --cassandra=[Cassandra IP server]')
	print('  --hss_s6a=[HSS S6A Interface IP server]')
	print('  --from_docker_file')
	print('---------------------------------------------------------------------------------------------- HSS Not Mandatory -----')
	print('  --apn1=[First APN] // MUST HAVE one "." in the name')
	print('  --apn2=[Second APN]')
	print('  --ltek=[LTE KEY]')
	print('  --op=[Operator Key]')
	print('  --users=[Number of users to provision]')
	print('  --imsi=[IMSI of the 1st user]')
	print('  --realm=[Realm of your EPC]')
	print('  --nb_mmes=[Number of MME instances to provision (Default = 1)]')
	print('  --env_for_entrypoint    [generates a hss-env.list interpreted by the entrypoint]')

argvs = sys.argv
argc = len(argvs)
cwd = os.getcwd()

myHSS = hssConfigGen()

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-kind=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-kind=(.+)$', myArgv, re.IGNORECASE)
		myHSS.kind = matchReg.group(1)
	elif re.match('^\-\-cassandra=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-cassandra=(.+)$', myArgv, re.IGNORECASE)
		myHSS.cassandra_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-hss_s6a=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-hss_s6a=(.+)$', myArgv, re.IGNORECASE)
		myHSS.hss_s6a_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-apn1=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-apn1=(.+)$', myArgv, re.IGNORECASE)
		myHSS.apn1 = matchReg.group(1)
	elif re.match('^\-\-apn2=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-apn2=(.+)$', myArgv, re.IGNORECASE)
		myHSS.apn2 = matchReg.group(1)
	elif re.match('^\-\-ltek=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-ltek=(.+)$', myArgv, re.IGNORECASE)
		myHSS.ltek = matchReg.group(1)
	elif re.match('^\-\-op=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-op=(.+)$', myArgv, re.IGNORECASE)
		myHSS.op = matchReg.group(1)
	elif re.match('^\-\-users=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-users=(.+)$', myArgv, re.IGNORECASE)
		myHSS.users = matchReg.group(1)
	elif re.match('^\-\-imsi=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-imsi=(.+)$', myArgv, re.IGNORECASE)
		myHSS.imsi = matchReg.group(1)
	elif re.match('^\-\-realm=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-realm=(.+)$', myArgv, re.IGNORECASE)
		myHSS.realm = matchReg.group(1)
	elif re.match('^\-\-nb_mmes=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-nb_mmes=(.+)$', myArgv, re.IGNORECASE)
		myHSS.nb_mmes = int(matchReg.group(1))
	elif re.match('^\-\-from_docker_file', myArgv, re.IGNORECASE):
		myHSS.fromDockerFile = True
	elif re.match('^\-\-env_for_entrypoint', myArgv, re.IGNORECASE):
		myHSS.envForEntrypoint = True
	else:
		Usage()
		sys.exit('Invalid Parameter: ' + myArgv)

if myHSS.kind == '':
	Usage()
	sys.exit('missing kind parameter')

if myHSS.kind == 'HSS':
	if re.search('\.', myHSS.apn1) is None:
		Usage()
		sys.exit('missing a dot (".") in APN1 name')
	elif str(myHSS.cassandra_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing Cassandra IP address')
	elif str(myHSS.hss_s6a_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing HSS S6A IP address')
	elif len(myHSS.ltek) != 32:
		Usage()
		sys.exit('LTE Key SHALL have 32 characters')
	elif len(myHSS.op) != 32:
		Usage()
		sys.exit('OP Key SHALL have 32 characters')
	else:
		if myHSS.envForEntrypoint:
			myHSS.GenerateHssEnvList()
		else:
			myHSS.GenerateHssConfigurer()
		sys.exit(0)
else:
	Usage()
	sys.exit('invalid kind parameter')
