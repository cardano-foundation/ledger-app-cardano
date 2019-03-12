## Get App Version

**Description**

Gets the version of the app running on Ledger. 
Could be called at any time

**Command**

|Field|Value|
|-----|-----|
| INS | `0x01` |
| P1 | unused |
| P2 | unused |
| Lc | 0 |

**Response**

|Field|Length|
|-----|-----|
|major| 1 |
|minor| 1 |
|patch| 1 |
|flags| 1 |

Tuple [`major`, `minor`, `patch`] represents app version.

Flag meanings could be found in [src/getVersion.c](../src/getVersion.c)

|Mask|Value|Meaning|
|----|-----|-------|
|0x01|0x01 |Devel version of the app|


**Ledger responsibilities**

- Check:
  - Check `P1 == 0`
  - Check `P2 == 0`
  - Check `Lc == 0`
- Respond with app version


