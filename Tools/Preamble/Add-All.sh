#! /bin/sh

find ../../Sources ../../Headers \( \( -path ../../Headers/External -o -path ../../Headers/boost \) -prune \) -o \( \( -name "*.cpp" -o -name "*.hpp" \) -exec ./Add.sh {} \; -exec git add {} \; \)
