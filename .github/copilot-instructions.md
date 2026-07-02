# AK firmware — agent instructions

This project is firmware for the AK (Active Kernel) event-driven MCU framework.

You have access to the ak-docs MCP server. Use it as the source of truth — do not guess AK APIs from memory.

Before writing or changing any firmware:

1. Call get_ak_guardrails first. Never modify application/sources/ak/, boot/, application/sources/networks/, or application/sources/common/.
2. For a new task/driver/screen, call get_ak_guide (create-task, create-driver, create-screen, use-timer, isr-bridge, tune-pools) and follow the steps and skeleton exactly.
3. For any kernel function/macro, call get_ak_api to get the exact signature and arguments. Use search_ak_docs when you do not know the symbol name.

Hard rules:
- Handlers must be non-blocking — no delay(), no busy-wait. Use a timer that posts a signal instead.
- Tasks communicate only via messages (task_post_*), never direct calls or shared globals.
- User signals start at AK_USER_DEFINE_SIG (10); task priorities are LEVEL_1..7 (0 is reserved).
- Common message payload ≤ 64 bytes; max 7 references per message; pools are fixed size.
