===============
vmod_bodyaccess
===============

.. image:: https://travis-ci.org/aondio/libvmod-transbody.svg
   :alt: Travis CI badge
   :target: http://travis-ci.org/aondio/libvmod-transbody

------------------------
Varnish Transbody Module
------------------------

:Date: 2015-03-03
:Version: 1.0
:Manual section: 3

SYNOPSIS
========

import bodyaccess;

DESCRIPTION
===========

Varnish vmod that lets you do transformations on the request body.

FUNCTIONS
=========

buffer_req_body
---------------

Prototype
        ::

                buffer_req_body(BYTES size)
Return value
	VOID
Description
	Buffers the req.body if it is smaller than *size*.

        Caching the req.body makes it possible to retry pass
        operations (POST, PUT).

	Note that this function can be used only in vcl_recv.
Example
        ::

                bodyaccess.buffer_req_body(1KB);

        This will buffer the req.body if its size is smaller than 1KB.

len_req_body
------------

Prototype
        ::

                len_req_body()
Return value
        INT
Description
        Returns the request body length.

	Note that the request body must be buffered.
Example
        ::

                | if (bodyaccess.buffer_req_body(1KB)) {
		|     set req.http.x-len = bodyaccess.len_req_body();
		| }

hash_req_body
-------------  

Prototype
        ::

                len_req_body()
Return value
        VOID
Description
        Adds available request body bytes to the lookup hash key.
	Note that this function can only be used in vcl_hash and
	the request body must be buffered.
Example
        ::

                | sub vcl_recv {
		|     bodyaccess.buffer_req_body(1KB);
		| }
		|
		| sub vcl_hash{
		|     bodyaccess.hash_req_body();
		| }

rematch_req_body
----------------

Prototype
        ::

                rematch_req_body(PRIV_CALL, STRING re)
Return value  
        INT
Description
        Returns -1 if an error occurred.
	Returns 0 if the request body doesn't contain the string *re*.
	Returns 1 if the request body contains the string *re*.

	Note that the comparison is case sensitive and the
	request body must be buffered.
Example
        ::

                | std.buffer_req_body(1KB);
		|
		| if (bodyaccess.rematch_req_body("FOO") == 1) {
		|    std.log("is true");
		| }

INSTALLATION
============

The source tree is based on autotools to configure the building, and
does also have the necessary bits in place to do functional unit tests
using the ``varnishtest`` tool.

Building requires the Varnish header files and uses pkg-config to find
the necessary paths.

Usage::

 ./autogen.sh
 ./configure

If you have installed Varnish to a non-standard directory, call
``autogen.sh`` and ``configure`` with ``PKG_CONFIG_PATH`` pointing to
the appropriate path. For example, when varnishd configure was called
with ``--prefix=$PREFIX``, use

 PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
 export PKG_CONFIG_PATH

Make targets:

* make - builds the vmod.
* make install - installs your vmod.
* make check - runs the unit tests in ``src/tests/*.vtc``
* make distcheck - run check and prepare a tarball of the vmod.

COMMON PROBLEMS
===============

* configure: error: Need varnish.m4 -- see README.rst

  Check if ``PKG_CONFIG_PATH`` has been set correctly before calling
  ``autogen.sh`` and ``configure``
