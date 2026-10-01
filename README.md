# BlazingSystems — Project Portfolio

A curated collection of practical software, embedded-system, networking, and browser-based projects developed as engineering exercises and working prototypes.

## Portfolio Structure

| Area | Project | Type | Status |
|---|---|---|---|
| Marine tools | Marine Operations Logbook | Offline web application | Demonstration build |
| Marine tools | Draft Reading Trainer | Offline training application | Working build |
| Marine tools | Daily Operations Update | Offline reporting tool | Demonstration build |
| Games | BlazeSystems Arcade | Browser game/launcher study | Demonstration build |
| Games | Last Stand | HTML5 strategy game | Working build |
| Embedded | ESP8266 Thermostat | Firmware + local web UI | Source-ready |
| Networking | Captive Portal UI | Standalone portal interface study | Demonstration build |

## Design Principles

Projects in this repository emphasize:

- offline-first operation;
- compatibility with modest hardware;
- local data storage where practical;
- simple interfaces that remain usable without cloud services;
- clear separation between demonstrations and hardware-validated builds;
- synthetic sample data in public examples.

## Live Preview

The repository includes a portfolio landing page at `index.html`. Finished HTML projects use a `preview.html` portfolio landing page that links to the runnable `index.html` demonstration. This keeps project presentation separate from the working demo while remaining GitHub Pages friendly.

## Technical Notes

Hardware-dependent projects should be tested on the intended board and electrical load before deployment. Browser demonstrations are educational/portfolio builds and are not substitutes for certified commercial, maritime, medical, safety, or industrial systems.

## Repository Notice

Public examples are intentionally generic. They do not contain client records, employer records, production credentials, private network backups, or proprietary operational datasets.

See [NOTICE.md](NOTICE.md) for the public-demo policy.
