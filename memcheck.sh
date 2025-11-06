#!/bin/bash

# This script runs Valgrind with proper suppression of system-level leaks

valgrind \
    --suppressions=valgrind.supp \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    ./webserv "$@"
