#!/bin/bash

echo "============================================="
echo "     Lucida Micro Service Registry Tools"
echo "          made in May 31, 2017"
echo "============================================="
echo ""

create_folder () {
	cd ../lucida
	cp -rf template/$1 .
	mv $1 $2
	cd $2
	find ./ -type f -exec sed -i "s/template/$2/g" {} \;
	find ./ -type f -exec sed -i "s/TPL/$2/g" {} \;
	find . -depth -name '*template*' -execdir rename "s/template/$2/g" {} \;
	find . -depth -name '*TPL*' -execdir rename "s/TPL/$2/g" {} \;
	cd ..
	echo "[Info] Template folder is ready!"
}

OP=""
if [ "$1" = "add" ]; then
	OP="add"
elif [ "$1" = "delete" ]; then
	OP="delete"
else
	echo "### Specify what you want to do (add or delete)"
	printf "### Enter you want to do: "
	read OP
	echo ""
fi

if [ "$OP" = "add" ]; then
	NAME_VALID=1
	while [ $NAME_VALID -ne 0 ]; do
		echo "### Specify your service name (e.g. musicservice)."
		printf "### Enter your service name: "
		read NAME
		if [ "$NAME" = "" ]; then
			echo "[Error] Service name cannot be empty! Please try another one!"
		else
			NAME_VALID=0
		fi
	done

	echo ""
	echo "### Specify the programming language you want to you in your programming. If C++/Java/Python, then template will be provided."
	printf "### Enter the programming language: "
	read LAN
	LAN="$(tr [A-Z] [a-z] <<< "$LAN")"
	if [ "$LAN" = "c++" ]; then
		LAN="cpp"
	fi

	if [ "$LAN" = "cpp" -o "$LAN" = "java" -o "$LAN" = "python" ]; then
		# do copy template folder of cpp to lucida
		if [ -d $NAME ]; then
			echo "[Error] service already exists!"
			exit 1
		else
			create_folder $LAN $NAME
		fi
	else
		# create an empty folder
		if [ -d $NAME ]; then
			echo "[Error] service already exists!"
			exit 1
		else
			mkdir $NAME
			echo "[Info] Template folder is ready!"
		fi
	fi
fi