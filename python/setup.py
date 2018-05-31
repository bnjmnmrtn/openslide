from setuptools import setup, Extension

convert = Extension('openslide_zstack._convert', sources=['openslide_zstack/_convert.c'])

setup(name='openslide_zstack',
      version='1.1.1',
      description='zstack enabled openslide bindings',
      url='https://gitlab.com/techcyte/openslide-zstack',
      author='Benjamin Martin',
      author_email='mbenja.086@gmail.com',
      license='MIT',
      packages=['openslide_zstack'],
      ext_modules=[convert],
)
