<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OpenAirInterface Core Network Mobility Management Entity Feature Set</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

1. [MME Fundamentals](#1-mme-fundamentals)
2. [OAI MME Available Functions](#2-oai-mme-available-functions)
3. [OAI MME Conformance Functions](#3-oai-mme-conformance-functions)

# 1. MME Fundamentals #

(from [Wikipedia.org](https://en.wikipedia.org/wiki/System_Architecture_Evolution))

The Mobility Management Entity is the key control-node for the LTE access-network.

* It is responsible for idle mode UE (User Equipment) paging and tagging procedure including retransmissions.
* It is involved in the bearer activation/deactivation process and is also responsible for choosing the SGW for a UE at the initial attach and at time of intra-LTE handover involving Core Network (CN) node relocation.
* It is responsible for authenticating the user (by interacting with the HSS).
* The Non Access Stratum (NAS) signaling terminates at the MME and it is also responsible for generation and allocation of temporary identities to UEs.
* It checks the authorization of the UE to camp on the service provider's Public Land Mobile Network (PLMN) and enforces UE roaming restrictions.
* The MME is the termination point in the network for ciphering/integrity protection for NAS signaling and handles the security key management.
* Lawful interception of signaling is also supported by the MME.
* The MME also provides the control plane function for mobility between LTE and 2G/3G access networks with the S3 interface terminating at the MME from the SGSN.
* The MME also terminates the S6a interface towards the HSS for roaming UEs. 

# 2. OAI MME Available Functions #

**ID** | **Interface** | **Status**         | **Comments**                                   | **Protocols**
------ | ------------- | ------------------ | ---------------------------------------------- | -------------
1      | S1-MME        | :heavy_check_mark: | Still Rel10. Should be upgraded to Rel15 soon. | S1AP/SCTP
2      | S11           | :heavy_check_mark: | Should be upgraded to Rel15 soon.              | GTPv2-C/UDP
3      | S3            | :x:                | To interface SGSN, not planned.                | GTPv2-C/UDP
4      | S6a           | :heavy_check_mark: | Still Rel14. Should be upgraded to Rel15 soon. | freeDiameter/TCP-SCTP
5      | S10           | :heavy_check_mark: |                                                | GTPv2-C/UDP

# 3. OAI MME Conformance Functions #

Based on document **3GPP TS 23.401 V15.5.0 §4.4.2**.

| **ID** | **Classification**                                           | **Status**         | **Comments**                                   |
| ------ | ------------------------------------------------------------ | ------------------ | ---------------------------------------------- |
| 1      | NAS signalling                                               | :heavy_check_mark: |                                                |
| 2      | NAS signalling security                                      | :heavy_check_mark: |                                                |
| 3      | Inter CN node signalling for mobility between 3GPP access networks (terminating S3) | :x:                | Intra LTE HO only       |
| 4      | UE Reachability in ECM IDLE state                            | :heavy_check_mark: | :construction: Work in Progress                |
|        | -- including control,                                        |                    |                                                |
|        |    execution of paging retransmission and                    |                    |                                                |
|        |    optionally Paging Policy Differentiation                  |                    |                                                |
| 5      | Tracking Area list management                                | :heavy_check_mark: |                                                |
| 6      | Mapping from UE location (e.g. TAI) to time zone             | :x:                |                                                |
|        | -- Signalling a UE time zone change associated with mobility |                    |                                                |
| 7      | PDN GW and Serving GW selection                              | :heavy_check_mark: | spgw selection & neighboring MME selection via WRR |
| 8      | MME selection for handovers with MME change                  | :heavy_check_mark: | S1 (inter (S10) and intra MME S1AP handover), X2 HO supported |
| 9      | SGSN selection for handovers to 2G/3G 3GPP access networks   | :x:                |                                                |
| 10     | Roaming (S6a towards home HSS)                               | :x:                |                                                |
| 11     | Authentication                                               | :heavy_check_mark: | Also NAS messages inside the S10 at mobility   |
| 12     | Authorization                                                | :heavy_check_mark: | HSS (UE AMBR + defaults), PCRF (APN/ Bearer Level QoS) |
| 13     | Bearer management functions                                  | :heavy_check_mark: | Dedicated bearers are supported                |
|        | --  including dedicated bearer establishment                 |                    |                                                |
| 14     | Lawful Interception of signalling traffic                    | :x:                |                                                |
| 15     | Warning message transfer function                            | :x:                |                                                |
|        | -- including selection of appropriate eNodeB                 | :x:                |                                                |
| 16     | UE Reachability procedures                                   | :heavy_check_mark: |                                                |
| 17     | Support Relaying function (RN Attach/Detach)                 | :x:                |                                                |
| 18     | Change of UE presence in Presence Reporting Area             | :x:                |                                                |
|        | -- reporting upon PCC request                                |                    |                                                |
| 19     | For the Control Plane CIoT EPS Optimisation                  | :x:                | Future : NB IoT , SMS                          |
|        | a) transport of user data (IP and Non IP)                    |                    |                                                |
|        | b) local Mobility Anchor point                               |                    |                                                |
|        | c) header compression (for IP user data)                     |                    |                                                |
|        | d) ciphering and integrity protection of user data           |                    |                                                |
|        | e) Lawful Interception of user traffic not transported via the Serving GW (e.g. traffic using T6a) | |                             |
