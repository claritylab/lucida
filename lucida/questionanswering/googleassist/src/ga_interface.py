#!/usr/bin/env python

import json

import os, sys
sys.path.append(os.getcwd())
import configuration

import grpc
import google.auth.transport.grpc
import google.auth.transport.requests
import google.oauth2.credentials

from google.assistant.embedded.v1alpha1 import embedded_assistant_pb2
from google.rpc import code_pb2
from tenacity import retry, stop_after_attempt, retry_if_exception, wait_random, after_log

import assistant_helpers
import audio_helpers

import logging
logger = logging.getLogger(__name__)

ASSISTANT_API_ENDPOINT = 'embeddedassistant.googleapis.com'
END_OF_UTTERANCE = embedded_assistant_pb2.ConverseResponse.END_OF_UTTERANCE
DIALOG_FOLLOW_ON = embedded_assistant_pb2.ConverseResult.DIALOG_FOLLOW_ON
CLOSE_MICROPHONE = embedded_assistant_pb2.ConverseResult.CLOSE_MICROPHONE
DEFAULT_GRPC_DEADLINE = 60 * 3 + 5

class GAInterface(object):
    """Google Assistant Interface that supports follow-on conversations.

    Args:
      conversation_stream(ConversationStream): audio stream
        for recording query and playing back assistant answer.
      channel: authorized gRPC channel for connection to the
        Google Assistant API.
      deadline_sec: gRPC deadline in seconds for Google Assistant API call.
    """

    @retry(reraise=True, stop=stop_after_attempt(3), wait=wait_random(min=1, max=3), after=after_log(logger, logging.WARN))
    def _authorise(self, credentials):
        # Refresh OAuth 2.0 access token.
        http_request = google.auth.transport.requests.Request()
        credentials.refresh(http_request)
        # Create an authorized gRPC channel.
        grpc_channel = google.auth.transport.grpc.secure_authorized_channel(credentials, http_request, ASSISTANT_API_ENDPOINT)
        logging.debug('Connecting to %s...', ASSISTANT_API_ENDPOINT)
        # Create Google Assistant API gRPC client.
        self.assistant = embedded_assistant_pb2.EmbeddedAssistantStub(grpc_channel)

    def __init__(self, user):
        # Load OAuth 2.0 credentials.
        with open(configuration.CREDENTIALS_DIR + "/" + user + ".json", 'r') as f:
            credentials = google.oauth2.credentials.Credentials(token=None, **json.load(f))
        self._authorise(credentials)
        self.deadline = DEFAULT_GRPC_DEADLINE

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

    @retry(reraise=True, stop=stop_after_attempt(3), retry=retry_if_exception(is_grpc_error_unavailable))
    def converse(self, request_id, context=None):
        """Send a voice request to the Assistant and playback the response.

        Returns: True if conversation should continue.
        """
        # Configure audio source and sink.
        audio_source = audio_helpers.RawSource(open("/tmp/lucida/speech/" + str(request_id) + ".raw", 'rb'))
        audio_sink = audio_helpers.RawSink(open("/tmp/lucida/speech/" + str(request_id) + "_out.raw", 'wb'))

        # Create conversation stream.
        self.conversation_stream = audio_helpers.ConversationStream(source=audio_source, sink=audio_sink)

        # Opaque blob provided in ConverseResponse that,
        # when provided in a follow-up ConverseRequest,
        # gives the Assistant a context marker within the current state
        # of the multi-Converse()-RPC "conversation".
        # This value, along with MicrophoneMode, supports a more natural
        # "conversation" with the Assistant.
        self.conversation_state = context

        continue_conversation = False

        self.conversation_stream.start_recording()
        logging.debug('Recording audio request.')

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
                self.conversation_stream.stop_playback()
                return  {'request_id': request_id, 'error': 'Server error: %s' % resp.error.message}
            if resp.event_type == END_OF_UTTERANCE:
                logging.debug('End of audio request detected')
                self.conversation_stream.stop_recording()
            if resp.result.spoken_request_text:
                conversation_request = resp.result.spoken_request_text
                logging.debug('Transcript of user request: "%s".',
                             resp.result.spoken_request_text)
                logging.debug('Playing assistant response.')
            if len(resp.audio_out.audio_data) > 0:
                self.conversation_stream.write(resp.audio_out.audio_data)
            if resp.result.spoken_response_text:
                conversation_response = resp.result.spoken_response_text
                logging.debug(
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
                logging.debug('Expecting follow-on query from user.')
            elif resp.result.microphone_mode == CLOSE_MICROPHONE:
                continue_conversation = False
        logging.debug('Finished playing assistant response.')
        self.conversation_stream.stop_playback()
        return  {'request_id': request_id, 'dialog_follow_on': continue_conversation, 'context': self.conversation_state}

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

