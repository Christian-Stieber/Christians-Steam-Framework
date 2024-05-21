#! /bin/bash

/usr/bin/protoc "$@" 2>&1 | grep -v "libprotobuf WARNING .* No syntax specified for the proto file: " | grep -v "warning: Import .*\.proto is unused\."
exit 0
