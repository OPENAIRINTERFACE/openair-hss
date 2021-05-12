#!/bin/bash

set -euo pipefail

HSS_DOMAIN=${HSS_FQDN#*.}
HSS_HOSTNAME=${HSS_FQDN%%.*}
MME_HOSTNAME=`echo $HSS_HOSTNAME | sed -e "s#hss#mme#"`
# Defaults
ROAMING_ALLOWED=${ROAMING_ALLOWED:-true}
# Defaults for data_provisioning_users script
# Values in bits per second
UE_AMBR_UL=${UE_AMBR_UL:-20000000}
UE_AMBR_DL=${UE_AMBR_DL:-40000000}
APN1_AMBR_UL=${APN1_AMBR_UL:-10000000}
APN1_AMBR_DL=${APN1_AMBR_DL:-20000000}


CONFIG_FILES=`ls /openair-hss/etc/*.conf /openair-hss/etc/hss*json`

for c in ${CONFIG_FILES}; do
    # grep variable names (format: ${VAR}) from template to be rendered
    VARS=$(grep -oP '@[a-zA-Z0-9_]+@' ${c} | sort | uniq | xargs)

    # create sed expressions for substituting each occurrence of ${VAR}
    # with the value of the environment variable "VAR"
    EXPRESSIONS=""
    for v in ${VARS}; do
	NEW_VAR=`echo $v | sed -e "s#@##g"`
        if [[ "${!NEW_VAR}x" == "x" ]]; then
            echo "Error: Environment variable '${NEW_VAR}' is not set." \
                "Config file '$(basename $c)' requires all of $VARS."
            exit 1
        fi
        EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}|g"
    done
    EXPRESSIONS="${EXPRESSIONS#';'}"

    # render template and inline replace config file
    sed -i "${EXPRESSIONS}" ${c}
done

# Creating a log folder
mkdir -p /openair-hss/logs

# Create the certificates
pushd /openair-hss/scripts
./wait-for-it.sh  ${cassandra_Server_IP}:9042
./data_provisioning_users --ue-ambr-max-requested-bandwidth-ul ${UE_AMBR_UL} --ue-ambr-max-requested-bandwidth-dl ${UE_AMBR_DL} --apn ${APN1} --apn1-ambr-max-requested-bandwidth-dl ${APN1_AMBR_DL} --apn1-ambr-max-requested-bandwidth-ul ${APN1_AMBR_UL} --apn2 ${APN2} --key ${LTE_K} --imsi-first ${FIRST_IMSI} --msisdn-first 00000001 --mme-identity oai-mme.${REALM} --no-of-users ${NB_USERS} --realm ${REALM} --truncate False --verbose True --cassandra-cluster ${cassandra_Server_IP}
./data_provisioning_mme --id 3 --mme-identity ${MME_HOSTNAME}.${REALM} --realm ${REALM} --ue-reachability 1 --truncate False  --verbose True -C ${cassandra_Server_IP}
./make_certs.sh ${HSS_HOSTNAME} ${REALM} ${PREFIX}
popd

exec "$@"
