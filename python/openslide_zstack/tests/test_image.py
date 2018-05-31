""" Test the python image class bindings
"""
import os
import pytest
from ..zstack_image import ZStackImage
from .. import logging

SAMPLE_FILES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sample_files")

def test_no_file():
    """ Test a condition when we provide an invalid filepath to the library
        TODO: Add files that are not supported by openslide
    """
    with pytest.raises(IOError) as excinfo:
        ZStackImage("badfilepath")
    assert "Could not open file" in str(excinfo.value)

def test_happy_path():
    """ Test the happy path and make sure all functions work for a sample file
    """
    img = ZStackImage(os.path.join(SAMPLE_FILES_DIR, "frymire.tiff"))
    about = lambda x, y: abs(x - y) < 0.05

    expected_level_stats = [
        { 'dimensions': (1118, 1105), 'downsample': 1.0 },
        { 'dimensions': (559, 552), 'downsample': 2.0 },
        { 'dimensions': (279, 276), 'downsample': 4.0 },
        { 'dimensions': (139, 138), 'downsample': 8.0 },
    ]

    assert img.zlevel_count() == 1
    assert img.zlevel_offset(0) == 0.0
    assert img.level_count(0) == len(expected_level_stats)

    for level, expected in enumerate(expected_level_stats):
        assert img.level_dimensions(0, level) == expected['dimensions']
        assert about(img.level_downsample(0, level), expected['downsample'])
        _ = img.get_crop(0, level, (0, 0, 100, 100))

@pytest.mark.skip("Used for manual testing")
def test_warning_messages():
    """ Test the tiff loglevel handler
    """
    logging.set_tiff_loglevel(logging.LOG_LEVEL_ERROR)
    img = ZStackImage(os.path.join(SAMPLE_FILES_DIR, "HumanFecalZstackCS.svs"))
    _ = img.get_crop(0, 0, (0, 0, 100, 100))
