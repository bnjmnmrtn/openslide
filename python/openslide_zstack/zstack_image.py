""" Python wrapper around the zstack enabled openslide library
"""
import os
from . import lowlevel

class ZStackImage(object):
    """ Class which allows a user to interface with openslide via python.  The image is
        opened upon class construction and held onto until it is destroyed.
    """

    def __init__(self, filename):
        if not os.path.exists(filename):
            raise IOError('Could not open file {0}'.format(filename))
        self.osr = lowlevel.open_img(filename)

    def zlevel_count(self):
        """ Get the number of zlevels in an image
        """
        return lowlevel.get_zlevel_count(self.osr)

    def zlevel_offset(self, zlevel):
        """ Get the number microns that a given zlevel is offset by
        """
        if zlevel < 0 or zlevel >= self.zlevel_count():
            raise ValueError("Z level must be between 0 and {0}".format(self.zlevel_count()))
        return lowlevel.get_zlevel_offset(self.osr, zlevel)

    def level_count(self, zlevel):
        """ Get the number of levels for a given z plane
        """
        if zlevel < 0 or zlevel >= self.zlevel_count():
            raise ValueError("Z level must be between 0 and {0}".format(self.zlevel_count()))
        return lowlevel.get_level_count(self.osr, zlevel)

    def level_dimensions(self, zlevel, level):
        """ Get level dimensions for a given level in a z plane
        """
        if zlevel < 0 or zlevel >= self.zlevel_count():
            raise ValueError("Z level must be between 0 and {0}".format(self.zlevel_count()))
        if level < 0 or level >= self.level_count(zlevel):
            raise ValueError("Level must be between 0 and {0}".format(self.level_count(zlevel)))
        dimensions = lowlevel.get_level_dimensions(self.osr, zlevel, level)
        return (dimensions[0], dimensions[1])

    def level_downsample(self, zlevel, level):
        """ Get the downsample value of a given level
        """
        if zlevel < 0 or zlevel >= self.zlevel_count():
            raise ValueError("Z level must be between 0 and {0}".format(self.zlevel_count()))
        if level < 0 or level >= self.level_count(zlevel):
            raise ValueError("Level must be between 0 and {0}".format(self.level_count(zlevel)))
        return lowlevel.get_level_downsample(self.osr, zlevel, level)

    def best_level_for_downsample(self, zlevel, downsample):
        """ Get the best level for a requested downsample
        """
        if zlevel < 0 or zlevel >= self.zlevel_count():
            raise ValueError("Z level must be between 0 and {0}".format(self.zlevel_count()))
        return lowlevel.get_best_level_for_downsample(self.osr, zlevel, downsample)

    def get_crop(self, zlevel, level, rect):
        """ Returns a PIL image given a zlevel and level
            @rect: A tuple of the form (x, y, width, height)
        """
        if zlevel < 0 or zlevel >= self.zlevel_count():
            raise ValueError("Z level must be between 0 and {0}".format(self.zlevel_count()))
        if level < 0 or level >= self.level_count(zlevel):
            raise ValueError("Level must be between 0 and {0}".format(self.level_count(zlevel)))

        return lowlevel.read_region(self.osr, zlevel, rect[0], rect[1], level, rect[2], rect[3])
