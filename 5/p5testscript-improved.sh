#!/bin/bash

# If test stuff is under a different dir from the binaries (e.g. test/),
# set BIN_DIR to relative path from script to binaries (e.g. ..)
BIN_DIR=.
if [ ! -e $BIN_DIR/enc_server ]; then
  # try going up a dir if binaries not found
  BIN_DIR=..
fi

POINTS=0

#Make sure we have the right number of arguments
if test $# -gt 3 -o $# -lt 2; then
  echo "USAGE: $0 <enc-port> <dec-port> [--no-color]" 1>&2
  exit 1
fi

#Record the ports passed in
ENC_PORT=$1
DEC_PORT=$2

BOLD=$(tput bold)
restore=$(tput sgr0)

BLACK=$(tput setaf 0)
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
MAGENTA=$(tput setaf 5)
CYAN=$(tput setaf 6)
WHITE=$(tput setaf 7)
GREY="$(tput setaf 8)"

if [ $3 -a $3 = "--no-color" ]; then
  BOLD=''
  restore=''

  BLACK=''
  RED=''
  GREEN=''
  YELLOW=''
  BLUE=''
  MAGENTA=''
  CYAN=''
  WHITE=''
  GREY=''
fi

restore() { echo -n $restore; }
title() {
  echo
  echo "${BOLD}${WHITE}$*${restore}"
  echo
}
header() { echo "${WHITE}${@:2} ${BLUE}($1 pts)"; }
pass() { echo "${GREEN}  PASS"; }
fail() { echo "${RED}  FAIL: $*"; }
warn() { echo "${YELLOW}$*"; }
info() {
  echo "${GREY}$*";
  # echo -n
}

cleanup() {
  info "- killing any running servers (ignore not permitted errors)"
  killall -q -u $USER dec_client
  killall -q -u $USER dec_server
  killall -q -u $USER enc_client
  killall -q -u $USER enc_server

  info "- removing tempfiles"
  rm -f ciphertext*
  rm -f plaintext*_*
  rm -f key20
  rm -f key70000
}

if [ ! -e $BIN_DIR/enc_server ]; then
  echo "${RED}ERR: cannot find binaries!"
  restore
  exit 1
fi

title "== CS344 Program 5 Grading Script =="

cleanup

info "- starting servers"
$BIN_DIR/enc_server $ENC_PORT &
$BIN_DIR/dec_server $DEC_PORT &

info "- generating key20"
$BIN_DIR/keygen 20 >key20

header 5 "does key20 exist?"
[ -s key20 ] || rm -f key20 # remove if empty
if [ -f key20 ]; then
  pass "key20 exists"
  POINTS=$((POINTS + 5))
else
  fail "key20 does not exist"
fi

header 5 "is length of key20 21?"
if [ $(stat -c '%s' key20) -eq 21 ]; then
  pass "key20 has correct length"
  POINTS=$((POINTS + 5))
else
  fail "key20 has length $(stat -c '%s' key20)"
fi

info "- generating key70000"
$BIN_DIR/keygen 70000 >key70000
header 5 "is size of key70000 70001?"
if [ $(stat -c '%s' key70000) -eq 70001 ]; then
  pass "key70000 has correct length"
  POINTS=$((POINTS + 5))
else
  fail "key70000 has length $(stat -c '%s' key70000)"
fi

header 10 "error given about short key?"
restore
$BIN_DIR/enc_client plaintext1 key20 $ENC_PORT
if [ $? -eq 1 ]; then
  pass "error reported"
  POINTS=$((POINTS + 10))
else
  fail "no error thrown"
fi

header 20 "returns encrypted text? ${YELLOW}Double check manually!"
echo -n $BLUE
$BIN_DIR/enc_client plaintext1 key70000 $ENC_PORT
if [ $? -eq 0 ]; then
  pass "no errors thrown"
  POINTS=$((POINTS + 20))
else
  fail "error reported"
fi

info "- creating ciphertext1 file"
$BIN_DIR/enc_client plaintext1 key70000 $ENC_PORT >ciphertext1
header 10 "does ciphertext1 exist?"
[ -s ciphertext1 ] || rm -f ciphertext1 # remove if empty
if [ -f key20 ]; then
  pass "ciphertext1 exists"
  POINTS=$((POINTS + 10))
else
  fail "ciphertext1 does not exist"
fi

header 10 "is size of ciphertext1 same as plaintext1?"
if [ $(stat -c '%s' ciphertext1) -eq $(stat -c '%s' plaintext1) ]; then
  pass "ciphertext1 has correct length"
  POINTS=$((POINTS + 10))
else
  fail "ciphertext1 has length $(stat -c '%s' ciphertext1), should be $(stat -c '%s' plaintext1)"
fi

header 5 "does ciphertext1 look encrypted? ${YELLOW}Double check manually!"
echo -n $BLUE
cat ciphertext1
if [ $? -eq 0 ]; then
  pass "no errors thrown"
  POINTS=$((POINTS + 5))
else
  fail "error reported"
fi

header 5 "dec -> enc invalid connection reported?"
restore
$BIN_DIR/dec_client ciphertext1 key70000 $ENC_PORT
if [ $? -eq 2 ]; then
  pass "error reported"
  POINTS=$((POINTS + 5))
else
  fail "no error thrown"
fi

header 20 "does decrypted message match source?"
echo -n "${CYAN}Original : "
cat plaintext1
echo -n "${BLUE}Decrypted: "
$BIN_DIR/dec_client ciphertext1 key70000 $DEC_PORT
cmp -s plaintext1 <($BIN_DIR/dec_client ciphertext1 key70000 $DEC_PORT)
if [ $? -eq 0 ]; then
  pass "output matches"
  POINTS=$((POINTS + 20))
else
  fail "output does not match"
fi

info "- decrypting ciphertext1 to plaintext1_a"
$BIN_DIR/dec_client ciphertext1 key70000 $DEC_PORT >plaintext1_a
header 10 "does plaintext1_a exist?"
[ -s plaintext1_a ] || rm -f plaintext1_a # remove if empty
if [ -f plaintext1_a ]; then
  pass "plaintext1_a exists"
  POINTS=$((POINTS + 10))
else
  fail "plaintext1_a does not exist"
fi

header 5 "does decrypted message file match source?"
cmp -s plaintext1 plaintext1_a
if [ $? -eq 0 ]; then
  pass "output matches"
  POINTS=$((POINTS + 5))
else
  fail "output does not match"
fi

info "- testing concurrent encyption"
header 5 "is error reported for fail input plaintext5?"
restore

rm -f ciphertext*
rm -f plaintext*_*

$BIN_DIR/enc_client plaintext1 key70000 $ENC_PORT >ciphertext1 &
$BIN_DIR/enc_client plaintext2 key70000 $ENC_PORT >ciphertext2 &
$BIN_DIR/enc_client plaintext3 key70000 $ENC_PORT >ciphertext3 &
$BIN_DIR/enc_client plaintext4 key70000 $ENC_PORT >ciphertext4 &
# \/ this one should throw error
$BIN_DIR/enc_client plaintext5 key70000 $ENC_PORT >ciphertext5
if [ $? -ne 0 ]; then
  pass "error reported for plaintext5"
  POINTS=$((POINTS + 5))
else
  fail "no error thrown"
fi

info "- waiting for programs to complete"
wait $( jobs -l | grep enc_client | awk '{ print $2 }' )
sleep 1

header 20 "are correct ciphertexts generated with concurrent encryption?"
ALLMATCH=1
for f in text{1,2,3,4}; do
  if [ $(stat -c '%s' "cipher$f") -ne $(stat -c '%s' "plain$f") ]; then
    fail "size of cipher$f does not match plain$f"
    restore
    stat -c '%n %s' "plain$f" "cipher$f" | column -t
    ALLMATCH=0
  fi
done
if [ $ALLMATCH -eq 1 ]; then
  pass "size of ciphertexts match plaintexts"
  POINTS=$((POINTS + 20))
fi

info "- testing concurrent decryption"
$BIN_DIR/dec_client ciphertext1 key70000 $DEC_PORT >plaintext1_a &
$BIN_DIR/dec_client ciphertext2 key70000 $DEC_PORT >plaintext2_a &
$BIN_DIR/dec_client ciphertext3 key70000 $DEC_PORT >plaintext3_a &
$BIN_DIR/dec_client ciphertext4 key70000 $DEC_PORT >plaintext4_a &

info "- waiting for programs to complete"
wait $( jobs -l | grep dec_client | awk '{ print $2 }' )
sleep 1

header 15 "are correct output files generated with concurrent decryption?"
ALLMATCH=1
for f in plaintext{1,2,3,4}; do
  cmp -s "$f" "${f}_a"
  if [ $? -ne 0 ]; then
    fail "${f}_a does not match ${f}"
    restore
    stat -c '%n %s' "$f" "${f}_a" | column -t
    ALLMATCH=0
  fi
done
if [ $ALLMATCH -eq 1 ]; then
  pass "decrypted text matches original"
  POINTS=$((POINTS + 15))
fi

cleanup
title "FINAL SCORE: ${BLUE}${POINTS}${WHITE} / 150"
restore
