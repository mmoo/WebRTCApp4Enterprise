#!/bin/sh

mvn clean install -DskipTests
cp target/WebRTCApp4.war ~/softwares/ant-media-server/webapps/
rm -rf ~/softwares/ant-media-server/webapps/WebRTCApp4
cd ~/softwares/ant-media-server/
./start-debug.sh
