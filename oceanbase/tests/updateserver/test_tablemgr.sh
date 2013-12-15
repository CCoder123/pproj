#!/bin/sh

mkdir -p ./data/raid1
rm -rf ./data/raid1/*
ln -s ~/ ./data/raid1/store1
rm ./data/raid1/store1/*
cp ./sst4test/*.sst ./data/raid1/store1

./test_tablemgr ./data "^raid[0-9]$" "^store[0-9]$"
./test_sstablemgr ./data "^raid[0-9]$" "^store[0-9]$"
./test_storemgr ./data "^raid[0-9]$" "^store[0-9]$"

