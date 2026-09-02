# BRIEFING — 2026-09-01T09:56:30Z

## Mission
Perform exhaustive specification mining of all ArduStim TFTv2 and Pattern Gen wheel pattern definitions, bitmasks, signal layouts, and engine presets for ECUSniff.

## 🔒 My Identity
- Archetype: Specification Miner
- Roles: Teamwork specialist, Spec Miner
- Working directory: g:\semester 7\ECUSniff\.agents\survey_spec_miner
- Original parent: 3238efb3-ee5d-4798-adf9-be5de9e2bccf
- Milestone: M1 - Survey & Specification Mining

## 🔒 Key Constraints
- Probing only, do NOT implement code changes in the main codebase (read-only mining)
- Exhaustively enumerate all wheel definitions in `wheel_defs.h` and associated sources
- Map exact ArduStim friendly names, categories, segments, teeth, cycle lengths, bit definitions
- Identify bitmask conventions and edge cases

## Current Parent
- Conversation ID: 3238efb3-ee5d-4798-adf9-be5de9e2bccf
- Updated: 2026-09-01T09:56:30Z

## Loaded Skills
- Source: g:\semester 7\ECUSniff\.agents\skills\ecu-pattern-designer\SKILL.md
  - Local copy: g:\semester 7\ECUSniff\.agents\survey_spec_miner\ecu-pattern-designer.md
  - Core methodology: Automotive timing math, 0-720 deg cycles, missing tooth formulas, RMT buffer rules, sniffer phase-locking.

## Task Summary
- **What to build**: Comprehensive spec mining report (`spec_mining_report.md`) & `handoff.md`
- **Success criteria**: All ~70 wheel definitions indexed with friendly name, short name, brand, pattern type, cycle length, segments/teeth, bitmasks, edge cases, and detailed breakdown of critical patterns.
- **Interface contracts**: `wheel_defs.h`, `enums.h`, `ardustim.h`, `PinMapping.h`, `WheelPatternManager.cpp`, `gear_generator.js`, `scope_generator.js`
- **Code layout**: Output reports in `.agents/survey_spec_miner/`

## Key Decisions & Findings
- Total wheel patterns: exactly 70 presets (indices 0 to 69).
- Array length match: 100% verified against metadata.
- Bit layout: Bit 0 = CKP (`0x01`), Bit 1 = CMP1 (`0x02`), Bit 2 = CMP2 (`0x04`), Bit 3 = KNOCK (`0x08`).
- Active CMP2 presets: `BMW_N20` (dual cam VVT, values 6 and 7) and `GM_LS1_CRANK_AND_CAM` (value 4).
- Timing formula: $T_{\text{seg}} = \frac{D \times 10^6}{6 \times E \times RPM}\ \mu\text{s}$.

## Artifact Index
- g:\semester 7\ECUSniff\.agents\survey_spec_miner\spec_mining_report.md — Full specification mining report
- g:\semester 7\ECUSniff\.agents\survey_spec_miner\handoff.md — Handoff report for orchestrator
- g:\semester 7\ECUSniff\.agents\survey_spec_miner\parsed_wheels.json — Complete parsed JSON dataset of all 70 wheels
- g:\semester 7\ECUSniff\.agents\survey_spec_miner\all_70_table.md — Full Markdown table of all 70 wheels
