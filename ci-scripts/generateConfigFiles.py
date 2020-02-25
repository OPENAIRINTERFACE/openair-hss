#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the OAI Public License, Version 1.1  (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.openairinterface.org/?page_id=698
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *      contact@openairinterface.org
# */
#---------------------------------------------------------------------

import os
import re
import sys

def GenerateHssConfigurer(cassandra_IP, hss_s6a_IP):
	hssFile = open('./hss-cfg.sh', 'w')
	hssFile.write('#!/bin/bash\n')
	hssFile.write('\n')
	hssFile.write('cd /home/scripts\n')
	hssFile.write('\n')
	# CASSANDRA ADDRESS SHOULD BE THE CASSANDRA CONTAINER default address
	hssFile.write('Cassandra_Server_IP=\'' + cassandra_IP + '\'\n')
	hssFile.write('PREFIX=\'/usr/local/etc/oai\'\n')
	# The following variables could be later be passed as parameters
	hssFile.write('MY_REALM=\'openairinterface.org\'\n')
	hssFile.write('MY_APN=\'apn.oai.svc.cluster.local\'\n')
	hssFile.write('MY_LTE_K=\'8baf473f2f8fd09487cccbd7097c6862\'\n')
	hssFile.write('MY_OP_K=\'11111111111111111111111111111111\'\n')
	hssFile.write('\n')
	hssFile.write('rm -Rf $PREFIX\n')
	hssFile.write('\n')
	hssFile.write('mkdir -p $PREFIX\n')
	hssFile.write('mkdir $PREFIX/freeDiameter\n')
	hssFile.write('mkdir $PREFIX/logs\n')
	hssFile.write('mkdir -p logs\n')
	hssFile.write('\n')
	hssFile.write('# provision users\n')
	hssFile.write('./data_provisioning_users --apn $MY_APN --apn2 internet --key $MY_LTE_K --imsi-first 311480100001101 --msisdn-first 00000001 --mme-identity mme.$MY_REALM --no-of-users 10 --realm $MY_REALM --truncate True --verbose True --cassandra-cluster $Cassandra_Server_IP\n')
	hssFile.write('# provision mme\n')
	hssFile.write('./data_provisioning_mme --id 3 --mme-identity mme.$MY_REALM --realm $MY_REALM --ue-reachability 1 --truncate True  --verbose True -C $Cassandra_Server_IP\n')
	hssFile.write('\n')
	hssFile.write('cp ../etc/acl.conf ../etc/hss_rel14_fd.conf $PREFIX/freeDiameter\n')
	hssFile.write('cp ../etc/hss_rel14.conf ../etc/hss_rel14.json $PREFIX\n')
	hssFile.write('cp ../etc/oss.json $PREFIX\n')
	hssFile.write('\n')
	hssFile.write('declare -A HSS_CONF\n')
	hssFile.write('HSS_CONF[@PREFIX@]=$PREFIX\n')
	hssFile.write('HSS_CONF[@REALM@]=$MY_REALM\n')
	hssFile.write('HSS_CONF[@HSS_FQDN@]="hss.${HSS_CONF[@REALM@]}"\n')
	hssFile.write('HSS_CONF[@cassandra_Server_IP@]=$Cassandra_Server_IP\n')
	hssFile.write('HSS_CONF[@OP_KEY@]=$MY_OP_K\n')
	hssFile.write('HSS_CONF[@ROAMING_ALLOWED@]=\'true\'\n')
	hssFile.write('for K in "${!HSS_CONF[@]}"; do    egrep -lRZ "$K" $PREFIX | xargs -0 -l sed -i -e "s|$K|${HSS_CONF[$K]}|g"; done\n')
	# HSS S6A is for the moment the default OAI-HSS Container
	# We could later dedicate a container new interface
	hssFile.write('sed -i -e \'s/#ListenOn.*$/ListenOn = "' + hss_s6a_IP + '";/g\' $PREFIX/freeDiameter/hss_rel14_fd.conf\n')
	hssFile.write('../src/hss_rel14/bin/make_certs.sh hss ${HSS_CONF[@REALM@]} $PREFIX\n')
	hssFile.close()

def GenerateMMEConfigurer(mme_s6a_IP, hss_s6a_IP):
	mmeFile = open('./mme-cfg.sh', 'w')
	mmeFile.write('#!/bin/bash\n')
	mmeFile.write('\n')
	mmeFile.write('cd /home/scripts\n')
	mmeFile.write('\n')
	mmeFile.write('INSTANCE=1\n')
	mmeFile.write('PREFIX=\'/usr/local/etc/oai\'\n')
	# The following variables could be later be passed as parameters
	mmeFile.write('MY_REALM=\'openairinterface.org\'\n')
	mmeFile.write('\n')
	mmeFile.write('rm -Rf $PREFIX\n')
	mmeFile.write('\n')
	mmeFile.write('mkdir -p $PREFIX\n')
	mmeFile.write('mkdir $PREFIX/freeDiameter\n')
	mmeFile.write('\n')
	mmeFile.write('cp ../etc/mme_fd.sprint.conf $PREFIX/freeDiameter/mme_fd.conf\n')
	mmeFile.write('cp ../etc/mme.conf  $PREFIX\n')
	mmeFile.write('\n')
	mmeFile.write('declare -A MME_CONF\n')
	mmeFile.write('\n')
	mmeFile.write('MME_CONF[@MME_S6A_IP_ADDR@]="' + mme_s6a_IP + '"\n')
	mmeFile.write('MME_CONF[@INSTANCE@]=$INSTANCE\n')
	mmeFile.write('MME_CONF[@PREFIX@]=$PREFIX\n')
	mmeFile.write('MME_CONF[@REALM@]=$MY_REALM\n')
	mmeFile.write('MME_CONF[@PID_DIRECTORY@]=\'/var/run\'\n')
	mmeFile.write('MME_CONF[@MME_FQDN@]="mme.${MME_CONF[@REALM@]}"\n')
	mmeFile.write('MME_CONF[@HSS_HOSTNAME@]=\'hss\'\n')
	mmeFile.write('MME_CONF[@HSS_FQDN@]="${MME_CONF[@HSS_HOSTNAME@]}.${MME_CONF[@REALM@]}"\n')
	mmeFile.write('MME_CONF[@HSS_IP_ADDR@]="' + hss_s6a_IP + '"\n')
	mmeFile.write('MME_CONF[@MCC@]=\'208\'\n')
	mmeFile.write('MME_CONF[@MNC@]=\'93\'\n')
	mmeFile.write('MME_CONF[@MME_GID@]=\'32768\'\n')
	mmeFile.write('MME_CONF[@MME_CODE@]=\'3\'\n')
	mmeFile.write('MME_CONF[@TAC_0@]=\'600\'\n')
	mmeFile.write('MME_CONF[@TAC_1@]=\'601\'\n')
	mmeFile.write('MME_CONF[@TAC_2@]=\'602\'\n')
	# For the moment -- do not care
	mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S1_MME@]=\'eth0\'\n')
	mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S1_MME@]=\'' + mme_s6a_IP + '/24\'\n')
	mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S11@]=\'eth0\'\n')
	mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S11@]=\'' + mme_s6a_IP + '/24\'\n')
	mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S10@]=\'eth0\'\n')
	mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S10@]=\'' + mme_s6a_IP + '/24\'\n')
	mmeFile.write('MME_CONF[@OUTPUT@]=\'CONSOLE\'\n')
	mmeFile.write('MME_CONF[@SGW_IPV4_ADDRESS_FOR_S11_TEST_0@]=\'' + hss_s6a_IP + '/24\'\n')
	mmeFile.write('MME_CONF[@SGW_IPV4_ADDRESS_FOR_S11_0@]=\'' + hss_s6a_IP + '/24\'\n')
	mmeFile.write('MME_CONF[@PEER_MME_IPV4_ADDRESS_FOR_S10_0@]=\'0.0.0.0/24\'\n')
	mmeFile.write('MME_CONF[@PEER_MME_IPV4_ADDRESS_FOR_S10_1@]=\'0.0.0.0/24\'\n')
	mmeFile.write('\n')
	mmeFile.write('TAC_SGW_TEST=\'7\'\n')
	mmeFile.write('tmph=`echo "$TAC_SGW_TEST / 256" | bc`\n')
	mmeFile.write('tmpl=`echo "$TAC_SGW_TEST % 256" | bc`\n')
	mmeFile.write('MME_CONF[@TAC-LB_SGW_TEST_0@]=`printf "%02x\\n" $tmpl`\n')
	mmeFile.write('MME_CONF[@TAC-HB_SGW_TEST_0@]=`printf "%02x\\n" $tmph`\n')
	mmeFile.write('\n')
	mmeFile.write('MME_CONF[@MCC_SGW_0@]=${MME_CONF[@MCC@]}\n')
	mmeFile.write('MME_CONF[@MNC3_SGW_0@]=`printf "%03d\\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`\n')
	mmeFile.write('TAC_SGW_0=\'600\'\n')
	mmeFile.write('tmph=`echo "$TAC_SGW_0 / 256" | bc`\n')
	mmeFile.write('tmpl=`echo "$TAC_SGW_0 % 256" | bc`\n')
	mmeFile.write('MME_CONF[@TAC-LB_SGW_0@]=`printf "%02x\\n" $tmpl`\n')
	mmeFile.write('MME_CONF[@TAC-HB_SGW_0@]=`printf "%02x\\n" $tmph`\n')
	mmeFile.write('\n')
	mmeFile.write('MME_CONF[@MCC_MME_0@]=${MME_CONF[@MCC@]}\n')
	mmeFile.write('MME_CONF[@MNC3_MME_0@]=`printf "%03d\\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`\n')
	mmeFile.write('TAC_MME_0=\'601\'\n')
	mmeFile.write('tmph=`echo "$TAC_MME_0 / 256" | bc`\n')
	mmeFile.write('tmpl=`echo "$TAC_MME_0 % 256" | bc`\n')
	mmeFile.write('MME_CONF[@TAC-LB_MME_0@]=`printf "%02x\\n" $tmpl`\n')
	mmeFile.write('MME_CONF[@TAC-HB_MME_0@]=`printf "%02x\\n" $tmph`\n')
	mmeFile.write('\n')
	mmeFile.write('MME_CONF[@MCC_MME_1@]=${MME_CONF[@MCC@]}\n')
	mmeFile.write('MME_CONF[@MNC3_MME_1@]=`printf "%03d\\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`\n')
	mmeFile.write('TAC_MME_1=\'602\'\n')
	mmeFile.write('tmph=`echo "$TAC_MME_1 / 256" | bc`\n')
	mmeFile.write('tmpl=`echo "$TAC_MME_1 % 256" | bc`\n')
	mmeFile.write('MME_CONF[@TAC-LB_MME_1@]=`printf "%02x\\n" $tmpl`\n')
	mmeFile.write('MME_CONF[@TAC-HB_MME_1@]=`printf "%02x\\n" $tmph`\n')
	mmeFile.write('\n')
	mmeFile.write('for K in "${!MME_CONF[@]}"; do \n')
	mmeFile.write('  egrep -lRZ "$K" $PREFIX | xargs -0 -l sed -i -e "s|$K|${MME_CONF[$K]}|g"\n')
	mmeFile.write('  ret=$?;[[ ret -ne 0 ]] && echo "Tried to replace $K with ${MME_CONF[$K]}"\n')
	mmeFile.write('done\n')
	mmeFile.write('\n')
	mmeFile.write('# Generate freeDiameter certificate\n')
	mmeFile.write('./check_mme_s6a_certificate $PREFIX/freeDiameter mme.${MME_CONF[@REALM@]}\n')
	mmeFile.close()

#-----------------------------------------------------------
# Usage()
#-----------------------------------------------------------
def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('generateConfigFiles.py')
	print('   Prepare a bash script to be run in the workspace where either HSS and/or MME are being built.')
	print('   That bash script will copy configuration template files and adapt to your configuration.')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python3 generateConfigFiles.py [options]')
	print('  --help  Show this help.')
	print('---------------------------------------------------------------------------------------------------- HSS Options -----')
	print('  --kind=HSS')
	print('  --cassandra=[Cassandra IP server]')
	print('  --hss_s6a=[HSS S6A Interface IP server]')
	print('---------------------------------------------------------------------------------------------------- MME Options -----')
	print('  --kind=MME')
	print('  --hss_s6a=[HSS S6A Interface IP server]')
	print('  --mme_s6a=[MME S6A Interface IP server]')

argvs = sys.argv
argc = len(argvs)
cwd = os.getcwd()

kind = ''
cassandra_IP = ''
hss_s6a_IP = ''
mme_s6a_IP = ''

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-kind=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-kind=(.+)$', myArgv, re.IGNORECASE)
		kind = matchReg.group(1)
	elif re.match('^\-\-cassandra=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-cassandra=(.+)$', myArgv, re.IGNORECASE)
		cassandra_IP = matchReg.group(1)
	elif re.match('^\-\-hss_s6a=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-hss_s6a=(.+)$', myArgv, re.IGNORECASE)
		hss_s6a_IP = matchReg.group(1)
	elif re.match('^\-\-mme_s6a=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s6a=(.+)$', myArgv, re.IGNORECASE)
		mme_s6a_IP = matchReg.group(1)
	else:
		Usage()
		sys.exit('Invalid Parameter: ' + myArgv)

if kind == '':
	Usage()
	sys.exit('missing kind parameter')

if kind == 'HSS':
	if cassandra_IP == '':
		Usage()
		sys.exit('missing Cassandra IP address')
	elif hss_s6a_IP == '':
		Usage()
		sys.exit('missing HSS S6A IP address')
	else:
		GenerateHssConfigurer(cassandra_IP, hss_s6a_IP)
		sys.exit(0)

if kind == 'MME':
	if mme_s6a_IP == '':
		Usage()
		sys.exit('missing MME S6A IP address')
	elif hss_s6a_IP == '':
		Usage()
		sys.exit('missing HSS S6A IP address')
	else:
		GenerateMMEConfigurer(mme_s6a_IP, hss_s6a_IP)
		sys.exit(0)
