####
# based on the lucida base image
FROM lucida_base

#### environment variables
ENV LUCIDAROOT /usr/local/lucida/lucida
ENV LD_LIBRARY_PATH /usr/local/lib

## install ASR
RUN mkdir -p /usr/local/lucida/lucida/speechrecognition/kaldi_gstreamer_asr
ADD . /usr/local/lucida/lucida/speechrecognition/kaldi_gstreamer_asr
WORKDIR "/usr/local/lucida/lucida/speechrecognition/kaldi_gstreamer_asr"
RUN /bin/bash install_kaldi.sh
