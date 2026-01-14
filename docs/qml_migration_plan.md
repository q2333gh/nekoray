# QML Migration Plan

Goal: Gradually migrate the UI to pure QML while keeping the C++ core stable and testable. Each phase must build and run independently and be easy to roll back.

Principles
- Keep functionality stable: add QML shell first, then replace modules.
- Keep C++ as the core logic provider; QML is the view layer.
- Every phase is buildable and testable.
- Use clear acceptance criteria and tests for each step.

Phase 0: Preparation and Baseline (1-3 days)
- Create a UI migration branch and milestones.
- Inventory MainWindow features and signal/slot usage.
- Decide Qt/QML modules (Qt Quick, Controls, QML, Network).
- Define a stable UI API layer (e.g., UiFacade or AppController) for QML.
- Create a baseline test checklist and at least 1-2 automated smoke tests.

Acceptance
- Current build passes.
- Baseline tests run successfully.

Phase 1: QML Shell (2-5 days)
- Add a QML entry point (QQuickWindow/QQuickView).
- Start with an empty QML home view.
- Keep QWidget UI as the default, add a runtime flag to switch.
- Expose UiFacade to QML and validate basic calls.

Acceptance
- Both UI modes can launch.
- QML can call a simple C++ method (e.g., ping).

Phase 2: Low-Risk Modules (1-2 weeks)
- Migrate low-risk screens first (About/Settings/Log Viewer).
- Option A: QWidget main UI with embedded QML panels.
- Option B: QML main UI with QWidget subwindows as needed.
- Expose models via QAbstractListModel to QML.

Acceptance
- Migrated modules feature parity with QWidget versions.
- Manual tests pass.

Phase 3: Core List and Actions (2-4 weeks)
- Migrate Server list and its core actions (add, select, copy, delete, test).
- Implement QML models for sorting, filtering, and selection.
- Keep QWidget version as a fallback.

Acceptance
- Feature parity for server list and actions.
- Performance acceptable with large lists.

Phase 4: Tray and System Integration (1-2 weeks)
- Drive tray menu from QML triggers but use C++ logic.
- Keep hotkeys, system proxy, VPN logic in C++.

Acceptance
- Tray actions work (Exit/Start/Stop/Add from Clipboard).
- Start/exit flows complete without deadlock.

Phase 5: Full Switch (1-2 weeks)
- Make QML the default UI.
- Keep QWidget UI as a fallback behind a flag.
- Gradually remove QWidget UI after validation.

Acceptance
- QML is stable as default.
- QWidget fallback still works during transition.

Testing Strategy (ongoing)
- Manual smoke: startup, exit, add node, URL Test, copy, subscription update.
- Automated tests:
  - QML unit tests (qmltestrunner).
  - UI automation (WinAppDriver/Squish/pytest-qt).
- For each migrated module, run side-by-side checks versus QWidget.

Risks and Mitigations
- Performance with large lists: use QML ListView with delegate recycling.
- Cross-thread updates: keep models in C++ and push updates safely to QML.
- Missing actions: maintain a feature matrix and check off items per phase.
