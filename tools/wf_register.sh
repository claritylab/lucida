#!/bin/bash

echo "============================================="
echo "       Lucida Workflow Registry Tools"
echo "          made in May 31, 2017"
echo "============================================="
echo ""

check_valid () {
	if [ "$1" = "NAME" ]; then
		python wf_mongo.py check name $2
		return $?
	fi
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
		echo "### Specify your workflow name (e.g. MSWF)."
		printf "### Enter your workflow name: "
		read NAME
		if [ "$NAME" = "" ]; then
			echo "[Error] Workflow name cannot be empty! Please try another one!"
		else
			check_valid NAME $NAME
			NAME_VALID=$?
			if [ $NAME_VALID -ne 0 ]; then
				echo "[Error] Workflow name used! Please try another one!"
			fi
		fi
	done

	echo ""
	INPUT_VALID=1
	while [ $INPUT_VALID -ne -0 ]; do
		echo "### Specify the input type of your service (text, image or text_image)"
		printf "### Enter the input type: "
		read INPUT
		if [ "$INPUT" = "" ]; then
			echo "[Error] Service input type cannot be empty! Please try another one!"
		else
			INPUT_VALID=0
		fi
	done

	echo ""
	CLASSIFIER_VALID=1
	while [ $CLASSIFIER_VALID -ne -0 ]; do
		echo "### Specify the path of your workflow classifier data"
		printf "### Enter the path: "
		read CLASSIFIER
		if [ "$CLASSIFIER" = "" ]; then
			echo "[Error] Path cannot be empty! Please try another one!"
		else
			if [ -f $CLASSIFIER ]; then
				CLASSIFIER_VALID=0
			else
				echo "[Error] File not found! Please try another one!"
			fi
		fi
	done

	echo ""
	CODE_VALID=1
	while [ $CODE_VALID -ne -0 ]; do
		echo "### Specify the path of your workflow class code"
		printf "### Enter the path: "
		read PA
		if [ "$PA" = "" ]; then
			echo "[Error] Path cannot be empty! Please try another one!"
		else
			if [ -f $PA ]; then
				CODE=$(<$PA)
				CODE_VALID=0
			else
				echo "[Error] File not found! Please try another one!"
			fi
		fi
	done
	
	echo "$CODE" | python wf_mongo.py add $NAME $INPUT $PWD/$CLASSIFIER
	
	if [ $? = 0 ]; then
		echo "[Info] Workflow registration succeed!"
	else
		echo "[Error] Workflow registration fail!"
	fi

elif [ "$OP" = "delete" ]; then
	NAME_VALID=0
	while [ $NAME_VALID -ne 1 ]; do
		echo "### Specify the workflow name you want to delete (e.g. MSWF)."
		printf "### Enter your workflow name: "
		read NAME
		if [ "$NAME" = "" ]; then
			echo "[Error] Workflow name cannot be empty! Please try another one!"
		else
			check_valid NAME $NAME
			NAME_VALID=$?
			if [ $NAME_VALID -ne 1 ]; then
				echo "[Error] Workflow name not exists! Please try another one!"
			fi
		fi
	done

	python wf_mongo.py delete $NAME

	if [ $? = 0 ]; then
		echo "[Info] Workflow deleted!"
	else
		echo "[Error] Workflow not exists!"
	fi
fi
