#/*
# * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# * contributor license agreements.  See the NOTICE file distributed with
# * this work for additional information regarding copyright ownership.
# * The OpenAirInterface Software Alliance licenses this file to You under
# * the terms found in the LICENSE file in the root of this source tree.
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about the OpenAirInterface (OAI) Software Alliance:
# *       contact@openairinterface.org
# */
#---------------------------------------------------------------------

import os
import re
import sys
import subprocess
import time

CI_NETWORK_DBN='192.168.28.0/26'
CI_NETWORK_S11='192.168.28.128/26'
CI_NETWORK_S6A='192.168.31.0/24'
CI_NETWORK_S10='192.168.32.0/24'
CI_NETWORK_S1C='192.168.33.0/24'

CI_CASS_IP_ADDR='192.168.28.2'
CI_HSS_DBN_ADDR='192.168.28.3'
CI_HSS_S6A_ADDR='192.168.31.2'

CI_MME_S6A_ADDR='192.168.31.3'
CI_MME_S11_ADDR='192.168.28.130'
CI_MME_S10_ADDR='192.168.32.2'
CI_MME_S1C_ADDR='192.168.33.2'

CI_DUMMY_SPGWC_ADDR='192.168.28.131'
CI_DUMMY_SPGWC2_ADDR='192.168.31.4'

class deploySanityCheckTest():
    def __init__(self):
        self.action = 'None'
        self.tag = ''

    def createNetworks(self):
        # first check if already up?
        try:
            res = subprocess.check_output('docker network ls | egrep -c "ci-dbn|ci-s11|ci-s6a|ci-s10|ci-s1c"', shell=True, universal_newlines=True)
            if int(str(res.strip())) > 0:
                self.removeNetworks()
        except:
            pass

        subprocess_run_w_echo('docker network create --attachable --subnet ' + CI_NETWORK_DBN + ' --ip-range ' + CI_NETWORK_DBN + ' ci-dbn')
        subprocess_run_w_echo('docker network create --attachable --subnet ' + CI_NETWORK_S11 + ' --ip-range ' + CI_NETWORK_S11 + ' ci-s11')
        subprocess_run_w_echo('docker network create --attachable --subnet ' + CI_NETWORK_S6A + ' --ip-range ' + CI_NETWORK_S6A + ' ci-s6a')
        subprocess_run_w_echo('docker network create --attachable --subnet ' + CI_NETWORK_S10 + ' --ip-range ' + CI_NETWORK_S10 + ' ci-s10')
        subprocess_run_w_echo('docker network create --attachable --subnet ' + CI_NETWORK_S1C + ' --ip-range ' + CI_NETWORK_S1C + ' ci-s1c')

    def removeNetworks(self):
        try:
            subprocess_run_w_echo('docker network rm ci-dbn ci-s6a ci-s10 ci-s11 ci-s1c')
        except:
            pass

    def deployCassandra(self):
        # first check if already up? If yes, remove everything.
        try:
            res = subprocess.check_output('docker ps -a | grep -c "ci-cassandra"', shell=True, universal_newlines=True)
            if int(str(res.strip())) > 0:
                self.removeAllContainers()
        except:
            pass

        subprocess_run_w_echo('docker run --name ci-cassandra --network ci-dbn --ip ' + CI_CASS_IP_ADDR + ' -d -e CASSANDRA_CLUSTER_NAME="OAI HSS Cluster" -e CASSANDRA_ENDPOINT_SNITCH=GossipingPropertyFileSnitch cassandra:2.1')
        # waiting for the service to be properly started
        doLoop = True
        while doLoop:
            try:
                res = subprocess.check_output('docker exec ci-cassandra /bin/bash -c "nodetool status"', shell=True, universal_newlines=True)
                rackFound = re.search('UN  ' + CI_CASS_IP_ADDR, str(res))
                if rackFound is not None:
                    doLoop = False
            except:
                time.sleep(1)
                pass
        subprocess_run_w_echo('docker exec ci-cassandra /bin/bash -c "nodetool status" | tee archives/cassandra_status.log')
        subprocess_run_w_echo('docker cp src/hss_rel14/db/oai_db.cql ci-cassandra:/home')
        time.sleep(2)
        doLoop = True
        while doLoop:
            try:
                res = subprocess.check_output('docker exec ci-cassandra /bin/bash -c "cqlsh --file /home/oai_db.cql ' + CI_CASS_IP_ADDR + '"', shell=True, universal_newlines=True)
                print('docker exec ci-cassandra /bin/bash -c "cqlsh --file /home/oai_db.cql ' + CI_CASS_IP_ADDR + '"')
                doLoop = False
            except:
                time.sleep(2)
                pass

    def deployHSS(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-hss:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            subprocess_run_w_echo('python3 ci-scripts/generateConfigFiles.py --kind=HSS --cassandra=' + CI_CASS_IP_ADDR + ' --hss_s6a=' + CI_HSS_S6A_ADDR + ' --from_docker_file --env_for_entrypoint')
            # SDB to Cassandra will be on `eth0`
            subprocess_run_w_echo('docker create --privileged --name ci-oai-hss --network ci-dbn --ip ' + CI_HSS_DBN_ADDR + ' --env-file ./hss-env.list oai-hss:' + self.tag)
            # S6A to MME will be on `eth1`
            subprocess_run_w_echo('docker network connect --ip ' + CI_HSS_S6A_ADDR + ' ci-s6a ci-oai-hss')
            subprocess_run_w_echo('docker start ci-oai-hss')
            # Recovering the config part of the entrypoint execution
            tmpAwkFile=open('tmp.awk', 'w')
            tmpAwkFile.write('BEGIN{ok=1}{if ($0 ~/jsonConfig/){ok=0};if(ok==1){print $0}}END{}\n')
            tmpAwkFile.close()
            time.sleep(3)
            subprocess.run('docker logs ci-oai-hss 2>&1 | awk -f tmp.awk > archives/hss_config.log', shell=True)
            subprocess.run('rm -f tmp.awk', shell=True)

        else:
            # SDB to Cassandra will be on `eth0`
            subprocess_run_w_echo('docker run --privileged --name ci-oai-hss --network ci-dbn --ip ' + CI_HSS_DBN_ADDR + ' -d oai-hss:' + self.tag + ' /bin/bash -c "sleep infinity"')
            # S6A to MME will be on `eth1`
            subprocess_run_w_echo('docker network connect --ip ' + CI_HSS_S6A_ADDR + ' ci-s6a ci-oai-hss')
            subprocess_run_w_echo('python3 ci-scripts/generateConfigFiles.py --kind=HSS --cassandra=' + CI_CASS_IP_ADDR + ' --hss_s6a=' + CI_HSS_S6A_ADDR + ' --from_docker_file')
            subprocess_run_w_echo('docker cp ./hss-cfg.sh ci-oai-hss:/openair-hss/scripts')
            subprocess_run_w_echo('docker exec ci-oai-hss /bin/bash -c "cd /openair-hss/scripts && chmod 777 hss-cfg.sh && ./hss-cfg.sh" > archives/hss_config.log')

    def deployMME(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-mme:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            subprocess_run_w_echo('wget --quiet https://raw.githubusercontent.com/OPENAIRINTERFACE/openair-mme/develop/ci-scripts/generateConfigFiles.py -O ci-scripts/generateMmeConfigFiles.py')
            subprocess_run_w_echo('python3 ci-scripts/generateMmeConfigFiles.py --kind=MME --hss_s6a=' + CI_HSS_S6A_ADDR + ' --mme_s6a=' + CI_MME_S6A_ADDR + ' --mme_s1c_IP=' + CI_MME_S1C_ADDR + ' --mme_s1c_name=eth3 --mme_s10_IP=' + CI_MME_S10_ADDR + ' --mme_s10_name=eth2 --mme_s11_IP=' + CI_MME_S11_ADDR + ' --mme_s11_name=eth1 --spgwc0_s11_IP=' + CI_DUMMY_SPGWC_ADDR + ' --from_docker_file --env_for_entrypoint')
            # S6A to HSS will be on `eth0`
            # S11 to SPGW-C will be on `eth1`
            # S10 to another MME will be on `eth2`
            # S1C to eNBs will be on `eth3`
            # In fact, these interfaces allocation is random.
            # entrypoint now takes care of it
            subprocess_run_w_echo('docker create --privileged --name ci-oai-mme --network ci-s6a --ip ' + CI_MME_S6A_ADDR + ' --env-file ./mme-env.list oai-mme:' + self.tag)
            subprocess_run_w_echo('docker network connect --ip ' + CI_MME_S11_ADDR + ' ci-s11 ci-oai-mme')
            subprocess_run_w_echo('docker network connect --ip ' + CI_MME_S10_ADDR + ' ci-s10 ci-oai-mme')
            subprocess_run_w_echo('docker network connect --ip ' + CI_MME_S1C_ADDR + ' ci-s1c ci-oai-mme')
            subprocess_run_w_echo('docker start ci-oai-mme')
            # Recovering the config part of the entrypoint execution
            tmpAwkFile=open('tmp.awk', 'w')
            tmpAwkFile.write('BEGIN{ok=1}{if ($0 ~/Initializing shared logging/){ok=0};if(ok==1){print $0}}END{}\n')
            tmpAwkFile.close()
            time.sleep(3)
            subprocess.run('docker logs ci-oai-mme 2>&1 | awk -f tmp.awk > archives/mme_config.log', shell=True)
            subprocess.run('rm -f tmp.awk', shell=True)
        else:
            # S6A to HSS will be on `eth0`
            subprocess_run_w_echo('docker run --privileged --name ci-oai-mme --network ci-s6a --ip ' + CI_MME_S6A_ADDR + ' -d oai-mme:' + self.tag + ' /bin/bash -c "sleep infinity"')
            # S11 to SPGW-C will be on `eth1`
            subprocess_run_w_echo('docker network connect --ip ' + CI_MME_S11_ADDR + ' ci-s11 ci-oai-mme')
            # S10 to another MME will be on `eth2`
            subprocess_run_w_echo('docker network connect --ip ' + CI_MME_S10_ADDR + ' ci-s10 ci-oai-mme')
            # S1C to eNBs will be on `eth3`
            subprocess_run_w_echo('docker network connect --ip ' + CI_MME_S1C_ADDR + ' ci-s1c ci-oai-mme')
            subprocess_run_w_echo('wget --quiet https://raw.githubusercontent.com/OPENAIRINTERFACE/openair-mme/develop/ci-scripts/generateConfigFiles.py -O ci-scripts/generateMmeConfigFiles.py')
            subprocess_run_w_echo('python3 ci-scripts/generateMmeConfigFiles.py --kind=MME --hss_s6a=' + CI_HSS_S6A_ADDR + ' --mme_s6a=' + CI_MME_S6A_ADDR + ' --mme_s1c_IP=' + CI_MME_S1C_ADDR + ' --mme_s1c_name=eth3 --mme_s10_IP=' + CI_MME_S10_ADDR + ' --mme_s10_name=eth2 --mme_s11_IP=' + CI_MME_S11_ADDR + ' --mme_s11_name=eth1 --spgwc0_s11_IP=' + CI_DUMMY_SPGWC_ADDR + ' --from_docker_file')
            subprocess_run_w_echo('docker cp ./mme-cfg.sh ci-oai-mme:/openair-mme/scripts')
            subprocess_run_w_echo('docker exec ci-oai-mme /bin/bash -c "cd /openair-mme/scripts && chmod 777 mme-cfg.sh && ./mme-cfg.sh" > archives/mme_config.log')

    def startHSS(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-hss:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            print('there is an entrypoint')
        else:
            subprocess_run_w_echo('docker exec -d ci-oai-hss /bin/bash -c "nohup ./bin/oai_hss -j ./etc/hss_rel14.json --reloadkey true > hss_check_run.log 2>&1"')

    def startMME(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-mme:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            print('there is an entrypoint -- no need')
        else:
            subprocess_run_w_echo('docker exec -d ci-oai-mme /bin/bash -c "nohup ./bin/oai_mme -c ./etc/mme.conf > mme_check_run.log 2>&1"')

    def stopHSS(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-hss:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            print('there is an entrypoint -- no need')
        else:
            subprocess_run_w_echo('docker exec ci-oai-hss /bin/bash -c "killall oai_hss"')

    def stopMME(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-mme:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            print('there is an entrypoint')
        else:
            subprocess_run_w_echo('docker exec ci-oai-mme /bin/bash -c "killall oai_mme"')

    def logsHSS(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-hss:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            # Recovering the run part of the entrypoint execution
            tmpAwkFile=open('tmp.awk', 'w')
            tmpAwkFile.write('BEGIN{ok=0}{if ($0 ~/jsonConfig/){ok=1};if(ok==1){print $0}}END{}\n')
            tmpAwkFile.close()
            time.sleep(1)
            subprocess.run('docker logs ci-oai-hss 2>&1 | awk -f tmp.awk > archives/hss_check_run.log', shell=True)
            subprocess.run('rm -f tmp.awk', shell=True)
        else:
            subprocess_run_w_echo('docker cp ci-oai-hss:/openair-hss/hss_check_run.log archives')

    def logsMME(self):
        res = ''
        # first check if tag exists
        try:
            res = subprocess.check_output('docker image inspect oai-mme:' + self.tag, shell=True, universal_newlines=True)
        except:
            sys.exit(-1)

        # check if there is an entrypoint
        entrypoint = re.search('entrypoint', str(res))
        if entrypoint is not None:
            # Recovering the run part of the entrypoint execution
            tmpAwkFile=open('tmp.awk', 'w')
            tmpAwkFile.write('BEGIN{ok=0}{if ($0 ~/Initializing shared logging/){ok=1};if(ok==1){print $0}}END{}\n')
            tmpAwkFile.close()
            time.sleep(1)
            subprocess.run('docker logs ci-oai-mme 2>&1 | awk -f tmp.awk > archives/mme_check_run.log', shell=True)
            subprocess.run('rm -f tmp.awk', shell=True)
        else:
            subprocess_run_w_echo('docker cp ci-oai-mme:/openair-mme/mme_check_run.log archives')

    def removeAllContainers(self):
        try:
            subprocess_run_w_echo('docker rm -f ci-cassandra ci-oai-hss ci-oai-mme')
        except:
            pass

def subprocess_run_w_echo(cmd):
    print(cmd)
    subprocess.run(cmd, shell=True)

def Usage():
    print('----------------------------------------------------------------------------------------------------------------------')
    print('sanityCheckDeploy.py')
    print('   Deploy a Sanity Check Test in the pipeline.')
    print('----------------------------------------------------------------------------------------------------------------------')
    print('Usage: python3 sanityCheckDeploy.py [options]')
    print('  --help  Show this help.')
    print('---------------------------------------------------------------------------------------------- Mandatory Options -----')
    print('  --action={CreateNetworks,RemoveNetworks,...}')
    print('-------------------------------------------------------------------------------------------------------- Options -----')
    print('  --tag=[Image Tag in registry]')
    print('------------------------------------------------------------------------------------------------- Actions Syntax -----')
    print('python3 sanityCheckDeploy.py --action=CreateNetworks')
    print('python3 sanityCheckDeploy.py --action=RemoveNetworks')
    print('python3 sanityCheckDeploy.py --action=DeployCassandra')
    print('python3 sanityCheckDeploy.py --action=DeployHSS --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=DeployMME --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=StartHSS --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=StartMME --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=StopHSS --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=StopMME --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=RetrieveLogsHSS --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=RetrieveLogsMME --tag=[tag]')
    print('python3 sanityCheckDeploy.py --action=RemoveAllContainers')

#--------------------------------------------------------------------------------------------------------
#
# Start of main
#
#--------------------------------------------------------------------------------------------------------

DSCT = deploySanityCheckTest()

argvs = sys.argv
argc = len(argvs)

while len(argvs) > 1:
    myArgv = argvs.pop(1)
    if re.match('^\-\-help$', myArgv, re.IGNORECASE):
        Usage()
        sys.exit(0)
    elif re.match('^\-\-action=(.+)$', myArgv, re.IGNORECASE):
        matchReg = re.match('^\-\-action=(.+)$', myArgv, re.IGNORECASE)
        action = matchReg.group(1)
        if action != 'CreateNetworks' and \
           action != 'RemoveNetworks' and \
           action != 'DeployCassandra' and \
           action != 'RemoveAllContainers' and \
           action != 'DeployHSS' and \
           action != 'DeployMME' and \
           action != 'StartHSS' and \
           action != 'StartMME' and \
           action != 'StopHSS' and \
           action != 'StopMME' and \
           action != 'RetrieveLogsHSS' and \
           action != 'RetrieveLogsMME':
            print('Unsupported Action => ' + action)
            Usage()
            sys.exit(-1)
        DSCT.action = action
    elif re.match('^\-\-tag=(.+)$', myArgv, re.IGNORECASE):
        matchReg = re.match('^\-\-tag=(.+)$', myArgv, re.IGNORECASE)
        DSCT.tag = matchReg.group(1)

if DSCT.action == 'CreateNetworks':
    DSCT.createNetworks()
elif DSCT.action == 'RemoveNetworks':
    DSCT.removeNetworks()
elif DSCT.action == 'DeployCassandra':
    DSCT.deployCassandra()
elif DSCT.action == 'DeployHSS':
    if DSCT.tag == '':
        print('Missing OAI-HSS image tag')
        Usage()
        sys.exit(-1)
    DSCT.deployHSS()
elif DSCT.action == 'DeployMME':
    if DSCT.tag == '':
        print('Missing OAI-MME image tag')
        Usage()
        sys.exit(-1)
    DSCT.deployMME()
elif DSCT.action == 'StartHSS':
    if DSCT.tag == '':
        print('Missing OAI-HSS image tag')
        Usage()
        sys.exit(-1)
    DSCT.startHSS()
elif DSCT.action == 'StartMME':
    if DSCT.tag == '':
        print('Missing OAI-MME image tag')
        Usage()
        sys.exit(-1)
    DSCT.startMME()
elif DSCT.action == 'StopHSS':
    if DSCT.tag == '':
        print('Missing OAI-HSS image tag')
        Usage()
        sys.exit(-1)
    DSCT.stopHSS()
elif DSCT.action == 'StopMME':
    if DSCT.tag == '':
        print('Missing OAI-MME image tag')
        Usage()
        sys.exit(-1)
    DSCT.stopMME()
elif DSCT.action == 'RetrieveLogsHSS':
    if DSCT.tag == '':
        print('Missing OAI-HSS image tag')
        Usage()
        sys.exit(-1)
    DSCT.logsHSS()
elif DSCT.action == 'RetrieveLogsMME':
    if DSCT.tag == '':
        print('Missing OAI-MME image tag')
        Usage()
        sys.exit(-1)
    DSCT.logsMME()
elif DSCT.action == 'RemoveAllContainers':
    DSCT.removeAllContainers()

sys.exit(0)
