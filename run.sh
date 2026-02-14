#!/usr/bin/env bash

helpFunction() {
  echo ""
  echo "Usage: $0 <command>"
  echo -e "\tcommand: Tells the script what to do. Options:"
  echo -e "\t\tbuild: Builds and compiles the example"
  echo -e "\t\trun:   Builds and then runs the example"
  echo -e "\t\ttests: Builds and then runs the tests"
  echo -e "\t\thelp:  Shows this prompt"
}

buildProgram() {
    cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
    cmake --build build
}

if [[ $1 == "help" || -z $1 ]]; then
    helpFunction
elif [[ $1 == "build" ]]; then
    buildProgram
elif [[ $1 == "run" ]]; then
    buildProgram && ./bin/listener vcan0
elif [[ $1 == "tests" ]]; then
    buildProgram && ctest --test-dir build --output-on-failure
fi
