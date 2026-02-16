#!/bin/bash

function init {
    mkdir -p src
    ln -s ../AD include
    ln -s ../AD
    chown -h mobile:mobile ./AD
    chown -h mobile:mobile ./include
}

function main {
    case "$1" in
        "--init"|"-i"|"-init")
            init
            ;;
        *)
            echo "Not selected, automatically created"
            init
            ;;
    esac
}

main $@