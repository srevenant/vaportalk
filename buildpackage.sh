#!/bin/sh

set -ex

git-buildpackage --git-upstream-branch=master --git-upstream-tree=branch
