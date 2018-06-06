#!/bin/bash

curPath=`pwd`
echo "ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-download"
ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-download
echo "ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-upload"
ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-upload
echo "ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-upload-nsc"
ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-upload-nsc
echo "ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-download-nsc"
ln -s $curPath/ip-over-ndn $curPath/pure-ip-multi-tcp-download-nsc

echo "ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-single-tcp-upload"
ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-single-tcp-upload
echo "ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-multi-tcp-download"
ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-multi-tcp-download
echo "ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-multi-tcp-download-nsc"
ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-multi-tcp-download-nsc
echo "ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-multi-tcp-upload-nsc"
ln -s $curPath/ip-over-ndn $curPath/ip-over-ndn-multi-tcp-upload-nsc


ln -s ip-over-ndn ip-over-ndn-multi-tcp-upload-nsc
ln -s ip-over-ndn pure-ip-multi-tcp-upload-nsc
