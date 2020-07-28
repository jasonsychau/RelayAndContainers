#!/bin/sh

./next-wait "container-a" $OTHER_PORT

# run commands

./next-pass "container-a" $OTHER_PORT $MY_PORT

