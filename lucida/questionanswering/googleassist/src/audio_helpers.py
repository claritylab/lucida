# Copyright (C) 2017 Google Inc.
# Copyright (C) 2017 Kamal Galrani
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

"""Helper functions for audio streams."""

import logging
import threading
import time
import math
import array

DEFAULT_AUDIO_SAMPLE_RATE = 16000
DEFAULT_AUDIO_SAMPLE_WIDTH = 2
DEFAULT_AUDIO_ITER_SIZE = 3200


def normalize_audio_buffer(buf, volume_percentage, sample_width=2):
    """Adjusts the loudness of the audio data in the given buffer.

    Volume normalization is done by scaling the amplitude of the audio
    in the buffer by a scale factor of 2^(volume_percentage/100)-1.
    For example, 50% volume scales the amplitude by a factor of 0.414,
    and 75% volume scales the amplitude by a factor of 0.681.
    For now we only sample_width 2.

    Args:
      buf: byte string containing audio data to normalize.
      volume_percentage: volume setting as an integer percentage (1-100).
      sample_width: size of a single sample in bytes.
    """
    if sample_width != 2:
        raise Exception('unsupported sample width:', sample_width)
    scale = math.pow(2, 1.0*volume_percentage/100)-1
    # Construct array from bytes based on sample_width, multiply by scale
    # and convert it back to bytes
    arr = array.array('h', buf)
    for idx in range(0, len(arr)):
        arr[idx] = int(arr[idx]*scale)
    buf = arr.tostring()
    return buf


def align_buf(buf, sample_width):
    """In case of buffer size not aligned to sample_width pad it with 0s"""
    remainder = len(buf) % sample_width
    if remainder != 0:
        buf += b'\0' * (sample_width - remainder)
    return buf


class RawSource(object):
    """Audio source that reads audio data from a PCM audio file.

    Reads are throttled to emulate the given sample rate and silence
    is returned when the end of the file is reached.

    Args:
      fp: file-like stream object to read from.
      sample_rate: sample rate in hertz.
      sample_width: size of a single sample in bytes.
    """
    def __init__(self, fp, sample_rate=DEFAULT_AUDIO_SAMPLE_RATE, sample_width=DEFAULT_AUDIO_SAMPLE_WIDTH):
        self._fp = fp
        self._fp.seek(0)
        self._sample_rate = sample_rate
        self._sample_width = sample_width
        self._sleep_until = 0

    def read(self, size):
        """Read bytes from the stream and block until sample rate is achieved.

        Args:
          size: number of bytes to read from the stream.
        """
        now = time.time()
        missing_dt = self._sleep_until - now
        if missing_dt > 0:
            time.sleep(missing_dt)
        self._sleep_until = time.time() + self._sleep_time(size)
        data = self._fp.read(size)
        #  When reach end of audio stream, pad remainder with silence (zeros).
        if not data:
            return b'\x00' * size
        return data

    def close(self):
        """Close the underlying stream."""
        self._fp.close()

    def _sleep_time(self, size):
        sample_count = size / float(self._sample_width)
        sample_rate_dt = sample_count / float(self._sample_rate)
        return sample_rate_dt

    def start(self):
        pass

    def stop(self):
        pass

    @property
    def sample_rate(self):
        return self._sample_rate


class RawSink(object):
    """Audio sink that writes audio data to a PCM audio file.

    Args:
      fp: file-like stream object to write data to.
      sample_rate: sample rate in hertz.
      sample_width: size of a single sample in bytes.
    """
    def __init__(self, fp, sample_rate=DEFAULT_AUDIO_SAMPLE_RATE, sample_width=DEFAULT_AUDIO_SAMPLE_WIDTH):
        self._fp = fp

    def write(self, data):
        """Write bytes to the stream.

        Args:
          data: frame data to write.
        """
        self._fp.write(data)

    def close(self):
        """Close the underlying stream."""
        self._fp.close()

    def start(self):
        pass

    def stop(self):
        pass


class ConversationStream(object):
    """Audio stream that supports half-duplex conversation.

    A conversation is the alternance of:
    - a recording operation
    - a playback operation

    Excepted usage:

      For each conversation:
      - start_recording()
      - read() or iter()
      - stop_recording()
      - start_playback()
      - write()
      - stop_playback()

      When conversations are finished:
      - close()

    Args:
      source: file-like stream object to read input audio bytes from.
      sink: file-like stream object to write output audio bytes to.
      iter_size: read size in bytes for each iteration.
      sample_width: size of a single sample in bytes.
    """
    def __init__(self, source, sink, iter_size=DEFAULT_AUDIO_ITER_SIZE, sample_width=DEFAULT_AUDIO_SAMPLE_WIDTH):
        self._source = source
        self._sink = sink
        self._iter_size = iter_size
        self._sample_width = sample_width
        self._stop_recording = threading.Event()
        self._start_playback = threading.Event()
        self._volume_percentage = 50

    def start_recording(self):
        """Start recording from the audio source."""
        self._stop_recording.clear()
        self._source.start()
        self._sink.start()

    def stop_recording(self):
        """Stop recording from the audio source."""
        self._stop_recording.set()

    def start_playback(self):
        """Start playback to the audio sink."""
        self._start_playback.set()

    def stop_playback(self):
        """Stop playback from the audio sink."""
        self._start_playback.clear()
        self._source.stop()
        self._sink.stop()

    @property
    def volume_percentage(self):
        """The current volume setting as an integer percentage (1-100)."""
        return self._volume_percentage

    @volume_percentage.setter
    def volume_percentage(self, new_volume_percentage):
        logging.info('Volume set to %s%%', new_volume_percentage)
        self._volume_percentage = new_volume_percentage

    def read(self, size):
        """Read bytes from the source (if currently recording).

        Will returns an empty byte string, if stop_recording() was called.
        """
        if self._stop_recording.is_set():
            return b''
        return self._source.read(size)

    def write(self, buf):
        """Write bytes to the sink (if currently playing).

        Will block until start_playback() is called.
        """
        self._start_playback.wait()
        buf = align_buf(buf, self._sample_width)
        buf = normalize_audio_buffer(buf, self.volume_percentage)
        return self._sink.write(buf)

    def close(self):
        """Close source and sink."""
        self._source.close()
        self._sink.close()

    def __iter__(self):
        """Returns a generator reading data from the stream."""
        return iter(lambda: self.read(self._iter_size), b'')

    @property
    def sample_rate(self):
        return self._source._sample_rate
