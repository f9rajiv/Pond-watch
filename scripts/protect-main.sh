#!/usr/bin/env bash
set -euo pipefail

repo="${1:-f9rajiv/ladybug-starter}"
branch="${2:-main}"

if ! command -v gh >/dev/null 2>&1; then
  echo "GitHub CLI (gh) is required. Install with: brew install gh"
  exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
  echo "Log in first: gh auth login"
  exit 1
fi

echo "Protecting ${repo}:${branch} ..."

gh api \
  --method PUT \
  "repos/${repo}/branches/${branch}/protection" \
  --input - <<EOF
{
  "required_status_checks": {
    "strict": true,
    "contexts": ["compile"]
  },
  "enforce_admins": false,
  "required_pull_request_reviews": {
    "required_approving_review_count": 1,
    "dismiss_stale_reviews": true
  },
  "restrictions": null,
  "required_linear_history": false,
  "allow_force_pushes": false,
  "allow_deletions": false,
  "block_creations": false,
  "required_conversation_resolution": false
}
EOF

echo "Done. '${branch}' now requires a pull request and passing compile checks."
