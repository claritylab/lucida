#!/bin/bash

echo "============================================="
echo "     Lucida Micro Service Building Tools"
echo "          made in May 31, 2017"
echo "============================================="
echo ""

check_valid () {
	if [ "$1" = "NAME" ]; then
		python service_mongo.py check name $2
		return $?
	elif [ "$1" = "ACN" ]; then
		python service_mongo.py check acronym $2
		return $?
	elif [ "$1" = "HOST_PORT" ]; then
		python service_mongo.py check_host_port $2 $3
		return $?
	fi
}

create_folder () {
	cp -rf template/$1 .
	mv $1 $2
	CLASS_PATH="$PWD"/"$2"/class.txt
	cd $2
	find ./ -type f -exec sed -i "s/template/$2/g" {} \;
	find ./ -type f -exec sed -i "s/TPL/$3/g" {} \;
	find . -depth -name '*template*' -execdir rename "s/template/$2/g" {} \;
	find . -depth -name '*TPL*' -execdir rename "s/TPL/$3/g" {} \;
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
			check_valid NAME $NAME
			NAME_VALID=$?
			if [ $NAME_VALID -ne 0 ]; then
				echo "[Error] Service name used! Please try another one!"
			fi
		fi
	done

	echo ""
	ACN_VALID=1
	while [ $ACN_VALID -ne 0 ]; do
		echo "### Specify the acronym of your service (e.g. MS)."
		printf "### Enter the acronym of your service: "
		read ACN
		if [ "$ACN" = "" ]; then
			echo "[Error] Service acronym cannot be empty! Please try another one!"
		else
			check_valid ACN $ACN
			ACN_VALID=$?
			if [ $ACN_VALID -ne 0 ]; then
				echo "[Error] Service acronym used! Please try another one!"
			fi
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

	echo ""
	HOST_PORT_VALID=1
	while [ $HOST_PORT_VALID -ne 0 ]; do
		HOST_VALID=1
		while [ $HOST_VALID -ne 0 ]; do
			echo "### Specify the host/port information for your service. "
			printf "### Enter the host: "
			read HOST
			if [ "$HOST" = "" ]; then
				echo "[Error] Service host cannot be empty! Please try another one!"
			else
				HOST_VALID=0
			fi
		done

		PORT_VALID=1
		while [ $PORT_VALID -ne 0 ]; do
			printf "### Enter the port: "
			read PORT
			if [ "$PORT" = "" ]; then
				echo "[Error] Service port cannot be empty! Please try another one!"
			else
				PORT_VALID=0
			fi
		done
		
		check_valid HOST_PORT $HOST $PORT
		HOST_PORT_VALID=$?
		if [ $HOST_PORT_VALID -ne 0 ]; then
			echo "[Error] Service host/port pair already used! Please try another one!"
		fi
	done

	echo ""
	INPUT_VALID=1
	while [ $INPUT_VALID -ne -0 ]; do
		echo "Specify the input type of your service (text, image or text_image)"
		printf "### Enter the input type: "
		read INPUT
		if [ "$INPUT" = "" ]; then
			echo "[Error] Service input type cannot be empty! Please try another one!"
		else
			INPUT_VALID=0
		fi
	done

	echo ""
	LEARN_VALID=1
	while [ $LEARN_VALID -ne -0 ]; do
		echo "Specify the learn type of your service (text, image or none)"
		printf "### Enter the learn type: "
		read LEARN
		if [ "$LEARN" = "" ]; then
			echo "[Error] Service learn type cannot be empty! Please try another one!"
		else
			LEARN_VALID=0
		fi
	done

	if [ "$LAN" = "cpp" -o "$LAN" = "java" -o "$LAN" = "python" ]; then
		# do copy template folder of cpp to lucida
		cd ../lucida ; \
		if [ -d $NAME ]; then
			echo "[Error] service already exists!"
			exit 1
		else
			create_folder $LAN $NAME $ACN
		fi
	else
		# create an empty folder
		cd ../lucida ; \
		if [ -d $NAME ]; then
			echo "[Error] service already exists!"
			exit 1
		else
			mkdir $NAME
			touch "$NAME"/class.txt
			CLASS_PATH="$PWD"/"$NAME"/class.txt
			echo "[Info] Template folder is ready!"
		fi
	fi

	cd ../tools
	python service_mongo.py add $NAME $ACN $HOST $PORT $INPUT $LEARN $CLASS_PATH
	if [ $? = 0 ]; then
		echo "[Info] Service registration succeed!"
	else
		rm -rf ../lucida/"$NAME"
		echo "[Error] Service registration fail!"
	fi

elif [ "$OP" = "delete" ]; then
	NAME_VALID=0
	while [ $NAME_VALID -ne 1 ]; do
		echo "### Specify the service name you want to delete (e.g. musicservice)."
		printf "### Enter your service name: "
		read NAME
		if [ "$NAME" = "" ]; then
			echo "[Error] Service name cannot be empty! Please try another one!"
		else
			check_valid NAME $NAME
			NAME_VALID=$?
			if [ $NAME_VALID -ne 1 ]; then
				echo "[Error] Service name not exists! Please try another one!"
			fi
		fi
	done

	python service_mongo.py delete $NAME

	cd ../lucida
	if [ -d $NAME ]; then
		rm -rf $NAME
		echo "[Info] service already deleted!"
	else
		echo "[Error] service not exists!"
	fi
fi

