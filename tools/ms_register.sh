#!/bin/bash

echo "============================================="
echo "     Lucida Micro Service Registry Tools"
echo "          made in May 31, 2017"
echo "============================================="
echo ""

check_valid () {
	if [ "$1" = "NAME" ]; then
		python ms_mongo.py check name $2
		return $?
	elif [ "$1" = "ACN" ]; then
		python ms_mongo.py check acronym $2
		return $?
	elif [ "$1" = "HOST_PORT" ]; then
		python ms_mongo.py check_host_port $2 $3
		return $?
	fi
}

create_folder () {
	cp -rf template/$1 .
	mv $1 $2
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
		echo "### Specify the input type of your service (text, image)"
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
		echo "### Specify the learn type of your service (text, image or none)"
		printf "### Enter the learn type: "
		read LEARN
		if [ "$LEARN" = "" ]; then
			echo "[Error] Service learn type cannot be empty! Please try another one!"
		else
			LEARN_VALID=0
		fi
	done

	echo "[Info] Waiting......"
	python ms_mongo.py add $NAME $ACN $HOST $PORT $INPUT $LEARN
	if [ $? = 0 ]; then
		echo "[Info] Service registration succeed!"
	else
		echo "[Error] Service registration fail!"
	fi

	echo ""
	TEMP_VALID=1
	while [ $TEMP_VALID -ne -0 ]; do
		echo "### Specify if you want to generate the folder for your service automatically."
		printf "### Enter if you want the folder [y/n]: "
		read TEMP
		if [ "$TEMP" = "y" ]; then
			TEMP_VALID=0
		elif [ "$TEMP" = "n" ]; then
			exit 0
		else
			echo "Please input y/n for generating folder!"
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
			create_folder $LAN $NAME $ACN
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

	python ms_mongo.py delete $NAME

	if [ $? = 0 ]; then
		echo "[Info] Service deleted!"
	else
		echo "[Error] Service not exists!"
	fi
fi
