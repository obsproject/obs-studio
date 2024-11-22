.. OBS Studio documentation master file, created by
   sphinx-quickstart on Wed Oct 25 00:03:21 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. _contents:

Welcome to OBS Studio's documentation!
======================================

* **Setting up an OBS development environment?** :wiki:`Use the Wiki <Install-Instructions>`
* **Developing your first OBS plugin?** `Use the obs-plugintemplate <https://github.com/obsproject/obs-plugintemplate#obs-plugin-template>`_

.. toctree::
   :caption: Core Concepts
   :maxdepth: 1

   backend-design
   plugins
   frontends
   graphics
   scripting

.. toctree::
   :caption: API Reference
   :maxdepth: 2

   OBS Core <reference-core>
   Modules <reference-modules>
   Core API Object <reference-core-objects>
   Platform/Utility <reference-libobs-util>
   Callbacks (libobs/callback) <reference-libobs-callback>
   Graphics (libobs/graphics) <reference-libobs-graphics>
   Media I/O (libobs/media-io) <reference-libobs-media-io>
   reference-frontend-api

.. toctree::
   :caption: Additional Resources
   :maxdepth: 1
   :hidden:

   Build Instructions <https://obsproject.com/wiki/Install-Instructions>
   Plugin Template <https://github.com/obsproject/obs-plugintemplate#obs-plugin-template>