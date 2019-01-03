## Get App Version

**Description**

Gets the version of the app running on Ledger. 
Could be called at any time

**Command**

|Field|Value|
|-----|-----|
| INS | ❓ |
| P1 | unused |
| P2 | unused |
| Lc | 0 |

**Response**

|Field|Length|
|-----|-----|
|major| 1 |
|minor| 1 |
|patch| 1 |

Tuple [`major`, `minor`, `patch`] represents app version.


**Error codes**
- 0x9000 OK
- ❓ other codes


**Ledger responsibilities**

- Check:
  - Check `P1 == 0`
  - Check `P2 == 0`
  - Check `Lc == 0`
- Respond with app version


