#! /usr/bin/env bash

rm -f ho
echo "We are resuming a session for the CacheGen tool development"  > ho
echo "Relevant context has been concatenated in this file"         >> ho
# -------------------------------------------------------------
echo "" >> ho
echo "I have attached the following " >> ho
echo "- planning/PROJECT_CORE.md"     >> ho
echo "- planning/PROJECT_STATUS.md"   >> ho
echo "- CLAUDE.md"                    >> ho
echo "- templates/TASK_TEMPLATE.md"   >> ho
echo "All experiment files need to follow the task template exactly. " >> ho
# -------------------------------------------------------------
echo "" >> ho
echo "The previous session hand off file path: " >> ho
echo "- session_handoffs/session_handoff-$1.md"     >> ho
echo "" >> ho
## -------------------------------------------------------------
echo "" >> ho
cat planning/PROJECT_CORE.md          >> ho
cat planning/PROJECT_STATUS.md        >> ho
cat CLAUDE.md                         >> ho
cat templates/TASK_TEMPLATE.md        >> ho
cat session_handoffs/session_handoff-$1.md >> ho
