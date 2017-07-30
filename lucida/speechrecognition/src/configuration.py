# configuration.py

import logging, logging.config, yaml
import os, sys, errno
import click
import re
from urlparse import urlparse
import json

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)7s: %(name)10s: %(message)s", datefmt="%Y-%m-%d %H:%M:%S")

GST_DEBUG = os.environ.get('GST_DEBUG')
CONFIG = dict(
    master = "http://localhost:8081",
    retry_after = 5,
    data_directory = "/tmp/lucida/speech",
    max_segment_duration = 180,
    max_call_duration = 3600,
    silence_timeout = 2,
    initial_silence_timeout = 5,
    silence_threshold = -20,
    response_timeout = 30,
    worker_verbosity = "info",
    gstreamer_verbosity = "none",

    config_options = dict(
        master_prompt  = "URL for Lucida commandcenter",
        retry_after_prompt = "Number of seconds to wait between subsequent reconnects to master", retry_after_min = 1, retry_after_max = 30,
        data_directory_prompt = "Directory to store speech transcriptions and raw audio",
        max_segment_duration_prompt = "Maximum length of audio segment in seconds", max_segment_duration_min = 10, max_segment_duration_max = 300,
        max_call_duration_prompt = "Maximum length of audio segment in seconds", max_call_duration_min = 60, max_call_duration_max = 18000,
        silence_timeout_prompt = "Maximum silence (in seconds) that should be tolerated", silence_timeout_min = 1, silence_timeout_max = 10,
        initial_silence_timeout_prompt = "Maximum initial silence (in seconds) that should be tolerated", initial_silence_timeout_min = 1, initial_silence_timeout_max = 30,
        silence_threshold_prompt = "Audio segments with RMS below this value will be considered silent", silence_threshold_min = -100, silence_threshold_max = 100,
        response_timeout_prompt = "Maximum time (in seconds) between end of speech and receiving of transcription that should be tolerated", response_timeout_min = 5, response_timeout_max = 60,
        worker_verbosity_prompt = "Verbosity level for worker", worker_verbosity_choices = ["critical", "error", "warning", "info", "debug"],
        gstreamer_verbosity_prompt = "Verbosity level for GStreamer", gstreamer_verbosity_choices = ["none", "error", "warning", "fixme", "info", "debug", "log", "trace", "memdump"]
    )
)

def validate_master(ctx, param, value):
    if value.startswith("localhost"):
        value = "http://" + value
    if value.startswith("//"):
        value = "http:" + value
    scheme = re.compile('[A-Za-z0-9]{2,5}://')
    if scheme.match(value) == None:
        value = "http://" + value
    value = urlparse(value)
    if value.hostname == None:
        if ctx == None:
            logger.error("Invalid URL provided for Lucida commandcenter!!!")
            return False
        raise click.BadParameter("Invalid URL provided for Lucida commandcenter!!!")
    if value.port != None:
        url = value.hostname + ":" + str(value.port) + "/worker/ws/speech"
    else:
        url = value.hostname + "/worker/ws/speech"
    if value.scheme == "http" or value.scheme == "ws":
        url = "ws://" + url
    elif value.scheme == "https" or value.scheme == "wss":
        url = "wss://" + url
    else:
        if ctx == None:
            logger.error("Unrecognized scheme '%s' while parsing URL for Lucida commandcenter!!!" % (value.scheme))
            return False
        raise click.BadParameter("Unrecognized scheme '%s' while parsing URL for Lucida commandcenter!!!" % (value.scheme))
    return url

def validate_directory(ctx, param, value):
    if not os.path.isdir(value):
        mkdir = raw_input("Specified data directory doesn't exist! Do you want to create it? [y/N]: ")
        if mkdir == "y" or mkdir == "Y":
            try:
                os.makedirs(value, 0700)
            except Exception as e:
                if ctx == None:
                    logger.error("Error while creating data directory!!! %s" % (e))
                    return False
                raise click.BadParameter("Error while creating data directory!!! %s" % (e))
        else:
            if ctx == None:
                logger.error("Aborting...")
                return False
            raise click.BadParameter("Please enter a valid data directory...")
    elif not os.access(value, os.R_OK) or not os.access(value, os.W_OK) or not os.access(value, os.X_OK):
        if ctx == None:
            logger.log("Permission denied while accessing data directory!!!")
            return False
        raise click.BadParameter("Permission denied while accessing data directory!!! Please try again...")
    return value

@click.command()
@click.option("--master", prompt=CONFIG['config_options']['master_prompt'],
    default=CONFIG['master'], type=click.STRING, callback=validate_master, required=True, show_default=True, help=CONFIG['config_options']['master_prompt'])
@click.option("--retry-after", prompt=CONFIG['config_options']['retry_after_prompt'],
    default=CONFIG['retry_after'], type=click.IntRange(min=CONFIG['config_options']['retry_after_min'], max=CONFIG['config_options']['retry_after_max']),
    required=True, show_default=True, help=CONFIG['config_options']['retry_after_prompt'])
@click.option("--data-directory", prompt=CONFIG['config_options']['data_directory_prompt'],
    default=CONFIG['data_directory'], type=click.STRING, callback=validate_directory, required=True, show_default=True, help=CONFIG['config_options']['data_directory_prompt'])
@click.option("--max-segment-duration", prompt=CONFIG['config_options']['max_segment_duration_prompt'],
    default=CONFIG['max_segment_duration'], type=click.IntRange(min=CONFIG['config_options']['max_segment_duration_min'], max=CONFIG['config_options']['max_segment_duration_max']),
    required=True, show_default=True, help=CONFIG['config_options']['max_segment_duration_prompt'])
@click.option("--max-call-duration", prompt=CONFIG['config_options']['max_call_duration_prompt'],
    default=CONFIG['max_call_duration'], type=click.IntRange(min=CONFIG['config_options']['max_call_duration_min'], max=CONFIG['config_options']['max_call_duration_max']),
    required=True, show_default=True, help=CONFIG['config_options']['max_call_duration_prompt'])
@click.option("--silence-timeout", prompt=CONFIG['config_options']['silence_timeout_prompt'],
    default=CONFIG['silence_timeout'], type=click.IntRange(min=CONFIG['config_options']['silence_timeout_min'], max=CONFIG['config_options']['silence_timeout_max']),
    required=True, show_default=True, help=CONFIG['config_options']['silence_timeout_prompt'])
@click.option("--initial-silence-timeout", prompt=CONFIG['config_options']['initial_silence_timeout_prompt'],
    default=CONFIG['initial_silence_timeout'], type=click.IntRange(min=CONFIG['config_options']['initial_silence_timeout_min'],
    max=CONFIG['config_options']['initial_silence_timeout_max']), required=True, show_default=True, help=CONFIG['config_options']['initial_silence_timeout_prompt'])
@click.option("--silence-threshold", prompt=CONFIG['config_options']['silence_threshold_prompt'],
    default=CONFIG['silence_threshold'], type=click.IntRange(min=CONFIG['config_options']['silence_threshold_min'], max=CONFIG['config_options']['silence_threshold_max']),
    required=True, show_default=True, help=CONFIG['config_options']['silence_threshold_prompt'])
@click.option("--response-timeout", prompt=CONFIG['config_options']['response_timeout_prompt'],
    default=CONFIG['response_timeout'], type=click.IntRange(min=CONFIG['config_options']['response_timeout_min'], max=CONFIG['config_options']['response_timeout_max']),
    required=True, show_default=True, help=CONFIG['config_options']['response_timeout_prompt'])
@click.option("--worker-verbosity", prompt=(CONFIG['config_options']['worker_verbosity_prompt'] + " " + str(CONFIG['config_options']['worker_verbosity_choices'])),
    default=CONFIG['worker_verbosity'], type=click.Choice(CONFIG['config_options']['worker_verbosity_choices']), required=True,
    show_default=True, help=CONFIG['config_options']['worker_verbosity_prompt'])
@click.option("--gstreamer-verbosity", prompt=(CONFIG['config_options']['gstreamer_verbosity_prompt'] + " " + str(CONFIG['config_options']['gstreamer_verbosity_choices'])),
    default=CONFIG['gstreamer_verbosity'], type=click.Choice(CONFIG['config_options']['gstreamer_verbosity_choices']), required=True,
    show_default=True, help=CONFIG['config_options']['gstreamer_verbosity_prompt'])
def first_run(master, retry_after, data_directory, max_segment_duration, max_call_duration, silence_timeout, initial_silence_timeout, silence_threshold, response_timeout, worker_verbosity, gstreamer_verbosity):
    conf = dict(
        master = master,
        retry_after = retry_after,
        data_directory = data_directory,
        max_segment_duration = max_segment_duration,
        max_call_duration = max_call_duration,
        silence_timeout = silence_timeout,
        initial_silence_timeout = initial_silence_timeout,
        silence_threshold = silence_threshold,
        response_timeout = response_timeout,
        worker_verbosity = worker_verbosity,
        gstreamer_verbosity = gstreamer_verbosity
    )
    conf_str = (
                 "# Configuration file for speech recognition worker\n\n"
                 "# " + CONFIG['config_options']['master_prompt'] + "\n"
                 "master: '" + conf['master'] + "'\n\n"
                 "# " + CONFIG['config_options']['retry_after_prompt'] + "\n"
                 "retry_after: " + str(conf['retry_after']) + "\n\n"
                 "# " + CONFIG['config_options']['data_directory_prompt'] + "\n"
                 "data_directory: '" + conf['data_directory'] + "'\n\n"
                 "# " + CONFIG['config_options']['max_segment_duration_prompt'] + "\n"
                 "max_segment_duration: " + str(conf['max_segment_duration']) + "\n\n"
                 "# " + CONFIG['config_options']['max_call_duration_prompt'] + "\n"
                 "max_call_duration: " + str(conf['max_call_duration']) + "\n\n"
                 "# " + CONFIG['config_options']['silence_timeout_prompt'] + "\n"
                 "silence_timeout: " + str(conf['silence_timeout']) + "\n\n"
                 "# " + CONFIG['config_options']['initial_silence_timeout_prompt'] + "\n"
                 "initial_silence_timeout: " + str(conf['initial_silence_timeout']) + "\n\n"
                 "# " + CONFIG['config_options']['response_timeout_prompt'] + "\n"
                 "response_timeout: " + str(conf['response_timeout']) + "\n\n"
                 "# " + CONFIG['config_options']['silence_threshold_prompt'] + "\n"
                 "silence_threshold: " + str(conf['silence_threshold']) + "\n\n"
                 "# " + CONFIG['config_options']['worker_verbosity_prompt'] + " " + str(CONFIG['config_options']['worker_verbosity_choices']) + "\n"
                 "worker_verbosity: '" + conf['worker_verbosity'] + "'\n\n"
                 "# " + CONFIG['config_options']['gstreamer_verbosity_prompt'] + " " + str(CONFIG['config_options']['gstreamer_verbosity_choices']) + "\n"
                 "gstreamer_verbosity: '" + conf['gstreamer_verbosity'] + "'"
               )
    with open("worker_config.yaml", "w") as worker_config:
        worker_config.write(conf_str)
    logger.warn("Saving configuration to file and quitting...")
    sys.exit(0)

def process(conf):
    conf['worker_verbosity'] = logging.getLevelName(conf['worker_verbosity'].upper())
    if GST_DEBUG == None:
        conf['gstreamer_verbosity'] = str(CONFIG['config_options']['gstreamer_verbosity_choices'].index(conf['gstreamer_verbosity']))
    else:
        logger.warning("Using GST_DEBUG='%s' value from environment. If you don't want this unset GST_DEBUG before running this script" % GST_DEBUG)
        conf['gstreamer_verbosity'] = GST_DEBUG
    if ( not validate_master(None, None, conf['master']) or not validate_directory(None, None, conf['data_directory']) or
      not isinstance( conf['silence_timeout'], int ) or not isinstance( conf['initial_silence_timeout'], int ) or
      not isinstance( conf['response_timeout'], int ) or not isinstance( conf['worker_verbosity'], int ) or
      not isinstance( conf['silence_threshold'], int ) or not isinstance( conf['retry_after'], int ) or
      not isinstance( conf['max_segment_duration'], int ) or not isinstance( conf['max_call_duration'], int ) or
      conf['silence_timeout'] > CONFIG['config_options']['silence_timeout_max'] or conf['silence_timeout'] < CONFIG['config_options']['silence_timeout_min'] or
      conf['initial_silence_timeout'] > CONFIG['config_options']['initial_silence_timeout_max'] or conf['initial_silence_timeout'] < CONFIG['config_options']['initial_silence_timeout_min'] or
      conf['response_timeout'] > CONFIG['config_options']['response_timeout_max'] or conf['response_timeout'] < CONFIG['config_options']['response_timeout_min'] or
      conf['retry_after'] > CONFIG['config_options']['retry_after_max'] or conf['retry_after'] < CONFIG['config_options']['retry_after_min'] or
      conf['silence_threshold'] > CONFIG['config_options']['silence_threshold_max'] or conf['silence_threshold'] < CONFIG['config_options']['silence_threshold_min'] or
      conf['max_segment_duration'] > CONFIG['config_options']['max_segment_duration_max'] or conf['max_segment_duration'] < CONFIG['config_options']['max_segment_duration_min'] or
      conf['max_call_duration'] > CONFIG['config_options']['max_call_duration_max'] or conf['max_call_duration'] < CONFIG['config_options']['max_call_duration_min'] ):
        raise ValueError
    return conf

def load():
    try:
        with open("logger_config.yaml") as f:
            logging.config.dictConfig(yaml.safe_load(f))
        logger.info("Loaded logger configuration from logger_config.yaml")
    except Exception as e:
        logger.error("Error while loading configuration from logger_config.yaml: %s" % (e))

    global CONFIG

    conf = dict()
    try:
        with open("worker_config.yaml") as f:
            conf = process(yaml.safe_load(f))
        logger.info("Loaded worker configuration from worker_config.yaml: %s" % str(conf))
    except:
        first_run()
    return conf
