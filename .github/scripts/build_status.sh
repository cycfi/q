#!/bin/bash
###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# Draws and publishes Q's build grid, status/build.svg on the www branch. Q's
# builds are spread over workflows that run on different changes, so the
# grid takes each one's latest develop run: this run for the workflow calling
# it, the latest completed run for the others. Cells are prefixed Q or QPlug.
# Needs GH_TOKEN, and runs from the repository root.

set -euo pipefail

workflows=(
   "build.yml:Q"
   "q_plug.yml:QPlug"
   "q_plug-backends.yml:QPlug"
)

: > jobs.jsonl
for entry in "${workflows[@]}"; do
   wf=${entry%%:*}
   prefix=${entry#*:}
   run=$(gh run list -R "$GITHUB_REPOSITORY" -w "$wf" -b develop -L 20 \
      --json databaseId,status \
      --jq "[.[] | select(.databaseId == $GITHUB_RUN_ID or .status == \"completed\")][0].databaseId // empty")
   if [ -z "$run" ]; then
      echo "No develop run of $wf yet"
      continue
   fi
   gh api "repos/$GITHUB_REPOSITORY/actions/runs/$run/jobs" --paginate \
      --jq ".jobs[] | {name: (\"$prefix \" + .name), conclusion}" >> jobs.jsonl
done

python3 .github/scripts/status_grid.py --title "Q build" \
   --exclude ' Status$' --out build.svg jobs.jsonl
bash .github/scripts/publish_status.sh build.svg
