# Copyright (C) 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Sample that implements gRPC client for Google Assistant API."""

import json
import logging
import os.path

import click
import grpc
import google.auth.transport.grpc
import google.auth.transport.requests
import google.oauth2.credentials

from google.assistant.embedded.v1alpha1 import embedded_assistant_pb2
from google.rpc import code_pb2
from tenacity import retry, stop_after_attempt, retry_if_exception

import assistant_helpers
import audio_helpers


ASSISTANT_API_ENDPOINT = 'embeddedassistant.googleapis.com'
END_OF_UTTERANCE = embedded_assistant_pb2.ConverseResponse.END_OF_UTTERANCE
DIALOG_FOLLOW_ON = embedded_assistant_pb2.ConverseResult.DIALOG_FOLLOW_ON
CLOSE_MICROPHONE = embedded_assistant_pb2.ConverseResult.CLOSE_MICROPHONE
DEFAULT_GRPC_DEADLINE = 60 * 3 + 5


class SampleAssistant(object):
    """Sample Assistant that supports follow-on conversations.

    Args:
      conversation_stream(ConversationStream): audio stream
        for recording query and playing back assistant answer.
      channel: authorized gRPC channel for connection to the
        Google Assistant API.
      deadline_sec: gRPC deadline in seconds for Google Assistant API call.
    """

    def __init__(self, conversation_stream, channel, deadline_sec):
        self.conversation_stream = conversation_stream

        # Opaque blob provided in ConverseResponse that,
        # when provided in a follow-up ConverseRequest,
        # gives the Assistant a context marker within the current state
        # of the multi-Converse()-RPC "conversation".
        # This value, along with MicrophoneMode, supports a more natural
        # "conversation" with the Assistant.
        self.conversation_state = None

        # Create Google Assistant API gRPC client.
        self.assistant = embedded_assistant_pb2.EmbeddedAssistantStub(channel)
        self.deadline = deadline_sec

    def __enter__(self):
        return self

    def __exit__(self, etype, e, traceback):
        if e:
            return False
        self.conversation_stream.close()

    def is_grpc_error_unavailable(e):
        is_grpc_error = isinstance(e, grpc.RpcError)
        if is_grpc_error and (e.code() == grpc.StatusCode.UNAVAILABLE):
            logging.error('grpc unavailable error: %s', e)
            return True
        return False

    @retry(reraise=True, stop=stop_after_attempt(3),
           retry=retry_if_exception(is_grpc_error_unavailable))
    def converse(self):
        """Send a voice request to the Assistant and playback the response.

        Returns: True if conversation should continue.
        """
        continue_conversation = False

        self.conversation_stream.start_recording()
        logging.info('Recording audio request.')

        def iter_converse_requests():
            for c in self.gen_converse_requests():
                assistant_helpers.log_converse_request_without_audio(c)
                yield c
            self.conversation_stream.start_playback()

        # This generator yields ConverseResponse proto messages
        # received from the gRPC Google Assistant API.
        for resp in self.assistant.Converse(iter_converse_requests(),
                                            self.deadline):
            assistant_helpers.log_converse_response_without_audio(resp)
            if resp.error.code != code_pb2.OK:
                logging.error('server error: %s', resp.error.message)
                break
            if resp.event_type == END_OF_UTTERANCE:
                logging.info('End of audio request detected')
                self.conversation_stream.stop_recording()
            if resp.result.spoken_request_text:
                logging.info('Transcript of user request: "%s".',
                             resp.result.spoken_request_text)
                logging.info('Playing assistant response.')
            if len(resp.audio_out.audio_data) > 0:
                self.conversation_stream.write(resp.audio_out.audio_data)
            if resp.result.spoken_response_text:
                logging.info(
                    'Transcript of TTS response '
                    '(only populated from IFTTT): "%s".',
                    resp.result.spoken_response_text)
            if resp.result.conversation_state:
                self.conversation_state = resp.result.conversation_state
            if resp.result.volume_percentage != 0:
                self.conversation_stream.volume_percentage = (
                    resp.result.volume_percentage
                )
            if resp.result.microphone_mode == DIALOG_FOLLOW_ON:
                continue_conversation = True
                logging.info('Expecting follow-on query from user.')
            elif resp.result.microphone_mode == CLOSE_MICROPHONE:
                continue_conversation = False
        logging.info('Finished playing assistant response.')
        self.conversation_stream.stop_playback()
        return continue_conversation

    def gen_converse_requests(self):
        """Yields: ConverseRequest messages to send to the API."""

        converse_state = None
        if self.conversation_state:
            logging.debug('Sending converse_state: %s',
                          self.conversation_state)
            converse_state = embedded_assistant_pb2.ConverseState(
                conversation_state=self.conversation_state,
            )
        config = embedded_assistant_pb2.ConverseConfig(
            audio_in_config=embedded_assistant_pb2.AudioInConfig(
                encoding='LINEAR16',
                sample_rate_hertz=self.conversation_stream.sample_rate,
            ),
            audio_out_config=embedded_assistant_pb2.AudioOutConfig(
                encoding='LINEAR16',
                sample_rate_hertz=self.conversation_stream.sample_rate,
                volume_percentage=self.conversation_stream.volume_percentage,
            ),
            converse_state=converse_state
        )
        # The first ConverseRequest must contain the ConverseConfig
        # and no audio data.
        yield embedded_assistant_pb2.ConverseRequest(config=config)
        for data in self.conversation_stream:
            # Subsequent requests need audio data, but not config.
            yield embedded_assistant_pb2.ConverseRequest(audio_in=data)


@click.command()
@click.option('--audio-source',
              metavar='<audio source>', show_default=True,
              default=os.path.join(click.get_app_dir('google-oauthlib-tool'),
                                   'credentials.json'),
              help='Path to read OAuth2 credentials.')
@click.option('--credentials',
              metavar='<credentials>', show_default=True,
              default=os.path.join(click.get_app_dir('google-oauthlib-tool'),
                                   'credentials.json'),
              help='Path to read OAuth2 credentials.')
@click.option('--verbose', '-v', is_flag=True, default=False,
              help='Verbose logging.')
@click.option('--input-audio-file', '-i',
              metavar='<input file>',
              help='Path to input audio file. '
              'If missing, uses audio capture')
@click.option('--output-audio-file', '-o',
              metavar='<output file>',
              help='Path to output audio file. '
              'If missing, uses audio playback')
@click.option('--audio-sample-rate',
              default=audio_helpers.DEFAULT_AUDIO_SAMPLE_RATE,
              metavar='<audio sample rate>', show_default=True,
              help='Audio sample rate in hertz.')
@click.option('--audio-sample-width',
              default=audio_helpers.DEFAULT_AUDIO_SAMPLE_WIDTH,
              metavar='<audio sample width>', show_default=True,
              help='Audio sample width in bytes.')
@click.option('--audio-iter-size',
              default=audio_helpers.DEFAULT_AUDIO_ITER_SIZE,
              metavar='<audio iter size>', show_default=True,
              help='Size of each read during audio stream iteration in bytes.')
@click.option('--audio-block-size',
              default=audio_helpers.DEFAULT_AUDIO_DEVICE_BLOCK_SIZE,
              metavar='<audio block size>', show_default=True,
              help=('Block size in bytes for each audio device '
                    'read and write operation..'))
@click.option('--audio-flush-size',
              default=audio_helpers.DEFAULT_AUDIO_DEVICE_FLUSH_SIZE,
              metavar='<audio flush size>', show_default=True,
              help=('Size of silence data in bytes written '
                    'during flush operation'))
@click.option('--grpc-deadline', default=DEFAULT_GRPC_DEADLINE,
              metavar='<grpc deadline>', show_default=True,
              help='gRPC deadline in seconds')
def main(credentials, verbose,
         input_audio_file, output_audio_file,
         audio_sample_rate, audio_sample_width,
         audio_iter_size, audio_block_size, audio_flush_size,
         grpc_deadline, once, *args, **kwargs):
    """Samples for the Google Assistant API.

    Examples:
      Run the sample with microphone input and speaker output:

        $ python -m googlesamples.assistant

      Run the sample with file input and speaker output:

        $ python -m googlesamples.assistant -i <input file>

      Run the sample with file input and output:

        $ python -m googlesamples.assistant -i <input file> -o <output file>
    """
    # Setup logging.
    logging.basicConfig(level=logging.DEBUG if verbose else logging.INFO)

    # Load OAuth 2.0 credentials.
    try:
        with open(credentials, 'r') as f:
            credentials = google.oauth2.credentials.Credentials(token=None,
                                                                **json.load(f))
            http_request = google.auth.transport.requests.Request()
            credentials.refresh(http_request)
    except Exception as e:
        logging.error('Error loading credentials: %s', e)
        logging.error('Run google-oauthlib-tool to initialize '
                      'new OAuth 2.0 credentials.')
        return

    # Create an authorized gRPC channel.
    grpc_channel = google.auth.transport.grpc.secure_authorized_channel(
        credentials, http_request, api_endpoint)
    logging.info('Connecting to %s', api_endpoint)

    # Configure audio source and sink.
    audio_device = None
    if input_audio_file:
        audio_source = audio_helpers.WaveSource(
            open(input_audio_file, 'rb'),
            sample_rate=audio_sample_rate,
            sample_width=audio_sample_width
        )
    else:
        audio_source = audio_device = (
            audio_device or audio_helpers.SoundDeviceStream(
                sample_rate=audio_sample_rate,
                sample_width=audio_sample_width,
                block_size=audio_block_size,
                flush_size=audio_flush_size
            )
        )
    if output_audio_file:
        audio_sink = audio_helpers.WaveSink(
            open(output_audio_file, 'wb'),
            sample_rate=audio_sample_rate,
            sample_width=audio_sample_width
        )
    else:
        audio_sink = audio_device = (
            audio_device or audio_helpers.SoundDeviceStream(
                sample_rate=audio_sample_rate,
                sample_width=audio_sample_width,
                block_size=audio_block_size,
                flush_size=audio_flush_size
            )
        )
    # Create conversation stream with the given audio source and sink.
    conversation_stream = audio_helpers.ConversationStream(
        source=audio_source,
        sink=audio_sink,
        iter_size=audio_iter_size,
        sample_width=audio_sample_width,
    )

    with SampleAssistant(conversation_stream,
                         grpc_channel, grpc_deadline) as assistant:
        # If file arguments are supplied:
        # exit after the first turn of the conversation.
        if input_audio_file or output_audio_file:
            assistant.converse()
            return

        # If no file arguments supplied:
        # keep recording voice requests using the microphone
        # and playing back assistant response using the speaker.
        # When the once flag is set, don't wait for a trigger. Otherwise, wait.
        wait_for_user_trigger = not once
        while True:
            if wait_for_user_trigger:
                click.pause(info='Press Enter to send a new request...')
            continue_conversation = assistant.converse()
            # wait for user trigger if there is no follow-up turn in
            # the conversation.
            wait_for_user_trigger = not continue_conversation

            # If we only want one conversation, break.
            if once and (not continue_conversation):
                break


if __name__ == '__main__':
    main()
