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
import subprocess

class verifySanityCheckDeployment():
	def __init__(self):
		self.job_name = ''

	def checkLogs(self):
		hss_status = self.analyze_check_run_log('HSS')
		mme_status = self.analyze_check_run_log('MME')
		if not hss_status:
			print ('HSS did not deploy properly')
		if not mme_status:
			print ('MME did not deploy properly')
		if not hss_status or not mme_status:
			sys.exit('Sanity Check Deployment went wrong')
		else:
			print ('Sanity Check Deployment is OK')

	def analyze_check_run_log(self, nfType):
		logFileName = nfType.lower() + '_check_run.log'

		cwd = os.getcwd()
		status = False
		if os.path.isfile(cwd + '/archives/' + logFileName):
			myCmd = 'iconv -f ISO-8859-1 -t UTF-8//TRANSLIT ' + cwd + '/archives/' + logFileName + ' -o ' + cwd + '/archives/' + logFileName + '.conv'
			subprocess.run(myCmd, shell=True)
			myCmd = 'mv ' + cwd + '/archives/' + logFileName + '.conv '  + cwd + '/archives/' + logFileName
			subprocess.run(myCmd, shell=True)
			nb_opc_generation = 0
			freeDiameterUp = False
			connectionWithMME = False
			connectionWithHSS = False
			sctp_status = False
			with open(cwd + '/archives/' + logFileName, 'r') as logfile:
				for line in logfile:
					result = re.search('Compute opc', line)
					if result is not None:
						nb_opc_generation += 1
					result = re.search('The freeDiameter engine has been started|Diameter identity of MME', line)
					if result is not None:
						freeDiameterUp = True
					result = re.search('STATE_OPEN.*mme', line)
					if result is not None:
						connectionWithMME = True
					result = re.search('Peer hss.* is now connected', line)
					if result is not None:
						connectionWithHSS = True
					result = re.search('Received SCTP_INIT_MSG', line)
					if result is not None:
						sctp_status = True
				logfile.close()
			if nfType == 'HSS':
				if nb_opc_generation > 0 and freeDiameterUp and connectionWithMME:
					status = True
			if nfType == 'MME':
				if freeDiameterUp and connectionWithHSS and sctp_status:
					status = True

		return status

def Usage():
	print('----------------------------------------------------------------------------------------------------------------------')
	print('verifySanityCheckDeployment.py')
	print('   Verify the Sanity Check Deployment in the pipeline.')
	print('----------------------------------------------------------------------------------------------------------------------')
	print('Usage: python3 verifySanityCheckDeployment.py [options]')
	print('  --help  Show this help.')
	print('---------------------------------------------------------------------------------------------- Mandatory Options -----')
	print('  --job_name=[Jenkins Job name]')
	print('  --job_id=[Jenkins Job Build ID]')

#--------------------------------------------------------------------------------------------------------
#
# Start of main
#
#--------------------------------------------------------------------------------------------------------

argvs = sys.argv
argc = len(argvs)

vscd = verifySanityCheckDeployment()

while len(argvs) > 1:
	myArgv = argvs.pop(1)
	if re.match('^\-\-help$', myArgv, re.IGNORECASE):
		Usage()
		sys.exit(0)
	elif re.match('^\-\-job_name=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_name=(.+)$', myArgv, re.IGNORECASE)
		vscd.job_name = matchReg.group(1)
	elif re.match('^\-\-job_id=(.+)$', myArgv, re.IGNORECASE):
		matchReg = re.match('^\-\-job_id=(.+)$', myArgv, re.IGNORECASE)
		vscd.job_id = matchReg.group(1)
	else:
		sys.exit('Invalid Parameter: ' + myArgv)

if vscd.job_name == '' or vscd.job_id == '':
	sys.exit('Missing Parameter in job description')

vscd.checkLogs()
