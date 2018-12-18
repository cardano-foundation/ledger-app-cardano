# Cardano Ledger App design document


## Cardano app communication
Cardano app communicates with APDU protocol.
Each "logical" call consists of a series of APDU exchanges where APDU is in the following form

### Command


|field   |CLA|INS|P1 |P2 |Lc |Data|
|--------|---|---|---|---|---|----|
|size (B)| 1 | 1 | 1 | 1 | 1 |var |


Where
- CLA is ????
- INS is the instruction number
- P1 and P2 are instruction parameters
- Lc is length of the data body
- Data is binary data


### Response

???



## GetVersion call

Get version proceeds as following

*Command*

|CLA|INS| P1 | P2 | Lc | Le|
|---|---|----|----|----|---|
|   |   |  0 |  0 | 0  | ? |

*Ledger responsibilities*
- Check P1 == 0
- Check P2 == 0
- Check Lc == 0

*Response*

- SW = ????
- Data: 3 bytes [Major, Minor, Patch] representing app version
