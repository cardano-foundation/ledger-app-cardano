# Derive Address

**IOHK nicolas/vincent**: just as a note, the deterministic address recovery mechanism is describe in the BIP32, the path model used in IOHK and recommended to use is BIP44.

Derive `v2` address for a given BIP44 path, return, and show it to the user for confirmation.
We expect this call to be used for the address verification purposes (i.e., matching address on Ledger with the one on the screen).

❓(IOHK): Should we display the address to the user always or optionally? (Probably always unless there is another usecase of this call)

Note: Unlike BTC Ledger app which returns both public key and the corresponding address in the same instruction call, we split these two functionalities as they serve different purposes. Notably:
- `DeriveAddress` is weaker than `GetExtendedPublicKey` (Extended public key enables derivining non-hardened child keys, address does not). As such, (in the future) the app might apply more restrictions/user confirmations to get the public key.
- `GetAddress` is typically called only for the purpose of address verification. As such, it should belong to a valid address BIP32 path.

❓(IOHK):Check following statement:
Note: Derive address rejects any path that is not at least an "address" -- currently there is no use for "addresses" representing whole account (e.g., path 44'/1815'/0') nor addresses representing chain (e.g., path 44'1815'/0'/0)

❓(IOHK): Should we restrict this to only to valid chain number (e.g., internal/external)? Or even just to an external chain? (Note: There probably isn't much reason to let the user verify change addresses but we recommend allowing it anyway)
**IOHK nicolas/vincent**: you might want to provide the application a way to recover the change address (this will be useful for wallet recovery). So you need to restrict it to internal/external addresses but still allow the internal address too.

**Command**

| Field | Value                   |
| ----- | ----------------------- |
| CLA   | `0xD7`                  |
| INS   | `0x11`                  |
| P1    | unused❓ (see TBD above) |
| P2    | unused                  |
| Lc    | variable                |

**Response**

| Field   | Length   |
| ------- | -------- |
| address | variable |

Where `address` is encoded in ❓(VL) TBD: our choices are either raw bytes (byte-efficient) or base58-encoded (1:1 corresponsence with what the user sees. We need to decide which one is better for the protocol.

**Ledger responsibilities**

- ❓(IOHK/VL): specify if/when the address needs to be verified by the user
- For checking the input, Ledger has the same responsibilities as in `GetPublicKey`.
- On top of that, the address *cannot* be that of an account, or of external/internal address chain root, i.e. it needs to have 
  - `path_len >= 5`,
  - `path[2] == 0'` (account), and
  - ❓(see above): `path[3] in [0,1]` (internal/external chain)
- If the address needs to be verified, Ledger should show it *after* it responds to the host. Note that until user confirms the address, Ledger should not process any subsequent instruction call.
