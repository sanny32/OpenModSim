#!/usr/bin/env bash
set -euo pipefail

: "${GITHUB_REF:?}" "${PREFIX:?}"

gh cache list --ref "$GITHUB_REF" --key "$PREFIX" --limit 100 \
    --json id,key --jq '.[] | "\(.id) \(.key)"' |
    while read -r id key; do
        case "$key" in
            "$PREFIX"*)
                if [[ "$key" != "${KEEP:-}" ]]; then
                    gh cache delete "$id"
                fi
                ;;
        esac
    done
