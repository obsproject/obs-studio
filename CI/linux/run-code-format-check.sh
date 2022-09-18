#!/bin/bash

#
# This script enables running code format checks in a control
# fashion independent of the CI environment, but with the
# results of the CI environment.
#
# Execution takes place inside a docker container to be
# able to target the very specific software versions that
# are used for checking, without infecting the (local)
# development system.
#
# Example: clang-format-13 is not necessarily conveniently
#    available on the local system
#
#
set -e

DOCKER=docker
# DOCKER=podman

OBS_GIT_WORKING_COPY=$(git rev-parse --show-toplevel)
CI_LINUX="${OBS_GIT_WORKING_COPY}/CI/linux"
IMAGE_NAME_AND_TAG=obs-studio-build-format-check:latest

${DOCKER} build "${CI_LINUX}" \
    --file "${CI_LINUX}/run-code-format-check.dockerfile" \
    --tag "${IMAGE_NAME_AND_TAG}"

#
# Note that the format check actually modifies source files
# in place, so we cannot mount the OBS git repository
# in read-only mode.
#
${DOCKER} run -it --rm \
    -v "${OBS_GIT_WORKING_COPY}":/obs-git-working-copy \
    "${IMAGE_NAME_AND_TAG}" \
    /bin/bash -c 'pushd /obs-git-working-copy && ./CI/format-check.sh'

${DOCKER} run -it --rm \
    -v "${OBS_GIT_WORKING_COPY}":/obs-git-working-copy \
    "${IMAGE_NAME_AND_TAG}" \
    /bin/bash -c 'pushd /obs-git-working-copy && ./CI/check-changes.sh'

${DOCKER} run -it --rm \
    -v "${OBS_GIT_WORKING_COPY}":/obs-git-working-copy \
    "${IMAGE_NAME_AND_TAG}" \
    /bin/bash -c 'pushd /obs-git-working-copy && ./CI/check-cmake.sh'
