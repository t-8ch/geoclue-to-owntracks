geoclue-to-owntracks
====================

Publishes data aquired from the
[geoclue2](https://www.freedesktop.org/wiki/Software/GeoClue/) to an
[OwnTracks](http://owntracks.org/) instance.

Supported
---------

* Location: Whatever GeoClue does
  * ModemManager (GPS and Mobile network lookup)
  * Wifi database
  ...

* OwnTracks protcols
  * MQTT

TODO
----

* Queue (probably persistent)
* Proper teardown (will need a owntracks/mosquitto GSource)
* HTTP transport
* Support for more batteries

Dependencies
------------

* json-c
* libgeoclue2
* glib2
* mosquitto
