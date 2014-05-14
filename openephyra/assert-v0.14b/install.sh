#!/bin/bash

echo "-------------------------------"
echo "Starting ASSERT installation..."
echo "-------------------------------"

pwd=`pwd`
sed "s=@ASSERT@=$pwd=g" scripts/assert.in > scripts/assert
chmod +x scripts/assert

#--- installing the resource file which will add some environment variables ---#
sed "s=@ASSERT@=$pwd=g" data/cshrc.in > $pwd/.cshrc
if [ -f ~/.cshrc ] 
then
	echo "Found a .cshrc file... copying information to it if not already present..."
	found=`grep 'source.*assert-v0.14b\/.cshrc$' ~/.cshrc | wc -l | awk '{print $1}'`
	if [[ $found = 0 ]]; then
		echo "source $pwd/.cshrc" >> ~/.cshrc
	fi
else
	echo "Creating a .cshrc file..."
	echo "source $pwd/.cshrc" > ~/.cshrc
fi


#--- installing the resource file which will add some environment variables ---#
sed "s=@ASSERT@=$pwd=g" data/bashrc.in > $pwd/.bashrc
if [ -f ~/.bashrc ] 
then
	echo "Found a .bashrc file... copying information to it if not already present..."
	found=`grep 'source.*assert-v0.14b\/.bashrc$' ~/.bashrc | wc -l | awk '{print $1}'`
	if [[ $found = 0 ]]; then
		echo "source $pwd/.bashrc" >> ~/.bashrc
	fi
else
	echo "Creating a .bashrc file..."
	echo "source $pwd/.bashrc" > ~/.bashrc
fi


#---- update the cgi script with environment information ----#
sed "s=@ASSERT@=$pwd=g" $pwd/www/cgi-bin/assert/assert-cgi.py.in > tmp
/bin/mv tmp $pwd/www/cgi-bin/assert/assert-cgi.py.in


#--- checking if system has sed ---#
echo -n "checking if sed exists... "

if [ -x `which sed` ] 
then
	echo "yes"
else
	echo "no"
    echo "----------------------- ERROR -------------------------------------"                                                                                              
    echo " ASSERT requires sed"
    echo " Please install it, or add it to the path, and re-run ./install.sh                  "
    echo "-------------------------------------------------------------------"
	exit
fi
	

#--- checking if system has awk ---#
echo -n "checking if awk exists... "

if [ -x `which awk` ] 
then
	echo "yes"
else
	echo "no"
    echo "----------------------- ERROR -------------------------------------"                                                                                              
    echo " ASSERT requires awk "
    echo " Please install it, or add it to the path, and re-run ./install.sh                  "
    echo "-------------------------------------------------------------------"
	exit
fi
	

#--- checking if system has python ---#
echo -n "checking if python exists... "

if [ -x `which python` ] 
then
	echo "yes"
else
	echo "no"
    echo "----------------------- ERROR -------------------------------------"                                                                                              
    echo " ASSERT requires Python >= 2.3                              "
    echo " Please install it, or add it to the path, and re-run ./install.sh                  "
    echo "-------------------------------------------------------------------"
	exit
fi
	

#--- check the version of python ---#

echo -n "checking for python version 2.3... "
if [ `python -V 2>&1| grep '2\.[3-9]'|wc -l| cut -c0-7 | sed 's/^ *//g'` -eq 1 ] 
then
	echo "yes"
else
	echo "no"
	echo "----------------------- WARNING------------------------------"
	echo " Tagging opinions in text requires Python >= 2.3             "
	echo " Please install Python version 2.3 or greater and re-run     "
	echo " ./install.sh if you plan to tag opinions in text            "
	echo "-------------------------------------------------------------"
fi	
	
python_exec=`which python`
for file in `find . -name "*.py.in"`
do
	echo creating ${file%.in}
	echo "#!$python_exec" > ${file%.in}
	cat $file >> ${file%.in} 
	chmod +x ${file%.in}
done


#--- check the perl exists ---#

echo -n "checking if perl exists... "

if [ -x `which perl` ] 
then
	echo "yes"
else
	echo "no"
    echo "----------------------- ERROR ------------------------------"                                                                                              
    echo " ASSERT requires Perl5                                      "
    echo " Please install it and re-run ./install.sh                  "
    echo "------------------------------------------------------------"
fi
	

#--- check the version of perl ---#

echo -n "checking for perl5..."
if [ `perl -v 2>&1| grep '5\.'|wc -l` -eq 1 ] 
then
	echo "yes"
else
	echo "no"
	echo "----------------------- ERROR -------------------------------"
	echo " Please install Perl5 and re-run ./install.sh                "
	echo "-------------------------------------------------------------"
	exit
fi	
	
perl_exec=`which perl`
for file in `find . -name "*.pl.in"`
do
	echo creating ${file%.in}
	echo "#!$perl_exec" > ${file%.in}
	cat $file >> ${file%.in} 
	chmod +x ${file%.in}
done


#--- check if yamcha works properly ---#

echo -n "checking if YamCha works..."
yamcha_flag=0

packages/yamcha-0.23/bin/yamcha -V -m models/propbank-arguments.model < samples/ref/test.data.ref > samples/test.svm-scores 
if [ `diff samples/test.svm-scores samples/ref/test.svm-scores.ref | wc -l | sed 's/^  *//g'` -eq 0 ] 
then
	#--- yamcha works out of the box ---#
	echo "yes"
	yamcha_flag=1
else
	echo "no!"
	echo "re-compiling YamCha..."
	cd packages/yamcha-0.23
	./configure --prefix=`pwd`
	make clean
	make
	make install
	cd ../..

	echo -n "checking if YamCha works..."
	packages/yamcha-0.23/bin/yamcha -V -m models/propbank-arguments.model < samples/ref/test.data.ref > samples/test.svm-scores 
	if [ `diff samples/test.svm-scores samples/ref/test.svm-scores.ref | wc -l | sed 's/^  *//g'` -eq 0 ] 

	then
		echo "yes"
		yamcha_flag=1
	else
		echo "no!"
		echo "----------------------- ERROR -------------------------------"
		echo " There seems to be a problem installing YamCha, please send  "
		echo " this error output to spradhan@cslr.colorado.edu             "
		echo "-------------------------------------------------------------"
		exit
    fi
fi

#--- now lets test if tiny svm works properly ---#

echo -n "checking if TinySVM works..."
tinysvm_flag=0

if [ `packages/TinySVM-0.09/bin/svm_learn 2>&1| grep 'TinySVM - tiny SVM package' | wc -l | sed 's/^  *//g'` -eq 1 ] 
then
	#--- TinySVM works out of the box ---#
	echo "yes"
	tinysvm_flag=1
else
	echo "no!"
	echo "re-compiling TinySVM..."
	cd packages/TinySVM-0.09
	./configure --prefix=`pwd`
	make clean
	make
	make install
	cd ../..

	echo -n "checking if TinySVM works..."
	if [ `packages/TinySVM-0.09/bin/svm_learn 2>&1| grep TinySVM | wc -l  | sed 's/^  *//g'` -eq 1 ] 
	then
		echo "yes"
		tinysvm_flag=1
	else
		echo "no!"

		echo "----------------------- ERROR -------------------------------"
		echo " There seems to be a problem compiling TinySVM, please send  "
		echo " this error output to spradhan@cslr.colorado.edu             "
		echo "-------------------------------------------------------------"
		exit
    fi
fi


#--- now lets test if tgrep2 works properly ---#

echo -n "checking if TGrep2 works..."
tgrep2_flag=0

if [ `packages/Tgrep2/tgrep2 2>&1| grep "Usage" | wc -l | sed 's/^  *//g'` -eq 1 ] 
then
	#--- TGrep2 works out of the box ---#
	echo "yes"
	tgrep2_flag=1
else
	echo "no!"
	echo "re-compiling TGrep2..."
	cd packages/Tgrep2
	make clean
	make
	cd ../..

	echo -n "checking if TGrep2 works..."
	if [ `packages/Tgrep2/tgrep2 2>&1| grep "Usage" | wc -l  | sed 's/^  *//g'` -eq 1 ] 
	then
		echo "yes"
		tgrep2_flag=1
	else
		echo "no!"

		echo "----------------------- ERROR -------------------------------"
		echo " There seems to be a problem compiling TGrep2, please send  "
		echo " this error output to spradhan@cslr.colorado.edu             "
		echo "-------------------------------------------------------------"
		exit
    fi
fi


#--- now let's check if the Charniak parser functions properly ---#

echo -n "checking if Charniak parser works..."
charniak_flag=0

if [ `packages/CharniakParser/bin/parseStdin 2>&1| grep "error: Need" | wc -l  | sed 's/^  *//g'` -eq 1 ] 
then
	#--- Charniak Parser works out of the box ---#
	echo "yes"
	charniak_flag=1
else
	echo "no!"
	echo "re-compiling Charniak parser..."
	cd packages/CharniakParser

	touch *.o *.C
	make parseStdin
	cp parseStdin bin
	cd ../..

	echo -n "checking if Charniak parser works..."
	if [ `packages/CharniakParser/bin/parseStdin 2>&1| grep "error: Need" | wc -l  | sed 's/^  *//g'` -eq 1 ] 
	then
		echo "yes"
		charniak_flag=1
	else
		echo "no!"

		echo "-------------- ERROR ------------------"
		echo " There seems to be a problem compiling "
		echo " Charniak parser.                      "
		echo " Please send this error output to      "
		echo " spradhan@cslr.colorado.edu            "
		echo "---------------------------------------"
		exit
    fi
fi

#--- check if apache is present and install the server ---#

echo -n "checking if apache present... "

apache_flag=0

if [ -e `locate apache2ctl  | grep '\/apache2ctl$'` -a `locate apache2ctl  | grep '\/apache2ctl$'` ]
then
	echo "yes"

	apache_exec=`locate apache2ctl  | grep '\/apache2ctl$'`
	httpd_dir=`$apache_exec -V | grep HTTPD_ROOT | sed 's/^[^"][^"]*"//g; s/".*$//g;'`
	config_file=`$apache_exec -V | grep SERVER_CONFIG_FILE | sed 's/^[^"][^"]*"//g; s/".*$//g;'`
	#echo ${httpd_dir}/${config_file}

	html_root=`grep "^DocumentRoot" ${httpd_dir}/${config_file} | sed 's/^[^"][^"]*"//g; s/".*$//g;'`
	#echo $html_root
	cgi_root=`echo $html_root | sed 's=/[^/][^/]*$==g'`/cgi-bin
	apache_root=`echo $html_root | sed 's=/[^/][^/]*$==g'`
	#echo $cgi_root

	if [ -e $html_root -a -e $cgi_root ] 
	then
		#echo "found the directories"
		echo -n "Apache installation was found in [$apache_root].  Is this correct? (y/n) : "

		read x
		if [ $x = "n" ] 
		then
			echo -n "Please enter the Apache www root directory, usually it is /var/www/: "
			read apache_root 
			
			echo -n "Apache installation specified to be in [$apache_root].  Is this correct? (y/n) : "
			read correct
			
			if [ $correct = "y" ] 
			then
				if [ -w $html_root -a -w $cgi_root ] 
				then
					#echo "directories are writable"
					cp -r www/cgi-bin/assert $cgi_root
					cp -r  www/html/assert $html_root
					
					chmod -R go+rX $html_root/assert/
					chmod -R go+rX $cgi_root/assert/

					apache_flag=1
				else
					echo "------------------ ERROR --------------------"
					echo " You do not have enough permission to install"
					echo " the web server files!  Might want to run    "
					echo "./install.sh as root                         "
					echo "---------------------------------------------"
					apache_flag=0
				fi
			else
				echo "-------------------- ERROR ---------------"
				echo " Problem setting apache directory, server "
				echo " installation failed! Try re-running      "
				echo " ./install.sh                             "  
				echo "------------------------------------------"
				apache_flag=0
			fi

		elif [ $x = "y" ] 
		then
			apache_flag=1
			if [ -w $html_root -a -w $cgi_root ] 
			then
				#echo "directories are writable"
				cp -r www/cgi-bin/assert $cgi_root
				cp -r www/html/assert $html_root
				
				chmod -R go+rX $html_root/assert/
				chmod -R go+rX $cgi_root/assert/
			else
				echo "------------------ ERROR --------------------"
				echo " You do not have enough permission to install"
				echo " the web server files!  Might want to run    "
				echo "./install.sh as root                         "
				echo "---------------------------------------------"
				apache_flag=0
			fi
		fi
	else
		apache_flag=0
		echo "Could not find the directories"
	fi
else
	echo "no"
	echo "---------------- WARNING ---------------------"
	echo " Apache web server package required for the   "
	echo " parser to function in server mode.           "
	echo " It can be found at http://www.apache.org/    "
	echo ""
	echo " Please re-run install.sh after installing    "
	echo " apache server                                "
	echo "----------------------------------------------"

	apache_flag=0
fi

#--- testing if ASSERT works properly ---#

assert_flag=0
if [ $yamcha_flag=1 -a $tinysvm_flag=1 -a $charniak_flag=1 ]
then
	echo "----------------------------------------"
	echo " Testing if ASSERT works properly.      "
	echo " This will take a minute to finish "
	echo "----------------------------------------"
	
	cd samples
	../scripts/assert --tag=argument test.txt > /dev/null
	
	if [ -e test.parses -a `diff test.parses ref/test.parses.ref | wc -l  | sed 's/^  *//g'` -eq 0 ]
	then
		echo "yes, ASSERT works!"
		rm -rf test.parses
		assert_flag=1
	else
		echo "no, ASSERT does not work!"
		assert_flag=0
	fi
	cd ..
fi


if [ $apache_flag -eq 1 -a $assert_flag -eq 1 ] 
then
	echo "-------------------------------"
	echo "ASSERT installation successful!"
	echo "-------------------------------"
elif [ $assert_flag -eq 1 ]
then
	echo "----------------------------------------------"
	echo "ASSERT batch and client mode installation successful!    "
	echo "However,  ASSERT will not work in web server mode."
	echo "----------------------------------------------"
else
	echo "--------------------------------------"
	echo "ASSERT installation failed!           "
	echo "Please contact spradhan@cslr.colorado "
	echo "with the installation output!         "
	echo "--------------------------------------"
fi






# Additions to be made:
# - add checks for awk and sed executables.
#
#
#
#
#
#
