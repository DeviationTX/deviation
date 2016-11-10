# Test copy - may be modified - not guaranteed to work
silver13


The Deviation firmware project is hosted at http://www.deviationtx.com

There you can find links to:
  * pre-built binaries: http://www.deviationtx.com/downloads-new/category/1-deviation-releases
  * the wiki: http://www.deviationtx.com/wiki
  * the forums for asking questions: http://www.deviationtx.com/forums
  * the bug-tracker for reporting issues: http://www.deviationtx.com/mantisbt

Building Deviation requires setting up a cross-compiler.  On certain
transmitters (like the devo7e), the firmware takes up nearly 100% of the
available ROM, and we have found that only specific versions of the
cross-compiler can build it successfully.

To facilitate building Deviation consistently, we have provided Docker images
to guarantee consistent builds across platforms.  Instruction for building with
Docker can be found here: http://www.deviationtx.com/wiki/development/docker

While not recommended, other methods of building Deviation are documented
here: http://www.deviationtx.com/wiki/development
