# LPB Neon Aero Portal

**Status:** WORKING SOURCE / TARGET-INTEGRATION REQUIRED  
**Canonical recovered build:** v1.2 multi-coinslot fix

A lightweight custom captive portal frontend for LPB Piso WiFi Lite.

## Reconciled features

- Insert Coin flow
- Multi-coinslot selection
- Coin status polling
- Time/amount display
- Claim/cancel flow
- Wi-Fi rates modal
- Pause/resume and voucher-oriented portal structure
- Lightweight single-page UI intended for low-resource captive-portal environments

## Integration notes

This frontend depends on the LPB server endpoints and template variables supplied by the target installation. The recovered source references LPB-side routes such as coin-status and claim actions, so browser-only testing cannot prove full production behavior.

Before replacing a live portal:

1. Back up the original LPB portal.
2. Test on a non-production unit.
3. Confirm the exact server endpoints and template variables used by that LPB version.
4. Test every available coin slot.
5. Test claim, cancel, pause/resume, voucher, reconnect, and mobile captive-browser behavior.

## Why this build is canonical

v1.2 is the latest recovered multi-coinslot fix branch. Earlier v1/v1.1 packages are preserved as historical versions rather than treated as current.

## Provenance

Recovered from the BlazingSystems project archive and reconciled against earlier portal iterations before publication.
