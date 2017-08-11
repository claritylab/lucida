import logging
from copy import copy

#These are the sequences need to get colored ouput
RESET = "\033[0m"
BOLD = "\033[1m"

COLORS = {
    'CRITICAL': '\033[41m',
    'ERROR': '\033[31m',
    'WARNING': '\033[33m',
    'INFO': '\033[36m',
    'DEBUG': '\033[37m'
}

class ColoredFormatter(logging.Formatter):
    def __init__(self, fmt, datefmt):
        logging.Formatter.__init__(self, fmt=fmt, datefmt=datefmt)

    def format(self, record):
        record_cpy = copy(record)

        if record_cpy.levelname in COLORS:
            record_cpy.levelname = COLORS[record_cpy.levelname] + record_cpy.levelname.rjust(7)
        record_cpy.levelname = BOLD + record_cpy.levelname + RESET

        record_cpy.name = BOLD + record_cpy.name.rjust(10) + RESET

        return logging.Formatter.format(self, record_cpy)

class ColoredLogger(logging.Logger):
    fmt = '%(asctime)s - %(levelname)7s: %(name)10s: %(message)s'
    datefmt = '%Y-%m-%d %H:%M:%S'

    def __init__(self, name):
        logging.Logger.__init__(self, name, logging.DEBUG)

        color_formatter = ColoredFormatter(fmt=self.fmt, datefmt=self.datefmt)

        console = logging.StreamHandler()
        console.setFormatter(color_formatter)

        self.addHandler(console)
        return


logging.setLoggerClass(ColoredLogger)
