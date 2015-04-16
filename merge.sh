#! /bin/bash
git checkout master
git pull
git checkout experimental
git pull

git checkout master
git merge experimental
git push
git checkout experimental

pushd ../libmaus-debian
git checkout master
git pull
git checkout experimental
git pull

git checkout master
git merge experimental
git push
git checkout experimental
popd
