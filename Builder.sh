#!/bin/sh

version_major=0
version_minor=0
version_patch=2

echo "BuilderScript Ver. $version_major.$version_minor.$version_patch"

main_branch=
packaging_branch=
work_dir=
config_file=
result_dir=
hook_dir=
chrooted=""
target_branch=

while getopts "m:p:h:w:r:ct:" opt; do
    case $opt in
	m)
	    echo "(m)ain branch: $OPTARG" >&2
	    main_branch=$OPTARG
	    ;;
	p)
	    echo "(p)ackaging branch: $OPTARG" >&2
	    packaging_branch=$OPTARG
	    ;;
	w)
	    echo "(w)orking dir: $OPTARG" >&2
	    work_dir=$OPTARG
	    ;;
	h)
	    echo "(h)ook dir: $OPTARG" >&2
	    hook_dir=$OPTARG
	    ;;
	r)
	    echo "(r)esult dir: $OPTARG" >&2
	    result_dir=$OPTARG
	    ;;
	c)
	    echo "Considering (c)hrooted build" >&2
	    chrooted=""
	    ;;
	t)
        echo "(t)arget branch: $OPTARG" >&2
	    target_branch="$OPTARG"
	    ;;
	\p)
	    echo "Invalid option: -$OPTARG" >&2
	    exit 1
	    ;;
	:)
	    echo "Option -$OPTARG requires an argument." >&2
	    exit 1
	    ;;
    esac
done

# convert to absolute path
result_dir=$(readlink -f "$result_dir")
hook_dir=$(readlink -f "$hook_dir")

# do not continue if any command fails
set -ex

# clean up
rm -rf "$work_dir"
mkdir "$work_dir"

if [ -z "$target_branch" ]; then
    bzr branch "$target_branch" "$work_dir/trunk"
    cd "$work_dir/trunk"
    bzr merge "$main_branch"
else
    # pull main branch and merge in packaging branch
    bzr branch $main_branch "$work_dir/trunk"
    cd "$work_dir/trunk"
fi

bzr merge  "$packaging_branch"

#mv packaging/debian trunk/
#cd trunk

if [ -f autogen.sh ]; then
    autoreconf -f -i
    aclocal
    grep -q IT_PROG_INTLTOOL configure.* && intltoolize
fi

# This is potentially dangerous
# but we force a native source format
# to prevent from dpkg-buildpackage bailing out
sed -i 's/quilt/native/g' debian/source/format

# Place any project specific replacements here
# sed -i 's/--with-xi/--with-xi --enable-gcov/g' debian/rules

# Extract some packaging information
version=`dpkg-parsechangelog | awk '/^Version/ {print $2}' | sed -e "s/\(.*\)-[0-9]ubuntu.*/\1/"`+bzr${trunkrev}
version=${version}ubuntu0+${packaging_rev}
sourcename=`dpkg-parsechangelog | awk '/^Source/ {print $2}'`
# Generate the actual source package
cd ..
tar -czf ${sourcename}_${version}.orig.tar.gz trunk

cd trunk

if [ -z "$chrooted" ]; then
    export BUILD_DIR=$(readlink -f ".")
    export RESULT_DIR=$result_dir
    
    cd "$hook_dir"
    ./D00dependency_hooks
    cd "$BUILD_DIR"
    debuild -uc -us -d
    cd "$hook_dir"
    "./B00build_hooks"
    cd "$BUILD_DIR"
else
    # Make our result dir known to pbuilder env
    echo "$result_dir" > 'ReportDir'
    pdebuild -- \
	--inputfile "ReportDir" \
	--hookdir "$hook_dir" \
	--bindmounts "$result_dir"
fi

rm -f -- "ReportDir"

