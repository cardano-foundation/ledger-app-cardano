# SignTransaction

**Description**

Given transaction inputs and transaction outputs (addresses + amounts), construct and sign transaction.

Note: Unlike Bitcoin or Ethereum, we do not plan to support signing already pre-constructed transactions. Instead, Ledger performs the transaction serialization and returns it to the client. Rationale behind this is that
1) we need to support attested transaction inputs which are not part of standardized in Cardano
2) we need to support BIP32 change address outputs (Ledger should not display change addresses)
3) we need to support long output addresses (Cardano addresses can be >10kb long. As a consequence, Ledger cannot assume that it can safely store even a single full output address)


TODO: design streaming protocol for this call
