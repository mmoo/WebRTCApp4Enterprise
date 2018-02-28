#!/bin/sh

mvn clean install -DskipTests -Dgpg.skip=true
OUT=$?

if [ $OUT -ne 0 ]; then
    exit $OUT
fi

PASS=21236161
NODE1=10.2.41.113
NODE2=10.2.40.95

sshpass -p "$PASS" scp target/WebRTCAppEE.war ubuntu@$NODE1:./ant-media-server/webapps/
OUT=$?

if [ $OUT -ne 0 ]; then
    exit $OUT
fi

sshpass -p "$PASS" scp target/WebRTCAppEE.war ubuntu@$NODE2:./ant-media-server/webapps/
OUT=$?

if [ $OUT -ne 0 ]; then
    exit $OUT
fi

sshpass -p "$PASS" ssh ubuntu@$NODE1 'rm -rf ./ant-media-server/webapps/WebRTCAppEE'
sshpass -p "$PASS" ssh ubuntu@$NODE2 'rm -rf ~/ant-media-server/webapps/WebRTCAppEE'
