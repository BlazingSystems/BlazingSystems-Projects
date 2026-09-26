# BlazingSystems Projects

Verified usable builds and release-ready source from **BlazingSystems**.

This repository contains projects that are usable as delivered or are sufficiently complete to serve as the canonical public build. Hardware-dependent projects still carry explicit validation notes where appropriate.

## Projects

### Marine / Field Tools

- [Vessel LogBook V10.4](marine/vessel-logbook/) — **FINAL / USABLE**
  - offline vessel/cargo reporting and draft-survey workstation
  - canonical recovered build: V10.4 Audio + Haptic Final

- [DraftSight Trainer V5](marine/draftsight-trainer/) — **FINAL / AUDITED**
  - offline vessel draught-mark reading trainer with timed scoring and configurable scenarios

- [Vessel Daily Updates](marine/vessel-daily-updates/) — **USABLE**
  - lightweight standalone daily vessel-update tool

### Games / Browser Tools

- [BlazeSystems Arcade](games/blazesystems-arcade/) — **USABLE / PORTABLE HYBRID**
  - latest canonical branch from the earlier Friv/SNES/MULTIEMU lineage

- [Last Stand](games/last-stand/) — **USABLE HTML5 BUILD**
  - lightweight browser strategy/endless-mode game

### Embedded

- [ESP8266 Thermostat](embedded/esp8266-thermostat/) — **SOURCE READY**
  - standalone AP thermostat UI with DS18B20 support, relay control, persistence, hysteresis, and anti-short-cycle logic
  - exact-board hardware validation still required

### Networking / PisoWiFi

- [LPB Neon Aero Portal](networking/lpb-neon-aero-portal/) — **WORKING SOURCE**
  - LPB PisoWiFi Lite captive-portal rebuild with multi-coinslot handling
  - target LPB endpoint/version integration must still be validated on the deployment server

## Repository rules

1. One project per directory.
2. Canonical builds are selected by reconciliation, not filename alone.
3. Older/superseded branches move to Archives rather than being deleted.
4. Source-only or unvalidated development belongs in Labs.
5. Prototypes and reverse-engineering work belong in Experiments.
6. Private financial, family, health, account, credential, or employer-confidential material is excluded.
7. Hardware-dependent software is not described as production-ready without real validation.

## Related repositories

- Labs: https://github.com/BlazingSystems/BlazingSystems-Labs
- Experiments: https://github.com/BlazingSystems/BlazingSystems-Experiments
- Archives: https://github.com/BlazingSystems/BlazingSystems-Archives
- Master recovery ledger: https://github.com/BlazingSystems/BlazingSystems/blob/main/PROJECTS.md
