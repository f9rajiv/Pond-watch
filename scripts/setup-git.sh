#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"

git -C "$repo_root" config core.hooksPath .githooks
chmod +x "$repo_root/.githooks/pre-push"

echo "Git hooks enabled for $(basename "$repo_root")."
echo "Direct pushes to main are blocked locally."
echo ""
echo "Next: run ./scripts/protect-main.sh after 'gh auth login' to protect main on GitHub."
