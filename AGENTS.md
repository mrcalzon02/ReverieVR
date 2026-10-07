# ReverieVR AI Agent Entry Point

This is a routing document, not a second copy of project doctrine.

Before reading, changing, validating, committing, or pushing ReverieVR:

1. Load **AI Project Manager v3.0** and its **Repository Execution Protocol** when available.
2. Read `docs/project/INSTRUCTIONS.md` completely.
3. Read `docs/project/EXECUTION_CONTRACT.md` completely.
4. Read `docs/project/EXECUTION_STATE.md`, then the relevant portions of `docs/project/BACKLOG.md` and `docs/project/ACCEPTANCE_LEDGER.md`.
5. Read `docs/project/APK_BUILD_ORDER.md` before implementation work.
6. Reconcile those documents with actual repository and remote `main` state before mutation.
7. Use `docs/records/` for durable architectural decisions.
8. For native game/module work, read `docs/native/README.md`, `docs/native/CONTRACT_INDEX.md`, and the linked API/content standards before implementation; update mapped documentation in the same scoped change when implementation changes and run `python3 scripts/verify-native-doc-sync.py` with the appropriate change-range option.
9. Follow DEEFM for every material operation: **Intent -> Execute -> Observe -> Verify -> Claim**.

Conversation history, memory, prompts, and handoffs are context. They are not proof of repository state.
