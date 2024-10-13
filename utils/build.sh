#!/bin/bash

echo "Remove dma-proxy module to free CMA memory"
modprobe -r dma_proxy

echo "Start of build"
make

echo "Insert dma-proxy module"
modprobe dma_proxy
