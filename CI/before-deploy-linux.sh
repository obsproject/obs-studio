set -ex

export GIT_TAG=$(git describe --abbrev=0)

echo "git tag: $GIT_TAG"

cd ./build

sudo checkinstall -y --type=debian --fstrans=no --nodoc \
	--backup=no --deldoc=yes --install=no \
	--pkgname=obs-studio --pkgversion="$GIT_TAG" \
	--pkglicense="GPLv2" --maintainer="OBSProject" \
	--pkggroup="video" \
	--pkgsource="https://github.com/obsproject/obs-studio" \
	--pakdir="../nightly"

cd -
