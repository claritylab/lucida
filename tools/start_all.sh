# Modify the paths below for a subset of services.
declare -a services=(
	"$(pwd)/../lucida/commandcenter"
	"$(pwd)/../lucida/speechrecognition/kaldi_gstreamer_asr"
	"$(pwd)/../lucida/imagematching/opencv_imm"
	"$(pwd)/../lucida/questionanswering/OpenEphyra"
	"$(pwd)/../lucida/calendar"
	"$(pwd)/../lucida/djinntonic/dig"
	"$(pwd)/../lucida/djinntonic/face"
	"$(pwd)/../lucida/djinntonic/imc")

for i in "${services[@]}"
do
   echo "Starting service in $i"
   cd $i
   make start_server
done
