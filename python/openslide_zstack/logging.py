""" Python wrapper around the zstack logging utilities
"""
from . import lowlevel

LOG_LEVEL_NONE    = 0
LOG_LEVEL_ERROR   = 1
LOG_LEVEL_WARNING = 2

def set_tiff_loglevel(level):
    """ Set the log level for the underlying tiff directory.
        Use the values LOG_LEVEL_NONE, LOG_LEVEL_ERROR, or LOG_LEVEL_WARNING
    """
    if level not in [LOG_LEVEL_NONE, LOG_LEVEL_WARNING, LOG_LEVEL_ERROR]:
        raise ValueError("Invalid log level")
    lowlevel.set_tiff_message_verbosity(level)
