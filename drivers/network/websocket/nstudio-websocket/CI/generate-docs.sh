#!/bin/bash
set -e
echo "-- Generating documentation."
echo "-- Node version: $(node -v)"
echo "-- NPM version: $(npm -v)"
echo "-- Python3 version: $(python3 -V)"

git fetch origin
git checkout ${CHECKOUT_REF/refs\/heads\//}

cd docs
bash build_docs.sh

echo "-- Documentation successfully generated."

if git diff --quiet; then
	echo "-- No documentation changes to commit."
	exit 0
fi

# Exit if we aren't a CI run to prevent messing with the current git config
if [ -z "${IS_CI}" ]; then
	exit 0
fi

REMOTE_URL="$(git config remote.origin.url)"
TARGET_REPO=${REMOTE_URL/https:\/\/github.com\//github.com/}
GITHUB_REPO=https://${GH_TOKEN:-git}@${TARGET_REPO}

git config user.name "Github Actions"
git config user.email "$COMMIT_AUTHOR_EMAIL"

git add ./generated
git pull
git commit -m "docs(ci): Update generated docs - $(git rev-parse --short HEAD) [skip ci]"
git push -q $GITHUB_REPO
