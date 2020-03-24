<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OpenAirInterface Core Network Feature Set</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

1. [OpenAirInterface Core Network Block Diagram](#1-openairinterface-core-network-block-diagram)
2. [OpenAirInterface Core Network Fundamentals](#2-oai-core-network-fundamentals)
3. [OpenAirInterface Core Network Deployment](#3-oai-core-network-deployment)
4. [OpenAirInterface HSS Feature List](./FEATURE_SET_HSS.md)

# 1. OpenAirInterface Core Network Block Diagram #

![Block Diagram](./images/oai_cn_block_diagram.png)

# 2. OAI Core Network Fundamentals #

*  Network Access Control Functions
   -  Authentication and authorization
   -  Admission control
   -  Policy and charging enforcement
*  Packet Routing and Transfer Functions
   -  IP header compression function
   -  Packet screening.
*  Mobility Management Functions
   -  Reachability management for UE in ECM IDLE state
*  Security Functions
*  Radio Resource Management Functions
*  Network Management Functions (O&M)
   -  GTP C signaling based Load and Overload Control
   -  Load balancing between MME instances
   -  MME control of overload
   -  PDN GW control of overload

# 3. OAI Core Network Deployment #

*  Target OS
   -  Ubuntu 18.04 (bionic) server edition
   -  Red Hat Entreprise Linux 8
*  Hardware Requirements
   -  x86-64 Intel/AMD CPU
   -  At least one 1 network device
*  Linux Kernel
   -  Generic kernel is enough
*  Deployment feasible on:
   -  PC
   -  Server
   -  Container
   -  Virtual Machine


