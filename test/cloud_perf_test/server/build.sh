dir="$(dirname "$0")" || exit 1
cd "$dir" || exit 1

namespace="$1"
tag="$2"
base_image="$3"

if [ $# -ne 3 ]; then
    cat >&2 <<EOF
ERROR: Wrong number of arguments

Synopsis: $0  <namespace>  <tag>  <base image>
EOF
    exit 1
fi

if ! sh "resources/docker/build.sh" "$namespace" "$tag" "$base_image"; then
    cat 1>&2 <<EOF
ERROR:
ERROR:
ERROR: ######################################################################
ERROR: ######################################################################
ERROR: ##                                                                  ##
ERROR: ##                         BUILD FAILED!                            ##
ERROR: ##                                                                  ##
ERROR: ######################################################################
ERROR: ######################################################################
EOF
    exit 1
fi

if ! sh "resources/docker/publish.sh" "$namespace" "$tag"; then
    cat 1>&2 <<EOF
ERROR:
ERROR:
ERROR: ######################################################################
ERROR: ######################################################################
ERROR: ##                                                                  ##
ERROR: ##                         PUBLISH FAILED!                          ##
ERROR: ##                                                                  ##
ERROR: ######################################################################
ERROR: ######################################################################
EOF
    exit 1
fi
