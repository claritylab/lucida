: "${LUCIDAROOT:?Need to set LUCIDAROOT non-empty}"
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 7F0CEB10
sudo apt-get update
sudo apt-get -y upgrade
sudo apt-get install -y \
        apache2 \
        apache2-utils \
        libapache2-mod-wsgi \
        python-pip
cp -f $LUCIDAROOT/commandcenter/apache/conf/000-default.conf /etc/apache2/sites-available/000-default.conf
sudo a2enmod ssl # for https
