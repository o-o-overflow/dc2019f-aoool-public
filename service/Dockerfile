from ubuntu:18.04
run echo "travis_fold:start:Dapt\033[33;1mservice Dockerfile apt\033[0m" && \
    apt-get -qq update && apt-get install -qq python3 xinetd && \
    echo "\ntravis_fold:end:Dapt\r"
# python3-pip, etc.

run apt-get -qq install netcat-openbsd net-tools

run apt-get -qq install libboost-system1.65.1 libboost-filesystem1.65.1 libboost-regex1.65.1

copy setup.sh /setup.sh
run /setup.sh
run rm -f /setup.sh

copy deployment/aoool /aoool/bin/aoool
copy deployment/files/aoool/etc/ /aoool/etc/
copy deployment/files/aoool/var/www/main/ /aoool/var/www/main/

# The flagbot will periodically echo the flag > this file (flag_path in info.yaml)
run touch /flag && chmod 644 /flag && echo "OOO{this is a test flag}" > /flag

copy aoool.conf /etc/aoool.conf
copy banner_fail /banner_fail
copy wrapper /wrapper
copy schitzo /opt/schitzo

expose 8080
cmd chmod go-rwx /proc && xinetd -filelog - -dontfork -f /etc/aoool.conf
