#!/usr/bin/env bash
PARAMS=""
while (( "$#" )); do
    case "$1" in
        -b|--binary-name)
            BINARY_NAME=$2
            shift 2
            ;;
        -t|--tag-file)
            TAG_FILE=$2
            shift 2
            ;;
        -p|--policy-dir)
            POLICY_DIR=$2
            shift 2
            ;;
        --) # end argument parsing
            shift
            break
            ;;
        -*|--*=) # unsupported flags
        echo "Error: Unsupported flag $1" >&2
        exit 1
        ;;
        *) # preserve positional arguments
            PARAM="$PARAMS $1"
            shift
            ;;
    esac
done
# set positional arguments in their proper place
eval set -- "$PARAMS"

dot="$(cd "$(dirname "$0")"; pwd)"
POLICY_DIR=$dot/$POLICY_DIR
TAG_FILE=$dot/$TAG_FILE
FREEDOM_E_SDK_DIR=$dot/../freedom-e-sdk
TEST_OUTPUT_DIR=output/$BINARY_NAME

export RISCV_PATH=$ISP_PREFIX

make -C $FREEDOM_E_SDK_DIR software PROGRAM=$BINARY_NAME BOARD=freedom-e300-hifive1

mkdir -p $TEST_OUTPUT_DIR
echo "Running qemu test for $BINARY_NAME."
cd $TEST_OUTPUT_DIR && qemu-system-riscv32 -nographic -machine sifive_e -kernel $FREEDOM_E_SDK_DIR/software/$BINARY_NAME/$BINARY_NAME --policy-validator-cfg "policy-path=$POLICY_DIR,tag-info-file=$TAG_FILE" -serial file:uart.log -D pex.log > pex.log
