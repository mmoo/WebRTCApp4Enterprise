#!/bin/sh

mvn clean install -DskipTests -Dgpg.skip=true
cp target/WebRTCAppEE.war ~/softwares/ant-media-server/webapps/
rm -rf ~/softwares/ant-media-server/webapps/WebRTCAppEE
cd ~/softwares/ant-media-server/
./start-debug.sh
