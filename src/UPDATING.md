# General notes:

**Back up your model files before updating!!**

While we'll try and warn you here if the nightlies write out model files that older versions won't read, there are no guarantees!

From [PhracturedBlue's note](http://www.deviationtx.com/forum/5-news-announcements/1416-nightly-builds):

> Note that there are no guarantees made on these builds at all. They are fully auto-generated, and contain a snapshot of whatever the code looked like when the build happened. There is no manual checking done on these builds.  Also note that we do not keep previous versions of these nightly build. The only available snapshot is the latest one.
>
> Make sure to keep the .zip file. If you need to report a crash bug, you must send me back the debug.zip file along with the errors.txt file on the tx for us to be able to debug.

Always use the `media` directory from the filesystem with each build. Better yet, install everything but the `models` directory.

# Warnings

In reverse chronological order.

## Upgrading past 4.0.1

Hardware configuration data has moved from `tx.ini` to `hardware.ini`. If you haven't modded your transmitter, this shouldn't be an issue. If you have, you'll want to edit `hardware.ini` to match what you did to `tx.ini`.

## Upgrading from prior to 4.0

See [the release notes 4.0](http://www.deviationtx.com/release-notes/25-version-4-0-0). I'd recommend [upgrading to 4.0.1](http://deviationtx.com/repository/Deviation-Releases/Deviation-4.0.1/) before upgrading to any nightlies.

# Model specific notes

## Devo6

Use the Devo8 file system.

## Devo7e

Use the Devo10 file system.

You **must** install the `protocols` directory as well as the `media` directory.