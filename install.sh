#!/bin/sh

mvn clean install -DskipTests
cp target/WebRTCAppEE.war ~/softwares/ant-media-server/webapps/
rm -rf ~/softwares/ant-media-server/webapps/WebRTCAppEE
cd ~/softwares/ant-media-server/
./start-debug.sh
