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

class mmeConfigGen():
	def __init__(self):
		self.kind = ''
		self.hss_s6a_IP = ipaddress.ip_address('0.0.0.0')
		self.mme_s6a_IP = ipaddress.ip_address('0.0.0.0')
		self.mme_s1c_IP = ipaddress.ip_address('0.0.0.0')
		self.mme_s1c_name = ''
		self.mme_s10_IP = ipaddress.ip_address('0.0.0.0')
		self.mme_s10_name = ''
		self.mme_s11_IP = ipaddress.ip_address('0.0.0.0')
		self.mme_s11_name = ''
		self.spgwc0_s11_IP = ipaddress.ip_address('0.0.0.0')
		self.mme_gid = '32768'
		self.mme_code = '3'
		self.mcc = '208'
		self.mnc = '93'
		self.tac_list = "600 601 602"
		self.realm = 'openairinterface.org'
		self.fromDockerFile = False
		self.envForEntrypoint = False

	def GenerateMMEConfigurer(self):
		mmeFile = open('./mme-cfg.sh', 'w')
		mmeFile.write('#!/bin/bash\n')
		mmeFile.write('\n')
		if self.fromDockerFile:
			mmeFile.write('cd /openair-mme/scripts\n')
		else:
			mmeFile.write('cd /home/scripts\n')

		# We cannot have S10 and S11 sharing the same interface and the same IP address since they are using the same port
		useLoopBackForS10 = False
		if (self.mme_s10_name == self.mme_s11_name) and (self.mme_s10_IP == self.mme_s11_IP):
			print ('Using the same interface name and the same IP address for S11 and S10 is not allowed.')
			print ('Starting a virtual interface on loopback for S10')
			useLoopBackForS10 = True
			mmeFile.write('\n')
			mmeFile.write('# Using the same interface name and the same IP address for S11 and S10 is not allowed.\n')
			mmeFile.write('# Starting a virtual interface on loopback for S10\n')
			mmeFile.write('ifconfig lo:s10 127.0.0.10 up\n')
			mmeFile.write('echo "ifconfig lo:s10 127.0.0.10 up --> OK"\n')

		mmeFile.write('\n')
		mmeFile.write('INSTANCE=1\n')
		if self.fromDockerFile:
			mmeFile.write('PREFIX=\'/openair-mme/etc\'\n')
		else:
			mmeFile.write('PREFIX=\'/usr/local/etc/oai\'\n')
		# The following variables could be later be passed as parameters
		mmeFile.write('MY_REALM=\'' + self.realm + '\'\n')
		mmeFile.write('\n')
		if not self.fromDockerFile:
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
		mmeFile.write('MME_CONF[@MME_S6A_IP_ADDR@]="' + str(self.mme_s6a_IP) + '"\n')
		mmeFile.write('MME_CONF[@INSTANCE@]=$INSTANCE\n')
		mmeFile.write('MME_CONF[@PREFIX@]=$PREFIX\n')
		mmeFile.write('MME_CONF[@REALM@]=$MY_REALM\n')
		mmeFile.write('MME_CONF[@PID_DIRECTORY@]=\'/var/run\'\n')
		mmeFile.write('MME_CONF[@MME_FQDN@]="mme.${MME_CONF[@REALM@]}"\n')
		mmeFile.write('MME_CONF[@HSS_HOSTNAME@]=\'hss\'\n')
		mmeFile.write('MME_CONF[@HSS_FQDN@]="${MME_CONF[@HSS_HOSTNAME@]}.${MME_CONF[@REALM@]}"\n')
		mmeFile.write('MME_CONF[@HSS_IP_ADDR@]="' + str(self.hss_s6a_IP) + '"\n')
		mmeFile.write('MME_CONF[@HSS_REALM@]=$MY_REALM\n')
		mmeFile.write('MME_CONF[@MCC@]=\'' + self.mcc + '\'\n')
		mmeFile.write('MME_CONF[@MNC@]=\'' + self.mnc + '\'\n')
		mmeFile.write('MME_CONF[@MME_GID@]=\'' + self.mme_gid + '\'\n')
		mmeFile.write('MME_CONF[@MME_CODE@]=\'' + self.mme_code + '\'\n')
		tacs = self.tac_list.split();
		if len(tacs) < 3:
			i = len(tacs)
			base_tac = int(tacs[i - 1]) + 1
			while (i < 3):
				tacs.append(str(base_tac))
				base_tac += 1
				i += 1
		i = 0
		while i < 3:
			mmeFile.write('MME_CONF[@TAC_' + str(i) + '@]=\'' + str(tacs[i]) + '\'\n')
			i += 1

		mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S1_MME@]=\'' + self.mme_s1c_name + '\'\n')
		mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S1_MME@]=\'' + str(self.mme_s1c_IP) + '/24\'\n')
		mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S11@]=\'' + self.mme_s11_name + '\'\n')
		mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S11@]=\'' + str(self.mme_s11_IP) + '/24\'\n')
		if useLoopBackForS10:
			mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S10@]=\'lo:s10\'\n')
			mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S10@]=\'127.0.0.10/24\'\n')
		else:
			mmeFile.write('MME_CONF[@MME_INTERFACE_NAME_FOR_S10@]=\'' + self.mme_s10_name + '\'\n')
			mmeFile.write('MME_CONF[@MME_IPV4_ADDRESS_FOR_S10@]=\'' + str(self.mme_s10_IP) + '/24\'\n')
		mmeFile.write('MME_CONF[@OUTPUT@]=\'CONSOLE\'\n')
		mmeFile.write('MME_CONF[@SGW_IPV4_ADDRESS_FOR_S11_0@]=\'' + str(self.spgwc0_s11_IP) + '\'\n')
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
		mmeFile.write('TAC_SGW_0=\'' + tacs[0] + '\'\n')
		mmeFile.write('tmph=`echo "$TAC_SGW_0 / 256" | bc`\n')
		mmeFile.write('tmpl=`echo "$TAC_SGW_0 % 256" | bc`\n')
		mmeFile.write('MME_CONF[@TAC-LB_SGW_0@]=`printf "%02x\\n" $tmpl`\n')
		mmeFile.write('MME_CONF[@TAC-HB_SGW_0@]=`printf "%02x\\n" $tmph`\n')
		mmeFile.write('\n')
		mmeFile.write('MME_CONF[@MCC_MME_0@]=${MME_CONF[@MCC@]}\n')
		mmeFile.write('MME_CONF[@MNC3_MME_0@]=`printf "%03d\\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`\n')
		mmeFile.write('TAC_MME_0=\'' + tacs[1] + '\'\n')
		mmeFile.write('tmph=`echo "$TAC_MME_0 / 256" | bc`\n')
		mmeFile.write('tmpl=`echo "$TAC_MME_0 % 256" | bc`\n')
		mmeFile.write('MME_CONF[@TAC-LB_MME_0@]=`printf "%02x\\n" $tmpl`\n')
		mmeFile.write('MME_CONF[@TAC-HB_MME_0@]=`printf "%02x\\n" $tmph`\n')
		mmeFile.write('\n')
		mmeFile.write('MME_CONF[@MCC_MME_1@]=${MME_CONF[@MCC@]}\n')
		mmeFile.write('MME_CONF[@MNC3_MME_1@]=`printf "%03d\\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`\n')
		mmeFile.write('TAC_MME_1=\'' + tacs[2] + '\'\n')
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
		if self.fromDockerFile:
			mmeFile.write('./check_mme_s6a_certificate $PREFIX mme.${MME_CONF[@REALM@]}\n')
		else:
			mmeFile.write('./check_mme_s6a_certificate $PREFIX/freeDiameter mme.${MME_CONF[@REALM@]}\n')
		mmeFile.close()

	def GenerateMMEEnvList(self):
		mmeFile = open('./mme-env.list', 'w')
		mmeFile.write('# Environment Variables used by the OAI-MME Entrypoint Script\n')
		mmeFile.write('REALM=' + self.realm + '\n')
		mmeFile.write('PREFIX=/openair-mme/etc\n')
		mmeFile.write('INSTANCE=1\n')
		mmeFile.write('PID_DIRECTORY=/var/run\n')
		mmeFile.write('\n')
		mmeFile.write('HSS_IP_ADDR=' + str(self.hss_s6a_IP) + '\n')
		mmeFile.write('HSS_HOSTNAME=hss\n')
		mmeFile.write('HSS_FQDN=hss.' + self.realm + '\n')
		mmeFile.write('HSS_REALM=' + self.realm + '\n')
		mmeFile.write('\n')
		mmeFile.write('MCC=' + self.mcc + '\n')
		mmeFile.write('MNC=' + self.mnc + '\n')
		mmeFile.write('MME_GID=' + self.mme_gid + '\n')
		mmeFile.write('MME_CODE=' + self.mme_code + '\n')
		tacs = self.tac_list.split();
		if len(tacs) < 3:
			i = len(tacs)
			base_tac = int(tacs[i - 1]) + 1
			while (i < 3):
				tacs.append(str(base_tac))
				base_tac += 1
				i += 1
		i = 0
		while i < 3:
			mmeFile.write('TAC_' + str(i) + '=' + str(tacs[i]) + '\n')
			i += 1
		mmeFile.write('\n')
		mmeFile.write('MME_FQDN=mme.' + self.realm + '\n')
		mmeFile.write('MME_S6A_IP_ADDR=' + str(self.mme_s6a_IP) + '\n')
		mmeFile.write('MME_INTERFACE_NAME_FOR_S1_MME=' + self.mme_s1c_name + '\n')
		mmeFile.write('MME_IPV4_ADDRESS_FOR_S1_MME=' + str(self.mme_s1c_IP) + '\n')
		mmeFile.write('MME_INTERFACE_NAME_FOR_S11=' + self.mme_s11_name + '\n')
		mmeFile.write('MME_IPV4_ADDRESS_FOR_S11=' + str(self.mme_s11_IP) + '\n')
		mmeFile.write('MME_INTERFACE_NAME_FOR_S10=' + self.mme_s10_name + '\n')
		mmeFile.write('MME_IPV4_ADDRESS_FOR_S10=' + str(self.mme_s10_IP) + '\n')
		mmeFile.write('\n')
		mmeFile.write('OUTPUT=CONSOLE\n')
		mmeFile.write('\n')
		mmeFile.write('SGW_IPV4_ADDRESS_FOR_S11_0=' + str(self.spgwc0_s11_IP) + '\n')
		mmeFile.write('PEER_MME_IPV4_ADDRESS_FOR_S10_0=0.0.0.0\n')
		mmeFile.write('PEER_MME_IPV4_ADDRESS_FOR_S10_1=0.0.0.0\n')
		mmeFile.write('\n')
		mmeFile.write('MCC_SGW_0=' + self.mcc + '\n')
		mmeFile.write('MNC3_SGW_0=%03d\n' % int(self.mnc))
		mmeFile.write('TAC_LB_SGW_0=%02X\n' % int(int(tacs[0]) % 256))
		mmeFile.write('TAC_HB_SGW_0=%02X\n' % int(int(tacs[0]) / 256))
		mmeFile.write('\n')
		mmeFile.write('MCC_MME_0=' + self.mcc + '\n')
		mmeFile.write('MNC3_MME_0=%03d\n' % int(self.mnc))
		mmeFile.write('TAC_LB_MME_0=%02X\n' % int(int(tacs[1]) % 256))
		mmeFile.write('TAC_HB_MME_0=%02X\n' % int(int(tacs[1]) / 256))
		mmeFile.write('\n')
		mmeFile.write('MCC_MME_1=' + self.mcc + '\n')
		mmeFile.write('MNC3_MME_1=%03d\n' % int(self.mnc))
		mmeFile.write('TAC_LB_MME_1=%02X\n' % int(int(tacs[2]) % 256))
		mmeFile.write('TAC_HB_MME_1=%02X\n' % int(int(tacs[2]) / 256))
		mmeFile.write('\n')
		mmeFile.write('TAC_LB_SGW_TEST_0=%02X\n' % int(int(tacs[2]) % 256))
		mmeFile.write('TAC_HB_SGW_TEST_0=%02X\n' % int(int(tacs[2]) / 256))
		mmeFile.write('\n')
		mmeFile.write('SGW_IPV4_ADDRESS_FOR_S11_TEST_0=0.0.0.0\n')
		mmeFile.close()

#-----------------------------------------------------------
# Usage()
#-----------------------------------------------------------
def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('generateConfigFiles.py')
	print('   Prepare a bash script to be run in the workspace where MME is being built.')
	print('   That bash script will copy configuration template files and adapt to your configuration.')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python3 generateConfigFiles.py [options]')
	print('  --help  Show this help.')
	print('---------------------------------------------------------------------------------------------------- MME Options -----')
	print('  --kind=MME')
	print('  --hss_s6a=[HSS S6A Interface IP server]')
	print('  --mme_s6a=[MME S6A Interface IP server]')
	print('  --mme_s1c_IP=[MME S1-C Interface IP address]')
	print('  --mme_s1c_name=[MME S1-C Interface name]')
	print('  --mme_s10_IP=[MME S10 Interface IP address]')
	print('  --mme_s10_name=[MME S10 Interface name]')
	print('  --mme_s11_IP=[MME S11 Interface IP address]')
	print('  --mme_s11_name=[MME S11 Interface name]')
	print('  --spgwc0_s11_IP=[SPGW-C Instance 0 - S11 Interface IP address]')
	print('---------------------------------------------------------------------------------------------- MME Not Mandatory -----')
	print('  --mme_gid=[MME Group ID for GUMMEI]')
	print('  --mme_code=[MME code for GUMMEI]')
	print('  --mcc=[MCC for TAI list and GUMMEI]')
	print('  --mnc=[MNC for TAI list and GUMMEI]')
	print('  --tac_list=["TACs for managed TAIs"]')
	print('  --realm=["REALM"]')
	print('  --from_docker_file')
	print('  --env_for_entrypoint	[generates a mme-env.list interpreted by the entrypoint]')

argvs = sys.argv
argc = len(argvs)
cwd = os.getcwd()

myMME = mmeConfigGen()

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-kind=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-kind=(.+)$', myArgv, re.IGNORECASE)
		myMME.kind = matchReg.group(1)
	elif re.match('^\-\-hss_s6a=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-hss_s6a=(.+)$', myArgv, re.IGNORECASE)
		myMME.hss_s6a_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-mme_s6a=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s6a=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s6a_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-mme_s1c_IP=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s1c_IP=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s1c_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-mme_s1c_name=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s1c_name=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s1c_name = matchReg.group(1)
	elif re.match('^\-\-mme_s10_IP=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s10_IP=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s10_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-mme_s10_name=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s10_name=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s10_name = matchReg.group(1)
	elif re.match('^\-\-mme_s11_IP=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s11_IP=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s11_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-mme_s11_name=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_s11_name=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_s11_name = matchReg.group(1)
	elif re.match('^\-\-spgwc0_s11_IP=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-spgwc0_s11_IP=(.+)$', myArgv, re.IGNORECASE)
		myMME.spgwc0_s11_IP = ipaddress.ip_address(matchReg.group(1))
	elif re.match('^\-\-mme_gid=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_gid=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_gid = matchReg.group(1)
	elif re.match('^\-\-mme_code=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mme_code=(.+)$', myArgv, re.IGNORECASE)
		myMME.mme_code = matchReg.group(1)
	elif re.match('^\-\-mcc=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mcc=(.+)$', myArgv, re.IGNORECASE)
		myMME.mcc = matchReg.group(1)
	elif re.match('^\-\-mnc=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-mnc=(.+)$', myArgv, re.IGNORECASE)
		myMME.mnc = matchReg.group(1)
	elif re.match('^\-\-tac_list=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-tac_list=(.+)$', myArgv, re.IGNORECASE)
		myMME.tac_list = str(matchReg.group(1))
	elif re.match('^\-\-realm=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-realm=(.+)$', myArgv, re.IGNORECASE)
		myMME.realm = matchReg.group(1)
	elif re.match('^\-\-from_docker_file', myArgv, re.IGNORECASE):
		myMME.fromDockerFile = True
	elif re.match('^\-\-env_for_entrypoint', myArgv, re.IGNORECASE):
		myMME.envForEntrypoint = True
	else:
		Usage()
		sys.exit('Invalid Parameter: ' + myArgv)

if myMME.kind == '':
	Usage()
	sys.exit('missing kind parameter')

if myMME.kind == 'MME':
	if str(myMME.mme_s6a_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing MME S6A IP address')
	elif str(myMME.hss_s6a_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing HSS S6A IP address')
	elif str(myMME.mme_s1c_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing MME S1-C IP address')
	elif myMME.mme_s1c_name == '':
		Usage()
		sys.exit('missing MME S1-C Interface name')
	elif str(myMME.mme_s10_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing MME S10 IP address')
	elif myMME.mme_s10_name == '':
		Usage()
		sys.exit('missing MME S10 Interface name')
	elif str(myMME.mme_s11_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing MME S11 IP address')
	elif myMME.mme_s11_name == '':
		Usage()
		sys.exit('missing MME S11 Interface name')
	elif str(myMME.spgwc0_s11_IP) == '0.0.0.0':
		Usage()
		sys.exit('missing SPGW-C0 IP address')
	else:
		if myMME.envForEntrypoint:
			myMME.GenerateMMEEnvList()
		else:
			myMME.GenerateMMEConfigurer()
		sys.exit(0)
else:
	Usage()
	sys.exit('unsupported kind parameter')
