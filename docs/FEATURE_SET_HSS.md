<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OpenAirInterface Core Home Subscriber Server Feature Set</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

1. [HSS Fundamentals](#1-hss-fundamentals)
2. [OAI HSS Available Interfaces](#2-oai-hss-available-interfaces)
3. [OAI HSS Architecture](#3-oai-hss-architecture)

# 1. HSS Fundamentals #

(from [Wikipedia.org](https://en.wikipedia.org/wiki/System_Architecture_Evolution))

The Home Subscriber Server is a central database that contains user-related and subscription-related information.

The functions of the HSS include functionalities such as:

*  mobility management
*  call and session establishment support
*  user authentication and
*  access authorization.

The HSS is based on pre-Rel-4 Home Location Register (HLR) and Authentication Center (AuC). 

# 2. OAI HSS Available Interfaces #

| **ID** | **Interface** | **Status**         | **Comment**                           |
| ------ | ------------- | ------------------ | ------------------------------------- |
| 1      | S6a           | :heavy_check_mark: | between HSS and MME                   |
| 2      | S6d           | :x:                | between HSS and SGSN                  |
| 3      | S6t           | :x:                | between HSS and SCEF                  |
| 4      | S6c           | :x:                | between HSS and central SMS functions |
|        |               |                    | (SMS-GMSC, SMS Router)                |

SCEF = Service Capability Exposure Function

# 3. OAI HSS Architecture #

Contributed fork of freeDiameter allowing duplicate AVPs among several dictionaries

Persistence of data done by Cassandra


